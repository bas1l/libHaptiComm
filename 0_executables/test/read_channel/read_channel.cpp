#include <iostream>
#include <sys/mman.h>
#include "ad5383.h"
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <sys/ioctl.h>
using namespace std;

int main(int argc, char *argv[])
{
    std::cout << "Neutral::Begin." << std::endl;

    struct timespec t;
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    
    /**************** C'EST AVEC CETTE OPTION QUE TOUT BUG *********************
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }
    ***************************************************************************/
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }

    
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
	values.reserve(AD5383::num_channels);
    
    for(int j = 0; j < AD5383::num_channels; ++j)
    {
		values[j] = ad.read_channel(j);
    }

    std::cout << "Current Voltage in channels <[channel]: value>" << std::endl;
    for(int j = 0; j < AD5383::num_channels; ++j)
    {
		std::cout << "[" << j << "]: " values[j] << std::endl;
    }
    
    long ms = 900;
    std::cout << "overruns : " << std::dec << ad.execute_trajectory(values, ms *1000000) << std::endl;
    std::cout << "Done." << std::endl;
    ad.spi_close();
  
    return 0;
}
