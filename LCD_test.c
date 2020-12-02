//Alán García, 12-02-20, Tests out the functionality of the LCD screen.

volatile unsigned char * myTCCR1A= (unsigned char *) 0x80;
volatile unsigned char * myTCCR1B= (unsigned char *) 0x81;
volatile unsigned char * myTCCR1C= (unsigned char *) 0x82;
volatile unsigned char * myTIMSK1= (unsigned char *) 0x6F;
volatile unsigned int * myTCNT1= (unsigned int *) 0x84;
volatile unsigned char * myTIFR1= (unsigned char *) 0x36;

volatile unsigned char * port_a= (unsigned char *) 0x22;
volatile unsigned char * myDDRA= (unsigned char *) 0x21;

void Delay(unsigned int freq);
void lcd_init();
void lcd_command(unsigned char command);
void lcd_data(unsigned char data);

setup(){
  *myTCCR1A=0x00;
  *myTCCR1B=0x00;
  *myTCCR1C=0x00;
  *myDDRA |= 0xFF;
  lcd_init();
  }

loop(){

  }

void Delay(unsigned int freq){
  double half_period= (1.0/double(freq))/2.0f;
  unsigned int ticks= half_period/0.0000000625;
  *myTCCR1B &=0xF8;
  *myTCNT1= (unsigned int) (65536-ticks);
  *myTCCR1B |= 0b00000001;
  while((*myTIFR1 & 0x01)==0);
  *myTCCR1B &=0xF8;
  *myTIFR1 |=0x01;
  }
  
  void lcd_init(){
    *port_a &= 0xFB //enable E to pin 24
    *port_a |= 0x08; //k pin tp pin 25, backlight on
    lcd_command(0x33);
    lcd_command(0x32);
    lcd_command(0x28);  //4-bit mode, pins 26,27, 28, 29, onto data pins D4, D5, D6, D7 respectfully
    lcd_command(0x0C);
    lcd_command(0x01);  //clear LCD
    Delay(500);
    lcd_command(0x06);  //shift cursor right
    }
  
  void lcd_command(unsigned char command){
    *port_a = command & 0xF0;
    *port_a &= 0xFC; //RS to pin 22, R/W to pin 23
    *port_a |= 0x04;
    Delay(50);
    *port_a &= 0xFB;
    Delay(100);
    *port_a = command<<4;
    *port_a |= 0x04;
    Delay(50);
    *port_a &= 0xFB;
    Delay(100);
    }
  
  void lcd_data(unsigned char data){
    *port_a = data & 0xF0;
    *port_a |= 0x01;  //RS to pin 22
    *port_a &= 0xFD;  //R/W to pin 23
    *port_a |= 0x04;
    Delay(50);
    *port_a &= 0xFB;
    *port_a = data<<4;
    *port_a |= 0x04;
    Delay(50);
    *port_a = 0xFB;
    Delay(100);
    }
  
  void lcd_print(char * str){
    while(*str!= 0){
      lcd_data(*str);
      str++;
      }
    }
  
