#include <wiringPi.h>
int main (void)
{
  
    //int numGPIO=4;
  wiringPiSetup () ;
  pinMode (numGPIO, OUTPUT) ;
  while(true){
    for (int numGPIO=0;numGPIO<28;numGPIO++)
    {
      digitalWrite (numGPIO, HIGH) ;
    }
    delay (500) ;

    for (int numGPIO=0;numGPIO<28;numGPIO++)
    {
      digitalWrite (numGPIO,  LOW) ;
    } 
    delay (500) ;
  }
  
  
  return 0 ;
}