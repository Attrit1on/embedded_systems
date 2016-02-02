/*  Lab 1: Out of phase blinking of LEDs
 *  
 *  File:       out_of_phase_led_blink.ino
 *  Version:    1.00
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    01 February, 2016
 *  Modified:   01 February, 2016
 *  
 *  
 *  
 *  This program is created for an embedded systems class.  
 *  The program blinks two LEDs(more detail below) whenever a 
 *  'g' is transmitted to the UART and stops all blinking 
 *  whenever an 's' is transmitted in the same fashion.
 *  
 *  When receiving the activation, LED1(pin 13) is turned on with 
 *  LED2(pin 12) left off.   The program then loops through the 
 *  following sequence continuously until the stop signal is 
 *  received.
 *  
 *  ---LED Sequence---
 *  Initial state: LED1 on, LED off
 *  Sequence Start
 *  LED1 and LED2 toggle after 2 second
 *  LED1 and LED2 toggle again after 1 second
 *  Sequence End
 *  
 *  Copyright (C) 2015, Trey Harrison
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */
 
#define LED1 13
#define LED2 12

void setup() {
  Serial.begin(9600);
  Serial.println("-----Out of Phase LED Blink V1.00-----");
  Serial.println("Enter a 'g' to start flashing the LEDs");
  Serial.println("Enter a 's' to stop flashing the LEDs");
  Serial.println("--------------------------------------\n");
  pinMode(LED2, OUTPUT);
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, 0);
  digitalWrite(LED2, 0);
}

boolean activeLEDs=0;
void loop() {
  // Activate the LED sequence into initialization state if appropriate 
  // control byte has been received
  LEDActivation(checkSerialBuffer());
  // Run the LED control sequence if the active flag is true
  if (activeLEDs)
    controlLEDs();
}

/* controlLEDs()
 * Facilitates control of the LED flashing sequence when active
 * 
 */
unsigned long oldTime = 0;
boolean longWait=1;
void controlLEDs() {
    uint16_t timePassed = millis()-oldTime;
    if ((longWait&&(timePassed>2000)) || (!longWait&&(timePassed>1000))) {
      digitalWrite(LED1, !longWait);
      digitalWrite(LED2, longWait);
      oldTime = millis();
      longWait = !longWait;
    }
}

/* LEDActivation()
 * When the argument is true, activate the LED sequence
 * Starts by activating LED1 and disabling LED2
 * Records the current "processor time" in ms
 * Activates the control sequence for LED1 and LED2
 * 
 */
void LEDActivation(boolean activate) {
  if (!activate) return;
  else if (activate && !activeLEDs) {
    digitalWrite(LED1,1);
    digitalWrite(LED2,0);
    oldTime=millis();
    activeLEDs=1;
  }
}

/* checkSerialBuffer()
 * Reads in one char per program loop, if any are available
 * Returns true if the received character is a 'g', false otherwise
 * Forces LED1 and LED2 off when a 's' is received until another 'g'
 * is received
 * 
 */
boolean checkSerialBuffer() {
  // Check if any bytes are available in the UART buffer
  if (Serial.available()) {
    // Temporarily store the first character in the serial buffer
    char input = Serial.read();
    // If input is 'g', signal LED activation
    if (input == 'g') {
        return 1;
    }
    // If input is 's', disable all LED activity
    else if (input == 's') {
      if (activeLEDs) {
        activeLEDs=0;
        digitalWrite(LED1,0);
        digitalWrite(LED2,0);
      }
    }
    // Account for user error
    else {
      Serial.println("Your input is invalid, please read the instructions above.\n");
    }
  }
  return 0;
}
