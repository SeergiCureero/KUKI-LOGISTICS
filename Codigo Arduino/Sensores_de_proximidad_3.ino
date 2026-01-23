const int trigPins[3] = {2, 4, 6};
const int echoPins[3] = {3, 5, 7};
const int ledPins[3] = {9, 10, 11};

int distancias[3];
unsigned long tiempoAnterior[3] = {0, 0, 0};
bool estadoLed[3] = {false, false, false};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 3; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
  }
}

int medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long tiempo = pulseIn(echoPin, HIGH, 30000);
  if (tiempo == 0) return 999;

  return tiempo * 0.034 / 2;
}

int calcularIntervalo(int distancia) {
  if (distancia > 50) return -1;      // LED apagado si está lejos
  if (distancia > 30) return 800;     // parpadeo lento
  if (distancia > 15) return 300;     // parpadeo rápido
  return 0;                            // muy cerca → LED fijo
}

void loop() {
  unsigned long ahora = millis();

  for (int i = 0; i < 3; i++) {
    distancias[i] = medirDistancia(trigPins[i], echoPins[i]);
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(distancias[i]);
    Serial.println(" cm");

    int intervalo = calcularIntervalo(distancias[i]);

    if (intervalo == -1) {
      digitalWrite(ledPins[i], LOW);  // LED apagado
      estadoLed[i] = false;
    } 
    else if (intervalo == 0) {
      digitalWrite(ledPins[i], HIGH); // LED fijo
    } 
    else {
      if (ahora - tiempoAnterior[i] >= intervalo) {
        tiempoAnterior[i] = ahora;
        estadoLed[i] = !estadoLed[i];
        digitalWrite(ledPins[i], estadoLed[i]);
      }
    }

    delay(60); // evitar interferencias entre sensores
  }

  Serial.println("----------------");
}
