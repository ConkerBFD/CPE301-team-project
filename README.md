# CPE301-team-project
Evaporation Cooler project programmed and designed by Gavin Farrell & Alan Garcia

![Schematic](https://github.com/ConkerBFD/CPE301-team-project/blob/main/images/Schematic.png)

### System Constraints
- Voltage: 5V
- Max Current: 2.5 mA
- Operating Temperature: 0°C - 40°C
- Space Requirement: 25cm X 28cm X 16cm

### Components Used
- [Mega 2560 Microcontroller](https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf)
- [DHT11 Humidity Sensor](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
- [DS1307 RTC Module](https://datasheets.maximintegrated.com/en/ds/DS1307.pdf)
- [Servo Motor SG90](http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf)
- [L293D Chip](https://www.ti.com/lit/ds/symlink/l293.pdf)
- [Water Level Sensor Module](https://www.emartee.com/Attachment.php?name=42240.pdf)
- [LCD1602 Module](https://www.sparkfun.com/datasheets/LCD/ADM1602K-NSA-FBS-3.3v.pdf)
- [10k Potentiometer](https://components101.com/potentiometer)
- LEDs x4
- Buttons x3
- 330 Ohms Resistor x3

### Testing the system
This evaporation cooler is best used in an open room at room temperature and low humidity (20-35%). The open room meets the space requirements and the room temperature meets operating temperature standards. To meet voltage and current standards, the breadboard should simply be connected to any of the Mega2560's 5V power and ground pins. Partially submerge the water level sensor in the reservoir, and position the fan so that it will direct evaporating water towards the vent. After connecting the Mega2560 to power, simply press the disable button, which is the first of the 3 buttons, and ensure that your water reservoir stays full. Adjusting system vents can be done by using the two buttons after the disable button, they raise and lower fan angle respectively.
