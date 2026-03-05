#include <ArduinoBLE.h>

// ================= VARIABLES =================
String msg;   // compat/debug
char dir;

// Cuantas lecturas seguidas sin linea toleramos
const uint8_t MAX_MISS = 10;

// Velocitats (0..255)
int velRecto  = 80;
int velGiro1  = 32;
int velGiro2  = 48;
int velGiro3  = 64;

// Tiempos
int tiempoPeriodoLectura = 100;
unsigned long tiempoActualLectura = 0;

// Pines control
#define selectorModoBLT 2
#define botonStart 3
#define botonStop 4

// Leds
#define Luz_VERDE 5
#define Luz_ROJO 6
#define Luz_blutuch 7

bool setRojo = false;   // (no usat, el deixo per compat)
bool setVerde = false;  // (no usat, el deixo per compat)

bool COMSBLT = false;

// Pines sensores
#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

// ===== Comunicación MEGA =====
String msgMega = "";
uint8_t zonaRecibida = 0;  // feedback del MEGA (Zx:YY)

// Nou estat RFID simplificat
uint8_t camino = 0;        // 0=unset, després 1/2/3 (no canvia fàcil)
uint8_t tagLeido = 0;      // 0=first, 1/2/3=station tag, 4=unknown/mismatch

// ================= UART1 LINE READER (NO BLOQUEJANT) =================
static bool readLineSerial1(String &out) {
  static char buf[32];
  static uint8_t idx = 0;

  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (c == '\r') continue;

    if (c == '\n') {
      buf[idx] = '\0';
      out = String(buf);
      idx = 0;
      out.trim();
      return true;
    }

    if (idx < sizeof(buf) - 1) buf[idx++] = c;
    else idx = 0; // overflow -> reset
  }
  return false;
}

// ===== helper: enviar como "<zona><dir><vel>\n" =====
static void sendCmd(uint8_t z, char d, int v) {
  if (z > 3) z = 3;         // permet 0..3
  v = constrain(v, 0, 255);

  char out[16];
  snprintf(out, sizeof(out), "%d%c%d\n", (int)z, d, v);
  Serial1.print(out);
}

// ================= LEDS =================
static void setLeds(bool green, bool red) {
  digitalWrite(Luz_VERDE, green ? HIGH : LOW);
  digitalWrite(Luz_ROJO,  red   ? HIGH : LOW);
}

static void updateLeds() {
  // Camino 1: leds apagats, excepte si tagLeido=4 (alternen)
  if (camino == 1) {
    if (tagLeido == 4) {
      const bool tick = ((millis() / 250) % 2) == 0;
      setLeds(!tick, tick); // alterna verd/vermell
    } else {
      setLeds(false, false);
    }
    return;
  }

  // Camino 2: normal = vermell fix; tagLeido=4 = pampalluga vermell
  if (camino == 2) {
    if (tagLeido == 4) {
      const bool tick = ((millis() / 250) % 2) == 0;
      setLeds(false, tick);
    } else {
      setLeds(false, true);
    }
    return;
  }

  // Camino 3: normal = verd fix; tagLeido=4 = pampalluga verd
  if (camino == 3) {
    if (tagLeido == 4) {
      const bool tick = ((millis() / 250) % 2) == 0;
      setLeds(tick, false);
    } else {
      setLeds(true, false);
    }
    return;
  }

  // Camino 0 (no establert): leds apagats
  setLeds(false, false);
}

// ================= BLE =================
// Actualmente está deshabilitado porque funciona solo en modo manual
/*void prog(BLEDevice peripheral) {
  ...
}*/

// ================= SENSORES =================
char lecturaSensor() {
  static char ultimaDirValida = 'z';
  static uint8_t missCount = 0;

  char direccion = 'z';

  bool sensor1 = digitalRead(PinSensor1);
  bool sensor2 = digitalRead(PinSensor2);
  bool sensor3 = digitalRead(PinSensor3);
  bool sensor4 = digitalRead(PinSensor4);
  bool sensor5 = digitalRead(PinSensor5);

  if (zonaRecibida == 3 && sensor5) {
    direccion = 's';
  } else if (zonaRecibida != 3 && (sensor1 && sensor2)) {
    direccion = 'l';
  } else if (zonaRecibida != 3 && (sensor2 && sensor3)) {
    direccion = 'n';
  } else if (sensor3 && sensor4) {
    direccion = 'p';
  } else if (sensor4 && sensor5) {
    direccion = 'r';
  } else if (sensor1) {
    direccion = 'k';
  } else if (sensor2) {
    direccion = 'm';
  } else if (sensor3) {
    direccion = 'o';
  } else if (sensor4) {
    direccion = 'q';
  } else if (sensor5) {
    direccion = 's';
  }

  if (direccion != 'z') {
    ultimaDirValida = direccion;
    missCount = 0;
    return direccion;
  }

  missCount++;
  if (missCount < MAX_MISS) return ultimaDirValida;
  return 'z';
}

// ================= LEER MEGA (RFID) =================
static inline bool isStationTag(uint8_t t) { return t == 1 || t == 2 || t == 3; }

static void applyTag(uint8_t tag) {
  // Si no és 1/2/3 => desconegut
  if (!isStationTag(tag)) {
    tagLeido = 4;
    return;
  }

  // Primera iteració / camí no establert:
  // només arrenca amb tag 2 o 3
  if (camino == 0 || tagLeido == 0) {
    if (tag == 2) { camino = 2; tagLeido = 2; }
    else if (tag == 3) { camino = 3; tagLeido = 3; }
    else {
      // tag 1: no arrenca, i no toquem tagLeido (segueix 0)
    }
    return;
  }

  // En funció del camí actual
  switch (camino) {
    case 2:
      // En camí 2 només reacciones a tag 1
      if (tag == 1) { camino = 1; tagLeido = 1; }
      else { tagLeido = 4; }   // inclòs tag 2 i tag 3 => mismatch
      break;

    case 3:
      // En camí 3 només reacciones a tag 1
      if (tag == 1) { camino = 1; tagLeido = 1; }
      else { tagLeido = 4; }
      break;

    case 1:
      // En camí 1 reacciones a 2 o 3
      if (tag == 2) { camino = 2; tagLeido = 2; }
      else if (tag == 3) { camino = 3; tagLeido = 3; }
      else { tagLeido = 4; }
      break;

    default:
      tagLeido = 4;
      break;
  }
}

void leerMega() {
  String line;
  if (!readLineSerial1(line)) return;

  line.trim();
  msgMega = line;

  // Format: Zx:YY
  if (msgMega.length() < 5 || msgMega[0] != 'Z' || msgMega[2] != ':') return;

  uint8_t z = (uint8_t)(msgMega[1] - '0');
  if (z > 3) return;
  zonaRecibida = z;

  uint8_t tag = (uint8_t)strtol(msgMega.substring(3).c_str(), nullptr, 16);
  applyTag(tag);

  Serial.print("RX -> ");
  Serial.print(msgMega);
  Serial.print(" | zona=");
  Serial.print(zonaRecibida);
  Serial.print(" | tag=");
  Serial.print(tag);
  Serial.print(" | camino=");
  Serial.print(camino);
  Serial.print(" | tagLeido=");
  Serial.println(tagLeido);
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  BLE.begin();
  Serial.println("RP2040 READY");
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStart, INPUT);
  pinMode(botonStop, INPUT);

  pinMode(Luz_VERDE, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
  pinMode(Luz_ROJO, OUTPUT);

  pinMode(PinSensor1, INPUT);
  pinMode(PinSensor2, INPUT);
  pinMode(PinSensor3, INPUT);
  pinMode(PinSensor4, INPUT);
  pinMode(PinSensor5, INPUT);

  setLeds(false, false);
}

// ================= LOOP =================
void loop() {
  COMSBLT = digitalRead(selectorModoBLT);
  digitalWrite(Luz_blutuch, COMSBLT ? HIGH : LOW);

  BLEDevice peripheral = BLE.available();

  // ----- MODO BLE -----
  /*
  DESHABILITADO
  if (peripheral && COMSBLT) {
    if (peripheral.localName().indexOf("Mando Kuki") < 0) return;

    BLE.stopScan();
    prog(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
    return;
  }*/

  // ----- MODO SENSORES -----
  if (!COMSBLT) {
    if (millis() - tiempoActualLectura >= (unsigned long)tiempoPeriodoLectura) {
      tiempoActualLectura = millis();

      // 1) RFID
      leerMega();

      // 2) LEDs segons estat
      updateLeds();

      // 3) Direcció
      dir = lecturaSensor();

      // 4) Vel
      // k l m n o p q r s
      int vOut = 0;
      char dirOut = dir;

      if (dir == 'o') {
        vOut = velRecto;
      } else if ((dir == 'n') || (dir == 'p')) {
        vOut = velGiro1;
      } else if ((dir == 'm') || (dir == 'q')) {
        vOut = velGiro1;
      } else if ((dir == 'l') || (dir == 'r')) {
        vOut = velGiro2;
      } else if ((dir == 'k') || (dir == 's')) {
        vOut = velGiro3;
      } else {
        // 'z' o altres: stop
        vOut = 0;
      }

      // (Ja no hi ha HOLD ni zonaOrdenada)
      sendCmd(zonaRecibida, dirOut, vOut);

      Serial.print("Dir: ");
      Serial.print(dirOut);
      Serial.print("    Vel: ");
      Serial.print(vOut);
      Serial.print("    ZonaRx: ");
      Serial.print(zonaRecibida);
      Serial.print("    Camino: ");
      Serial.println(camino);
    }
  }
}
