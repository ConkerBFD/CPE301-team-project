// Created on November 04 2020
// Main file for the swamp cooler code, switches to different states. 
// Takes in data from temperature and water level sensor.
// Worked on by Alan Garcia and Gavin Farrell

#include <LiquidCrystal.h>
#include <Wire.h>
#include <dht11.h>
#include "RTClib.h"

RTC_DS1307 rtc;
dht11 DHT11;

// Port A Addresses
volatile unsigned char *port_a = (unsigned char *) 0x23;
volatile unsigned char *myDDRA = (unsigned char *) 0x22;
volatile unsigned char *pin_a = (unsigned char *) 0x21;

// Port B Addresses
volatile unsigned char *myDDRB = (unsigned char *) 0x25;
volatile unsigned char *port_b = (unsigned char *) 0x26;
volatile unsigned char *pin_b = (unsigned char *) 0x24;

// Analog Addresses
volatile unsigned char *myADCSRA = (unsigned char*) 0x7A;
volatile unsigned char *myADCSRB = (unsigned char*) 0x7B;
volatile unsigned char *myADMUX = (unsigned char*) 0x7C;
volatile unsigned int *my_ADC_DATA = (unsigned int*) 0x78;
unsigned char bits_ADMUX[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

// Serial Addresses
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

LiquidCrystal lcd(8, 9, 4,5,6,7);      //RS to 8, E to pin 9, pins 4-7 to D4-D7
unsigned char int_char[10]= {'0','1','2','3','4','5','6','7','8','9'};

//character variable that decides which state the program is in, defaults to Disabled
unsigned char state;

// Temp threshold and current temp
float temp_threshold=20.0f;
float current_temp=0;

// Min water level and current water
unsigned int water_minimum=170;
unsigned int current_water=0;

//days for rtc
char day[7][12]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//each of the following functions is a state of the machine, will change the state variable within.
void Disabled(); //unsure if we need to pass in anything in
void Idle();
void Runnung();
void Error();

//functions to help with the operation
unsigned int button_press();
void all_state(); //functionally for most of the states
void timestamp();

//analog functions
void adc_init();
unsigned int adc_read(unsigned char adc_channel);
void adc_write();

setup(){
  state = 'D'; // at Disabled state by default.
  //we must decide our threshold and minimum water values
  //we must decide the addresses for our outputs and inputs
  *myDDRA |= 0xF0; //set pins 26-29 to outputs for the LEDs
  *myDDRA &= 0xFE; //set pin 22 to input for the push button
  *port_a |= 0x01; //enable pullup resistor on pin 22
  *myDDRB |= 0x01; //set pin 53 to output for the motor
  Serial.begin(9600);
  lcd.begin(16,2);
  rtc.begin();
}

loop(){
  switch(state){ //takes in the state character and changes states accordingly
    // Since we default to disabled, we don't need a case for D, we can just use default.
    case 'I':
      Idle();
      break;
    case 'R':
      Running();
      break;
    case 'E':
      Error();
      break;
    default:
      Disabled(); // Fail-safe, sets to Disabled state
      break;
    }
  }

unsigned int button_press(){
  if(!(*pin_a & 0x01)){
    for(volatile unsigned int i=0; i<1000; i++);//check if the input is only a small error
    
    if(!(*pin_a & 0x01)){
      return 1;  //returns 1 if the button has been properly pressed
    }
    return 0; //returns 0 if the button has not been properly pressed
  }
}
      
//This is the common functionallity of most of the states in this system
void all_state(){
  unsigned int column=15;
  lcd.setCursor(0,0);  //update current water level, send to LCD screen
  lcd.print("wtr_lvl: ");
  current_water=acd_read(0); //water sensor to A0;
  lcd.print(current_water);
  //update current temperature, send to LCD screen
  lcd.setCursor(0,1);
  lcd.print("temp: ");
  int chk = DHT11.read(12);  //temperature input pin to pin 12
  current_temp=(float) DHT11.temperature;
  lcd.print(current_temp);
  //vent position manipulation
  
}

void timestamp(){
  DateTime now= rtc.now();
  Serial.print(day[now.dayOfTheWeek()]);
  Serial.print(" ");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print("/");
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  unsigned int PST_time= now.hour()-16;   //PST timezone
  if(PST_time==0){   //convert to am/pm time
    Serial.print(12, DEC);
  }
  else{
    if(PST_time>12){
      Serial.print(PST_time-12, DEC);
    }
    else{
      Serial.print(PST_time, DEC);
    }
  }
  Serial.print(":");
  Serial.print(now.minute()-2, DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  if(PST_time<12){
    Serial.print(" am");
  }
  else{
    Serial.print(" pm");
  }
}
      
void adc_init(){
  *myADCSRA |= 0x80;
  *myADCSRA &= 0xD0;
  *myADCSRB &= 0xF0;
  *myADMUX &= 0x7F;
  *myADMUX |= 0x40;
  *myADMUX &= 0xC0;
}

unsigned int adc_read(unsigned char adc_channel){
  *myADMUX &= 0xE0;
  *myADCSRB &= 0xF7;
  if(adc_channel >7){
    *myADMUX |= bits_ADMUX[16-adc_channel];
    *myADCSRB |= 0x08; 
  }
  else{
    *myADMUX |= bits_ADMUX[adc_channel];
    *myADCSRB |= 0x00;
  }
  *myADCSRA |= 0x40;
  while((*myADCSRA & 0x40)!=0);
  return *my_ADC_DATA;
}
      
void adc_write(unsigned int data){
  
}
      
unsigned char Disabled(){
  *port_a |= 0x80;    //activate yellow LED (pin 29)
  while(button_press()){  }
  *port_a &= 0x7F;    //deactivate yellow LED
  state = 'I';          //returns I so that it can move to the Idle state
}
      
void Idle(){
  *port_a |= 0x40;       //activate green LED (pin 28)
  unsigned int change = 0; //checks if the state needs to change
  while(!change)
  {
    all_state();
    if(current_water<water_threshold){
      change = 1;
      state = 'E';
    }
    if(current_temp > 20.0f){
      change = 1;
      state = 'R';
    }
    if(button_press()){
      change = 1;
      state = 'D';
    }
  }
  *port_a &= 0xBF; //deactivate green LED
}
      
void Running(){
  *port_a |= 0x20;  //activate blue LED (pin 27)
  *port_b |= 0x01;  //activate motor on pin 53
  timestamp();      //timestamp for motor turning on
  Serial.print("motor turned on");
  Serial.println();
  unsigned int change=0; //checks if the state needs to change
  while(!change) {
    all_state();
    if(current_water>water_threshold){
      change = 1;
      state = 'E';
    }
    if(current_temp < 20.0f){
      change = 1;
      state = 'I';
    }
    if(button_press()){
      change = 1;
      state = 'D';
    }
  }
  *port_a &= 0xDF;  //deactivate blue LED
  *port_b &= 0xFE;  //deactivate motor
  timestamp();      //timestamp for motor turning off
  Serial.print("motor turned off");
  Serial.println();
}
      
void Error(){
  *port_a |= 0x10;  //activate red LED (pin 26)
  lcd.setCursor(11,0);
  lcd.print("error");//display error message on LCD screen
  while(current_water<water_threshold){
    all_state();
    if(button_press()){
      state = 'D';
    }
  }
  state = 'I'        //return to idle once water is above minimum
  *port_a &= 0xEF;  //deactivate red LEd
}
