#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <fstream>
#include <math.h>
#include <mutex>
#include <ncurses.h>
#include <queue>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <time.h>
#include <thread>
#include <vector> 

// haptiComm library
#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "utils.h"
#include "alphabet.h"

using namespace std;
using namespace std::chrono;

#ifdef _WIN32
#include <windows.h>
#define SYSERROR()  GetLastError()
#else
#include <errno.h>
#define SYSERROR()  errno
#endif


waveformLetter  getTapmove(int nbRep, int waitFor, double refreshRate_mHz, uint8_t channel);
static void     parseCmdLineArgs(int argc, char ** argv, int * nbRep, int * nmessage_sec);


int main(int argc, char *argv[])
{
    /*** Initialisation VARIABLES ***/
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    AD5383 *    ad   = new AD5383();
    DEVICE *    dev  = new DEVICE();
    WAVEFORM *  wf   = new WAVEFORM();
    ALPHABET *  alph = new ALPHABET();
    
    waveformLetter wfLetter;
    const char * cfgSource = "../libtacom/config.cfg";
    double  refreshRate_mHz;
    int     durationRefresh_ns; // * ns
    int     waitFor;
    int     nbRep;
    uint8_t channel;
    
    
    /*** Initialisation ENVIRONMENT ***/
    setlocale(LC_ALL, "");
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
    
    
    /*** Configuration VARIABLES ***/
    parseCmdLineArgs(argc, argv, &nbRep, &waitFor);
    cfg->configure(cfgSource, dev, wf, alph);
    ad->spi_open();
    ad->configure();
    
    
    refreshRate_mHz     = alph->getFreqRefresh_mHz();   
    durationRefresh_ns  = 1/refreshRate_mHz * ms2ns; // * ns
    channel             = dev->getActuator("mf2").chan;
    wfLetter            = getTapmove(nbRep, waitFor, refreshRate_mHz, channel);
    
    /*** work ***/
    ad->execute_trajectory(alph->getneutral(), durationRefresh_ns);
    ad->execute_selective_trajectory(wfLetter, durationRefresh_ns);
    
    /*** Free the environment and memory ***/
    delete cfg;
    delete dev;
    delete wf;
    delete alph;
    
    return 0;
}


waveformLetter getTapmove(int nbRep, int waitFor, double refreshRate_mHz, uint8_t channel)
{
    waveformLetter wL;
    vector<uint16_t> vec;
    int timeMove_ms = refreshRate_mHz* 15;
    int timeWait_ms = refreshRate_mHz* waitFor;
    int cpt;
    
    vec.clear();
    for(cpt=0; cpt<nbRep; cpt++)
    {
        vec.insert(vec.begin(), timeWait_ms, 2048);
        vec.insert(vec.begin(), timeMove_ms, 0);
    }
    wL.insert(std::pair<uint8_t, vector<uint16_t>>(channel, vec));
    
    fprintf(stderr, "size of the vec '%i\n\n", vec.size());
    return wL;
    
}

static void 
parseCmdLineArgs(int argc, char ** argv, int * nbRep, int * waitFor)
{
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-wait") == 0) {
            if (i == argc-1) { fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);}
            *waitFor = atoi(argv[i+1]);
            fprintf(stderr, "waitFor initialised to %i\n\n", *waitFor);
            i++;
        } 
        else if  (strcmp(argv[i], "-nrep") == 0) {
            if (i == argc-1) { fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);}
            *nbRep = atoi(argv[i+1]);
            fprintf(stderr, "nrep initialised to %i\n\n", *nbRep);
            i++;
        } 
        else {
            fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);
        }
    }
}


    































    
