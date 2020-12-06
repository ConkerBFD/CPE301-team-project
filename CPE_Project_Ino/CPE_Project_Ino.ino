// Created on November 04 2020
// Main file for the swamp cooler code, switches to different states. 
// Takes in data from temperature and water level sensor.
// Worked on by Alan Garcia and Gavin Farrell

#include <LiquidCrystal.h>
#include <dht_nonblocking.h>
#include <Wire.h>
#include <DS3231.h>
#include <Servo.h>


#define DISABLE 0
#define VENT_UP 1
#define VENT_DOWN 2

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const unsigned int DHT_SENSOR_PIN = 12;

DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );
Servo myservo;

// Clock variables
DS3231 clock;
RTCDateTime now;


// Port A Addresses
volatile unsigned char *port_a = (unsigned char *) 0x22;
volatile unsigned char *myDDRA = (unsigned char *) 0x21;
volatile unsigned char *pin_a = (unsigned char *) 0x20;

// Port B addresses
volatile unsigned char *myDDRB = (unsigned char *) 0x24;
volatile unsigned char *port_b = (unsigned char *) 0x25;
volatile unsigned char *pin_b = (unsigned char *) 0x23;

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

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);      //RS to 8, E to pin 9, pins 4-7 to D4-D7

//character variable that decides which state the program is in, defaults to Disabled
unsigned char state;

// Temp threshold and current temp
float temp_threshold = 20;
float current_temp = 0;
float humidity = 0;

// Min water level and current water
unsigned int water_threshold = 170;
unsigned int current_water = 0;
unsigned int vent_angle = 90;

//days for rtc
const char day[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//each of the following functions is a state of the machine, will change the state variable within.
unsigned char Disabled(); //unsure if we need to pass in anything in
void Idle();
void Runnung();
void Error();

//functions to help with the operation
int button_press();
void all_state(); //functionally for most of the states
void timestamp();

//analog functions
void adc_init();
unsigned int adc_read(unsigned char adc_channel);
void adc_write();

void setup(){
  state = 'D'; // at Disabled state by default.
  *myDDRA |= 0xF0; //set pins 26-29 to outputs for the LEDs
  *myDDRA &= 0xF8; //set pin 22 to input for the push button
  *port_a |= 0x07; //enable pullup resistor on pins 22, 23, 24
  *pin_b |= 0x04; // Set the L293D chip to single direction
  *myDDRB |= 0x07; //set pin 53, 52, and 51 to output for the motor. connect to 

  adc_init();
  
  Serial.begin(9600);

  myservo.attach(3);
  myservo.write(90);
  lcd.begin(16,2);
  clock.begin();
}

void loop(){
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

int button_press(){
  if((*pin_a & 0x01)){
    while (*pin_a&0x01) {}
    return DISABLE; // Returns true if button is pressed
  }
  
  if((*pin_a & 0x02)){
    while (*pin_a&0x02) {
      Serial.print("Moving down ");
      Serial.println(vent_angle);
      if (vent_angle > 60) myservo.write(vent_angle-=2);
   }
    return VENT_UP; // Returns true if button is pressed
  }
  
 if((*pin_a & 0x04)){
    while (*pin_a&0x04) {
      Serial.print("Moving up ");
      Serial.println(vent_angle);
      if (vent_angle < 120) myservo.write(vent_angle+=2);
    }
    return VENT_DOWN; // Returns true if button is pressed
  }
  
  return -1; //returns false if the button has not been properly pressed
}
      
//This is the common functionallity of most of the states in this system
void all_state(){
  
  // Check if the disable button has been pressed, if it has, return early so the program doesnt update and change states again.
  if (button_press() == DISABLE){
    state = (state == 'D' ? 'I' : 'D');
    return;
  }
  
  if (measure_environment(&current_temp, &humidity)){
    print_lcd_data();
    current_water = adc_read(0);
  }
  
  //vent position manipulation
  
}

void print_lcd_data(){
  lcd.clear();
  lcd.setCursor(0,0);
  if (state == 'E') // Print error to LCD if water is too low
  {
    lcd.print("Error: Water Low");
  } 
  else if (state == 'D') // Print disabled to lcd if system is currently disabled
  {
    lcd.print("Disabled");
  } 
  else  // If everything is running normal, continue monitoring info
  {
    lcd.print("T = ");
    lcd.print(current_temp);
    lcd.print(" deg. C");
    lcd.setCursor(0,1);
    lcd.print("H = ");
    lcd.print(humidity);
    lcd.print("%");
  }
}

void timestamp(){
  now = clock.getDateTime();
  
  Serial.print(day[now.day]);
  Serial.print(" ");
  Serial.print(now.month, DEC);
  Serial.print("/");
  Serial.print(now.day, DEC);
  Serial.print("/");
  Serial.print(now.year, DEC);
  Serial.print(" ");
  
  unsigned int PST_time= now.hour-16;   //PST timezone
  
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
  Serial.print(now.minute-2, DEC);
  Serial.print(":");
  Serial.print(now.second, DEC);
  
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
      
unsigned char Disabled(){
  *pin_a |= 0x80;    //activate yellow LED (pin 29)
  
  while(state == 'D')
    all_state();
    
  *port_a &= 0x7F;    //deactivate yellow LED
  state = 'I';          //returns I so that it can move to the Idle state
}
      
void Idle(){
  *pin_a |= 0x40;       //activate green LED (pin 28)
  unsigned int change = 0; //checks if the state needs to change
  while(state == 'I')
  {
    all_state();
    if(current_water<water_threshold)
      state = 'E';
    
    if(current_temp > temp_threshold)
      state = 'R';
  }
  *port_a &= 0xBF; //deactivate green LED
}
      
void Running(){
  *pin_a |= 0x20;  //activate blue LED (pin 27)
  *pin_b |= 0x01;  //activate motor on pin 53

  
  Serial.print("Motor turned on: ");
  timestamp();      //timestamp for motor turning on
  Serial.println();
  
  unsigned int change=0; //checks if the state needs to change
  while(state == 'R') {
    all_state();
    
    if(current_water < water_threshold)
      state = 'E';
   
    if(current_temp < temp_threshold)
      state = 'I';
  
  }
  
  *port_a &= 0xDF;  //deactivate blue LED
  *port_b &= 0xFE;  //deactivate motor
  
  Serial.print("Motor turned off: ");
  timestamp();      //timestamp for motor turning off
  Serial.println();
}
      
void Error(){
  *pin_a |= 0x10;  //activate red LED (pin 26)
  lcd.setCursor(0,0);
  lcd.print("Error: Water Low");//display error message on LCD screen
  while(current_water < water_threshold){
    all_state();
  }
  state = 'I';        //return to idle once water is above minimum
  *port_a &= 0xEF;  //deactivate red LEd
}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );
  
  /* Measure once every four seconds. */
  if( millis( ) - measurement_timestamp > 3000ul)
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      return( true );
    }
  }

  return( false );
}
