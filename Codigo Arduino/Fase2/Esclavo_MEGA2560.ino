q /* Esclavo (MEGA2560) — con 3 ultrasonidos + LEDs + parada de emergencia */
#include <SPI.h>
#include <MFRC522.h>

// ================= VARIABLES MOTOR =================
#define motor1A 2
#define motor1B 3
#define motor1Vel 4
#define motor2A 5
#define motor2B 6
#define motor2Vel 7
#define motor3A 8
#define motor3B 9
#define motor3Vel 10
#define motor4A 11
#define motor4B 12
#define motor4Vel 13
unsigned long lastCmdTime = 0;

// ================= VARIABLES RFID =================
#define RST_PIN 48
#define SS_PIN 53
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ================= VARIABLES SENSORES ULTRASONICOS =================
// NOTA: pines 22/23 ahora son del sensor 1 (igual que antes),
//       se añaden sensor 2 (24/25) y sensor 3 (26/27)
const int trigPins[3] = { 22, 24, 26 };
const int echoPins[3] = { 23, 25, 27 };
const int ledPins[3] = { 34, 33, 32 };

int distancias[3];
unsigned long tiempoAnteriorLed[3] = { 0, 0, 0 };
bool estadoLed[3] = { false, false, false };

bool paradaEmergencia = false;

// Umbral de parada (cm) — LED fijo = muy cerca = peligro
#define DISTANCIA_PARADA 15

// ================= ZONA / RFID CONTROL =================
int zonaActual = 0;

unsigned long ultimoEnvio = 0;
const unsigned long intervaloRFID = 200;
bool tarjetaDetectada = false;

int lastZonaSent = -1;
byte lastValorSent = 0x00;

// ================= ULTRASONIDO: medir distancia =================
int medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long tiempo = pulseIn(echoPin, HIGH, 30000);
  if (tiempo == 0) return 999;  // sin eco = libre
  return (int)(tiempo * 0.034 / 2);
}

// ================= ULTRASONIDO: intervalo de parpadeo LED =================
int calcularIntervalo(int distancia) {
  if (distancia > 50) return -1;   // LED apagado
  if (distancia > 30) return 800;  // parpadeo lento
  if (distancia > DISTANCIA_PARADA) return 300;  // parpadeo rápido
  return 0;                        // muy cerca → LED fijo (= parada)
}

// ================= ULTRASONIDO: leer sensores + gestionar LEDs =================
void actualizarUltrasonidos() {
  unsigned long ahora = millis();
  bool hayPeligro = false;

  for (int i = 0; i < 3; i++) {
    distancias[i] = medirDistancia(trigPins[i], echoPins[i]);

    int intervalo = calcularIntervalo(distancias[i]);

    if (intervalo == -1) {
      // Libre: LED apagado
      digitalWrite(ledPins[i], LOW);
      estadoLed[i] = false;
    } else if (intervalo == 0) {
      // Muy cerca: LED fijo → PELIGRO
      digitalWrite(ledPins[i], HIGH);
      hayPeligro = true;
    } else {
      // Zona de aviso: parpadeo
      if (ahora - tiempoAnteriorLed[i] >= (unsigned long)intervalo) {
        tiempoAnteriorLed[i] = ahora;
        estadoLed[i] = !estadoLed[i];
        digitalWrite(ledPins[i], estadoLed[i]);
      }
    }
  }

  // Actualizar flag de parada de emergencia
  paradaEmergencia = hayPeligro;

  // Si hay parada, detener motores inmediatamente
  if (paradaEmergencia) {
  setMotorSpeeds(0,0,0,0,true);
  Serial.println("⚠️  PARADA DE EMERGENCIA — obstáculo detectado");
}
}

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
    else idx = 0;  // overflow -> reset
  }
  return false;
}

// ================= MOTORES =================
void setMotorSpeeds(int v1, int v2, int v3, int v4, bool parada) {

  if (parada) {
    v1 = v2 = v3 = v4 = 0;
  }

  v1 = constrain(v1, -255, 255);
  v2 = constrain(v2, -255, 255);
  v3 = constrain(v3, -255, 255);
  v4 = constrain(v4, -255, 255);

  // Motor 1
  if (v1 >= 0) {
    digitalWrite(motor1A, HIGH);
    digitalWrite(motor1B, LOW);
  } else {
    digitalWrite(motor1A, LOW);
    digitalWrite(motor1B, HIGH);
    v1 = -v1;
  }

  // Motor 2
  if (v2 >= 0) {
    digitalWrite(motor2A, HIGH);
    digitalWrite(motor2B, LOW);
  } else {
    digitalWrite(motor2A, LOW);
    digitalWrite(motor2B, HIGH);
    v2 = -v2;
  }

  // Motor 3
  if (v3 >= 0) {
    digitalWrite(motor3A, HIGH);
    digitalWrite(motor3B, LOW);
  } else {
    digitalWrite(motor3A, LOW);
    digitalWrite(motor3B, HIGH);
    v3 = -v3;
  }

  // Motor 4
  if (v4 >= 0) {
    digitalWrite(motor4A, HIGH);
    digitalWrite(motor4B, LOW);
  } else {
    digitalWrite(motor4A, LOW);
    digitalWrite(motor4B, HIGH);
    v4 = -v4;
  }

  analogWrite(motor1Vel, v1);
  analogWrite(motor2Vel, v2);
  analogWrite(motor3Vel, v3);
  analogWrite(motor4Vel, v4);
}

// ================= RFID =================
void RFID() {
  if (millis() - ultimoEnvio < intervaloRFID) return;

  if (!mfrc522.PICC_IsNewCardPresent()) {
    tarjetaDetectada = false;
    lastZonaSent = -1;
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) return;

  byte sector = 1;
  byte blocDins = 0;
  byte posicion = 0;

  switch (zonaActual) {
    case 0:
    case 1:
      blocDins = 1;
      posicion = 1;
      break;
    case 2:
      blocDins = 2;
      posicion = 1;
      break;
    case 3:
      blocDins = 2;
      posicion = 8;
      break;
    default:
      mfrc522.PICC_HaltA();
      return;
  }

  byte bloque = sector * 4 + blocDins;

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte buffer[18];
  byte size = sizeof(buffer);

  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                               bloque, &key, &(mfrc522.uid))
      != MFRC522::STATUS_OK) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  if (mfrc522.MIFARE_Read(bloque, buffer, &size) != MFRC522::STATUS_OK) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  if (posicion >= 16) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  byte valor = buffer[posicion];

  if (!tarjetaDetectada || zonaActual != lastZonaSent || valor != lastValorSent) {
    Serial1.print("Z");
    Serial1.print(zonaActual);
    Serial1.print(":");
    if (valor < 0x10) Serial1.print("0");
    Serial1.print(valor, HEX);
    Serial1.println();

    Serial.print("Z");
    Serial.print(zonaActual);
    Serial.print(":");
    if (valor < 0x10) Serial.print("0");
    Serial.print(valor, HEX);
    Serial.println();

    ultimoEnvio = millis();
    tarjetaDetectada = true;
    lastZonaSent = zonaActual;
    lastValorSent = valor;
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  // Motores
  pinMode(motor1A, OUTPUT);
  pinMode(motor1B, OUTPUT);
  pinMode(motor1Vel, OUTPUT);
  pinMode(motor2A, OUTPUT);
  pinMode(motor2B, OUTPUT);
  pinMode(motor2Vel, OUTPUT);
  pinMode(motor3A, OUTPUT);
  pinMode(motor3B, OUTPUT);
  pinMode(motor3Vel, OUTPUT);
  pinMode(motor4A, OUTPUT);
  pinMode(motor4B, OUTPUT);
  pinMode(motor4Vel, OUTPUT);

  // Ultrasonidos + LEDs
  for (int i = 0; i < 3; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
  }

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();
}

// ================= LOOP =================
void loop() {
  // 1) Leer y actualizar los 3 ultrasonidos (actualiza paradaEmergencia)
  actualizarUltrasonidos();

  // 2) Leer instrucciones de la RP2040 (no-bloqueante)
  String line;

  if (readLineSerial1(line)) {

    int v1, v2, v3, v4;

    int parsed = sscanf(line.c_str(), "%d,%d,%d,%d", &v1, &v2, &v3, &v4);

    if (parsed == 4) {

      lastCmdTime = millis();

      setMotorSpeeds(v1, v2, v3, v4, paradaEmergencia);

      Serial.print("M:");
      Serial.print(v1);
      Serial.print(",");
      Serial.print(v2);
      Serial.print(",");
      Serial.print(v3);
      Serial.print(",");
      Serial.println(v4);
    }
  }

  // 3) RFID
  RFID();

  if (millis() - lastCmdTime > 500) {
  setMotorSpeeds(0,0,0,0,true);
  lastCmdTime = millis();
}
}
