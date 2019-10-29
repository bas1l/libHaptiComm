#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <queue>
#include <random>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "ad5383.h"
#include "alphabet.h"
#include "utils.h"
#include "waveform.h"

using namespace std;

int main(int argc, char *argv[])
{   
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
    
    
    int nb_range = 7;
    // shift in time/ms/value between 2 actuators in series into the app move
    int lag_inter_line = APPARENT_DURATION*APPARENT_PERCENTAGE_COVERING;
    int total_time = APPARENT_DURATION* (1+ APPARENT_PERCENTAGE_COVERING*(nb_range-1)) +1;//+1 for neutral statement
    
    std::vector<uint16_t> asdasdas;
    asdasdas.resize(total_time);
    std::vector<uint16_t> asdasdas2;
    asdasdas2.reserve(128);
    std::cout << "ttv : " << total_time << std::endl;
    std::vector<uint16_t> ttv;
    ttv.reserve(512);//total_time-251);
    std::cout << "result" << std::endl;
    std::vector<std::vector<uint16_t>> result(AD5383::num_channels, ttv);
    std::cout << "result end" << std::endl;
    
    //AD5383 ad;
    //ad.configure();
    DEVICE dev;
    dev.configure();
    WAVEFORM wf;
    wf.configure();
    ALPHABET alph(dev, wf);//, AD5383::num_channels);
    alph.configure();

    return 0;
}
