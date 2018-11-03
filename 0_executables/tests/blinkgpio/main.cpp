#include <wiringPi.h>
int main (void)
{
  
    //int numGPIO=4;
  wiringPiSetup () ;
  
    for (int numGPIO=0;numGPIO<28;numGPIO++)
    {
        pinMode (numGPIO, OUTPUT) ;
    }
  int a=0;
  int b=7;
    while(true){
        for (int numGPIO=a;numGPIO<b;numGPIO++)
        {
            digitalWrite (numGPIO, HIGH) ;
        }
        delay (500) ;

        for (int numGPIO=a;numGPIO<b;numGPIO++)
        { 
            digitalWrite (numGPIO,  LOW) ;
        } 
        delay (500) ;
    }
  
  
  return 0 ;
}