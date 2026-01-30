#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>

float Ax, Ay, Az;

BLEService S("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE servicio del mando.
// BLE Characteristic - custom 128-bit UUID, read and writable by central
BLEFloatCharacteristic X("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Y("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Vel("19B10004-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
void setup() {
  Serial.begin(9600);
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }
  // Meter el nombre del servicio y el nombre local del Bluetooth.
  BLE.setLocalName("Mando Kuki");
  BLE.setAdvertisedService(S);
  // Añadir las características al servicio.
  S.addCharacteristic(X);
  S.addCharacteristic(Y);
  S.addCharacteristic(Vel);
  // Añadir servicio
  BLE.addService(S);
  // Comenzar el advertise.
  BLE.advertise();
  Serial.println("BLE Mando, esperando a conexiones...");
}

void loop() {
  // Escuchando por BLE peripherals para conectarse.
  BLEDevice central = BLE.central();
  //  Sí, la central está conectada.
  if (central) {
    Serial.print("Conectada a central: ");
    // Imprime la dirección de la central.
    Serial.println(central.address());
    // Siempre que esté conectado a la central:
    while (central.connected()) {
      lectura();
      delay(100);

    // Cuando se desconecta:
    }
    Serial.print(F("Desconectado de central:  "));
    Serial.println(central.address());
  }
}
 
void lectura() {
// Lectura del acelerómetro y envío por Bluetooth.
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
 // Lectura del potenciómetro y envío por Bluetooth.
  int lectura_potenciometro = analogRead(A1);      
  float PotVel = (float)lectura_potenciometro / 1023.0; // Normalizar a 0.0–1.0
  PotVel = PotVel * 255.0;    // Ahora normalizado 0–10
  Serial.print("Velocidad: ");
  Serial.println(PotVel, 2);
  Vel.writeValue(PotVel);           
  
}





