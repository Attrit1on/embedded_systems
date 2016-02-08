/*  Lab 2: Introduction to Digital IO
 *  
 *  File:       switch_state_change_counter.ino
 *  Version:    1.01
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    04 February, 2016
 *  Modified:   08 February, 2016
 *  
 *  This program is created for an embedded systems course.  
 *  The program counts the state changes of a mechanical toggle switch wired 
 *  to digital pin 2. At each state change, the program increments a switchCount
 *  value(starting at 0 and incrementing through 15 before resetting) that is 
 *  then displayed on a 7-segment display in hexadecimal and also printed to the 
 *  serial monitor in hexadecimal and decimal.  Additionally, the switch functions 
 *  as a toggle switch that enables/disables an LED flashing sequence.
 *
 *  When receiving the activation, LED1(pin 13) is turned on with LED2(pin 12) 
 *  left off.   The program then loops through the following sequence 
 *  continuously until the toggle switch(pin 2) is turned off.
 *  
 *  ---LED Sequence---
 *  Initialization: LED1 on, LED2 off
 *  Sequence Start
 *  LED1 and LED2 toggle after 1 second
 *  Sequence End
 *  
 *  Copyright (C) 2016, Trey Harrison
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in thde hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Source: <https://github.com/Attrit1on/embedded_sytems>
 *  
 */

// State Variables
uint8_t switchCount = 0;
volatile boolean switchChangeFlag = 0;

void setup() {
  // Initialize serial monitor
  Serial.begin(9600);
  Serial.println("-----State Change Counter V1.00-----\n");
  printCount(switchCount);
  // Set pins 31:37,12:13 as outputs 
  DDRC |= 0x7F;
  DDRB |= 0xC0;
  // Set pin 2(PORTE[4]) as input
  DDRE &= 0xEF;
  // Enable internal pull-up for pin 2
  PORTE |= 0x10;
  // Initialize 7-segment display with 0 displayed
  PORTC = sevenSegmentControl(0);
  // Set state change interrupt for Pin 2(interrupt 0)
  attachInterrupt(0, setSwitchChangeFlag, CHANGE);
}

void loop() {
  if (switchChangeFlag) {
    switchChangeInterruptHandler();
  } else if (PINB&0xC0) {
    ledControl();
  }
}

/* setSwitchChangeFlag()
 * ISR called whenever Pin 2(Interrupt 0) changes state 
 * Sets a flag and returns
 * 
 */
void setSwitchChangeFlag() {
  switchChangeFlag = 1;
}

/* switchChangeInterruptHandler()
 * Performs necessary logic to appropriately update the program state
 * whenever an interrupt has been a state change interrupt on pin 2.
 * Also debounces the switch to ensure a single toggle doesn't result
 * in more than one toggle being perceived. Returns 1 if switch change
 * was not a false reading(>100ms passed since last toggle), else 
 * returns 0
 * 
 */
unsigned long lastSwitchTime = 0;
boolean switchChangeInterruptHandler() {
    switchChangeFlag = 0;
    // Debounce the switch
    if (lastSwitchTime-millis()>100) {
      if (switchCount++ > 14) {
        switchCount = 0;
      }
      // Update the 7-segment display with the new count
      PORTC = sevenSegmentControl(switchCount);
      // Print new count to serial monitor
      printCount(switchCount);

      if (PINB&0xC0) {
        PORTB &= 0x3F;
      } else {
        ledSequenceInitialization();
      }
      // Reset debounce timer
      lastSwitchTime=millis();
      return 1;
    }
    return 0;
}

/* controlLEDs()
 * Controls two LEDs, LED1(pin 13) and LED2(pin 12), that blink 
 * out of phase with a 2 second Period and 50% duty cycle. 
 * Function is only called in this code whenever activeState 
 * is true.  Returns 1 if LED states changed, else returns 0.
 *
 */
unsigned long oldTime = 0;
boolean ledControl() {
  if (millis() - oldTime > 1000) {
    // Check state of LED1
    if (PINB&0x80) {
      // LED1 on
      PORTB &= 0x7F;
      // LED2 off
      PORTB |= 0x40;
    } else {
      // LED1 off
      PORTB |= 0x80;
      // LED2 on
      PORTB &= 0xBF;
    }
    // Reset LED process timer
    oldTime = millis();
    return 1;
  }
  return 0;
}

/* ledSequenceInitialization() 
 * Sets LED1(Pin 13, PORTB[7]) high
 * 
 */
void ledSequenceInitialization() {
  // Set LED process timer
  oldTime = millis();
  // LED1 on
  PORTB |= 0x80;
}


/* sevenSegmentControl(uint8_t displayVal)
 * Returns the byte needed in a register to represent displayVal on 
 * a seven segment display in hex wired with all segments to a single 
 * port as described in the table below.  This code was executed with 
 * the display wired to PORTC of the Arduino Mega platform.
 * 
 * The segment field corresponds to the segment of the display to be
 * controlled(segment map to right of table).  The port_bit refers 
 * to the bits in a single port of the arduino to be used.  In this  
 * case, the pin field corresponds to the appropriate pins needed to 
 * correctly utilize PORTC of the Arduino Mega platform.
 * 
 * ---THIS ONLY WORKS FOR COMMON CATHODE DISPLAYS---
 * 
 *   PORTC[0:6] => Digital Pins 37:31
 *   ---------------------------
 *   |segment    port_bit   pin|
 *   |   A          0       37 |     segments
 *   |   B          1       36 |     |--A--|
 *   |   C          2       35 |     F     B
 *   |   D          3       34 |     |--G--|
 *   |   E          4       33 |     E     C
 *   |   F          5       32 |     |--D--|
 *   |   G          6       31 |
 *   ---------------------------
 *
 */
uint8_t sevenSegmentControl(uint8_t displayVal) {
  switch (displayVal) {
    case 0:  return 0x7E; break;
    case 1:  return 0x30; break;
    case 2:  return 0x6D; break;
    case 3:  return 0x79; break;
    case 4:  return 0x33; break;
    case 5:  return 0x5B; break;
    case 6:  return 0x1F; break;
    case 7:  return 0x70; break;
    case 8:  return 0x7F; break;
    case 9:  return 0x73; break;
    case 10: return 0x77; break;
    case 11: return 0x7F; break;
    case 12: return 0x4E; break;
    case 13: return 0x7E; break;
    case 14: return 0x4F; break;
    case 15: return 0x47; break;
    default: return 0x00; break;
  }
}

/* printCount(uint8_t a) {
 * Prints value of a to the serial monitor in the following format:
 * "count = (decimal) a    (hex) a\n"
 * Example:
 *  a=11 => 
 * "count = (decimal) 11    (hex) B\n"
 * 
 */
void printCount(uint8_t a) {
  Serial.print("count = (decimal) ");
  Serial.print(a, DEC);
  if (a < 10) {
    Serial.print("     (hex) ");
  } else {
    Serial.print("    (hex) ");
  }
  Serial.println(a, HEX);
}

