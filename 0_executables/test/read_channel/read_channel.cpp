#include <iostream>
#include <sys/mman.h>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ad5383.h"
using namespace std;

#define MAX_VAL 4095

int main(int argc, char *argv[])
{
    std::cout << "Begin." << std::endl;
    
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    std::vector<uint16_t> values(AD5383::num_channels);
	  values.reserve(AD5383::num_channels);
    
    std::cout << "Read..." << std::endl;
    for(int v = 0; v < MAX_VAL; v+=500){
        std::cout << "Apply (" << v << ") " << 5*(v/MAX_VAL) << "Volt" << std::endl;
        for(int j = 0; j < AD5383::num_channels; ++j)
        {
            ad.execute_single_channel(v, j);
            values[j] = ad.read_channel(j);
        }

        std::cout << "Current Voltage in channels <[channel]: value>" << std::endl;
        for(int j = 0; j < AD5383::num_channels; ++j)
        {
            std::cout << "[" << j << "]: " << values[j] << std::endl;
        }

        sleep(1);
    }
    
    std::cout << "Apply 0V before ending the executable." << std::endl;
    for(int j = 0; j < AD5383::num_channels; ++j)
    {
        ad.execute_single_channel(0, j);
        values[j] = ad.read_channel(j);
    }
    std::cout << "Current Voltage in channels <[channel]: value>" << std::endl;
    for(int j = 0; j < AD5383::num_channels; ++j)
    {
        std::cout << "[" << j << "]: " << values[j] << std::endl;
    }
    
    std::cout << "Done." << std::endl;
    ad.spi_close();
  
    return 0;
}
