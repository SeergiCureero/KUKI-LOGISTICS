/* Esclavo (MEGA2560) */
#include <SPI.h>
#include <MFRC522.h>
#include <NewPing.h>

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

// ================= VARIABLES SENSOR ULTRASONICO =================
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 200
unsigned int distancia;
bool paradaEmergencia = false;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// ================= ZONA / RFID CONTROL =================
// ✅ Ara permetem 0..3
int zonaActual = 0;

unsigned long ultimoEnvio = 0;
const unsigned long intervaloRFID = 200;
bool tarjetaDetectada = false;

// Per reenviar quan canvies zona/valor sense treure targeta
int lastZonaSent = -1;
byte lastValorSent = 0x00;

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

// ================= MOTORES =================
void moveKUKI(char direccion, bool parada, int velPWM) {
  // ✅ si paradaEmergencia, forcem 0
  if (parada) velPWM = 0;
  velPWM = constrain(velPWM, 0, 255);

  // Direccions
  switch(direccion){
        case 'a':
          // Adelante
          // Todos los motores se mueven adelante.
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Adelante");
        break;
        case 'b':
          // Diagonal ++
          //2 y 3 adelante, 1 y 4 parados
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, LOW);
          //Serial.println("Diagonal ++");
        break;
        case 'c':
          // Hacia la derecha
          // 2 y 3 adelante, 1 y 4 atras
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, HIGH);
          //Serial.println("Derecha");
        break;
        case 'd':
          // Diagonal +-
          // 1 y 4 atras, 2 y 3 parados
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, HIGH);
          //Serial.println("Diagonal +-");
        break;
        case 'e':
          // Todos los motores se mueven atras.
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, HIGH);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, HIGH);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, HIGH);
          //Serial.println("Atras");
        break;
        case 'f':
          // Diagonal --
          // 2 y 3 atras, 1 y 4 parados
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, HIGH);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, HIGH);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, LOW);
          //Serial.println("Diagonal --");
        break;
        case 'g':
        // Hacia la izquierda
        // 2 y 3 atras, 1 y 4 adelante
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, HIGH);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, HIGH);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Izquierda");
        break;
        case 'h':
          // Diagonal -+
          // 1 y 4 adelante, 2 y 3 parados
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Diagonal -+");
        break;
        case 'i':
          // Giro a izquierdas
          // 2 y 4 atras, 1 y 3 adelante
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, HIGH);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Giro a Izquierdas");
        break;
        case 'j':
          // Giro a derechas
          // 1 y 3 adelante, 2 y 4 atras
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, HIGH);
          //Serial.println("Giro a Derechas");
        break;
        
        
        // Giros petit
        case 'k':
          // Full Izquierda
          // MI al contrario, MD full
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, HIGH);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, HIGH);
          //Serial.println("Full Izquierda");
        break;
        case 'l':
          // Más Izquierda
          // MI a 0, MD Full
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, LOW);
          //Serial.println("Más Izqueirda");
        break;
        case 'm':
          // Izquierda
          // MI 25%, MD Full
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, 64);
          digitalWrite(motor2B, 0);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, 64);
          digitalWrite(motor4B, 0);
          //Serial.println("Izquierda");
        break;
        case 'n':
          // Poco Izquierda
          // MI 50%, MD Full
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, 128);
          digitalWrite(motor2B, 0);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, 128);
          digitalWrite(motor4B, 0);
          //Serial.println("Poco Izquierda");
        break;
        case 'o':
          // Adelante
          // Todos los motores se mueven adelante.
          digitalWrite(motor1A, HIGH);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, HIGH);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Adelante");
        break;
        case 'p':
          // Poco Derecha
          // MI Full, MD 50%
          digitalWrite(motor1A, 128);
          digitalWrite(motor1B, 0);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, 128);
          digitalWrite(motor3B, 0);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Poco Derecha");
        break;
        case 'q':
          // Derecha
          // MI Full, MD 25%
          digitalWrite(motor1A, 64);
          digitalWrite(motor1B, 0);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, 64);
          digitalWrite(motor3B, 0);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Derecha");
        break;
        case 'r':
          // Más Derecha
          // MI Full, MD 0
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Más Derecha");
        break;
        case 's':
          // Full Derecha
          // MI Full, MD al contrario
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, HIGH);
          digitalWrite(motor2A, HIGH);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, HIGH);
          digitalWrite(motor4A, HIGH);
          digitalWrite(motor4B, LOW);
          //Serial.println("Full Derecha");
        break;

    default:
      digitalWrite(motor1A, LOW); digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW); digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW); digitalWrite(motor4B, LOW);
      break;
  }

  // ✅ PWM (una sola vegada, net)
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

  // --- Definim sector/bloc dins sector/byte ---
  byte sector = 1;  // segons el que has dit
  byte blocDins = 0;
  byte posicion = 0;

  switch (zonaActual) {
    case 0: // ✅ pitstop: llegeix igual que zona 1
    case 1:
      blocDins = 1;
      posicion = 1;
      break;  // S1, B1, byte1

    case 2:
      blocDins = 2;
      posicion = 1;
      break;  // S1, B2, byte1

    case 3:
      blocDins = 2;
      posicion = 8;
      break;  // S1, B2, byte8

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

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(motor1A, OUTPUT); pinMode(motor1B, OUTPUT); pinMode(motor1Vel, OUTPUT);
  pinMode(motor2A, OUTPUT); pinMode(motor2B, OUTPUT); pinMode(motor2Vel, OUTPUT);
  pinMode(motor3A, OUTPUT); pinMode(motor3B, OUTPUT); pinMode(motor3Vel, OUTPUT);
  pinMode(motor4A, OUTPUT); pinMode(motor4B, OUTPUT); pinMode(motor4Vel, OUTPUT);

  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("MEGA READY (RFID + UART1) - zone 0 enabled");
}

void loop() {
  // 1) Llegir instruccions de la RP2040 (no-bloquejant)
  String line;
  if (readLineSerial1(line)) {
    msg = line;

    // Format: "<zona><dir><vel>" ex: "0n0" "1o200" "3q120"
    if (msg.length() >= 3) {
      char zc = msg[0];
      char dc = msg[1];
      int v = msg.substring(2).toInt();

      // ✅ accepta 0..3
      if (zc >= '0' && zc <= '3') zonaActual = zc - '0';
      else zonaActual = 1;

      vel = constrain(v, 0, 255);

      // Si pitstop (zona 0) o vel==0 -> direcció safe
      if (zonaActual == 0 || vel == 0) dc = 'n';

      moveKUKI(dc, paradaEmergencia, vel);
    }
  }

  // 2) RFID
  RFID();
}
