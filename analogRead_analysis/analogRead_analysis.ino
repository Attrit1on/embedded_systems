/*  Lab 3: Introduction to Watchdog Timers
 *  
 *  File:       analogRead_analysis.ino
 *  Version:    1.00
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    23 February, 2016
 *  Modified:   23 February, 2016
 *  
 *  When a 'c' is received via UART0 the program will sample analog pin 0
 *  30 times, printing back the value and elapsed time for each sample as well
 *  as a time average of all 30 samples.  Sending '\n' via serial is neglected
 *  but any other values sent will trigger an invalid input error.
 *  
 *  Copyright (C) 2016, Trey Harrison
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
 *  Source: <https://github.com/Attrit1on/embedded_systems>
 *  
 */

// Library for utilizing the AVR's integrated watchdog
#include <avr/wdt.h> 

void setup() {
  // Serial start and initial statements
  Serial.begin(115200);
  Serial.println("\n\n\n\n\n\n\n\n\n\n\n\n\n\n---analogRead Analysis V1.00---");
  Serial.println("Please enter a 'c' to begin sampling the analog port:\n");
}

// Program State variables
unsigned long sample_start_time=0, last_sample_start_time=0;
unsigned long sampling_time_summation = 0;
bool trigger_sampling=0;

void loop() {
  // Check if a valid input was received
  if (Serial.available()) {
    char input = Serial.read();
    if (input == 'c') {
      trigger_sampling = 1;
    }
    else if (input == '\n') {
      // do nothing
    }
    else {
      Serial.println("Error: Invalid Input Type");
      Serial.println("Please enter a 'c' to begin sampling the analog port:\n");
    }
  }
  if (trigger_sampling) {
    uint16_t sample=0, duration=0;
    sampling_time_summation = 0;
    for (uint8_t i = 1; i < 31; i++) {
      while (millis()-last_sample_start_time < 500);
      last_sample_start_time = millis();
      sample_start_time = micros();
      sample = analogRead(A0);
      duration = micros()-sample_start_time;
      sampling_time_summation += duration;
      printSample(i,sample, duration);
    }
    Serial.println("\navg conversion time = " + (String)(sampling_time_summation/30) + " usecs");
    trigger_sampling = 0;
  }
}

void printSample(uint8_t count, uint16_t sample_val, uint16_t time_span) {
  if (count < 10) {
    Serial.print("#0" + (String)count + ":   digital value = ");
  } 
  else {
    Serial.print("#" + (String)count + ":   digital value = ");
  }
  Serial.print(sample_val, HEX);
  if (sample_val < 0x10) {
    Serial.print("  ");
  }
  else if (sample_val < 0x100) {
    Serial.print(" ");
  }
  Serial.println("    Time = " + (String)time_span + " usecs");
}



