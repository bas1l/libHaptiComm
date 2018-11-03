#include <wiringPi.h>
int main (void)
{
  
    //int numGPIO=4;
    wiringPiSetup () ;
  
    int powerSupplyPin=7;
    int LEDPin=0;
    
    pinMode(powerSupplyPin, INPUT);
    pinMode(LEDPin, OUTPUT);
    
    while(true){
        if(digitalRead(powerSupplyPin))
        {
            digitalWrite(LEDPin, HIGH); delay (500);
            digitalWrite(LEDPin,  LOW); delay (500);
        }
    }
  
  
  return 0 ;
}