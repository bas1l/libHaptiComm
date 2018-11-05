#include <iostream>
#include <sys/mman.h>
#include <algorithm>
#include <thread>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <atomic>
#include <sys/ioctl.h>
#include <ncurses.h>
#include <math.h>
#include <unistd.h>

#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "utils.h"
#include "alphabet.h"

using namespace std;




void        work(ALPHABET* & alph);
static void parseCmdLineArgs(   int argc, char ** argv, 
                                const char *& cfgSource, const char *& scope);
static void usage();






int main(int argc, char *argv[])
{   
    /* CREATE VARIABLES
     * 
     */
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    DEVICE * dev = new DEVICE();
    WAVEFORM * wf  = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();
    const char * cfgSource;
    const char * scope;
    int exitStatus = 0;

    
    /* SETUP ENVIRONEMENT
     * printw and timer
     *
    */
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
    
    /* INITIALISE VARIABLES
     * 
     */
    setlocale(LC_ALL, "");
    parseCmdLineArgs(argc, argv, cfgSource, scope);
    cfg->configure(cfgSource, dev, wf, alph);
    
    
    /* WORK
     * 
     */
    work(alph);
    
    /* CLEAN
     * 
     */
    delete cfg;
    delete dev;
    delete wf;
    delete alph;
    
    return exitStatus;
}


void work(ALPHABET *& alph)
{
    // init drive electronics
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    
    struct symbol s;
    s.id = "a";
    s.motion = "tapMulti";
    s.actOverlap = 0.2;
    s.actList = {"ff1", "ff2", "ff3"};
    
    
    
    
    initscr();
    noecho();
    cbreak();
    
    //raw();
    
    
    WINDOW * wActuators = newwin(LINES-3, COLS/2-3, 1, 1);
    wborder(wActuators, '|','|','_','_',' ',' ',' ',' ');
    refresh();
    wrefresh(wActuators);
    keypad(wActuators, TRUE);
    
    /*
    WINDOW * wSignal = newwin(LINES/2-2, COLS/2-3, 1, COLS/2-1);
    wborder(wSignal, '|','|','_','_',' ',' ',' ',' ');
    wrefresh(wSignal);
    WINDOW * wStatus = newwin(LINES/2-2, COLS/2-3, LINES/2+1, COLS/2-1);
    wborder(wStatus, '|','|','_','_',' ',' ',' ',' ');
    wrefresh(wStatus);
    */
    
    int choice;
    bool goOn = true;
    while(1)
    {
        choice = wgetch(wActuators);
        switch(choice)
        {
            case 'a':
                goOn = false;
                break;
            case KEY_UP:
                goOn = false;
                break;
            case KEY_ENTER:
                goOn = false;
                break;
            case KEY_CLOSE:
                goOn = false;
                break;
            case KEY_EXIT:
                goOn = false;
                break;
        }
        
        if (false == goOn)
            break;
    }
    
    //alph->configure();
    //alph->insertSymbol(s);
    
    mssleep(500);
    refresh();
    endwin();
}

static void 
parseCmdLineArgs(int argc, char ** argv, const char *& cfgSource, const char *& scope)
{
    int i;

    cfgSource = "";
    scope = "";

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            usage();
        } else if (strcmp(argv[i], "-cfg") == 0) {
            if (i == argc-1) { usage(); }
            cfgSource = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "-scope") == 0) {
            if (i == argc-1) { usage(); }
            scope = argv[i+1];
            i++;
        } else {
            fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);
            usage();
        }
    }
}



static void
usage()
{
    fprintf(stderr,
        "\n"
        "usage: demo <options>\n"
        "\n"
        "The <options> can be:\n"
        "  -h             Print this usage statement\n"
        "  -cfg <source>  Parse the specified configuration file\n"
        "  -scope <name>  Application scope in the configuration source\n"
        "\n"
        "A configuration <source> can be one of the following:\n"
        "  file.cfg       A configuration file\n"
        "  file#file.cfg  A configuration file\n"
        "  exec#<command> Output from executing the specified command\n\n");
    exit(1);
}