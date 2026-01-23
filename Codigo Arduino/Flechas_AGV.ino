
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Cambia 0x27 por 0x3F si tu módulo I2C lo requiere
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables simuladas
float velocidad = 12.3;
int zona = 1;
char direccion = 'w'; // w=arriba, s=abajo, a=izq, d=der, q=arriba-izq, e=arriba-der, z=abajo-izq, c=abajo-der

// ---------------------------
// Flechas rectas
// ---------------------------
byte flechaArriba[8] = {
  0b00100,
  0b01110,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

byte flechaAbajo[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b01110,
  0b00100
};

byte flechaDerecha[8] = {
  0b00000,
  0b00100,
  0b00110,
  0b00111,
  0b11111,
  0b00111,
  0b00110,
  0b00100
};

byte flechaIzquierda[8] = {
  0b00000,
  0b00100,
  0b01100,
  0b11100,
  0b11111,
  0b11100,
  0b01100,
  0b00100
};

// ---------------------------------------------------
// Flechas diagonales
// ---------------------------------------------------

// Arriba-Izquierda 
byte flechaArribaIzq[8] = {
  0b10000, //   #    
  0b11000, //   ##   
  0b11100, //   ###  <-- cabeza triangular
  0b01000, //    #   
  0b00100, //     #  
  0b00000,
  0b00000,
  0b00000
};

// Arriba-Derecha 
byte flechaArribaDer[8] = {
  0b00001, //     #
  0b00011, //    ##
  0b00111, //   ###  <-- cabeza triangular
  0b00010, //     #
  0b00100, //    #
  0b00000,
  0b00000,
  0b00000
};

// Abajo-Izquierda 
byte flechaAbajoIzq[8] = {
  0b00000,
  0b00000,
  0b00100, //    # (cuerpo hacia el centro)
  0b01000, //   #
  0b11100, //  ###  <-- cabeza triangular
  0b11000, //  ##
  0b10000, //  #
  0b00000
};

// Abajo-Derecha 
byte flechaAbajoDer[8] = {
  0b00000,
  0b00000,
  0b00100, //    # (cuerpo hacia el centro)
  0b00010, //     #
  0b00111, //   ### <-- cabeza triangular
  0b00011, //    ##
  0b00001, //     #
  0b00000
};

void setup() {
  lcd.init();
  lcd.backlight();

  // Crear caracteres personalizados (índices 0..7)
  lcd.createChar(0, flechaArriba);
  lcd.createChar(1, flechaAbajo);
  lcd.createChar(2, flechaDerecha);
  lcd.createChar(3, flechaIzquierda);
  lcd.createChar(4, flechaArribaIzq);
  lcd.createChar(5, flechaArribaDer);
  lcd.createChar(6, flechaAbajoIzq);
  lcd.createChar(7, flechaAbajoDer);

  Serial.begin(9600);
  Serial.println(F("Usa w,a,s,d,q,e,z,c para direccion; 1-3 para zona."));
  Serial.println(F("Ejemplo: w (arriba), e (arriba-derecha), 2 (zona 2)"));
}

void loop() {
  // Leer del monitor serie sin bloquear
  while (Serial.available() > 0) {
    char c = Serial.read();
    // Dirección
    if (c == 'w' || c == 'a' || c == 's' || c == 'd' ||
        c == 'q' || c == 'e' || c == 'z' || c == 'c') {
      direccion = c;
    }
    // Zona (1..3)
    if (c == '1' || c == '2' || c == '3') {
      zona = c - '0';
    }
  }

  // Mostrar en LCD
  lcd.setCursor(0, 0);
  lcd.print(F("Vel:"));
  lcd.print(velocidad, 1);
  lcd.print(F("  ")); // espacio para limpiar arrastre

  lcd.print(F("Dir:"));
  // Escribir el carácter de flecha correspondiente
  switch (direccion) {
    case 'w': lcd.write(byte(0)); break; // arriba
    case 's': lcd.write(byte(1)); break; // abajo
    case 'd': lcd.write(byte(2)); break; // derecha
    case 'a': lcd.write(byte(3)); break; // izquierda
    case 'q': lcd.write(byte(4)); break; // arriba-izquierda
    case 'e': lcd.write(byte(5)); break; // arriba-derecha
    case 'z': lcd.write(byte(6)); break; // abajo-izquierda
    case 'c': lcd.write(byte(7)); break; // abajo-derecha
  }

  lcd.setCursor(0, 1);
  lcd.print(F("Zona:"));
  lcd.print(zona);
  lcd.print(F("      ")); // limpiar el resto de la línea

  delay(150);
}

