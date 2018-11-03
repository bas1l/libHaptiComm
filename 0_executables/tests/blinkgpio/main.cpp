#include <wiringPi.h>
int main (void)
{
  
    //int numGPIO=4;
    wiringPiSetup() ;
  
    int powerSupplyPin=25;
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