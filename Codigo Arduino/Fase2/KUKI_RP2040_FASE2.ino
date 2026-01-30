#include <ArduinoBLE.h>

// VARIABLES
String msg;
char dir;
int vel = 35;

#define selectorModoBLT 2
bool COMSBLT = false;

#define botonStart 3
bool Start = false;

#define botonStop 4
bool Stop = false;

#define Luz_VERDE 5
#define Luz_blutuch 6
#define Luz_ROJO 7

// Pines sensores (DI)
#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

void prog(BLEDevice peripheral) {
  Serial.println("Conectando...");
  if (!peripheral.connect()) {
    Serial.println("FALLO al conectarse!");
    return;
  }
  Serial.println("Conectado :D");

  Serial.println("Descubriendo atributos...");
  if (!peripheral.discoverAttributes()) {
    Serial.println("Descubrimiento de atributos FALLIDO!");
    peripheral.disconnect();
    return;
  }
  Serial.println("Atributos descubiertos");

  BLECharacteristic X   = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y   = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");

  if (!X || !Y || !Vel) {
    Serial.println("Falta alguna característica (X/Y/Vel)!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    if (X.canRead() && Y.canRead() && Vel.canRead()) {
      uint8_t bufX[4], bufY[4], bufVel[4];

      X.readValue(bufX, 4);
      Y.readValue(bufY, 4);
      Vel.readValue(bufVel, 4);

      float valX, valY, valVel;
      memcpy(&valX, bufX, sizeof(float));
      memcpy(&valY, bufY, sizeof(float));
      memcpy(&valVel, bufVel, sizeof(float));

      vel = (int)valVel;

      float threshold = 0.2;
      if (abs(valX) < threshold && abs(valY) < threshold) dir = 'n';
      else if (valX > threshold && valY > threshold) dir = 'h';
      else if (valX > threshold && valY < -threshold) dir = 'b';
      else if (valX < -threshold && valY > threshold) dir = 'f';
      else if (valX < -threshold && valY < -threshold) dir = 'd';
      else if (valX > threshold) dir = 'a';
      else if (valX < -threshold) dir = 'e';
      else if (valY > threshold) dir = 'g';
      else if (valY < -threshold) dir = 'c';
    }

    msg = String(dir) + String(vel);
    Serial1.println(msg);

    Serial.print("Mensaje enviado: ");
    Serial.println(msg);

    delay(100);
  }

  Serial.println("Periferico Desconectado");
}

void lecturaSensor(char direccion[5]) {

  //crea una variable de pines
  const uint8_t pins[5] = {
    PinSensor1, PinSensor2, PinSensor3, PinSensor4, PinSensor5
  };
  //lee del array de pines los estados de estos y los asocia a el indice de la variable dada, editandola asi.
  for (int i = 0; i < 5; i++) {
    direccion[i] = digitalRead(pins[i]);
  }

  //imprime por pantalla el resultado para debug
  for (int i = 0; i < 5; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(direccion[i] == HIGH ? "IMAN" : "NO");
    if (i < 4) Serial.print(" | ");
  }
  Serial.println();
}

void setup() {
  Serial1.begin(9600);
  Serial.begin(9600);

  BLE.begin();
  Serial.println("KUKI");

  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStart, INPUT);
  pinMode(botonStop, INPUT);

  pinMode(Luz_VERDE, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
  pinMode(Luz_ROJO, OUTPUT);

  // Sensores magneticos por DI :
  pinMode(PinSensor1, INPUT_PULLUP);
  pinMode(PinSensor2, INPUT_PULLUP);
  pinMode(PinSensor3, INPUT_PULLUP);
  pinMode(PinSensor4, INPUT_PULLUP);
  pinMode(PinSensor5, INPUT_PULLUP);
}

void loop() {
  COMSBLT = digitalRead(selectorModoBLT);

  digitalWrite(Luz_blutuch, COMSBLT ? HIGH : LOW);

  BLEDevice peripheral = BLE.available();

  if (peripheral && COMSBLT) {
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    if (peripheral.localName().indexOf("Mando Kuki") < 0) {
      Serial.println("Kuki no encontrado");
      return;
    }

    BLE.stopScan();
    prog(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }
  else if (!COMSBLT) {

    //lee los sensores reed y danos una dirección
    char direccion[5];
    lecturaSensor(direccion);

    for (int i = 0; i < 5; i++)
    {
      if (direccion[i] == HIGH)
      {
        switch (i)
        {
        case 0:
          Serial.println("c");
          break;
        case 1:
          Serial.println("b");
          break;
        case 2:
          Serial.println("a");
          break;
        case 3:
          Serial.println("g");
          break;
        case 4:
          Serial.println("h");
          break;
        
        default:
          Serial.println("n");
          break;
        }
        
      }
      
    }
    
  }
}
