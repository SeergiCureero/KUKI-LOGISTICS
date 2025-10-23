#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>

float Ax, Ay, Az;

BLEService S("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE LED Service
// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEFloatCharacteristic X("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Y("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
BLEFloatCharacteristic Vel("19B10004-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);
void setup() {
  Serial.begin(9600);
  while (!Serial);
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }
  // set advertised local name and service UUID:
  BLE.setLocalName("Mando Kuki");
  BLE.setAdvertisedService(S);
  // add the characteristic to the service
  S.addCharacteristic(X);
  S.addCharacteristic(Y);
  S.addCharacteristic(Vel);
  // add service
  BLE.addService(S);
  // start advertising
  BLE.advertise();
  Serial.println("BLE LED Peripheral, waiting for connections....");
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();
  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    // while the central is still connected to peripheral:
    while (central.connected()) {
      lectura();
      delay(100);

    // when the central disconnects, print it out:
    }
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}

void lectura() {

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
  
}




