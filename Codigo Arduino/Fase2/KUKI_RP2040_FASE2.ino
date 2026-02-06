#include <ArduinoBLE.h>

// ================= VARIABLES =================

String msg; // la dejamos por compatibilidad/debug
char dir = 'n';

// Velocidades
int velRecto = 150;  // SOLO cuando dir == 'a'
int velGiro  = 80;   // cuando dir != 'a' (todo lo demás)

// Pines control
#define selectorModoBLT 2
#define botonStart 3
#define botonStop 4

// Leds
#define Luz_VERDE 5
#define Luz_blutuch 6
#define Luz_ROJO 7

bool COMSBLT = false;

// Pines sensores
#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

// ===== helper: enviar como "<dir><vel>\n" sin String (evita crashes) =====
static void sendCmd(char d, int v) {
  if (v < 0) v = 0;
  if (v > 1023) v = 1023;

  char out[16];
  snprintf(out, sizeof(out), "%c%d\n", d, v);

  Serial1.print(out);

  Serial.print("TX -> ");
  Serial.print(out); // ya incluye \n
}

// ================= BLE =================

void prog(BLEDevice peripheral) {
  Serial.println("Conectando BLE...");

  if (!peripheral.connect()) {
    Serial.println("Error conexión");
    return;
  }

  Serial.println("Conectado");

  if (!peripheral.discoverAttributes()) {
    Serial.println("Error atributos");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic X   = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y   = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");

  if (!X || !Y || !Vel) {
    Serial.println("Característica faltante");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    if (X.canRead() && Y.canRead() && Vel.canRead()) {
      uint8_t bx[4], by[4], bv[4];

      int nx = X.readValue(bx, 4);
      int ny = Y.readValue(by, 4);
      int nv = Vel.readValue(bv, 4);

      // Solo si llegan 4 bytes por float
      if (nx == 4 && ny == 4 && nv == 4) {
        float valX, valY, valVel;

        memcpy(&valX, bx, 4);
        memcpy(&valY, by, 4);
        memcpy(&valVel, bv, 4);

        float t = 0.2f;

        // dirección desde acelerómetro
        if (abs(valX) < t && abs(valY) < t) dir = 'n';
        else if (valX > t && valY > t) dir = 'h';
        else if (valX > t && valY < -t) dir = 'b';
        else if (valX < -t && valY > t) dir = 'f';
        else if (valX < -t && valY < -t) dir = 'd';
        else if (valX > t) dir = 'a';
        else if (valX < -t) dir = 'e';
        else if (valY > t) dir = 'g';
        else if (valY < -t) dir = 'c';

        // Si quieres ignorar valVel y usar velocidades fijas:
        int vOut = (dir == 'a') ? velRecto : velGiro;

        // Enviar al MEGA
        sendCmd(dir, vOut);
      } else {
        Serial.print("BLE read bytes: ");
        Serial.print(nx); Serial.print(", ");
        Serial.print(ny); Serial.print(", ");
        Serial.println(nv);
      }
    }

    delay(100);
  }

  Serial.println("BLE desconectado");
}

// ================= SENSORES =================

char lecturaSensor() {
  char direccion = 'n';

  const uint8_t pins[5] = {
    PinSensor1, PinSensor2, PinSensor3, PinSensor4, PinSensor5
  };

  bool estado[5];

  // Leer sensores
  for (int i = 0; i < 5; i++) {
    estado[i] = digitalRead(pins[i]);
  }

  // Mostrar estados
  Serial.print("Sensores -> ");
  for (int i = 0; i < 5; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    if (estado[i] == HIGH) Serial.print("ON ");
    else Serial.print("OFF ");
    if (i < 4) Serial.print("| ");
  }
  Serial.println();

  // Primer sensor activo
  for (int i = 0; i < 5; i++) {
    if (estado[i] == HIGH) {
      switch (i) {
        case 0: direccion = 'h'; break;
        case 1: direccion = 'k'; break;
        case 2: direccion = 'a'; break;
        case 3: direccion = 'l'; break;
        case 4: direccion = 'b'; break;
      }
      break;
    }
  }

  return direccion;
}

// ================= SETUP =================

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  BLE.begin();

  Serial.println("KUKI INICIADO");
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStart, INPUT);
  pinMode(botonStop, INPUT);

  pinMode(Luz_VERDE, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
  pinMode(Luz_ROJO, OUTPUT);

  // Sensores (HIGH = activo)
  pinMode(PinSensor1, INPUT);
  pinMode(PinSensor2, INPUT);
  pinMode(PinSensor3, INPUT);
  pinMode(PinSensor4, INPUT);
  pinMode(PinSensor5, INPUT);
}

// ================= LOOP =================

void loop() {
  COMSBLT = digitalRead(selectorModoBLT);
  digitalWrite(Luz_blutuch, COMSBLT ? HIGH : LOW);

  BLEDevice peripheral = BLE.available();

  // ----- MODO BLE -----
  if (peripheral && COMSBLT) {
    if (peripheral.localName().indexOf("Mando Kuki") < 0) {
      return;
    }

    BLE.stopScan();
    prog(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }

  // ----- MODO SENSORES -----
  else if (!COMSBLT) {
    dir = lecturaSensor();

    // Si dir == 'a' -> rápido; si no -> lento
    int vOut = (dir == 'a') ? velRecto : velGiro;

    sendCmd(dir, vOut);

    delay(300);
  }
}
