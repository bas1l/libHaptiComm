#include <wiringPi.h>
int main (void)
{
  
    //int numGPIO=4;
  wiringPiSetup () ;
  
    for (int numGPIO=0;numGPIO<28;numGPIO++)
    {
        pinMode (numGPIO, OUTPUT) ;
    }
  
    while(true){
        //digitalWrite (8, HIGH) ;
        digitalWrite (7, HIGH) ;
        delay (500) ;
        digitalWrite (7,  LOW) ;
        //digitalWrite (8,  LOW) ;
        delay (500) ;
    }
  
  
  return 0 ;
}