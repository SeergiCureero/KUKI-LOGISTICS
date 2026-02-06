/* Establecemos comunicaciones entre Arduinos.
   Esclavo */

// VARIABLES
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

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(motor1A, OUTPUT); pinMode(motor1B, OUTPUT); pinMode(motor1Vel, OUTPUT);
  pinMode(motor2A, OUTPUT); pinMode(motor2B, OUTPUT); pinMode(motor2Vel, OUTPUT);
  pinMode(motor3A, OUTPUT); pinMode(motor3B, OUTPUT); pinMode(motor3Vel, OUTPUT);
  pinMode(motor4A, OUTPUT); pinMode(motor4B, OUTPUT); pinMode(motor4Vel, OUTPUT);

  msg = "n0";
}

void loop() {
  // Lee mensajes enviados por KUKI_RP2040.
  if (Serial1.available()) {
    msg = Serial1.readStringUntil('\n');
    msg.trim();

    Serial.print("RP2040 dice: ");
    Serial.println(msg);

    if (msg.length() < 2) return;

    switch (msg[0]) {
      case 'a':
        digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
        break;
      case 'b':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
        break;
      case 'c':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
        break;
      case 'd':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
        digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
        break;
      case 'e':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
        digitalWrite(motor2A, LOW);  digitalWrite(motor2B, HIGH);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, HIGH);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
        break;
      case 'f':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, LOW);  digitalWrite(motor2B, HIGH);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, HIGH);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
        break;
      case 'g':
        digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
        break;
      case 'h':
        digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, LOW);  digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
        break;
      case 'i':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW);  digitalWrite(motor3B, HIGH);
        digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
        break;
      case 'j':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, HIGH);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, HIGH);
        break;
      case 'k':
        digitalWrite(motor1A, LOW);  digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
        break;
      case 'l':
        digitalWrite(motor1A, HIGH);  digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, LOW); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW);  digitalWrite(motor4B, LOW);
        break;  
      default:
        digitalWrite(motor1A, LOW); digitalWrite(motor1B, LOW);
        digitalWrite(motor2A, LOW); digitalWrite(motor2B, LOW);
        digitalWrite(motor3A, LOW); digitalWrite(motor3B, LOW);
        digitalWrite(motor4A, LOW); digitalWrite(motor4B, LOW);
        break;
    }

    vel = map(msg.substring(1).toInt(), 0, 1023, 0, 255);

    analogWrite(motor1Vel, vel);
    analogWrite(motor2Vel, vel);
    analogWrite(motor3Vel, vel);
    analogWrite(motor4Vel, vel);
  }

  // petit delay opcional per no saturar CPU
  delay(5);
}
