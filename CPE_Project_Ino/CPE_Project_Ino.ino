// Created on November 04 2020
// Main file for the swamp cooler code, switches to different states. 
// Takes in data from temperature and water level sensor.
// Worked on by Alan Garcia and Gavin Farrell

// include the needed elegoo library code:
#include <LiquidCrystal.h>
#include <dht_nonblocking.h>
#include <DS3231.h>

#define DHT_SENSOR_TYPE DHT_TYPE_11

#define RDA 0x80
#define TBE 0x20  

#define ENABLE 5
#define DIRA 3
#define DIRB 4

#define GREEN 0
#define YELLOW 1
#define RED 2
#define BLUE 3

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

// Serial Addresses
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

// Common variables
static const unsigned int DHT_SENSOR_PIN = 2;

unsigned char current_state = 'D';

static float humidity = 0;
static int water_current = 0;
static int water_threshold = 100;
static float temp_current = 0;
static float temp_threshold = 15;

static unsigned int LEDs = 0x0F;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

DS3231 clock;
RTCDateTime dt;

void setup(){
  U0init(9600);
  adc_init();
  
  lcd.begin(16,2);
  clock.begin();
  clock.setDateTime(__DATE__,__TIME__);

  *myDDRA |= 0x0F;
  *myDDRB |= 0x0E;
  *port_a |= 0x10;
  *pin_b |= 0b00000100;
}

void loop(){
  switch (current_state){
    case 'R':
      LEDs = 0b00001000;
      if ((*pin_a&LEDs) != LEDs){
        LEDs &= (0x01 << BLUE);
        LEDs |= (0x01 << BLUE);
      }
      run_cooler();
      break;
    case 'I':
      LEDs = 0b00000001;
      if ((*pin_a&LEDs) != LEDs){
        LEDs &= (0x01 << GREEN);
        LEDs |= (0x01 << GREEN);
      }
      idle_cooler();
      break;
    case 'E':
      LEDs = 0b00000100;
      if ((*pin_a&LEDs) != LEDs){
        LEDs &= (0x01 << RED);
        LEDs |= (0x01 << RED);
      }
      error_cooler();
      break;
    default:
      LEDs = 0b00000010;
      if ((*pin_a&LEDs) != LEDs){
        LEDs &= (0x01 << YELLOW);
        LEDs |= (0x01 << YELLOW);
      }
      disable_cooler();
      break;
  }
  
}

void run_cooler(){
  if ((*pin_b&0b00001000) == 0){
    *port_b |= 0b00001000; //Turn motors on
    print_date(true);
  }
  data_update();
  
  if (temp_current < temp_threshold)
    current_state = 'I';

  if (water_current < water_threshold)
    current_state = 'E';
  
  if ((*pin_b&0b00001000) && current_state != 'R'){
    *port_b &= 0b11110111; //Turn motors on
    print_date(false);
  }
}

void idle_cooler(){
  
  data_update(); // Update and print temp data
  
  if (temp_current > temp_threshold) // If temp is too high, run the cooler
    current_state = 'R';

  if (water_current < water_threshold) // If water is too low, enter error state
    current_state = 'E';
}

void error_cooler(){
  data_update();
  if (water_current > water_threshold) // Wait until water is back to normal
    current_state = 'I';
}

void disable_cooler(){
  data_update();
}

void data_update(){
  measure_environment(&temp_current, &humidity);
  if ((*pin_a&LEDs) != LEDs){
    *pin_a |= LEDs;
    *pin_a &= LEDs;
  }
  if ((*pin_a & 0x10) != 0){ 
    current_state = (current_state == 'D' ? 'I' : 'D');  // Change the state
    while (*pin_a&0x10);   // Wait until the user releases the button to continue going
  }
}

void print_date(bool on){
  dt = clock.getDateTime();
  if (on)
    U0print("On - ");
  else 
    U0print("Off - ");
  
  U0printInt(dt.year);
  U0putchar('-');
  U0printInt(dt.month);
  U0putchar('-');
  U0printInt(dt.day);
  U0putchar(' ');
  U0printInt(dt.hour);
  U0putchar(':');
  U0printInt(dt.minute);
  U0putchar(':');
  U0printInt(dt.second);
  U0putchar('\n');
}

// Read the analog data from desired pin
unsigned int adc_read(unsigned char adc_channel_number)
{
  //Clear channel selection bits
  *myADMUX &= 0b11100000;
  *myADCSRB &= 0xFF;

  //Adjust ADMUX to read input from given channel
  for (int i = 0; i < adc_channel_number; i++, (*myADMUX)+=1);
  
  //Start the conversion
  *myADCSRA |= 0x40;
  
  //Wait for conversions to be complete
  while ((*myADCSRA & 0x40) != 0);

  //Return the data
  return *my_ADC_DATA;
}

// Initialize the analog digital converter addresses.
void adc_init()
{
  //setup ADCSRA
  *myADCSRA |= 0b10000000;
  *myADCSRA &= 0b11010000;

  //setup ADCSRB
  *myADCSRB &= 0b11110000;

  //setup ADMUX
  *myADMUX &= 0b01000000;
  *myADMUX |= 0b01000000; 
}

// Prints a string constant
void U0print(const char *string){
  while (*string != '\0'){
    U0putchar(*string);
    string++;
  }
}

// Prints an integer using recursive formula
void U0printInt(int number){
  if (number > 10)
    U0printInt(number/10); 
  
  U0putchar('0' + number%10);
}

// Initialize USART0 to "int" Baud
void U0init(unsigned long U0baud)
{
 unsigned long FCPU = 16000000; // Clock speed
 unsigned int tbaud; // Baud rate
 tbaud = (FCPU / 16 / U0baud - 1); 
 // Same as (FCPU / (16 * U0baud)) - 1;
 // Sets I/O control registers to establish UDR mode, currently set to standard.
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud; 
}

// Read USART0 RDA status bit and return non-zero true if set
unsigned char U0kbhit()
{
   return (((*myUCSR0A)&RDA) == RDA);
}

// Read input character from USART0 input buffer
unsigned char U0getchar()
{
  while(((*myUCSR0A)&RDA) != RDA){} //Wait until the data has recieved
  return *myUDR0; //Return the recieved character
}

// Wait for USART0 TBE to be set then write character to
// transmit buffer
void U0putchar(unsigned char U0pdata)
{
  while (((*myUCSR0A)&TBE) != TBE){} //Wait until any previous transfer has been complete
  *myUDR0 = U0pdata; //Set the UDR register with new data.
}

void print_info_to_LCD(bool error){
  lcd.clear();
  lcd.setCursor(0,0);
  if (error)
    lcd.print("Error: Water Low");
  else if (current_state != 'D'){
    lcd.print("T = ");
    lcd.print(temp_current);
    lcd.print(" deg. C");
    lcd.setCursor(0,1);
    lcd.print("H = ");
    lcd.print(humidity);
    lcd.print("%");
  } else 
    lcd.print("Disabled");
    
  
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
      print_info_to_LCD(current_state == 'E');
      water_current = adc_read(0);
      return( true );
    }
  }

  return( false );
}
