#include <ArduinoBLE.h>
//VARIABLES

String msg;
char dir;
int vel;
void setup() {
  // Pin GPIO5 com a RX
  // Pin GPIO4 com a TX
  Serial1.begin(9600); // UART1: RX=5, TX=4 
  Serial.begin(9600); // Debug por puerto USB
  while (!Serial);
  // initialize the BLE hardware
  BLE.begin();
  Serial.println("KUKI");
  // start scanning for Button Device BLE peripherals
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
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

  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
   
    if (peripheral.localName().indexOf("Mando Kuki") < 0) {
      Serial.println("Kuki no encontrado");
      return;  // If the name doesn't have "Button Device" in it then ignore it
    }
    // stop scanning
    BLE.stopScan();
    prog(peripheral);
    // peripheral disconnected, start scanning again
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }
}
void prog(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }
  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }
  // retrieve the LED characteristic
  BLECharacteristic X = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");
  if (!X) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  if (!Y) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  if (!Vel) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  while (peripheral.connected()) {
    // while the peripheral is connected
   if (X.canRead() && Y.canRead() &&  Vel.canRead()) {
      // Buffers para cada float (4 bytes)
      uint8_t bufX[4], bufY[4], bufVel[4];

      // Leer los valores del periférico
      X.readValue(bufX, 4);
      Y.readValue(bufY, 4);

      Vel.readValue(bufVel, 4);

      // Convertir los bytes a floats
      float valX, valY, valVel;
      float threshold = 0.2;
      memcpy(&valX, bufX, sizeof(float));
      memcpy(&valY, bufY, sizeof(float));
      memcpy(&valVel, bufVel, sizeof(float));

      vel = (int)valVel;  

       if (abs(valX) < threshold && abs(valY) < threshold) dir = 'n'; // centro
        else if (valX > threshold && valY > threshold) dir = 'b';      // arriba-derecha
        else if (valX > threshold && valY < -threshold) dir = 'h';     // arriba-izquierda
        else if (valX < -threshold && valY > threshold) dir = 'd';     // abajo-derecha
        else if (valX < -threshold && valY < -threshold) dir = 'f';    // abajo-izquierda
        else if (valX > threshold) dir = 'a';                           // arriba
        else if (valX < -threshold) dir = 'e';                          // abajo
        else if (valY > threshold) dir = 'c';                           // derecha
        else if (valY < -threshold) dir = 'g';                          // izquierda  

    }

    msg = String(dir) + String(vel);
    Serial1.println(msg);
    Serial.print("Mensaje enviado: ");
    Serial.println(msg);
    delay(100);
  }
  Serial.println("Peripheral disconnected");
}
