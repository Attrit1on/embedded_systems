/*  Lab 6: Introduction to Interrupt-Driven Applications
 *  
 *  File:       analogRead_comparison_v2.ino
 *  Version:    2.00
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    28 March, 2016
 *  Modified:   28 March, 2016
 *  
 *  This program compares the conversion times of three different methods of reading analog 
 *  port 0.  The first method is the default analogRead() function, the second method is polling 
 *  the completion of the conversion, and the final method uses an interrupt to signal the conversion
 *  completion.  When the user enters "a\n" into the serial terminal it will perform 30 samples of 
 *  the onboard ADC using analogRead() and report individual sample values and times along with an 
 *  average of all 30 sample times at the end.  The program performs this same process whenever the user
 *  enters "b\n" except that now the samples are being performed via direct port manipulation and conversion
 *  completion is polled.  Finally, whenever the user enters "c\n" the conversion will be started using
 *  port manipulation and the conversion completion signaled by an interrupt. There is a watchdog interrupt 
 *  that triggers if the user fails to enter a line feed, thus voiding any data received in such an invalid 
 *  format.  Finally, any user input during a conversion cycle will be rejected by the system(never registered).
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
#include <avr/wdt.h>

void setup() {
    // Serial start and initial statements
    Serial.begin(115200);
    Serial.println("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    Serial.println("************************analogRead Comparison V2.00************************");             
    Serial.println("This program compares the conversion time of the default analogRead()");
    Serial.println("function to polled and interrupt driven approaches that are controlled");
    Serial.println("via direct port manipulation\n");
    Serial.println("Program Initialization:");
    // Print instructions for user
    printInstructions();
}


bool serialTimeout = 0, invalidInput = 0, inputComplete = 0, inputCountError = 0;
char inputChar = 0;
void loop() {
    // Check for user error
    userErrorCheck();
    // Check for valid user input and sample accordingly
    if (inputComplete) {
        if (inputChar == 'a' || inputChar == 'A') AnalogSampling(0);
        else if (inputChar == 'b' || inputChar == 'B') AnalogSampling(1);
        else AnalogSampling(2);
        // Clear the input
        inputChar = 0;
        inputComplete = 0;
        // Check for user input
        if (Serial.available()) {
          Serial.println("**User input is not accepted during a conversion cycle");
        }
        // Clear the serial buffer
        while (Serial.available()) {
            Serial.read();
        }
        // Reprint instructions for user
        printInstructions();
    }
}

void serialEvent() {
  while (Serial.available() > 0) {
    // Enable watchdog if it is not already
    if (WDTCSR == 0x00) {
      enableWatchdogInterrupt();
    }
    // Pet the dog
    wdt_reset();
    char input = Serial.read();
    if ((input == 'a' || input == 'A' || input == 'b' || input == 'B' || input == 'c' || input == 'C') && inputChar == 0) {
      inputChar = input;
    }
    else if (input == '\n') {
      disableWatchdogInterrupt();
      inputComplete = 1;
      if (inputChar == 0) invalidInput = 1;
    }
    else {
      if (invalidInput == 1 || inputChar != 0) inputCountError = 1;
      invalidInput = 1;
    }
  }
}

/*  printInstructions() 
 *  Prints the program instructions when called
 *  
 */
void printInstructions() {
    Serial.println("---------------------------------------------------------------------------");
    Serial.println("Please enter which conversion method you would like followed by a line feed:");
    Serial.println("***Enter 'a' to begin a conversion set using analogRead()");
    Serial.println("***Enter 'b' to begin a conversion set with polled completion");
    Serial.println("***Enter 'c' to begin a conversion set with interrupt signaled completion\n\n");
}

/*  userErrorCheck() 
 *  Checks for and handles any errors by the user
 *  
 */
void userErrorCheck() {
    if (serialTimeout | (invalidInput&inputComplete)) {
        Serial.println("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        if (serialTimeout) {
            Serial.println("Error: You must end a transmission with a line feed!");
        }
        else if (inputCountError) {
            Serial.println("Error: Too many inputs entered at once, please obey the instruction prompt!");
        }
        else {
            Serial.println("Error: Invalid input, please obey the instruction prompt!");
        }
        // Reprint instructions for user
        printInstructions();
        // Clear error bits and input variable
        serialTimeout = invalidInput = inputComplete = inputCountError = 0;
        inputChar = 0;
    }
}

volatile uint32_t sample_time = 0;
/*  AnalogSampling() 
 *  Reports the individual and average duration of 30 ADC samples 
 *  using the selected sampling method.
 *  default_sampling = true  => analogRead()
 *  default_sampling = false => customAnalogRead()
 *  
 */
void AnalogSampling(uint8_t default_sampling) {
  if (default_sampling == 0) {
    Serial.println("Starting a conversion set using analogRead():");
  }
  else if (default_sampling == 1) {
    Serial.println("Starting a conversion set using direct port manipulation with polled completion:");
  }
  else {
    Serial.println("Starting a conversion set using direct port manipulation with interrupt signaled completion:");
  }
  uint32_t sample_time_sum = 0;
  for (int i = 1; i < 31; i++) {
    // Set time for next sample
    uint32_t next_sample_time = millis()+500;
    uint16_t sample_val = 0;
    // Record value of sample
    if (default_sampling == 0) {
      // Record time that this sample began
      sample_time = micros();
      // Start the sample
      sample_val = analogRead(A0);
      // Record time it took to take sample
      sample_time = micros() - sample_time;
    }
    else if (default_sampling == 1) {
      sample_val = polledAnalogRead(0);
    }
    else if (default_sampling == 2) {
      sample_val = interruptDrivenAnalogRead(0);
    }
    // Sum all sample times for computing the average
    sample_time_sum += sample_time;
    // Print the sample information in appropriate format
    printSample(i, sample_val, sample_time);
    // Wait till time for next sample
    while (millis() < next_sample_time);
  }
  // Print the average conversion time
  Serial.println("Average conversion time: " + (String)(sample_time_sum/30) + " usecs\n\n");
}

/*  polledAnalogRead() 
 *  Reports the individual and average duration of 30 ADC samples 
 *  using port manipulation along with the digital ADC value of 
 *  each sample.  In this function the conversion completion is 
 *  polled constantly.
 *  
 */
uint16_t polledAnalogRead(uint8_t channel) {
    // Configure the ADC for single ended input on selected channel
    ADMUX |= (channel|0b01000000);
    // Start the conversion
    ADCSRA |= 0x40;
    // Record time that this conversion began
    sample_time = micros();
    // Wait for reading to finish
    while (ADCSRA & 0x40);
    // Record time it took to take sample
    sample_time = micros() - sample_time;
    // Return the ADC digital value
    return ADCL|(ADCH<<8);
}

/* interruptDrivenAnalogRead()
 *  Reports the individual and average duration of 30 ADC samples
 *  using port manipulation along with the digital ADC value of 
 *  each sample.  In this function the conversion completion is
 *  signaled by an interrupt
 *  
 */
uint16_t interruptDrivenAnalogRead(uint8_t channel) {
  // Configure the ADC for single ended input on selected channel
  ADMUX |= (channel|0b01000000);
  // Set ADC for interrupt mode
  ADCSRA |= 0x08;
  // Start the conversion
  ADCSRA |= 0x40;
  // Record time that this conversion began
  sample_time = micros();
  uint32_t start_time = sample_time;
  int16_t dummy_val = random(100);
  while(start_time == sample_time) {
    // dummy processing load
    dummy_val *= random(100);
    dummy_val -= random(dummy_val)*(random(-100, 100) + random(-100, 100));
  }
  // Reset the ADC to initial state
  ADCSRA &= 0xF7;
  return ADCL|(ADCH<<8);
}

// Interrupt Service Routine for the ADC Interrupt Flag
ISR(ADC_vect) {
  sample_time = micros() - sample_time;
}

/*  printSample() 
 *  Prints the information of each ADC sample when called with appropriate formatting.
 *  
 */
void printSample(uint8_t count, uint16_t sample_val, uint16_t time_span) {
    // Print the sample count
    if (count < 10) {
        Serial.print("#0" + (String)count + ":   digital value = 0x");
    } 
    else {
        Serial.print("#" + (String)count + ":   digital value = 0x");
    }

    // Print the digital value of the sample in hex
    if (sample_val < 0x10) {
        Serial.print("00");
    }
    else if (sample_val < 0x100) {
        Serial.print("0");
    }
    Serial.print(sample_val, HEX); 

    // Print the time it took to do the conversion
    Serial.println("    Time = " + (String)time_span + " usecs");
}

/*  disableWatchdogInterrupt() 
 *  Disables the ATmega2560 watchdog and clears relevant flags
 *  
 */
void disableWatchdogInterrupt() {
    cli();          // Disable interrupts
    WDTCSR |= 0x98; // Enable changes to WDTCSR, clear interrupt flag
    WDTCSR  = 0x00; // Clear the watchdog timer control register(stop all activity)
    sei();          // Enable interrupts
}

/*  enableWatchdog()
 *  Enables the ATmega2560 watchdog in interrupt mode with a xxmicrosecond timeout
 *  
 */
void enableWatchdogInterrupt() {
    cli();          // Disable Interrupts
    MCUSR  &= 0xF7; // Clear the watchdog reset flag
    WDTCSR |= 0x18; // Enable changes to the watchdog
    WDTCSR  = 0xC1; // Config 32ms timeout, enable watchdog interrupt, and clear interrupt flag
    sei();          // Enable interrupts
}

// Watchdog timeout interrupt
ISR(WDT_vect) {
    // Disable watchdog and clear interrupt flag
    disableWatchdogInterrupt();
    // Set serial timeout error flag
    serialTimeout = 1;
}

