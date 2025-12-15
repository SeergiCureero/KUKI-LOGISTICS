/* Esclavo (MEGA2560) */
#include <SPI.h>      //RFID
#include <MFRC522.h>  //RFID
#include <Servo.h>    //SERVO
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
  //TARJETAS CONOCIDAS
byte Valle[4] = {0x13, 0xDD, 0x73, 0x19};
byte Diego[4] = {0x53, 0x98, 0x69, 0x19};
byte Sergi[4] = {0x93, 0x3D, 0x6F, 0x19};
byte Chema[4] = {0x1C, 0x02, 0x10, 0x39};
  //CAMINO
int camino = 0;

// VARIABLES SENSOR ULTRASONICO 
#define TRIGGER_PIN  22
#define ECHO_PIN     23
#define MAX_DISTANCE 200  // distancia maxima en cm
unsigned int distancia;           // distancia leída 
bool paradaEmergencia = false;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// VARIABLES SERVO (pin PWM 45)
Servo servoUltrasonico;

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

    /*
    Para gestionar la parada de emergencia, ponemos la velocidad de los motores a 0. 
    De esta manera podemos hacer que el robot tenga una dirección (osea, que se gire el servo del US) a la vez que esta parado.
    Esto es util porque podemos cambiar de dirección, comprovar que no hay nada, y salir del paro de emergencia
    */

    switch(direccion){
        case 'a':
            // Adelante
            // Todos los motores se mueven adelante.
            servoUltrasonico.write(90);
            digitalWrite(motor1A, HIGH);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, HIGH);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, HIGH);
            digitalWrite(motor4B, LOW);
            Serial.println("Adelante");
        break;
        case 'b':
            // Diagonal ++
            //2 y 3 adelante, 1 y 4 parados
            servoUltrasonico.write(135);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, HIGH);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, LOW);
            digitalWrite(motor4B, LOW);
            Serial.println("Diagonal ++");
        break;
        case 'c':
            // Hacia la derecha
            // 2 y 3 adelante, 1 y 4 atras
            servoUltrasonico.write(180);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, HIGH);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, HIGH);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, LOW);
            digitalWrite(motor4B, HIGH);
            Serial.println("Derecha");
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
            Serial.println("Diagonal +-");
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
            Serial.println("Atras");
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
            Serial.println("Diagonal --");
        break;
        case 'g':
        // Hacia la izquierda
        // 2 y 3 atras, 1 y 4 adelante
        servoUltrasonico.write(0);
        if(!paradaEmergencia){
            digitalWrite(motor1A, HIGH);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, HIGH);
            digitalWrite(motor3A, LOW);
            digitalWrite(motor3B, HIGH);
            digitalWrite(motor4A, HIGH);
            digitalWrite(motor4B, LOW);
            Serial.println("Izquierda");
        }
        break;
        case 'h':
            // Diagonal -+
            // 1 y 4 adelante, 2 y 3 parados
            servoUltrasonico.write(45);
            digitalWrite(motor1A, HIGH);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, LOW);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, HIGH);
            digitalWrite(motor4B, LOW);
            Serial.println("Diagonal -+");
        break;
        case 'i':
            // Giro a izquierdas
            // 2 y 4 atras, 1 y 3 adelante
            servoUltrasonico.write(45);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, HIGH);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, LOW);
            digitalWrite(motor3B, HIGH);
            digitalWrite(motor4A, HIGH);
            digitalWrite(motor4B, LOW);
            Serial.println("Giro a Izquierdas");
        break;
        case 'j':
            // Giro a derechas
            // 1 y 3 adelante, 2 y 4 atras
            servoUltrasonico.write(135);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, HIGH);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, HIGH);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, LOW);
            digitalWrite(motor4B, HIGH);
            Serial.println("Giro a Derechas");
        break;
        default:
            // Apaga motores
            servoUltrasonico.write(90);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, LOW);
            digitalWrite(motor3A, LOW);
            digitalWrite(motor3B, LOW);
            digitalWrite(motor4A, LOW);
            digitalWrite(motor4B, LOW);
            Serial.println("Motores Apagados");
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


int RFID(){
  //RFID
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }

      
      if (memcmp(mfrc522.uid.uidByte, Diego, mfrc522.uid.size) == 0) {
        Serial.println("Diego");
        return(1);
      }
      else if (memcmp(mfrc522.uid.uidByte, Valle, mfrc522.uid.size) == 0) {
        Serial.println("Valle");
        return(2);
      }
      else if (memcmp(mfrc522.uid.uidByte, Sergi, mfrc522.uid.size) == 0) {
        Serial.println("Sergi");
        return(3);
      }
      else if (memcmp(mfrc522.uid.uidByte, Chema, mfrc522.uid.size) == 0) {
        Serial.println("Chema");
        return(4);
      }
      else {
        Serial.println("Desconocido");
        return(0);
      }

      mfrc522.PICC_HaltA();
    }
  }
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

  //SERVO
  servoUltrasonico.attach(45);   // Pin de señal del servo
}


void loop() {

  /*
  //comentar para debug
  // Lee mensajes enviados por KUKI_RP2040.
  if (Serial1.available()) {
    msg = Serial1.readStringUntil('\n');
    Serial.print("RP2040 dice: ");
    Serial.println(msg);
  }*/

  //descomentar para debug
  // Lee mensajes enviados por SERIE.
  if (Serial.available()) {
    msg = Serial.readStringUntil('\n');
    Serial.print("RP2040 dice: ");
    Serial.println(msg);
  }

  camino = RFID();

  switch (camino)
  {
  case -1:
      Serial.println("Camino Desconocido");
    break;
  case 0:
      Serial.println("Esperando Camino");
    break;
  case 1:
      Serial.println("Camino DIEGO");
    break;
  case 2:
      Serial.println("Camino VALLE");
    break;
  case 3:
      Serial.println("Camino SERGI");
    break;
  case 4:
      Serial.println("Camino CHEMA");
    break;
  default:
      Serial.println("Esperando Camino");
    break;
  }

  if(camino > 0) 
  {
    //leemos la distancia
    ultraSonidos();
        
    // Lee el resto del string (a partir del char con índice 1) y asócialo a la velocidad dentro de unos valores aptos (mapeado).
    vel = map(msg.substring(1).toInt(), 0, 1023, 0, 255);
    moveKUKI(msg[0],paradaEmergencia,vel);
  }
  

  

}
