#include <iostream>
#include <sys/mman.h>

#include "ad5383.h"

using namespace std;

int main(int argc, char *argv[])
{
    std::cout << "Starting..." << std::endl;

    struct timespec t;
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }

    AD5383 ad;
    ad.spi_open();
    ad.configure();

    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    for(int i = -1; i <23; i++)
    {
        for(int j = 0; j < 30; ++j)
        {
	    if (j==i){
		values[j].push_back(0);
		values[j].push_back(2048);    
	    }
	    else {
            	values[j].push_back(2048);
            	values[j].push_back(2048);
	   }
        }
    }

    int ms = 500;
    std::cout << "overruns : " << std::dec << ad.execute_trajectory(values, ms*1000000) << std::endl;

    ad.spi_close();

    std::cout << "Done." << std::endl;

    return 0;
}
