//Alán García, 12-02-20, Tests out the functionality of the LCD screen.
#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4,5,6,7);
unsigned char int_char[10]= {'0','1','2','3','4','5','6','7','8','9'};
unsigned int thing=0;

void setup(){
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.print("temp:");
  lcd.setCursor(15,0);
  lcd.print('F');
  }

void loop(){
  unsigned int things=thing;
  unsigned int column=13;
  lcd.setCursor(column, 0);
  do{
    lcd.print(things%10);
    things=things/10;
    lcd.setCursor(--column,0);
    delay(50);
    }while(things!=0);
  thing++;
  }
