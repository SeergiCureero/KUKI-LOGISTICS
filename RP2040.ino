
//VARIABLES
String msg;
char dir;
int vel;
int velocidadDeTransmision = 200;


void setup() {
  // Pin GPIO5 com a RX
  // Pin GPIO4 com a TX
  Serial1.begin(9600); // UART1: RX=5, TX=4 
  Serial.begin(9600); // Debug por puerto USB
}
void loop() {
  /*
  // Envia mensaje al MEGA
  Serial1.println("Hola MEGA!");
  // Si recibe datos del MEGA, los muestra en el PC
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    Serial.print("MEGA dice: ");
    Serial.println(msg);
  }
  delay(1000);
  */


  msg = dir + vel;
  Serial1.println(msg);
  Serial.print("Mensaje enviado: ");
  Serial.println(msg);
  delay(velocidadDeTransmision);
  

}
