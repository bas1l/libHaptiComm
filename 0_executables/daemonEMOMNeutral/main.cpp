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

// sudo ./0_executables/daemonPowerSupply/daemonPS 6 150
int main (int argc, char** argv)
{
    /**
     * INITIALISATION
     */
    AD5383 ad;
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    struct timespec t;
    struct sched_param param;
    
    int d;
    
    /**
     * SET UP
     */
    for(int j = 0; j < AD5383::num_channels; ++j) 
    {
        values[j].push_back(2048);
    }
    
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }
    while(false == ad.spi_open()){}
    ad.configure();
    std::cout << "SPI connected." << std::endl;
    
    ms = 1;
    d= 60 * 1000;//in milliseconds atoi(argv[1]);
    
    
    /**
     * ACTION
     */
    while(true){
        delay (d);
        ad.execute_trajectory(values, ms *1000000);
    }
  



    return 0 ;
}