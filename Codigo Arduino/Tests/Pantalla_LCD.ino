
#include <LiquidCrystal_I2C.h>

// Dirección I2C (0x27 o 0x3F según el módulo)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

  lcd.backlight();  //color fondo    
  lcd.init();   // enciende la pantalla        
  lcd.setCursor(5, 1);
  lcd.print("Diego");

}

void loop() {

}
