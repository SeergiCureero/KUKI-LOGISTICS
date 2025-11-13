#include <ArduinoBLE.h>
//VARIABLES

// Mensaje para enviar por serial al MEGA
String msg;
char dir;
int vel = 35;

#define selectorModoBLT 2
bool COMSBLT = false;

#define botonStartSecuencia 3
bool StartSecuencia = false;

#define numeroSecuencias 4
int Secuencias = 0; //numero de secuencias que hace
int estatAnterior = LOW; // Emmagatzema l'estat anterior del botó
int estatActual = LOW;

// estos son led de estado aun no habilitados
#define Luz_start 5
#define Luz_blutuch 6

// estas son las intrucciones para que haga la figura 
char instrucciones[] =  {     'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n' }; 
float pasos[] =         { 1 , 10 ,  1 ,7 ,  1 , 10 ,  1 ,7 ,  1 , 10 ,  1 ,7 ,  1 , 10 ,  1 ,7 ,  1 , 10 ,  1 ,7       };   //el tiempo de la primera instruccion esta en la posición 1, no en la 0. la 0 corresponde a los pasos de la ultima instruccion.
int numeroInstruccion = 0;

unsigned long tiempoActual = 0;       //cuando se alcance este tiempo se ejecuta cierta parte del codigo
unsigned long tiempoAnterior = 0;     // Guardamos la ultima ejecucion
const unsigned long intervalo = 100;  // 100 ms

void prog(BLEDevice peripheral) {
  // conectando peripheral
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.println("Connected");
    digitalWrite(Luz_blutuch, HIGH);
  } else {
    Serial.println("Failed to connect!");
    return;
  }
  // Descubrir atributos del mando 
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }
  // Las caracteristicas del peripheral del mando 
  BLECharacteristic X = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Y = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
  BLECharacteristic Vel = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");
  if (!X) {
    Serial.println("Peripheral no encuentra la caracteristica de X");
    peripheral.disconnect();
    return;
  }
  if (!Y) {
    Serial.println("Peripheral no encuentra la caracteristica de Y");
    peripheral.disconnect();
    return;
  }
  if (!Vel) {
    Serial.println(Peripheral no encuentra la caracteristica de Vel);
    peripheral.disconnect();
    return;
  }
  while (peripheral.connected()) {
    // Siempre que el preipheral este conecado
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
        else if (valX > threshold && valY > threshold) dir = 'h';      // arriba-derecha
        else if (valX > threshold && valY < -threshold) dir = 'b';     // arriba-izquierda
        else if (valX < -threshold && valY > threshold) dir = 'f';     // abajo-derecha
        else if (valX < -threshold && valY < -threshold) dir = 'd';    // abajo-izquierda
        else if (valX > threshold) dir = 'a';                           // arriba
        else if (valX < -threshold) dir = 'e';                          // abajo
        else if (valY > threshold) dir = 'g';                           // derecha
        else if (valY < -threshold) dir = 'c';                          // izquierda  

    }


    
  // Si recibe datos del MEGA, los muestra en el PC
  
    msg = String(dir) + String(vel);
    Serial1.println(msg);
    Serial.print("Mensaje enviado: ");
    Serial.println(msg);

    delay(100);

  }
  Serial.println("Peripheral disconnected");
}


void setup() {
  // Pin GPIO5 com a RX
  // Pin GPIO4 com a TX
  
  Serial1.begin(9600); // UART1: RX=5, TX=4 
  Serial.begin(9600); // Debug por puerto USB
  // inicializar el modulo bluetooth del RP2040 KUKI
  BLE.begin();
  Serial.println("KUKI");
  // Empieza a buscar el mando por el modulo bluetooth
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  pinMode(selectorModoBLT, INPUT);
  pinMode(botonStartSecuencia, INPUT);
  pinMode(numeroSecuencias, INPUT);
  pinMode(Luz_start, OUTPUT);
  pinMode(Luz_blutuch, OUTPUT);
}

void loop() {

  //leemos el estado del selector de modo
  COMSBLT = digitalRead(selectorModoBLT);

  
  
  estatActual = digitalRead(numeroSecuencias);
  if ((estatAnterior == LOW && estatActual == HIGH) && !StartSecuencia) { 
    Secuencias += 1;
    Serial.println("Num sec: " + String(Secuencias) );
  }
  estatAnterior = estatActual;
  

  //leemos el estado del boton de StartSecuencia
  if (digitalRead(botonStartSecuencia)){
    //inicia la secuencia
    StartSecuencia = true;
    Serial.println("a");
    if(Secuencias == 0){Secuencias = 1;}
  }
    
  //REVISA SI EL NUMERO DE INSTRUCCIONES Y EL DE PASOS ES EL MISMO, SI NO, AVISA. El codigo se ejecutará bien y no dara error pero funcionará mal
  //(sizeof(instrucciones)/sizeof(instrucciones[0])) se divide el tamaño entero del array entre el tamaño del primer dato del array. sizeof() da el tamaño en bytes de TODO el array, pero el tamaño no indica el numero de elementos
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

  // Comprueba si peripheral ha sido descubierto
  BLEDevice peripheral = BLE.available();
  // siempre que peripheral y el selector de bluetooth este activo
  if (peripheral && COMSBLT) {
    
    // A descubierto a peripheral, imprimimos la dirección, el nombre local y el servicio
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
   
    if (peripheral.localName().indexOf("Mando Kuki") < 0) {
      Serial.println("mando kuki no encontrado");
      return; 
    }
    // para de escanear
    BLE.stopScan();
    
    prog(peripheral);
    
    
    // Peripheral desconectado, lo vuelve a buscar
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }

    // Si el selector de bluetooth esta desactivoado entonces entra en modo automatico
  else if (!COMSBLT){
    //automatico, sin comunicacion
    tiempoActual = millis();
    digitalWrite(Luz_blutuch, LOW);

    if ((tiempoActual - tiempoAnterior) >= (intervalo*pasos[numeroInstruccion])) {
      tiempoAnterior = tiempoActual;  // Actualiza el contador
      if (StartSecuencia)
      {
        Serial.println(Secuencias);
        if(Secuencias == 0){StartSecuencia = false;}  
        
        if(Secuencias > 0){
          digitalWrite(Luz_start, HIGH);
          Serial.println("hola");
          //ejecuta la secuencia solo si StartSecuencia es true (si se ha pulsado el boton y aun no se ha acabado la secuencia)
          Serial.println("Instruccion numero: " + String(numeroInstruccion) + " = " +  instrucciones[numeroInstruccion] + " | Vel:  " + vel);
          msg = instrucciones[numeroInstruccion] + String(vel);
          Serial1.println(msg);


          if(numeroInstruccion < sizeInstrucciones-1){
            numeroInstruccion += 1;
          }
          else{
            numeroInstruccion = 0;
            Secuencias -= 1; 
            digitalWrite(Luz_start, LOW);
          }
        }
        
      }
      
      
    }
  }
}
