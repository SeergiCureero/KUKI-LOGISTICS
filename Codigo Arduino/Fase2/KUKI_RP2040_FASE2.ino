#include <ArduinoBLE.h>

// Tiempos
int tiempoPeriodoLectura = 100;
unsigned long tiempoActualLectura = 0;

// Pines control
#define selectorModoBLT 2
#define botonStart 3
#define botonStop 4

// Leds
#define Luz_VERDE 5
#define Luz_ROJO 6
#define Luz_blutuch 7


bool COMSBLT = false;

// Pines sensores
#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

// ================= PID LINE FOLLOW =================

float Kp = 35.0;
float Ki = 0.0;
float Kd = 12.0;

float pidIntegral = 0;
float lastError = 0;

int baseSpeed = 120;

unsigned long lastPID = 0;

// ===== Comunicación MEGA =====
String msgMega = "";
uint8_t camino = 0x00;
uint8_t zonaRecibida = 0;  // feedback del MEGA (Zx:yy)

// ✅ estat intern (comença en ZONA 0)
uint8_t zonaOrdenada = 0;

// ================= LED / HOLD CONTROL =================
static const uint32_t PITSTEP_MS = 500;  // zona 0: 0.5s verd / 0.5s vermell
static const uint32_t BLINK_MS = 250;    // blink (error)
static const uint32_t HOLD_MS = 2000;    // stop 2s quan camí=0x01 a Z2/Z3

unsigned long holdUntil = 0;

// ✅ blink “latch” per estat
bool blinkRed = false;
bool blinkGreen = false;

static inline bool inHold() {
  return millis() < holdUntil;
}
static inline void startHold(unsigned long ms) {
  holdUntil = millis() + ms;
}

// ================= UART1 LINE READER (NO BLOQUEJANT) =================
static bool readLineSerial1(String &out) {
  static char buf[32];
  static uint8_t idx = 0;

  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (c == '\r') continue;

    if (c == '\n') {
      buf[idx] = '\0';
      out = String(buf);
      idx = 0;
      out.trim();
      return true;
    }

    if (idx < sizeof(buf) - 1) buf[idx++] = c;
    else idx = 0;  // overflow -> reset
  }
  return false;
}

// ===== helper: enviar como "<zona><dir><vel>\n" =====
static void sendMotorSpeeds(int v1, int v2, int v3, int v4) {

  char out[32];
  snprintf(out, sizeof(out), "%d,%d,%d,%d\n", v1, v2, v3, v4);
  Serial1.print(out);
}

// ================= LEDS =================
static void setLeds(bool green, bool red) {
  digitalWrite(Luz_VERDE, green ? HIGH : LOW);
  digitalWrite(Luz_ROJO, red ? HIGH : LOW);
}

static void updateLeds() {
  // HOLD: leds OFF i ja
  if (inHold()) {
    setLeds(false, false);
    return;
  }

  // ZONA 0: alterna verd/vermell 0.5s/0.5s
  if (zonaOrdenada == 0) {
    uint32_t phase = (millis() / PITSTEP_MS) % 2;
    if (phase == 0) setLeds(true, false);
    else setLeds(false, true);
    return;
  }

  // ZONA 2: per defecte FIX vermell, si blinkRed => blink vermell
  if (zonaOrdenada == 2) {
    if (blinkRed) {
      bool on = ((millis() / BLINK_MS) % 2) == 0;
      setLeds(false, on);
    } else {
      setLeds(false, true);
    }
    return;
  }

  // ZONA 3: per defecte FIX verd, si blinkGreen => blink verd
  if (zonaOrdenada == 3) {
    if (blinkGreen) {
      bool on = ((millis() / BLINK_MS) % 2) == 0;
      setLeds(on, false);
    } else {
      setLeds(true, false);
    }
    return;
  }

  // ZONA 1: leds OFF
  setLeds(false, false);
}

float readLinePosition() {

  int s1 = digitalRead(PinSensor1);
  int s2 = digitalRead(PinSensor2);
  int s3 = digitalRead(PinSensor3);
  int s4 = digitalRead(PinSensor4);
  int s5 = digitalRead(PinSensor5);

  int weights[5] = { -2, -1, 0, 1, 2 };
  int sensors[5] = { s1, s2, s3, s4, s5 };

  int sum = 0;
  int count = 0;

  for (int i = 0; i < 5; i++) {
    if (sensors[i]) {
      sum += weights[i];
      count++;
    }
  }

  if (count == 0) {
    return lastError;
  }

  return (float)sum / count;
}

float computePID(float error) {

  unsigned long now = millis();
  float dt = (now - lastPID) / 1000.0;

  if (dt <= 0) dt = 0.001;

  lastPID = now;

  pidIntegral += error * dt;
  pidIntegral = constrain(pidIntegral, -50, 50);

  float derivative = (error - lastError) / dt;

  float output = Kp * error + Ki * pidIntegral + Kd * derivative;

  lastError = error;

  return output;
}

void computeMotorSpeeds(int &m1, int &m2, int &m3, int &m4) {

  float pos = readLinePosition();

  float error = pos;

  float turn = computePID(error);

  // Reduir velocitat quan el gir és gran
  int speed = baseSpeed - abs(turn) * 0.4;
  speed = constrain(speed, 60, baseSpeed);

  int left = speed - turn;
  int right = speed + turn;

  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  // Motors esquerra
  m1 = left;
  m3 = left;

  // Motors dreta
  m2 = right;
  m4 = right;
}

// ================= BLE =================
void prog(BLEDevice peripheral) {
  Serial.println("Conectando BLE...");

  if (!peripheral.connect()) {
    Serial.println("Error conexión");
    return;
  }

  Serial.println("Conectado");

  if (!peripheral.discoverAttributes()) {
    Serial.println("Error atributos");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic X = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");

  if (!X || !Y || !Vel) {
    Serial.println("Característica faltante");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    leerMega();
    updateLeds();

    if (X.canRead() && Y.canRead() && Vel.canRead()) {
      uint8_t bx[4], by[4], bv[4];

      int nx = X.readValue(bx, 4);
      int ny = Y.readValue(by, 4);
      int nv = Vel.readValue(bv, 4);

      if (nx == 4 && ny == 4 && nv == 4) {
        float valX, valY, valVel;
        memcpy(&valX, bx, 4);
        memcpy(&valY, by, 4);
        memcpy(&valVel, bv, 4);

        float t = 0.2f;
      }
    }

    delay(100);
  }

  Serial.println("BLE desconectado");
}

// ================= LEER MEGA (RFID) =================
void leerMega() {
  String line;
  if (!readLineSerial1(line)) return;

  msgMega = line;

  // Format: Zx:YY
  if (msgMega.length() >= 5 && msgMega[0] == 'Z' && msgMega[2] == ':') {
    uint8_t z = (uint8_t)(msgMega[1] - '0');
    if (z > 3) return;

    String valorHex = msgMega.substring(3);
    uint8_t nuevoCamino = (uint8_t)strtol(valorHex.c_str(), NULL, 16);

    zonaRecibida = z;
    camino = nuevoCamino;

    // ========= FSM + blink latch =========
    switch (zonaOrdenada) {

      // ZONA 0: només surt si veu 02 o 03
      case 0:
        if (camino == 0x02) {
          zonaOrdenada = 2;
          blinkRed = false;
        } else if (camino == 0x03) {
          zonaOrdenada = 3;
          blinkGreen = false;
        }
        break;

      // ZONA 1: 02->2, 03->3, 00->0
      case 1:
        if (camino == 0x02) {
          zonaOrdenada = 2;
          blinkRed = false;
        } else if (camino == 0x03) {
          zonaOrdenada = 3;
          blinkGreen = false;
        } else if (camino == 0x00) {
          zonaOrdenada = 0;
        }
        break;

      // ZONA 2:
      // - si veu 01 => Z1 + HOLD + leds off (blink off)
      // - si veu altre => queda a Z2 i activa blink vermell
      case 2:
        if (camino == 0x01) {
          blinkRed = false;
          zonaOrdenada = 1;
          startHold(HOLD_MS);
        } else {
          blinkRed = true;  // tag raro
        }
        break;

      // ZONA 3:
      // - si veu 01 => Z1 + HOLD + leds off (blink off)
      // - si veu altre => queda a Z3 i activa blink verd
      case 3:
        if (camino == 0x01) {
          blinkGreen = false;
          zonaOrdenada = 1;
          startHold(HOLD_MS);
        } else {
          blinkGreen = true;  // tag raro
        }
        break;
    }

    Serial.print("RX -> ");
    Serial.print(msgMega);
    Serial.print(" | camino=0x");
    if (camino < 0x10) Serial.print("0");
    Serial.print(camino, HEX);
    Serial.print(" | zonaOrdenada=");
    Serial.print(zonaOrdenada);
    Serial.print(" | blinkR=");
    Serial.print(blinkRed ? "1" : "0");
    Serial.print(" | blinkG=");
    Serial.print(blinkGreen ? "1" : "0");
    Serial.print(" | hold=");
    Serial.println(inHold() ? "YES" : "NO");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  BLE.begin();
  Serial.println("RP2040 READY");
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStart, INPUT);
  pinMode(botonStop, INPUT);

  pinMode(Luz_VERDE, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
  pinMode(Luz_ROJO, OUTPUT);

  pinMode(PinSensor1, INPUT);
  pinMode(PinSensor2, INPUT);
  pinMode(PinSensor3, INPUT);
  pinMode(PinSensor4, INPUT);
  pinMode(PinSensor5, INPUT);

  setLeds(false, false);
}

// ================= LOOP =================
void loop() {

  COMSBLT = digitalRead(selectorModoBLT);
  digitalWrite(Luz_blutuch, COMSBLT ? HIGH : LOW);

  BLEDevice peripheral = BLE.available();

  // ===== MODO BLE =====
  if (peripheral && COMSBLT) {

    if (peripheral.localName().indexOf("Mando Kuki") < 0) return;

    BLE.stopScan();
    prog(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

    return;
  }

  // ===== MODO AUTONOMO (SENSORES) =====
  if (!COMSBLT) {

    if (millis() - tiempoActualLectura >= (unsigned long)tiempoPeriodoLectura) {

      tiempoActualLectura = millis();

      // 1️⃣ Leer RFID / estado desde MEGA
      leerMega();

      // 2️⃣ Actualizar LEDs
      updateLeds();

      // 3️⃣ HOLD o zona 0 → robot parado
      if (inHold() || zonaOrdenada == 0) {

        sendMotorSpeeds(0, 0, 0, 0);

        Serial.println("STOP");

      } else {

        int m1, m2, m3, m4;

        computeMotorSpeeds(m1, m2, m3, m4);

        sendMotorSpeeds(m1, m2, m3, m4);

        Serial.print("Motors: ");
        Serial.print(m1);
        Serial.print(",");
        Serial.print(m2);
        Serial.print(",");
        Serial.print(m3);
        Serial.print(",");
        Serial.println(m4);
      }
    }
  }
}
