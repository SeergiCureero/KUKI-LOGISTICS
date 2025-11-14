/* Establecemos comunicaciones entre Arduinos
__Esclavo__ */


//VARIABLES
String msg;
int vel;

//VARIABLES MOTOR
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



void setup() {
  Serial.begin(9600);     // USB hacia PC
  Serial1.begin(9600);    // UART1: TX1=18, RX1=19
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
}
void loop() {
  /*
  // Envia mensaje al RP2040
  Serial1.println("Hola RP2040!");
  // Si recibe datos del RP2040, los muestra en el PC
  */

  //Lee mensajes enviados por KUKI_RP2040
  if (Serial1.available()) {
    msg = Serial1.readStringUntil('\n');
    Serial.print("RP2040 dice: ");
    Serial.println(msg);
  }
  delay(1000);
  

  //MOVILIDAD
  /*
  El mensaje que recibirá de la master sera un int con la dirección en la que se debe mover seguido de la velocidad. 
  Direcciones:
        a
      h | b
       \|/
    g---+---c 
       /|\
      f | d
        e
  
  Además, se deberá considerar movimientos con puntos de giro fuera del centro del AGV (como los de un coche normal)
  - giro a izquierdas: i
  - giro a derechas: j


  EJEMPLO DE MENSAJE:
  a1023 -> adelante a toda velocidad
  f512 -> diagonal -- a mitad de velocidad
  00 -> parado, 0 velocidad
    */

  switch(msg[0]){
    case 'a':
      //adelante
      //todos los motores se mueven adelante
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'b':
      //diagonal ++
      //2 y 3 adelante, 1 y 4 parados
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, LOW);
      break;
    case 'c':
      //hacia la derecha
      //2 y 3 adelante, 1 y 4 atras
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    case 'd':
      //diagonal +-
      //1 y 4 atras, 2 y 3 parados
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
      //todos los motores se mueven atras
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
      //diagonal --
      //2 y 3 atras, 1 y 4 parados
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
      //hacia la izquierda
      //2 y 3 atras, 1 y 4 adelante
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, HIGH);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'h':
      //diagonal -+
      //1 y 4 adelante, 2 y 3 parados
      digitalWrite(motor1A, HIGH);
      digitalWrite(motor1B, LOW);
      digitalWrite(motor2A, LOW);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'i':
      //giro a izquierdas
      //2 y 4 atras, 1 y 3 adelante
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, LOW);
      digitalWrite(motor3B, HIGH);
      digitalWrite(motor4A, HIGH);
      digitalWrite(motor4B, LOW);
      break;
    case 'j':
      //giro a derechas
      //1 y 3 adelante, 2 y 4 atras
      digitalWrite(motor1A, LOW);
      digitalWrite(motor1B, HIGH);
      digitalWrite(motor2A, HIGH);
      digitalWrite(motor2B, LOW);
      digitalWrite(motor3A, HIGH);
      digitalWrite(motor3B, LOW);
      digitalWrite(motor4A, LOW);
      digitalWrite(motor4B, HIGH);
      break;
    default:
      //apaga motores
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
  //lee el resto del string (a partir del char con indice 1) y asocialo a la velocidad dentro de unos valores aptos (mapeado)
  vel = map(msg.substring(1).toInt(), 0, 1023, 0, 255);
  analogWrite(motor1Vel, vel);
  analogWrite(motor2Vel, vel);
  analogWrite(motor3Vel, vel);
  analogWrite(motor4Vel, vel);
}
