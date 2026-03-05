/* Esclavo (MEGA2560) — con RFID + ultrasonido + motores */
#include <SPI.h>
#include <MFRC522.h>

// ================= VARIABLES GENERALES =================
String msg = "";
int vel = 0;

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

// ================= VARIABLES RFID =================
#define RST_PIN 48
#define SS_PIN 53
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ================= VARIABLES SENSORES ULTRASONICOS =================
const int trigPin = 26;
const int echoPin = 27;
bool paradaEmergencia = false;
#define DISTANCIA_PARADA 25

// ================= ZONA / RFID CONTROL =================
int zonaActual = 0; // IMPORTANT: zona que determina qué bloque/posición leer

unsigned long ultimoEnvio = 0;
const unsigned long intervaloRFID = 200;
bool tarjetaDetectada = false;

int lastZonaSent = -1;
byte lastValorSent = 0x00;

// ================= ULTRASONIDO: medir distancia =================
int medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long tiempo = pulseIn(echoPin, HIGH, 30000);
  if (tiempo == 0) return 999;  // sin eco = libre

  return (int)(tiempo * 0.034 / 2);
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
void moveKUKI(char direccion, bool parada, int velPWM) {
  if (parada) {
    velPWM = 0;
    digitalWrite(motor1A, LOW); digitalWrite(motor1B, LOW);
    digitalWrite(motor2A, LOW); digitalWrite(motor2B, LOW);
    digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
    digitalWrite(motor4A, LOW); digitalWrite(motor4B, LOW);
  }

  velPWM = constrain(velPWM, 0, 255);

  switch (direccion) {
    case 'a':  // Adelante
    case 'o':  // Adelante (alias)
      digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
      break;

    case 'b':  // Diagonal ++
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
      break;

    case 'c':  // Derecha
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
      break;

    case 'd':  // Diagonal +-
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
      break;

    case 'e':  // Atras
      digitalWrite(motor1A, LOW); digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, LOW); digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW); digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, LOW); digitalWrite(motor4B, HIGH);
      break;

    case 'f':  // Diagonal --
      digitalWrite(motor1A, LOW); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW); digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW); digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, LOW); digitalWrite(motor4B, LOW);
      break;

    case 'g':  // Izquierda
      digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);  digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW);  digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
      break;

    case 'h':  // Diagonal -+
      digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
      break;

    case 'i':  // Giro izquierdas
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);  digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
      break;

    case 'j':  // Giro derechas
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
      break;

    // k..s (tu tens molts casos que en realitat fan el mateix)
    case 'k': case 'l': case 'm': case 'n': // variants esquerra
      digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
      break;

    case 'p': case 'q': case 'r': case 's': // variants dreta
      digitalWrite(motor1A, LOW);  digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
      break;

    default: // 'z' o qualsevol altra
      digitalWrite(motor1A, LOW); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW); digitalWrite(motor4B, LOW);
      velPWM = 0;
      break;
  }

  analogWrite(motor1Vel, velPWM);
  analogWrite(motor2Vel, velPWM);
  analogWrite(motor3Vel, velPWM);
  analogWrite(motor4Vel, velPWM);
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
                               bloque, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
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

  // Enviar només si canvia zona/valor (o primera detecció)
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

  // Ultrasonido
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();
}

// ================= LOOP =================
void loop() {
  // 1) Ultrasonido
  paradaEmergencia = (medirDistancia() <= DISTANCIA_PARADA);

  // 2) Leer instrucciones de la RP2040 (no-bloqueante)
  String line;
  if (readLineSerial1(line)) {
    msg = line;

    // Esperem: "<zona><dir><vel>"
    // Ex: "2o80" o "3z0"
    if (msg.length() >= 3) {
      const char zc = msg[0];
      const char dcIn = msg[1];
      const int vIn = msg.substring(2).toInt();

      int zParsed = -1;
      if (zc >= '0' && zc <= '3') zParsed = (zc - '0');

      // Si zona vàlida: actualitza zonaActual (important per RFID)
      if (zParsed >= 0) zonaActual = zParsed;

      vel = constrain(vIn, 0, 255);

      // Regles de seguretat: si zona 0 o vel 0 => stop
      char dc = dcIn;
      if (zonaActual == 0 || vel == 0) dc = 'z';

      moveKUKI(dc, paradaEmergencia, vel);

      Serial.print("RX <- ");
      Serial.print(msg);
      Serial.print(" | Dir: ");
      Serial.print(dc);
      Serial.print(" | Vel: ");
      Serial.print(vel);
      Serial.print(" | Parada: ");
      Serial.print(paradaEmergencia ? "YES" : "NO");
      Serial.print(" | ZonaActual: ");
      Serial.println(zonaActual);
    }
  }

  // 3) RFID
  RFID();
}
