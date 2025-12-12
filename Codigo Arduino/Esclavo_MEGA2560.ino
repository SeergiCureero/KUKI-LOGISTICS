/* Esclavo (MEGA2560) */
#include <SPI.h>      //RFID
#include <MFRC522.h>  //RFID
#include <Servo.h>    //SERVO

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
  //TARJETAS CONOCIDAS
byte Valle[4] = {0x13, 0xDD, 0x73, 0x19};
byte Diego[4] = {0x53, 0x98, 0x69, 0x19};
byte Sergi[4] = {0x93, 0x3D, 0x6F, 0x19};
byte Chema[4] = {0x1C, 0x02, 0x10, 0x39};

// VARIABLES SENSOR ULTRASONICO 


// VARIABLES SERVO
Servo servoUltrasonico;


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

  //SERVO
  servoUltrasonico.attach(44);   // Pin de señal del servo
}
void loop() {
  /*
  // Envia mensaje al RP2040
  Serial1.println("Hola RP2040!");
  // Si recibe datos del RP2040, los muestra en el PC.
  */

  // Lee mensajes enviados por KUKI_RP2040.
  if (Serial1.available()) {
    msg = Serial1.readStringUntil('\n');
    Serial.print("RP2040 dice: ");
    Serial.println(msg);
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }

      
      if (memcmp(mfrc522.uid.uidByte, Diego, mfrc522.uid.size) == 0) {
        Serial.println("Diego");
      }
      else if (memcmp(mfrc522.uid.uidByte, Valle, mfrc522.uid.size) == 0) {
        Serial.println("Valle");
      }
      else if (memcmp(mfrc522.uid.uidByte, Sergi, mfrc522.uid.size) == 0) {
        Serial.println("Sergi");
      }
      else if (memcmp(mfrc522.uid.uidByte, Chema, mfrc522.uid.size) == 0) {
        Serial.println("Chema");
      }
      else {
        Serial.println("Desconocido");
      }

      mfrc522.PICC_HaltA();
    }
  }


      
  

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

  switch(msg[0]){
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
      servoUltrasonico.write(90);
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
      servoUltrasonico.write(135);
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
      servoUltrasonico.write(180);
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
      servoUltrasonico.write(0);
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
      servoUltrasonico.write(45);
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
      servoUltrasonico.write(45);
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
      servoUltrasonico.write(135);
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
      servoUltrasonico.write(90);
      break;
  }
  // Lee el resto del string (a partir del char con índice 1) y asócialo a la velocidad dentro de unos valores aptos (mapeado).
  vel = map(msg.substring(1).toInt(), 0, 1023, 0, 255);
  analogWrite(motor1Vel, vel);
  analogWrite(motor2Vel, vel);
  analogWrite(motor3Vel, vel);
  analogWrite(motor4Vel, vel);
}
