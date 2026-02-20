/* Esclavo (MEGA2560) */
#include <SPI.h>      //RFID
#include <MFRC522.h>  //RFID
#include <NewPing.h>

// VARIABLES GENERALES
String msg;
int vel;

// VARIABLES MOTOR
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

// VARIABLES RFID
#define RST_PIN 48
#define SS_PIN 53
MFRC522 mfrc522(SS_PIN, RST_PIN);

// VARIABLES SENSOR ULTRASONICO 
#define TRIGGER_PIN  22
#define ECHO_PIN     23
#define MAX_DISTANCE 200  // distancia maxima en cm
unsigned int distancia;           // distancia leída 
bool paradaEmergencia = false;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

int zonaActual = 1;

unsigned long ultimoEnvio = 0;
const unsigned long intervaloRFID = 200;   // tiempo mínimo entre lecturas
bool tarjetaDetectada = false;



void moveKUKI(char direccion, bool parada, int vel){
    //MOVILIDAD
    /*
    El mensaje que recibirá de la master será un int con la dirección en la que se debe mover, seguido de la velocidad. 
    Direcciones:
            a
        h | b
        \|/
        g---+---c 
        /|\
        f | d
            e
    
    Además, se deberán considerar movimientos con puntos de giro fuera del centro del AGV (como los de un coche normal).
    - giro a izquierdas: i
    - giro a derechas: j


    EJEMPLO DE MENSAJE:
    a1023 -> adelante a toda velocidad
    f512 -> diagonal -- a mitad de velocidad
    00 -> parado, 0 velocidad
        */

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
          // Apaga motores
          digitalWrite(motor1A, LOW);
          digitalWrite(motor1B, LOW);
          digitalWrite(motor2A, LOW);
          digitalWrite(motor2B, LOW);
          digitalWrite(motor3A, LOW);
          digitalWrite(motor3B, LOW);
          digitalWrite(motor4A, LOW);
          digitalWrite(motor4B, LOW);
          //Serial.println("Motores Apagados");
        break;
    }

    if (paradaEmergencia)
    {
        analogWrite(motor1Vel, 0);
        analogWrite(motor2Vel, 0);
        analogWrite(motor3Vel, 0);
        analogWrite(motor4Vel, 0);
    }
    else
    {
        analogWrite(motor1Vel, vel);
        analogWrite(motor2Vel, vel);
        analogWrite(motor3Vel, vel);
        analogWrite(motor4Vel, vel);
    }
}

/*
void ultraSonidos(){
    //Ultrasonidos (US) y gestión de la parada de emergencia
    distancia = sonar.ping_cm();
    if (distancia > 0) {
        Serial.print("Distancia: ");
        Serial.print(distancia);
        Serial.println(" cm");
        if(distancia <= 5){
            paradaEmergencia = true;
            Serial.println("OBJETO EN TRAYECTORIA");
            Serial.println("PARANDO KUKI");
        }
        else{
            if(paradaEmergencia){
                //si se ha parado y ya no se detectan objetos en la trayectoria, esperar 1s y resetear variable
                delay(1000);
            }
            paradaEmergencia = false;
        }
    } 
    else {
        Serial.println("Ningún objecto detectado");
        if(paradaEmergencia){
            //si se ha parado y ya no se detectan objetos en la trayectoria, esperar 1s y resetear variable
            delay(1000);
        }
        paradaEmergencia = false;
    }
}
*/

void RFID() {

  if (millis() - ultimoEnvio < intervaloRFID) return;

  if (!mfrc522.PICC_IsNewCardPresent()) {
    tarjetaDetectada = false;
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) return;

  byte bloque;
  byte posicion;

  switch (zonaActual) {
    case 1:
      bloque = 5;
      posicion = 1;
      break;

    case 2:
      bloque = 6;
      posicion = 1;
      break;

    case 3:
      bloque = 6;
      posicion = 8;
      break;
  }

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

  byte valor = buffer[posicion];

  // Enviar solo una vez por detección
  if (!tarjetaDetectada) {

    Serial1.print("Z");
    Serial1.print((int)zonaActual + 1);
    Serial1.print(":");
    if (valor < 0x10) Serial1.print("0");
    Serial1.print(valor, HEX);
    Serial1.println();

    Serial.print("Z");
    Serial.print((int)zonaActual + 1);
    Serial.print(":");
    if (valor < 0x10) Serial.print("0");
    Serial.print(valor, HEX);
    Serial.println();

    ultimoEnvio = millis();
    tarjetaDetectada = true;
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}



void setup() {

  Serial.begin(9600);     // USB hacia PC
  Serial1.begin(9600);    // UART1: TX1=18, RX1=19

  //pinMode Motores
  pinMode(motor1A,OUTPUT);
  pinMode(motor1B,OUTPUT);
  pinMode(motor1Vel,OUTPUT);
  pinMode(motor2A,OUTPUT);
  pinMode(motor2B,OUTPUT);
  pinMode(motor2Vel,OUTPUT);
  pinMode(motor3A,OUTPUT);
  pinMode(motor3B,OUTPUT);
  pinMode(motor3Vel,OUTPUT);
  pinMode(motor4A,OUTPUT);
  pinMode(motor4B,OUTPUT);
  pinMode(motor4Vel,OUTPUT);

  //RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Lectura del UID");
}


void loop() {

  // Leer instrucciones de la RP2040
  if (Serial1.available()) {
    msg = Serial1.readStringUntil('\n');
    Serial.print("RP2040 dice: ");
    Serial.println(msg);
  }

  switch(msg[0]){
    case 1:
      zonaActual = 1;
    break;
    case 2:
      zonaActual = 2;
    break;
    case 3:
      zonaActual = 3;
    break;
    default:
      zonaActual = 1;
    break;
  }

  // Leer RFID y enviar dato
  RFID();

  paradaEmergencia = false;
  vel = map(msg.substring(2).toInt(), 0, 1023, 0, 255);
  moveKUKI(msg[1], paradaEmergencia, vel);
}
