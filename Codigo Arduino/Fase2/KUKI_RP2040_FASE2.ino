#include <ArduinoBLE.h>

// ================= VARIABLES =================
String msg;   // compat/debug
char dir = 'n';

// Cuantas lecturas seguidas sin linea toleramos
const uint8_t MAX_MISS = 10;

// Velocitats (0..255)
int velRecto = 124;
int velGiro1  = 32;
int velGiro2  = 48;
int velGiro3  = 64;

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

// ===== Comunicación MEGA =====
String msgMega = "";
uint8_t camino = 0x00;
uint8_t zonaRecibida = 0;     // feedback del MEGA (Zx:yy)

// ✅ estat intern (comença en ZONA 0)
uint8_t zonaOrdenada = 0;

// ================= LED / HOLD CONTROL =================
static const uint32_t PITSTEP_MS = 500;   // zona 0: 0.5s verd / 0.5s vermell
static const uint32_t BLINK_MS   = 250;   // blink (error)
static const uint32_t HOLD_MS    = 2000;  // stop 2s quan camí=0x01 a Z2/Z3

unsigned long holdUntil = 0;

// ✅ blink “latch” per estat
bool blinkRed = false;
bool blinkGreen = false;

static inline bool inHold() { return millis() < holdUntil; }
static inline void startHold(unsigned long ms) { holdUntil = millis() + ms; }

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
    else idx = 0; // overflow -> reset
  }
  return false;
}

// ===== helper: enviar como "<zona><dir><vel>\n" =====
static void sendCmd(uint8_t z, char d, int v) {
  if (z > 3) z = 3;         // permet 0..3
  v = constrain(v, 0, 255);

  char out[16];
  snprintf(out, sizeof(out), "%d%c%d\n", (int)z, d, v);
  Serial1.print(out);
}

// ================= LEDS =================
static void setLeds(bool green, bool red) {
  digitalWrite(Luz_VERDE, green ? HIGH : LOW);
  digitalWrite(Luz_ROJO,  red   ? HIGH : LOW);
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
    else            setLeds(false, true);
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

        if (abs(valX) < t && abs(valY) < t) dir = 'n';
        else if (valX > t && valY > t) dir = 'h';
        else if (valX > t && valY < -t) dir = 'b';
        else if (valX < -t && valY > t) dir = 'f';
        else if (valX < -t && valY < -t) dir = 'd';
        else if (valX > t) dir = 'a';
        else if (valX < -t) dir = 'e';
        else if (valY > t) dir = 'g';
        else if (valY < -t) dir = 'c';
      }
    }

    delay(100);
  }

  Serial.println("BLE desconectado");
}

// ================= SENSORES =================
char lecturaSensor() {
  static char ultimaDirValida = 'z';
  static uint8_t missCount = 0;

  char direccion = 'z';

  bool sensor1 = digitalRead(PinSensor1);
  bool sensor2 = digitalRead(PinSensor2);
  bool sensor3 = digitalRead(PinSensor3);
  bool sensor4 = digitalRead(PinSensor4);
  bool sensor5 = digitalRead(PinSensor5);

  if (zonaRecibida == 3 && sensor5) {
    direccion = 's';
  } else if (sensor1 && sensor2) {
    direccion = 'l';
  } else if (sensor3 && sensor4) {
    direccion = 'p';
  } else if (sensor4 && sensor5) {
    direccion = 'r';
  } else if (sensor1) {
    direccion = 'k';
  } else if (sensor2) {
    direccion = 'm';
  } else if (sensor3) {
    direccion = 'o';
  } else if (sensor4) {
    direccion = 'q';
  } else if (sensor5) {
    direccion = 's';
  }

  if (direccion != 'z') {
    ultimaDirValida = direccion;
    missCount = 0;
    return direccion;
  }

  missCount++;
  if (missCount < MAX_MISS) return ultimaDirValida;
  return 'z';
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
        if (camino == 0x02) { zonaOrdenada = 2; blinkRed = false; }
        else if (camino == 0x03) { zonaOrdenada = 3; blinkGreen = false; }
        break;

      // ZONA 1: 02->2, 03->3, 00->0
      case 1:
        if (camino == 0x02) { zonaOrdenada = 2; blinkRed = false; }
        else if (camino == 0x03) { zonaOrdenada = 3; blinkGreen = false; }
        else if (camino == 0x00) { zonaOrdenada = 0; }
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
          blinkRed = true;   // tag raro
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
          blinkGreen = true; // tag raro
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

  // ----- MODO BLE -----
  if (peripheral && COMSBLT) {
    if (peripheral.localName().indexOf("Mando Kuki") < 0) return;

    BLE.stopScan();
    prog(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
    return;
  }

  // ----- MODO SENSORES -----
  if (!COMSBLT) {
    if (millis() - tiempoActualLectura >= (unsigned long)tiempoPeriodoLectura) {
      tiempoActualLectura = millis();

      // 1) RFID
      leerMega();

      // 2) LEDs segons estat
      updateLeds();

      // 3) Direcció
      dir = lecturaSensor();

      // 4) Vel
      //k l m n o p q r s
      int vOut;
      char dirOut = dir;

      if(dir=='o'){
        vOut = velRecto;
      }
      else if((dir == 'n') || (dir == 'p')){
        vOut = velGiro1;
      }
      else if((dir == 'm') || (dir == 'q')){
        vOut = velGiro1;
      }
      else if((dir == 'l') || (dir == 'r')){
        vOut = velGiro2;
      }
      else if((dir == 'k') || (dir == 's')){
        vOut = velGiro3;
      }

     
      

      // HOLD o ZONA 0 => STOP
      if (inHold() || zonaOrdenada == 0) {
        dirOut = 'z';
        vOut = 0;
      }
      sendCmd(zonaOrdenada, dirOut, vOut);
      Serial.print("Dir: ");
      Serial.print(dirOut);
      Serial.print("    Vel: ");
      Serial.print(vOut);
      Serial.print("    Zona: ");
      Serial.println(zonaOrdenada);
    }
  }
}
