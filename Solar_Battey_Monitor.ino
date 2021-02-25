/* 
  Solar powered Attiny85_batteryMonitor 
  Ideas borrowed from:https://gist.github.com/dwhacks/7208805#file-attiny85_batterymonitor-ino
  modified for Water tank level indicators at remotelocations

 *@@ Voltage trigger levels.
 *
 * Battery voltage is read through a voltage divider and compared to the internal voltage reference.
 *
 * If 
 *    Vin ----+
 *            R1
 *            +----- Vout (BATI)
 *            R2
 *            |
 *            =
 *            .  (gnd)
 *
 * Then Vout = Vin * ( R2 / (R1 + R2) ) 
 *
 * 
 * eg. R1=12k R2=1k => Vout = Vin * (1000 / (1000 + 12000))
 *                            Vin * 0.0769
 *
 *     R1=20k R2=10k => Vout = Vin * 0.3333   Ileak = 0.4mA @ 12v
 *     R1=2k2 R2=1k  => Vout = Vin * 0.3125
 *     R1=3k3 R2=1k  => Vout = Vin * 0.232
 *     R1=3k9 R2=1k  => Vout = Vin * 0.204    Ileak = 2.4mA @ 12v
 *     R1=39k R2=10k => Vout = Vin * 0.204    Ileak = 0.24mA @ 12v
 *     R1=4k7 R2=1k  => Vout = Vin * 0.175
 *     R1=10k R2=1k  => Vout = Vin * 0.0909   Ileak = 1mA @ 12v
 *     R1=12k R2=1k  => Vout = Vin * 0.0769   Ileak = 0.92mA @ 12v
 *
 * Fully charged LiPo is 4.23v/cell, discharged is 2.7v/cell (nominal voltage 3.7v/cell)
 * For battery endurance, do not discharge below 3.0v/cell (aircraft users commonly use 2.9v/cell as limit)
 *
 * A 2-cell battery (nominally 7.46v)  varies     from 8.46v to 5.40v, with low-volt alert at 6.00v
 * A 3-cell battery (nominally 11.1v) thus varies from 12.9v to 8.10v, with low-volt alert at 9.00v
 * A 4-cell battery (nominally 14.8v) thus varies from 16.9v to 10.8v, with low-volt alert 12 12.0v
 *   NOTE: a 4-cell battery requires a different voltage divider than 2-and-3 cells (use 15:1 not 12:1)
 *
 *
 *@@ Analog read values for defined voltage levels
 *
 * In AVR-worldview, we use 12:1 voltage divider and read 10-bit ADC comparisons versus AREF (1.1v)
 *  
 * So 12v becomes ~1.00V when divided.   
 * Compared to 1.1v reference this gives an ADC result of 1024*(1.0/1.1) == 859
 *
 * An alternative approach is to use a smaller voltage divisor and compare
 * against Vcc (5.0v), but in practice a 12:1 divisor is easier to achieve
 * due to the standard first preference resistor value series.
 *
 * You can use these Emacs lisp defuns to calculate threshold analog values for your voltage levels
 *
 * for 4-cell, use a 15:1 divider
 *     (vlist2int (rn2div 15000 1000) 1.1 '(16 14.5 13 12)) => (931 844 756 698)
 *
 /* Use a 12:1 voltage divider */


 //#define INTERNAL (2) 
#define CELL_COUNT 3  // DEFINE THE NUMBER OF CELLS 2,3,4


#if CELL_COUNT == 4
/* Use a 15:1 voltage divider */

#define VL_FULL    931

#define VL_GOOD    844

#define VL_LOW     756

#define VL_CRIT    698

#elif CELL_COUNT == 3
/* Use a 12:1 voltage divider */

#define VL_FULL    859

#define VL_GOOD    788

#define VL_LOW     716

#define VL_CRIT    644

#elif CELL_COUNT == 2
/* Use a 12:1 voltage divider */


#define VL_FULL    558  //about 7.8v - 0.6v after divider

#define VL_GOOD    523  //about 7.3v - 0.562 after divider - seems closer to 7v on proto

#define VL_LOW     451  //about 6.3v - 0.485 after divider - closer to 6.15 on proto

#define VL_CRIT    430 //about 6v - 0.462 after divider

#endif
 
#include <SoftwareSerial.h>    //for checking voltage levels

#define RX    2   // physical Pin 7
#define TX    0   // physical Pin 5
SoftwareSerial Serial(RX, TX);


const int batteryPin = 2; //V+ from battery connected to analog1 physical pin 3 
const int switchPin = 1; //physical pin 6 to switch solar supply 
const int led = 3; //physical pin 2

// Define the number of samples to keep track of.  The higher the number,
// the more the readings will be smoothed, but the slower the output will
// respond to the input.  Using a constant rather than a normal variable lets
// use this value to determine the size of the readings array.
const int numReadings = 3;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average





void setup()
{
  analogReference(INTERNAL);
  pinMode(batteryPin, INPUT);   
  pinMode(switchPin, OUTPUT);
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  Serial.println("Initialization complete.");
  delay(1000);
   for (int thisReading = 0; thisReading < numReadings; thisReading++)
      readings[thisReading] = 0; // initialize all the readings to 0
    
  
}

void loop()
{  
    averageVoltage(); //get average voltage from function  
    int voltage = average;
    
    if (voltage == VL_LOW){ //if battery is low, turn on transistor swtich to charge battery
      digitalWrite(switchPin, HIGH);
    }
    if (voltage == VL_FULL){ //if battery is full, turn off transistor swtich
      digitalWrite(switchPin, LOW);
    }    

    
    if (voltage >= VL_FULL){ //if the battery is full or higher
      digitalWrite(led, HIGH);
      digitalWrite(switchPin, LOW);
    }
    
    else if (voltage >= VL_GOOD){
      digitalWrite(led, HIGH);
      delay(3000);
      digitalWrite(led, LOW);
      delay(300);           
    }
    
    else if (voltage >= VL_LOW){
      digitalWrite(led, HIGH);
      delay(3000);
      digitalWrite(led, LOW);
      delay(1000);
           
    }
    
    else if (voltage < VL_LOW){
      digitalWrite(switchPin, HIGH);
      digitalWrite(led, HIGH);
      delay(100);
      digitalWrite(led, LOW);
      delay(100);      
           
    }          
    
  Serial.println(voltage);
}

      void averageVoltage() {
        // subtract the last reading:
        total= total - readings[index];        
        // read from the sensor:  
        readings[index] = analogRead(batteryPin);
        // add the reading to the total:
        total= total + readings[index];      
        // advance to the next position in the array:  
        index = index + 1;                    
      
        // if we're at the end of the array...
        if (index >= numReadings)              
          // ...wrap around to the beginning:
          index = 0;                          
      
        // calculate the average:
        average = total / numReadings;          
        delay(1);        // delay in between reads for stability            
      }


//end
