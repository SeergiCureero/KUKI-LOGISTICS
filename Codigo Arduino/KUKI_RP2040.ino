#include <ArduinoBLE.h>
//VARIABLES

String msg;
char dir;
int vel;

//TODO definir pin input de selectorModoBLT y botonStartSecuencia
#define selectorModoBLT 2
bool COMSBLT = false;

#define botonStartSecuencia 3
bool startSecuencia = false;

#define selectorEscribeSecuencia 4
bool escribirSecuencia = false;
char secuenciaManual[16];


//Joystick para escribir la secuencia
//TODO COMPLETAR
#define val_X A0
#define val_Y A1
#define boton_ok 5
#define botonGiroIzquierdas 6
#define botonGiroDerechas 7
int threshold_X;
int threshold_Y;


char instrucciones[] =  {     'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n', 'a', 'n', 'j', 'n' }; 
float pasos[] =         { 1 , 10 ,  1 ,8.5 ,  1 , 10 ,  1 ,8.5 ,  1 , 10 ,  1 ,8.5 ,  1 , 10 ,  1 ,8.5 ,  1 , 10 ,  1 ,8.5       };   //el tiempo de la primera instruccion esta en la posición 1, no en la 0. la 0 corresponde a los pasos de la ultima instruccion.
int numeroInstruccion = 0;

unsigned long tiempoActual = 0;       //cuando se alcance este tiempo se ejecuta cierta parte del codigo
unsigned long tiempoAnterior = 0;     // Guardamos la ultima ejecucion
const unsigned long intervalo = 100;  // 100 ms

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
  // initialize the BLE hardware
  BLE.begin();
  Serial.println("KUKI");
  // start scanning for Button Device BLE peripherals
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
}

void loop() {

  //leemos el estado del selector de modo
  COMBLT = digitalRead(selectorModoBLT);


  //leemos el estado del boton de StartSecuencia
  if digitalRead(botonStartSecuencia){
    //inicia la secuencia
    startSecuencia = true;
  }

  //leemos el estado del selector de escribir secuencia
  escribirSecuencia = digitalRead(selectorEscribeSecuencia);
    
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

  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();
  if (peripheral && COMSBLT) {
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

  else if (!COMSBLT){
    //automatico, sin comunicacion. a elegir entre escribir secuencia y secuencia predefinida

    //modo escribir secuencia
    if (escribirSecuencia)
    {
      //modo de secuencia manual
      //se registra una secuencia y cuando se le da a marcha se ejecuta
      int i = 0;
      if (!startSecuencia && (secuenciaManual[0] = '0') && (i<((sizeof(secuenciaManual)/sizeof(secuenciaManual[0]))-1)))
      {
        //secuencia vacia. Llenar
        char direccion = 'n';
        /*
        El mensaje que recibirá de la master sera un int con la dirección en la que se debe mover seguido de la velocidad. 
        Direcciones:
              a
            h | b
             \|/
          g---+---c 
             /|\
            f | d
              e
        
        Además, se deberá considerar movimientos con puntos de giro fuera del centro del AGV (como los de un coche normal)
        - giro a izquierdas: i
        - giro a derechas: j
        */
        
        
        bool derecha = (analogRead(val_X) > threshold_X);
        bool izquierda = (analogRead(val_X) < threshold_X * -1);
        bool adelante = (analogRead(val_Y) > threshold_Y);
        bool atras = (analogRead(val_Y) < threshold_Y * -1);
        bool giroIzquierda = botonGiroIzquierdas;
        bool giroDerecha = botonGiroDerechas;
        bool centroX = !derecha && !izquierda;
        bool centroY = !adelante && !atras;

        if (adelante && centroX)
        {
          direccion = 'a';
        }
        else if (adelante && derecha)
        {
          direccion = 'b';
        }
        else if (centroY && derecha)
        {
          direccion = 'c';
        }
        else if (atras && derecha)
        {
          direccion = 'd';
        }
        else if (atras && centroX)
        {
          direccion = 'e';
        }
        else if (atras && izquierda)
        {
          direccion = 'f';
        }
        else if (centroY && izquierda)
        {
          direccion = 'g';
        }
        else if (adelante && izquierda)
        {
          direccion = 'h';
        }
        else if (centroX && centroY)
        {
          if (giroIzquierda){
            direccion = 'i';
          } 
          else if (giroDerecha && )
          {
            direccion = 'j';
          }
          else
          {
            direccion = 'n'
          } 
        }
        
        
        tiempoActual = millis();
        //guarda la direccion cada X tiempo (100ms*5), esto evita que guardemos muchas instrucciones que no queremos. El boton no registra un flanco, asi que solo entraremos cuando toque.
        if (boton_ok && ((tiempoActual - tiempoAnterior) >= intervalo*5))
        {
          tiempoAnterior = tiempoActual;  // Actualiza el contador
          secuenciaManual[i] = direccion;
          i++
        }
      }
      
      else if (startSecuencia){
        //igual que modo automatico pero con la lista cambiada
//TODO: no duplicar este codigo
        vel = 50;
        tiempoActual = millis();

        //valor de intervalo FIJO
        if ((tiempoActual - tiempoAnterior) >= intervalo*10) {
          tiempoAnterior = tiempoActual;  // Actualiza el contador
          if (startSecuencia)
          {
            //ejecuta la secuencia solo si startSecuencia es true (si se ha pulsado el boton y aun no se ha acabado la secuencia)
            Serial.println("Instruccion numero: " + String(numeroInstruccion) + " = " +  secuenciaManual[numeroInstruccion] + " | Vel:  " + vel);
            msg = secuenciaManual[numeroInstruccion] + String(vel);
            Serial1.println(msg);

            //15 = numero de instrucciones (en vdd son 16, pero no hay instruccion 16, llega hasta la 15)
            if(numeroInstruccion < 15){
              numeroInstruccion += 1;
            }
            else{
              numeroInstruccion = 0;
              //para la secuencia
              startSecuencia = false;
            }
          }         
        }
      }
    }

    //modo automatico
    else
    {
      //MODO AUTOMATICO
      vel = 50;
      tiempoActual = millis();

      if ((tiempoActual - tiempoAnterior) >= (intervalo*pasos[numeroInstruccion])) {
        tiempoAnterior = tiempoActual;  // Actualiza el contador
        if (startSecuencia)
        {
          //ejecuta la secuencia solo si startSecuencia es true (si se ha pulsado el boton y aun no se ha acabado la secuencia)
          Serial.println("Instruccion numero: " + String(numeroInstruccion) + " = " +  instrucciones[numeroInstruccion] + " | Vel:  " + vel);
          msg = instrucciones[numeroInstruccion] + String(vel);
          Serial1.println(msg);


          if(numeroInstruccion < sizeInstrucciones-1){
            numeroInstruccion += 1;
          }
          else{
            numeroInstruccion = 0;
            //para la secuencia
            startSecuencia = false;
          }
        }
        
        
      }
    }
    
    
    
  }
}
