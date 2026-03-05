/* Esclavo (MEGA2560) — con 3 ultrasonidos + LEDs + parada de emergencia */
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
// NOTA: pines sensor ultrasonico = 26/27
const int trigPin = 26;
const int echoPin = 27; 
bool paradaEmergencia = false;
// Umbral de parada (cm) — LED fijo = muy cerca = peligro
#define DISTANCIA_PARADA 25

// ================= ZONA / RFID CONTROL =================
int zonaActual = 0;

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

  //Si la distancia medida es menor o igual a la distancia de parada, devuelve true
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
  if (parada) 
  {
    velPWM = 0;
    digitalWrite(motor1A, LOW);
    digitalWrite(motor1B, LOW);
    digitalWrite(motor2A, LOW);
    digitalWrite(motor2B, LOW);
    digitalWrite(motor3A, LOW);
    digitalWrite(motor3B, LOW);
    digitalWrite(motor4A, LOW);
    digitalWrite(motor4B, LOW);
  }
  velPWM = constrain(velPWM, 0, 255);

  switch (direccion) {
    case 'a':  // Adelante
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'b':  // Diagonal ++
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'c':  // Derecha
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    case 'd':  // Diagonal +-
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    case 'e':  // Atras
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    case 'f':  // Diagonal --
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'g':  // Izquierda
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'h':  // Diagonal -+
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'i':  // Giro izquierdas
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'j':  // Giro derechas
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    case 'k':  // Full Izquierda
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'l':  // Más Izquierda
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'm':  // Izquierda suave
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'n':  // Poco Izquierda
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'o':  // Adelante (alias)
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'p':  // Poco Derecha
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'q':  // Derecha suave
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'r':  // Más Derecha
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 's':  // Full Derecha
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    default:
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
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

  // Ultrasonidos 
  for (int i = 0; i < 3; i++) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();


}

// ================= LOOP =================
void loop() {
  // 1) Lee el sensor ultrasonico y actualiza paradaEmergencia
  
  if(medirDistancia() <= DISTANCIA_PARADA){
    paradaEmergencia = true;
  }
  else{
    paradaEmergencia = false;
  }

  // 2) Leer instrucciones de la RP2040 (no-bloqueante)
  String line;
  if (readLineSerial1(line)) {
    msg = line;

    if (msg.length() >= 3) {
      char zc = msg[0];
      char dc = msg[1];
      int v = msg.substring(2).toInt();

      zonaActual = (zc >= '0' && zc <= '3') ? (zc - '0') : 1;
      vel = constrain(v, 0, 255);

      if (zonaActual == 0 || vel == 0) dc = 'z';

      moveKUKI(dc, paradaEmergencia, vel);
      Serial.print("Dir: ");
      Serial.print(dc);
      Serial.print(" | Vel: ");
      Serial.print(vel);
      Serial.print(" | Parada: ");
      Serial.print(paradaEmergencia);
      Serial.print(" | Zona: ");
      Serial.println(zc);
      
    }
  }

  // 3) RFID
  RFID();
}
