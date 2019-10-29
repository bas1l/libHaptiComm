#include <wiringPi.h>
#include <iostream>
#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi */

int main (int argc, char** argv)
{
  
    //int numGPIO=4;
    wiringPiSetup() ;
    
    int powerSupplyPin=atoi(argv[1]);
    int LEDPin=28;
    
    pinMode(powerSupplyPin, INPUT);
    pinMode(LEDPin, OUTPUT);
    
    while(true){
        if(digitalRead(powerSupplyPin) == HIGH)
        {
            digitalWrite(LEDPin, HIGH); delay (500);
            digitalWrite(LEDPin,  LOW); delay (500);
        }
    }
  
  
  return 0 ;
}