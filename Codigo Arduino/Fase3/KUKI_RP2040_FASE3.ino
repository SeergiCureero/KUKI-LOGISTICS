#include <ArduinoBLE.h>

// ================= BLE PERIPHERAL =================
BLEService kukiService("19b10000-e8f2-537e-4f6c-d104768a1214");

BLECharacteristic charX("19b10001-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 4);
BLECharacteristic charY("19b10002-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 4);
BLECharacteristic charVel("19b10003-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 4);
BLECharacteristic charCamino("19b10004-e8f2-537e-4f6c-d104768a1214", BLERead | BLENotify, 1);     // Arduino → App (solo notifica)
BLECharacteristic charCaminoCmd("19b10009-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 1);  // App → Arduino (comando puntual)
BLECharacteristic charDer("19b10005-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 1);
BLECharacteristic charIzq("19b10006-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 1);
BLECharacteristic charHab("19b10007-e8f2-537e-4f6c-d104768a1214", BLEWrite | BLERead, 1);
BLECharacteristic charSinFilo("19b10008-e8f2-537e-4f6c-d104768a1214", BLERead | BLENotify, 1);   // Arduino → App (sin filo)
BLECharacteristic charParada("19b1000a-e8f2-537e-4f6c-d104768a1214", BLERead | BLENotify, 1);    // Arduino → App (parada emergencia)

// ================= VARIABLES =================
char dir;

const uint8_t MAX_MISS = 10;

unsigned long tiempoParada = 0;
bool enParada = false;

int velRecto = 25;
int velGiro1 = 16;
int velGiro2 = 16;
int velGiro3 = 25;

int vOut = 0;
char dirOut = 'z';
bool apagarMotores = false;

const unsigned long PERIODO_MS = 100;
unsigned long tiempoActualLectura = 0;

float velAuto = 50.0f;  // velocidad automatico, arranca a 50, modificable desde app

#define Luz_VERDE 5
#define Luz_ROJO 6
#define Luz_Man 7
bool apagaLEDs = false;

#define PinSensor1 8
#define PinSensor2 9
#define PinSensor3 10
#define PinSensor4 11
#define PinSensor5 12

String msgMega = "";
uint8_t zonaRecibida = 0;
uint8_t camino = 0;
uint8_t tagLeido = 0;

BLEDevice central;

// ================= CAMINO CON NOTIFY =================
static void setCamino(uint8_t nuevoCamino) {
  camino = nuevoCamino;
  uint8_t c = nuevoCamino;
  charCamino.writeValue(&c, 1);
  Serial.print("setCamino -> ");
  Serial.println(camino);
}

// ================= UART1 LINE READER =================
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
    else idx = 0;
  }
  return false;
}

static void sendCmd(uint8_t z, char d, int v) {
  if (z > 3) z = 3;
  v = constrain(v, 0, 255);
  char out[16];
  snprintf(out, sizeof(out), "%d%c%d\n", (int)z, d, v);
  Serial1.print(out);
}

// ================= LEDS =================
static void setLeds(bool green, bool red) {
  digitalWrite(Luz_VERDE, green ? HIGH : LOW);
  digitalWrite(Luz_ROJO, red ? HIGH : LOW);
}

static void updateLeds() {
  if (!apagaLEDs) {
    if (camino == 1) {
      if (tagLeido == 4) {
        const bool tick = ((millis() / 250) % 2) == 0;
        setLeds(!tick, tick);
      } else setLeds(false, false);
      return;
    }
    if (camino == 2) {
      if (tagLeido == 4) {
        const bool tick = ((millis() / 250) % 2) == 0;
        setLeds(false, tick);
      } else setLeds(false, true);
      return;
    }
    if (camino == 3) {
      if (tagLeido == 4) {
        const bool tick = ((millis() / 250) % 2) == 0;
        setLeds(tick, false);
      } else setLeds(true, false);
      return;
    }
    setLeds(false, false);
  } else {
    setLeds(false, false);
    apagaLEDs = false;
  }
}

// ================= SENSORES =================
char lecturaSensor() {
  static char ultimaDirValida = 'z';
  static uint8_t missCount = 0;

  if (apagarMotores) {
    ultimaDirValida = 'z';
    missCount = MAX_MISS;
    return 'z';
  }

  char direccion = 'z';
  bool sensor1 = digitalRead(PinSensor1);
  bool sensor2 = digitalRead(PinSensor2);
  bool sensor3 = digitalRead(PinSensor3);
  bool sensor4 = digitalRead(PinSensor4);
  bool sensor5 = digitalRead(PinSensor5);

  if (camino == 3 && (sensor5 || (sensor4 && sensor5) || (sensor3 && sensor5) || (sensor3 && sensor4 && sensor5))) direccion = 's';
  else if (camino == 2 && (sensor1 && sensor2)) direccion = 'l';
  else if (camino == 2 && (sensor2 && sensor3)) direccion = 'n';
  else if (sensor3 && sensor4) direccion = 'p';
  else if (camino == 3 && (sensor4 && sensor5)) direccion = 'r';
  else if (sensor1) direccion = 'k';
  else if (sensor2) direccion = 'm';
  else if (sensor3) direccion = 'o';
  else if (sensor4) direccion = 'q';

  Serial.print("Direccion (sensor): ");
  Serial.println(direccion);

  if (direccion != 'z') {
    ultimaDirValida = direccion;
    missCount = 0;

    uint8_t sinFilo = 0;
    charSinFilo.writeValue(&sinFilo, 1);

    return direccion;
  }

  missCount++;
  if (missCount < MAX_MISS) return ultimaDirValida;
  else {
    const bool tick = ((millis() / 250) % 2) == 0;
    setLeds(!tick, tick);

    uint8_t sinFilo = 1;
    charSinFilo.writeValue(&sinFilo, 1);

    return 'z';
  }
}

// ================= LEER MEGA =================
static inline bool isStationTag(uint8_t t) {
  return t == 255 || t == 2 || t == 3;
}

static void applyTag(uint8_t tag) {
  if (!isStationTag(tag)) {
    tagLeido = 4;
    return;
  }
  if (camino == 0 || tagLeido == 0) {
    if (tag == 2) {
      setCamino(2);
      tagLeido = 2;
    } else if (tag == 3) {
      setCamino(3);
      tagLeido = 3;
    }
    return;
  }
  switch (camino) {
    case 2:
      if (tag == 255) {
        apagarMotores = true;
        apagaLEDs = true;
        setCamino(1);
        tagLeido = 1;
      } else tagLeido = 4;
      break;
    case 3:
      if (tag == 255) {
        apagaLEDs = true;
        setCamino(1);
        tagLeido = 1;
        apagarMotores = true;
      } else tagLeido = 4;
      break;
    case 1:
      if (tag == 2) {
        setCamino(2);
        tagLeido = 2;
      } else if (tag == 3) {
        setCamino(3);
        tagLeido = 3;
      } else tagLeido = 4;
      break;
    default: tagLeido = 4; break;
  }
}

void leerMega() {
  String line;
  if (!readLineSerial1(line)) return;
  line.trim();

  // ── Mensaje de parada de emergencia desde Mega ──
  if (line.length() >= 2 && line[0] == 'P') {
    uint8_t parada = (line[1] == '1') ? 1 : 0;
    charParada.writeValue(&parada, 1);
    Serial.print("Parada emergencia -> ");
    Serial.println(parada);
    return;
  }

  // ── Mensaje RFID normal ──
  msgMega = line;
  if (msgMega.length() < 5 || msgMega[0] != 'Z' || msgMega[2] != ':') return;
  uint8_t z = (uint8_t)(msgMega[1] - '0');
  if (z > 3) return;
  zonaRecibida = z;
  uint8_t tag = (uint8_t)strtol(msgMega.substring(3).c_str(), nullptr, 16);
  applyTag(tag);
  Serial.print("RX -> ");
  Serial.print(msgMega);
  Serial.print(" | camino=");
  Serial.print(camino);
  Serial.print(" | tagLeido=");
  Serial.println(tagLeido);
}

// ================= LEER COMANDO CAMINO DESDE APP =================
static void leerCaminoCmd() {
  uint8_t bufCmd[1];
  charCaminoCmd.readValue(bufCmd, 1);
  uint8_t caminoCmd = bufCmd[0];
  Serial.println(caminoCmd);

  // 255 = sin comando pendiente (centinela)
  if (caminoCmd != 255 && caminoCmd != camino) {
    setCamino(caminoCmd);
    Serial.print("App pidio camino: ");
    Serial.println(caminoCmd);
  }

  // Limpiar poniendo 255 para no aplicarlo otra vez
  if (caminoCmd != 255) {
    uint8_t nada = 255;
    charCaminoCmd.writeValue(&nada, 1);
  }
}

// ================= MODO AUTOMATICO =================
void modoAutomatico() {
  digitalWrite(Luz_Man, LOW);  // apagar luz manual en automatico

  leerMega();

  // Leer comando de camino y velocidad desde la app
  if (central) {
    leerCaminoCmd();

    // Leer velocidad desde app, actualizar velAuto si manda algo valido
    uint8_t bufVel[4];
    charVel.readValue(bufVel, 4);
    float velRecibida;
    memcpy(&velRecibida, bufVel, sizeof(float));
    if (velRecibida > 0) velAuto = velRecibida;
  }

  updateLeds();

  float valVel = velAuto;

  if (camino == 0 || camino == 4) {
    valVel = 0;
    dirOut = 'z';
  } else {
    int velRectoAuto = (int)valVel;
    int velGiro1Auto = (int)(valVel * 0.64f);
    int velGiro2Auto = (int)(valVel * 0.64f);
    int velGiro3Auto = (int)valVel;

    dir = lecturaSensor();
    dirOut = dir;

    if (apagarMotores) {
      apagarMotores = false;
      enParada = true;
      tiempoParada = millis();
    }

    if (enParada) {
      vOut = 0;
      dirOut = 'z';
      if (millis() - tiempoParada >= 2000) enParada = false;
    } else if (tagLeido == 4) {
      vOut = 0;
      dirOut = 'z';
    } else {
      if (dir == 'o') vOut = velRectoAuto;
      else if (dir == 'n' || dir == 'p') vOut = velGiro1Auto;
      else if (dir == 'm' || dir == 'q') vOut = velGiro1Auto;
      else if (dir == 'l' || dir == 'r') vOut = velGiro2Auto;
      else if (dir == 'k' || dir == 's') vOut = velGiro3Auto;
      else vOut = 0;
    }
  }

  sendCmd(camino, dirOut, vOut);

  Serial.print("[AUTO] TX <- camino=");
  Serial.print(camino);
  Serial.print(" | dir=");
  Serial.print(dirOut);
  Serial.print(" | vel=");
  Serial.println(vOut);
}

// ================= MODO MANUAL BLE =================
void modoManualBLE() {
  digitalWrite(Luz_Man, HIGH);  // encender luz manual

  uint8_t bufX[4], bufY[4], bufVel[4];
  uint8_t bufDer[1], bufIzq[1];

  charX.readValue(bufX, 4);
  charY.readValue(bufY, 4);
  charVel.readValue(bufVel, 4);
  charDer.readValue(bufDer, 1);
  charIzq.readValue(bufIzq, 1);

  float valX, valY, valVel;
  memcpy(&valX, bufX, sizeof(float));
  memcpy(&valY, bufY, sizeof(float));
  memcpy(&valVel, bufVel, sizeof(float));

  bool derecha = (bufDer[0] != 0);
  bool izquierda = (bufIzq[0] != 0);

  vOut = (int)valVel;

  const float threshold = 0.2f;
  if (abs(valX) < threshold && abs(valY) < threshold) dirOut = 'z';
  else if (valX > threshold && valY > threshold) dirOut = 'f';
  else if (valX > threshold && valY < -threshold) dirOut = 'h';
  else if (valX < -threshold && valY > threshold) dirOut = 'd';
  else if (valX < -threshold && valY < -threshold) dirOut = 'b';
  else if (valX > threshold) dirOut = 'g';
  else if (valX < -threshold) dirOut = 'c';
  else if (valY > threshold) dirOut = 'e';
  else if (valY < -threshold) dirOut = 'a';
  if (derecha) dirOut = 'j';
  else if (izquierda) dirOut = 'i';

  sendCmd(camino, dirOut, vOut);

  Serial.print("[MANUAL] TX");
  Serial.print(" | dir=");
  Serial.print(dirOut);
  Serial.print(" | vel=");
  Serial.println(vOut);
}

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial1.begin(9600);
  Serial.println("RP2040 READY");

  pinMode(Luz_VERDE, OUTPUT);
  pinMode(Luz_ROJO, OUTPUT);
  pinMode(Luz_Man, OUTPUT);  // luz manual

  pinMode(PinSensor1, INPUT);
  pinMode(PinSensor2, INPUT);
  pinMode(PinSensor3, INPUT);
  pinMode(PinSensor4, INPUT);
  pinMode(PinSensor5, INPUT);

  setLeds(false, false);
  digitalWrite(Luz_Man, LOW);

  // ── BLE Peripheral ──
  if (!BLE.begin()) {
    Serial.println("BLE FALLO");
    while (1);
  }

  BLE.setLocalName("Mando Kuki");
  BLE.setDeviceName("Mando Kuki");

  kukiService.addCharacteristic(charX);
  kukiService.addCharacteristic(charY);
  kukiService.addCharacteristic(charVel);
  kukiService.addCharacteristic(charCamino);
  kukiService.addCharacteristic(charCaminoCmd);
  kukiService.addCharacteristic(charDer);
  kukiService.addCharacteristic(charIzq);
  kukiService.addCharacteristic(charHab);
  kukiService.addCharacteristic(charSinFilo);
  kukiService.addCharacteristic(charParada);

  BLE.setAdvertisedService(kukiService);
  BLE.addService(kukiService);

  float cero = 0.0f;
  uint8_t ceroB = 0;
  uint8_t nadaCmd = 255;
  charX.writeValue((uint8_t *)&cero, 4);
  charY.writeValue((uint8_t *)&cero, 4);
  charVel.writeValue((uint8_t *)&cero, 4);
  charCamino.writeValue(&ceroB, 1);
  charCaminoCmd.writeValue(&nadaCmd, 1);
  charDer.writeValue(&ceroB, 1);
  charIzq.writeValue(&ceroB, 1);
  charHab.writeValue(&ceroB, 1);
  charSinFilo.writeValue(&ceroB, 1);
  charParada.writeValue(&ceroB, 1);

  BLE.advertise();
  Serial.println("BLE anunciando como 'Mando Kuki'");
}

// ================= LOOP =================
void loop() {
  central = BLE.central();

  if (central) {
    Serial.print("App conectada: ");
    Serial.println(central.address());

    while (central.connected()) {
      unsigned long ahora = millis();
      if (ahora - tiempoActualLectura >= PERIODO_MS) {
        tiempoActualLectura = ahora;

        uint8_t bufHab[1];
        charHab.readValue(bufHab, 1);
        bool habManual = (bufHab[0] != 0);

        if (habManual) modoManualBLE();
        else modoAutomatico();
      }
    }

    Serial.println("App desconectada.");

  } else {
    // Sin conexion BLE: automatico puro
    unsigned long ahora = millis();
    if (ahora - tiempoActualLectura >= PERIODO_MS) {
      tiempoActualLectura = ahora;
      modoAutomatico();
    }
  }
}
