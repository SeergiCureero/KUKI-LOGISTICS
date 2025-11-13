#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>

float Ax, Ay, Az;

BLEService S("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE servicio del mando
// BLE Characteristic - custom 128-bit UUID, read and writable by central
BLEFloatCharacteristic X("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Y("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Vel("19B10004-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
void setup() {
  Serial.begin(9600);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }
  // meter el nombre del servicio y el nombre local del bluetooth.
  BLE.setLocalName("Mando Kuki");
  BLE.setAdvertisedService(S);
  // añadir las caracteristicas al servicio
  S.addCharacteristic(X);
  S.addCharacteristic(Y);
  S.addCharacteristic(Vel);
  // añadir servicio
  BLE.addService(S);
  // comenzar el advertise
  BLE.advertise();
  Serial.println("BLE Mando, esperando a conexiones.....");
}

void loop() {
  // escuchando por BLE peripherals para conectarse:
  BLEDevice central = BLE.central();
  // si la central esta conectada
  if (central) {
    Serial.print("Conectada a central: ");
    // imprime la direccion de la central
    Serial.println(central.address());
    // siempre que este conectado a la central: 
    while (central.connected()) {
      lectura();
      delay(100);

    // cuando se desconecta:
    }
    Serial.print(F("desconectado de central:  "));
    Serial.println(central.address());
  }
}
 
void lectura() {
// lectura del acelorometro y envio por bluetooth
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(Ax, Ay, Az);
    Serial.print(Ax);
    Serial.print('\t');
    Serial.print(Ay);
    Serial.print('\t');
    Serial.println(Az);
    X.writeValue(Ax);
    Y.writeValue(Ay);
  }
 // lectura del potenciometro y envio por bluetooth
  int lectura_potenciometro = analogRead(A1);      
  float PotVel = (float)lectura_potenciometro / 1023.0; // Normalizar a 0.0–1.0
  PotVel = PotVel * 255.0;    // Ahora normalizado 0–10
  Serial.print("Velocidad: ");
  Serial.println(PotVel, 2);
  Vel.writeValue(PotVel);           
  
}





