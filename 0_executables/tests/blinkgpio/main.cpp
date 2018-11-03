#include <wiringPi.h>
int main (void)
{
  
    int numGPIO=4;
  wiringPiSetup () ;
  pinMode (numGPIO, OUTPUT) ;
  for (;;)
  {
    digitalWrite (numGPIO, HIGH) ; delay (500) ;
    digitalWrite (numGPIO,  LOW) ; delay (500) ;
  }
  return 0 ;
}