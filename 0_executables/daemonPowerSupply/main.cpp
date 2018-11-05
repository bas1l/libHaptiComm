#include <wiringPi.h>
#include <iostream>
#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi */

#include <sys/mman.h>
#include "ad5383.h"
#include <queue>
#include <chrono>
#include <ctime>
#include <random>
#include <sys/ioctl.h>
using namespace std;

int main (int argc, char** argv)
{
    /**
     * INITIALISATION
     */
    AD5383 ad;
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    struct timespec t;
    struct sched_param param;
    long ms;
    
    int powerSupplyPin;
    int LEDPin;
    int currStatement;
    int prevStatement;
    
    /**
     * SET UP
     */
    for(int j = 0; j < AD5383::num_channels; ++j) 
    {
        values[j].push_back(2048);
    }
    ms = 900;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }
    
    powerSupplyPin=atoi(argv[1]);
    LEDPin=28;
    wiringPiSetup() ;
    pinMode(powerSupplyPin, INPUT);
    pinMode(LEDPin, OUTPUT);
    currStatement = LOW;
    prevStatement = LOW;
    
    
    
    
    /**
     * ACTION
     */
    while(true){
        currStatement = digitalRead(powerSupplyPin);
        if(prevStatement == LOW && currStatement == HIGH)
        {
            prevStatement = currStatement;
            
            ad.spi_open();
            ad.configure();
            ad.execute_trajectory(values, ms *1000000);
            ad.spi_close();
            
            digitalWrite(LEDPin, HIGH); delay (500);
            digitalWrite(LEDPin,  LOW); delay (500);
        }
        else if (currStatement == LOW && prevStatement == HIGH){
            prevStatement = currStatement;
        }
    }
  



    return 0 ;
}