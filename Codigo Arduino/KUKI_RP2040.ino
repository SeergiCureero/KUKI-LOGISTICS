
#include <ArduinoBLE.h>
//VARIABLES

String msg;
char dir;
int vel = 35;


#define selectorModoBLT 2
bool COMSBLT = false;

#define botonStartSecuencia 3
bool StartSecuencia = false;

#define numeroSecuencias 4
int Secuencias = 0; // Número de secuencias que hace.
int estatAnterior = LOW; // Almacena el estado anterior del botón.
int estatActual = LOW;

#define Luz_start 5
#define Luz_blutuch 6

// El tiempo de la primera instrucción está en la posición 1, no en la 0. La 0 corresponde a los pasos de la última instrucción.
char instrucciones[] =  {     'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n' };    //(recto, para, gira, para) x5 
float pasos[] =         { 1 , 12 ,  1 ,8 ,  1 , 12  ,  1 ,8 ,  1 , 12 ,  1 ,8 ,  1 , 12 ,  1 ,8 ,  1 , 12 ,  1 ,8      };   
int numeroInstruccion = 0;

unsigned long tiempoActual = 0;       // Cuando se alcance este tiempo, se ejecuta cierta parte del código.
unsigned long tiempoAnterior = 0;     // Guardamos la última ejecución.
const unsigned long intervalo = 100;  // 100 ms

void prog(BLEDevice peripheral) {
  // Connect to the peripheral.
  Serial.println("Conectando...");
  if (peripheral.connect()) {
    Serial.println("Conectado :D");
    
  } else {
    Serial.println("FALLO al conectarse!");
    return;
  }
  // Discover peripheral attributes.
  Serial.println("Descubriendo atributos...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Atributos descubiertos");
  } else {
    Serial.println("Descubrimiento de atributos FALLIDO!");
    peripheral.disconnect();
    return;
  }
  // Retrieve the LED characteristic.
  BLECharacteristic X = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");
  if (!X) {
    Serial.println("Periferico no tiene la característica 'X!'");
    peripheral.disconnect();
    return;
  }
  if (!Y) {
    Serial.println("Periferico no tiene la característica 'Y!'");
    peripheral.disconnect();
    return;
  }
  if (!Vel) {
    Serial.println("Periferico no tiene la característica 'velocidad!'");
    peripheral.disconnect();
    return;
  }
  while (peripheral.connected()) {
    // While the peripheral is connected.
    if (X.canRead() && Y.canRead() &&  Vel.canRead()) {
      // Buffers para cada float (4 bytes)
      uint8_t bufX[4], bufY[4], bufVel[4];

      // Leer los valores del periférico.
      X.readValue(bufX, 4);
      Y.readValue(bufY, 4);

      Vel.readValue(bufVel, 4);

      // Convertir los bytes a floats.
      float valX, valY, valVel;
      float threshold = 0.2;
      memcpy(&valX, bufX, sizeof(float));
      memcpy(&valY, bufY, sizeof(float));
      memcpy(&valVel, bufVel, sizeof(float));

      vel = (int)valVel;  
      // Es para traducir los valores del acelerómetro para saber la dirección que hay que mandar.
       if (abs(valX) < threshold && abs(valY) < threshold) dir = 'n'; // centro
        else if (valX > threshold && valY > threshold) dir = 'h';      // arriba-derecha
        else if (valX > threshold && valY < -threshold) dir = 'b';     // arriba-izquierda
        else if (valX < -threshold && valY > threshold) dir = 'f';     // abajo-derecha
        else if (valX < -threshold && valY < -threshold) dir = 'd';    // abajo-izquierda
        else if (valX > threshold) dir = 'a';                           // arriba
        else if (valX < -threshold) dir = 'e';                          // abajo
        else if (valY > threshold) dir = 'g';                           // derecha
        else if (valY < -threshold) dir = 'c';                          // izquierda  

    }


    
  // Si recibe datos del MEGA, los muestra en el PC.
  
    msg = String(dir) + String(vel);
    Serial1.println(msg);
    Serial.print("Mensaje enviado: ");
    Serial.println(msg);

    delay(100);

  }
  Serial.println("Periferico Desconectado");
}


void setup() {
  
  
  Serial1.begin(9600); 
  Serial.begin(9600); // Debug por puerto USB
  // Initialize the BLE hardware.
  BLE.begin();
  Serial.println("KUKI");
  // Start scanning for Button Device BLE peripherals.
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStartSecuencia, INPUT);
  pinMode(numeroSecuencias, INPUT);
  pinMode(Luz_start, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
}

void loop() {

  // Leemos el estado del selector de modo.
  COMSBLT = digitalRead(selectorModoBLT);

  if (COMSBLT){
    digitalWrite(Luz_blutuch, HIGH);
  }
  else{
    digitalWrite(Luz_blutuch, LOW);
  }
  
  estatActual = digitalRead(numeroSecuencias);
  if ((estatAnterior == LOW && estatActual == HIGH) && !StartSecuencia) { 
    Secuencias += 1;
    Serial.println("Num sec: " + String(Secuencias) );
  }
  estatAnterior = estatActual;
  

  // Leemos el estado del botón de StartSecuencia.
  if (digitalRead(botonStartSecuencia)){
    // Inicia la secuencia.
    StartSecuencia = true;
    //Serial.println("a");
    //if(Secuencias == 0){Secuencias = 1;}
  }
    
  // REVISA SI EL NÚMERO DE INSTRUCCIONES Y EL DE PASOS ES EL MISMO, SI NO, AVISA. El código se ejecutará bien y no dará error, pero funcionará mal.
  //(sizeof(instrucciones)/sizeof(instrucciones[0])) se divide el tamaño entero del array entre el tamaño del primer dato del array. sizeof() 
  //da el tamaño en bytes de TODO el array, pero el tamaño no indica el numero de elementos
  int sizeInstrucciones = sizeof(instrucciones)/sizeof(instrucciones[0]);
  int sizePasos = sizeof(pasos)/sizeof(pasos[0]);
  if(sizeInstrucciones < sizePasos){
    Serial.print("_____ERROR Numero de instrucciones MENOR (<) que pasos_____");
    Serial.println(String(sizeInstrucciones) + " < " + String(sizePasos));
  }
  else if(sizeInstrucciones > sizePasos){
    Serial.print("_____ERROR Numero de instrucciones MAYOR (>) que pasos_____");
    Serial.println(String(sizeInstrucciones)  + " > " +  String(sizePasos));
  }

  // Check if a peripheral has been discovered.
  BLEDevice peripheral = BLE.available();
  if (peripheral && COMSBLT) {
    // Encuentra un periférico, coloca la dirección, el nombre local y el servicio anunciado.
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
    // Stop scanning.
    BLE.stopScan();
    
    prog(peripheral);
    
    
    // Peripheral disconnected, start scanning again.
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }

  else if (!COMSBLT){
    // Automático, sin comunicación.
    tiempoActual = millis();
    // Compara si ha pasado el tiempo suficiente para ejecutar la instrucción actual.
    if ((tiempoActual - tiempoAnterior) >= (intervalo*pasos[numeroInstruccion])) {
      tiempoAnterior = tiempoActual;  // Actualiza el contador
      if (StartSecuencia){
        digitalWrite(Luz_start, HIGH);
        Serial.println(Secuencias);
        if(Secuencias == 0){StartSecuencia = false;}  
        
        if(Secuencias > 0){
          
          Serial.println("hola");
          // Ejecuta la secuencia solo si StartSecuencia es true (si se ha pulsado el botón y aún no se ha acabado la secuencia).
          Serial.println("Instrucción número: " + String(numeroInstruccion) + " = " +  instrucciones[numeroInstruccion] + " | Vel:  " + vel);
          msg = instrucciones[numeroInstruccion] + String(vel);
          Serial1.println(msg);


          if(numeroInstruccion < sizeInstrucciones-1){
            numeroInstruccion += 1;
          }
          else{
            numeroInstruccion = 0;
            Secuencias -= 1; 
            
          }
        }
        
      }
      digitalWrite(Luz_start, LOW);
      
    }
  }
}
