// Created on November 04 2020
// Main file for the swamp cooler code, switches to different states. 
// Takes in data from temperature and water level sensor.
// Worked on by Alan Garcia and Gavin Farrell

#include <LiquidCrystal.h>
#include <dht_nonblocking.h>
#include <Wire.h>
#include <DS3231.h>
#include <Servo.h>


// Button returns for button_press function
#define DISABLE 0
#define VENT_UP 1
#define VENT_DOWN 2

// DHT11 Type and Pin
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const unsigned int DHT_SENSOR_PIN = 12;


// Elegoo Starter Kit Library Objects
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );  // Set temp sensor based on info define above.
Servo myservo;                                                  // Servo motor interface object
DS3231 clock;                                                   // RTC Module Interface object
RTCDateTime now;                                                // Date time contained, used when printing motor timestamp
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);                            // LCD pins, RS to 8, E to pin 9, pins 4-7 to D4-D7


/*
 *  Microcontroller Addresses
 */
 
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


/*
 *  Global variables
 */
 
//character variable that decides which state the program is in, defaults to Disabled
unsigned char state = 'D';

// Temp threshold and current temp
float temp_threshold = 20;
float current_temp = 0;
float humidity = 0;

// Min water level and current water
unsigned int water_threshold = 170;
unsigned int current_water = 0;
unsigned int vent_angle = 90;

// Days for rtc
const char day[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


/* 
 *  Function Prototypes
 */

// State functions
void Disabled();
void Idle();
void Runnung();
void Error();

// Global helper functions
int button_press();
void all_state();
void timestamp();

// Analog functions
void adc_init();
unsigned int adc_read(unsigned char adc_channel);
void adc_write();

// Initialize items to run within the program.
void setup(){
  *myDDRA |= 0xF0; // Set pins 26-29 to outputs for the LEDs
  *myDDRA &= 0xF8; // Set pin 22, 23, and 24 to input for the push buttons
  *port_a |= 0x07; // Enable pullup resistor on pins 22, 23, and 24
  *pin_b |= 0x04;  // Set the L293D chip to single direction
  *myDDRB |= 0x07; // Set pin 53, 52, and 51 to output for the motor.
  
  adc_init(); // Initialize Analog conversion registers
  Serial.begin(9600); // Initialize baud to 9600

  // Initialize Elegoo Library object's pins and settings.
  myservo.attach(3);
  myservo.write(90);
  lcd.begin(16,2);
  clock.begin();
}

void loop(){
  switch(state){ // Takes in the state character and changes states accordingly.
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
      Disabled();
      break;
    }
  }

// Button controller function, returns value based on which button is pressed, returns -1 if no button is pressed.
int button_press(){
  
  // Check if disable button is pressed
  if((*pin_a & 0x01)){
    while (*pin_a&0x01) {}
    return DISABLE; // Returns true if button is pressed
  }

  // Check if vent-down button is pressed
  if((*pin_a & 0x02)){
    while (*pin_a&0x02) {
      Serial.print("Moving down ");
      Serial.println(vent_angle);
      if (vent_angle > 60) myservo.write(vent_angle-=2);
   }
    return VENT_UP; // Returns true if button is pressed
  }

  // Check if vent-up button is pressed
  if((*pin_a & 0x04)){
    while (*pin_a&0x04) {
      Serial.print("Moving up ");
      Serial.println(vent_angle);
      if (vent_angle < 120) myservo.write(vent_angle+=2);
    }
    return VENT_DOWN; // Returns true if button is pressed
  }
  
  return -1; //returns -1 if no buttons have been pressed.
}
      
// This is the common functionallity of all states in this system
void all_state(){
  
  // Check if the disable button has been pressed, and handle vent manipulation
  if (button_press() == DISABLE){
    state = (state == 'D' ? 'I' : 'D');
    return;
  }

  // Measure temp, humidity, and water level every 4 seconds, and update LCD if new data has been read.
  if (measure_environment(&current_temp, &humidity)){
    print_lcd_data();
    current_water = adc_read(0);
  }
}

// Print Errors, Disabled information, and running information to LCD screen based on current state.
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

// Print the current time to the serial monitor.
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

// Initialize analog conversion registers.
void adc_init(){
  *myADCSRA |= 0x80;
  *myADCSRA &= 0xD0;
  *myADCSRB &= 0xF0;
  *myADMUX &= 0x7F;
  *myADMUX |= 0x40;
  *myADMUX &= 0xC0;
}

// Read and convert analog data from the given channel.
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

void Disabled(){
  *pin_a |= 0x80;    //activate yellow LED (pin 29)
  
  while(state == 'D')
    all_state();
    
  *port_a &= 0x7F;    //deactivate yellow LED
}
      
void Idle(){
  *pin_a |= 0x40;       //activate green LED (pin 28)
  while(state == 'I')
  {
    all_state();
    if(current_water<water_threshold) // If water level is too low, send error signal.
      state = 'E';
    
    if(current_temp > temp_threshold) // If temp is too high, start motors. 
      state = 'R';
  }
  *port_a &= 0xBF; //deactivate green LED
}
      
void Running(){
  *pin_a |= 0x20;  //activate blue LED (pin 27)
  *pin_b |= 0x01;  //activate motor on pin 53

  // Print "Motor on" timestamp to serial monitor.
  Serial.print("Motor turned on: ");
  timestamp();
  Serial.println();
  
  while(state == 'R') {
    all_state();
    
    if(current_water < water_threshold) // If water level is too low, send error signal.
      state = 'E';
   
    if(current_temp < temp_threshold) // If temp goes too low, change to Idle state.
      state = 'I';
  
  }
  
  *port_a &= 0xDF;  //deactivate blue LED
  *port_b &= 0xFE;  //deactivate motor

  // Print "Motor off" timestamp
  Serial.print("Motor turned off: ");
  timestamp();
  Serial.println();
}

void Error(){
  *pin_a |= 0x10;  //activate red LED (pin 26)
  
  while(current_water < water_threshold) // Continue displaying error message till water returns to safe level.
    all_state();
  
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
    if( dht_sensor.measure( temperature, humidity ) == true ) // If the measurement is successful, change measurement timestamp and return true
    {
      measurement_timestamp = millis( );
      return( true );
    }
  }
  // If measurement is unsuccessful, return false.
  return( false );
}