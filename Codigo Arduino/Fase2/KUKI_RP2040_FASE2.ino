#include <ArduinoBLE.h>

// ================= TIEMPOS =================
int tiempoPeriodoLectura = 100;
unsigned long tiempoActualLectura = 0;

// ================= PINES CONTROL =================
#define selectorModoBLT 2
#define botonStart 3
#define botonStop 4

// ================= LEDS =================
#define Luz_VERDE 5
#define Luz_ROJO 6
#define Luz_blutuch 7

bool COMSBLT = false;

// ================= SENSORES (REED / LINEA) =================
#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

// ================= CONTROL BIFURCACIONES =================
bool turning = false;
unsigned long turnUntil = 0;
unsigned long lastIntersection = 0;

const int TURN_SPEED = 150;
const int TURN_TIME  = 400;

// ================= PID LINE FOLLOW =================
float Kp = 35.0;
float Ki = 0.0;
float Kd = 12.0;

float pidIntegral = 0;
float lastError = 0;

int baseSpeed = 120;
unsigned long lastPID = 0;

// ================= COMUNICACIÓN MEGA =================
String msgMega = "";
uint8_t camino = 0x00;      // YY (hex) recibido
uint8_t zonaRecibida = 0;   // Zx recibido (0..3)

// ================= PARO INDEFINIDO (LATCH) =================
bool paroIndef = false;

// ================= LED TIMERS =================
static const uint32_t PITSTEP_MS = 500; // Z0: alterna 0.5s
static const uint32_t BLINK_MS   = 250; // paroIndef blink rojo

// ================= UART1 LINE READER (NO BLOQUEANTE) =================
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
    else {
      // overflow: resetea para no mezclar basura
      idx = 0;
    }
  }
  return false;
}

// ================= ENVIAR MOTORES A MEGA =================
static void sendMotorSpeeds(int v1, int v2, int v3, int v4) {
  char out[32];
  snprintf(out, sizeof(out), "%d,%d,%d,%d\n", v1, v2, v3, v4);
  Serial1.print(out);
}

// ================= LEDS =================
static void setLeds(bool green, bool red) {
  digitalWrite(Luz_VERDE, green ? HIGH : LOW);
  digitalWrite(Luz_ROJO,  red   ? HIGH : LOW);
}

static void updateLeds() {
  // 1) Paro indefinido: blink rojo
  if (paroIndef) {
    bool on = ((millis() / BLINK_MS) % 2) == 0;
    setLeds(false, on);
    return;
  }

  // 2) Zona 0: alterna verde/rojo
  if (zonaRecibida == 0) {
    uint32_t phase = (millis() / PITSTEP_MS) % 2;
    if (phase == 0) setLeds(true, false);
    else            setLeds(false, true);
    return;
  }

  // 3) Zona 2: rojo fijo
  if (zonaRecibida == 2) {
    setLeds(false, true);
    return;
  }

  // 4) Zona 3: verde fijo
  if (zonaRecibida == 3) {
    setLeds(true, false);
    return;
  }

  // 5) Zona 1 (o cualquier otra): leds apagados
  setLeds(false, false);
}

// ================= LINE POSITION =================
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

  // Si no ve nada, devolvemos el último error, pero OJO:
  // El paro real por "no sensores" lo hacemos ANTES en computeMotorSpeeds().
  if (count == 0) return lastError;

  return (float)sum / count;
}

// ================= PID =================
float computePID(float error) {
  unsigned long now = millis();

  // Primera vez: evita dt gigante
  if (lastPID == 0) {
    lastPID = now;
    lastError = error;
    pidIntegral = 0;
    return 0;
  }

  float dt = (now - lastPID) / 1000.0f;
  lastPID = now;

  // Limita dt (por si hay parones)
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.100f) dt = 0.100f;

  pidIntegral += error * dt;
  pidIntegral = constrain(pidIntegral, -50, 50);

  float derivative = (error - lastError) / dt;
  lastError = error;

  return Kp * error + Ki * pidIntegral + Kd * derivative;
}

// ================= MOTORES (PID + BIFURCACIÓN) =================
void computeMotorSpeeds(int &m1, int &m2, int &m3, int &m4) {

  int s1 = digitalRead(PinSensor1);
  int s2 = digitalRead(PinSensor2);
  int s3 = digitalRead(PinSensor3);
  int s4 = digitalRead(PinSensor4);
  int s5 = digitalRead(PinSensor5);

  int active = s1 + s2 + s3 + s4 + s5;

  // ✅ REQUERIMIENTO: solo se mueve si detecta algo
  if (active == 0) {
    m1 = m2 = m3 = m4 = 0;
    return;
  }

  bool intersection = active >= 4;
  bool branchRight  = s4 && s5;
  bool branchLeft   = s1 && s2;

  // ===== SI ESTAMOS GIRANDO =====
  if (turning) {
    if (millis() < turnUntil) {
      // giro forzado (derecha por defecto aquí)
      m1 =  TURN_SPEED; m3 =  TURN_SPEED;
      m2 = -TURN_SPEED; m4 = -TURN_SPEED;
      return;
    }
    turning = false;
  }

  // ===== DETECTAR BIFURCACIÓN =====
  if (!turning && intersection && (millis() - lastIntersection > 800)) {
    lastIntersection = millis();

    // camino 0x03: derecha (si hay rama derecha)
    if (camino == 0x03 && branchRight) {
      turning = true;
      turnUntil = millis() + TURN_TIME;

      m1 =  TURN_SPEED; m3 =  TURN_SPEED;
      m2 = -TURN_SPEED; m4 = -TURN_SPEED;
      return;
    }

    // camino 0x02: izquierda (si hay rama izquierda)
    if (camino == 0x02 && branchLeft) {
      turning = true;
      turnUntil = millis() + TURN_TIME;

      m1 = -TURN_SPEED; m3 = -TURN_SPEED;
      m2 =  TURN_SPEED; m4 =  TURN_SPEED;
      return;
    }
  }

  // ===== PID NORMAL =====
  float pos   = readLinePosition();
  float error = pos;

  float turn = computePID(error);

  int left  = baseSpeed - (int)turn;
  int right = baseSpeed + (int)turn;

  left  = constrain(left,  -255, 255);
  right = constrain(right, -255, 255);

  m1 = left;  m3 = left;
  m2 = right; m4 = right;
}

// ================= LEER MEGA (RFID) =================
void leerMega() {
  String line;
  if (!readLineSerial1(line)) return;

  msgMega = line;

  // Formato: Zx:YY
  if (msgMega.length() >= 5 && msgMega[0] == 'Z' && msgMega[2] == ':') {
    uint8_t z = (uint8_t)(msgMega[1] - '0');
    if (z > 3) return;

    String valorHex = msgMega.substring(3);
    uint8_t nuevoCamino = (uint8_t)strtol(valorHex.c_str(), NULL, 16);

    zonaRecibida = z;
    camino = nuevoCamino;

    // ✅ REQUERIMIENTO: en Z2 y Z3, si llega cualquier otra cosa ≠ 0x01 -> paro indefinido
    if ((zonaRecibida == 2 || zonaRecibida == 3) && camino != 0x01) {
      paroIndef = true;
    }

    // ✅ Para quitar el paro: en Z2/Z3 cuando llegue 0x01
    if ((zonaRecibida == 2 || zonaRecibida == 3) && camino == 0x01) {
      paroIndef = false;
    }

    // Debug
    Serial.print("RX -> ");
    Serial.print(msgMega);
    Serial.print(" | zona=");
    Serial.print(zonaRecibida);
    Serial.print(" | camino=0x");
    if (camino < 0x10) Serial.print("0");
    Serial.print(camino, HEX);
    Serial.print(" | paroIndef=");
    Serial.println(paroIndef ? "YES" : "NO");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  // BLE (no lo usamos, pero lo dejo inicializado por si está montado)
  BLE.begin();
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

  // Estado inicial
  zonaRecibida = 0;
  camino = 0x00;
  paroIndef = false;

  Serial.println("RP2040 READY");
}

// ================= LOOP =================
void loop() {
  COMSBLT = digitalRead(selectorModoBLT);
  digitalWrite(Luz_blutuch, COMSBLT ? HIGH : LOW);

  // ===== MODO BLE: IGNORADO =====
  // Si quieres, aquí podrías devolver directamente.
  if (COMSBLT) {
    // No hacemos nada en BLE
    // (si quieres que en BLE también esté parado, mandamos 0)
    sendMotorSpeeds(0, 0, 0, 0);
    updateLeds();
    return;
  }

  // ===== MODO AUTÓNOMO =====
  if (millis() - tiempoActualLectura >= (unsigned long)tiempoPeriodoLectura) {
    tiempoActualLectura = millis();

    // 1) Leer RFID / estado desde MEGA
    leerMega();

    // 2) LEDs según estado
    updateLeds();

    // 3) Condición de parada general:
    // - zona 0 -> parado
    // - paroIndef -> parado
    if (zonaRecibida == 0 || paroIndef) {
      sendMotorSpeeds(0, 0, 0, 0);
      Serial.print("STOP | zona=");
      Serial.print(zonaRecibida);
      Serial.print(" | paroIndef=");
      Serial.println(paroIndef ? "YES" : "NO");
      return;
    }

    // 4) Si no está parado, calcular motores (pero computeMotorSpeeds ya frena si no detecta sensores)
    int m1, m2, m3, m4;
    computeMotorSpeeds(m1, m2, m3, m4);
    sendMotorSpeeds(m1, m2, m3, m4);

    Serial.print("Motors: ");
    Serial.print(m1); Serial.print(",");
    Serial.print(m2); Serial.print(",");
    Serial.print(m3); Serial.print(",");
    Serial.print(m4);
    Serial.print(" | zona=");
    Serial.print(zonaRecibida);
    Serial.print(" | camino=0x");
    if (camino < 0x10) Serial.print("0");
    Serial.print(camino, HEX);
    Serial.print(" | paroIndef=");
    Serial.println(paroIndef ? "YES" : "NO");
  }
}
