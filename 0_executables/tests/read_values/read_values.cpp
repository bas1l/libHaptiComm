#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "GPIOClass.h"

#include <sys/mman.h>
#include "ad5383.h"
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <sys/ioctl.h>

using namespace std;

int main (void)
{
    std::cout << "Neutral::Begin." << std::endl;

    struct timespec t;
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }
    
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    long ms = 5;
    
    for(int j = 0; j < AD5383::num_channels; ++j)
    {
        for(int i = 0; i  < 50; ++i)
        {
            values[j].push_back(2048);
        }
    }
    
    
    string inputstate23;
    GPIOClass* gpio23 = new GPIOClass("23"); //create new GPIO object to be attached to  GPIO23

    if (gpio23->export_gpio() == - 1) {return -1;} //export GPIO23
    cout << " GPIO pins exported" << endl;
    
    if (gpio23->setdir_gpio("in") == -1) {return -1;} //GPIO23 set to input
    cout << " Set GPIO pin directions" << endl;
    
    gpio23->getval_gpio(inputstate23); //read state of GPIO23 input pin
    if (inputstate23 == "0") cout << "Power supply : OFF" << std::endl;
    
    
    while(1)
    {
        while (inputstate23 == "0")
        {
            gpio23->getval_gpio(inputstate23);
        };
        std::cout << "Power supply : ON" << std::endl;
        
        int a = ad.execute_trajectory(values, ms *1000000);
        std::cout << "Neutral : OK " << std::endl;
        std:cout << "overruns : " << std::dec << a << std::endl;
        
        while (inputstate23 == "1")
        {
            gpio23->getval_gpio(inputstate23);
            usleep(1 * 1000000);
        };
        cout << "Power supply : OFF" << std::endl;
    }
    
    std::cout << "Neutral::Done." << std::endl;
    ad.spi_close();
    
    return 0;
}