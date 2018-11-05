
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

#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "device.h"
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


std::vector<uint16_t>   createsine_vector(int freq, int ampl, int offset, double phase, int nsample);
int                     get_up(std::vector<uint16_t>& result, int nsample);
void                    changeVariables(char c, int * f, int * a, int * u);
int                     execute(AD5383& ad, std::vector<uint16_t>& values, long period_ns, int channel);

void    read_letters(std::queue<char> & letters, std::mutex & mutexLetters, std::atomic<bool> & work);
int     send_DAC(std::queue<char> & letters, std::mutex & mutexLetters, std::atomic<bool> & work, 
             ALPHABET* & alph, DEVICE* & dev);

static void parseCmdLineArgs(int argc, char ** argv, const char *& cfgSource, const char *& scope, int & nmessage_sec);
void        print_fau(int * f, int * a, int * u);
void        print_instructions();
static void usage();



int main(int argc, char *argv[])
{
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    DEVICE *    dev = new DEVICE();
    WAVEFORM *  wf  = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();
    const char * cfgSource;
    const char * scope;
    int nmessage_sec;
    int exitStatus = 0;

    
    setlocale(LC_ALL, "");
    parseCmdLineArgs(argc, argv, cfgSource, scope, nmessage_sec);
    
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
    
    cfg->parse(cfgSource, "HaptiComm");
    cfg->configureDevice(dev);
    cfg->configureWaveform(wf);
    alph->configure(dev, wf);
    
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    // global variable
    std::queue<char> letters;
    std::mutex mutexLetters;
    std::atomic<bool> work(true);
    //std::condition_variable cv;
    
    std::thread thread_readLetters(
                            read_letters, std::ref(letters), 
                            std::ref(mutexLetters), std::ref(work));
    std::thread thread_sendToDAC(
                            send_DAC, std::ref(letters),
                            std::ref(mutexLetters), std::ref(work), 
                            std::ref(alph), std::ref(dev));
    
    thread_sendToDAC.join();
    thread_readLetters.join();
    
    
    refresh();
    endwin();
    
    return exitStatus;
}

/***********************************
 *
 *          Creation of 
 *          arrays
 *          vectors
 *          matrices
 * 
 **********************************/


std::vector<uint16_t> createsine_vector(int freq, int ampl, int offset, double phase, int nsample)
{
    std::vector<uint16_t> sinus;
    float incr = 2*M_PI/((float)nsample);
    
    for(int i=0; i < nsample; i++){
        sinus.push_back((uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset));
    }
    
    return sinus;
}



/***********************************
 *
 *          patterns' makers
 * 
 **********************************/


static int amp_get_up = 500;
int get_up(std::vector<uint16_t>& result, int nsample)
{
    int offset = 2048;
    int freq = 1;
    double phase = M_PI;
    
    std::vector<uint16_t> s = createsine_vector(freq, amp_get_up, offset, phase, nsample);
    
    int go_up_idx = (int)(s.size()/4);
    result.insert(result.end(), s.begin(), s.begin()+go_up_idx);
    /*
    for(int i = 0; i<result.size(); i++)
    {
        printw("%i ", result[i]);
        refresh();
    }
  */
    return s[go_up_idx];
}
    






/***********************************
 *
 *          Functions 
 * 
 **********************************/



void changeVariables(char c, int * f, int * a, int * u)
{
    int fadd = 0;
    int aadd = 0;
    
    switch (c)
    {
        // value to add to the current frequency
        case 'q':
        {
            fadd = -1;
            break;
        }
        case 'w':
        {
            fadd = +1;
            break;
        }
        case 'a':
        {
            fadd = -10;
            break;
        }
        case 's':
        {
            fadd = +10;
            break;
        }
        case 'z':
        {
            fadd = -100;
            break;
        }
        case 'x':
        {
            fadd = +100;
            break;
        }
        // value to add to the current amplitude
        case 'e':
        {
            aadd = -1;
            break;
        }
        case 'r':
        {
            aadd = +1;
            break;
        }
        case 'd':
        {
            aadd = -10;
            break;
        }
        case 'f':
        {
            aadd = +10;
            break;
        }
        case 'c':
        {
            aadd = -100;
            break;
        }
        case 'v':
        {
            aadd = +100;
            break;
        }
        // other type of movement
        case 'n' :
        {// neutral statement
            *u = 2048;
            break;
        }
        default :
        {
            break;
        }
    }
    
        
    if (((*f+fadd) > 0) && ((*a+aadd) > 0)) {
        *f = *f+fadd;
        *a = *a+aadd;
    }
    
    if ((*f+fadd) <= 0) printw("f<=0");
    if ((*a+aadd) <= 0) printw("a<=0");
}



int execute(AD5383& ad, std::vector<uint16_t>& values, long period_ns, int channel)
{
    int vsize = values.size();
    int ret;
    unsigned long long missed = 0;
    int overruns = 0;
    
    struct timespec ts = {
        .tv_sec = 0,
                .tv_nsec = period_ns
    };
    
    struct itimerspec its = {
        .it_interval = ts,
                .it_value = ts
    };
    
    int _timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    if(_timer_fd == -1)
    {
        perror("execute_trajectory/timerfd_create");
        _timer_fd = 0;
        return overruns;
    }
    if(timerfd_settime(_timer_fd, 0, &its, NULL) == -1)
    {
        perror("execute_trajectory/timerfd_settime");
        close(_timer_fd);
        return overruns;
    }
    
    ret = read(_timer_fd, &missed, sizeof(missed));
    if (ret == -1)
    {
        perror("execute_trajectory/read");
        close(_timer_fd);
        return overruns;
    }
    
    for(unsigned int value_idx = 0; value_idx < vsize; ++value_idx)
    {
        //auto t1 = std::chrono::high_resolution_clock::now();
        
        ret = read(_timer_fd, &missed, sizeof(missed));
        if (ret == -1)
        {
            perror("execute_trajectory/read");
            close(_timer_fd);
            return overruns;
        }
        overruns += missed - 1;
        
        ad.execute_single_channel(values[value_idx], channel);
        
        //auto t2 = std::chrono::high_resolution_clock::now();
        //auto int_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        // converting integral duration to integral duration of shorter divisible time unit:
        // no duration_cast needed
        //std::chrono::duration<long, std::nano> dur_usec = int_ns;
        //printw("duration=%ld, ", dur_usec.count());
        //refresh();
    }

    
    //printw(" \n");
    //refresh();
    
    close(_timer_fd);

    return overruns;
}


int execute_up(AD5383 & ad, int channel, int nsample)
{
    std::vector<uint16_t> result;
    int u = get_up(result, nsample);
    
    for(int i=0; i<result.size(); i++)
    {
        ad.execute_single_channel(result[i], channel);
    }
    
    return u;
}

void read_letters(std::queue<char> & letters, std::mutex & mutexLetters, std::atomic<bool> & work)
{
    int ch = ERR;
    std::string str_used = "qwaszxerdfcvun";
    
    print_instructions();
    
    do
    {
        if (ch != ERR)
        {
            if (str_used.find(ch) != std::string::npos)
            {
                try
                {// using a local lock_guard to lock mtx guarantees unlocking on destruction / exception:
                    std::lock_guard<std::mutex> lk(mutexLetters);
                    letters.push(ch);
                }
                catch (std::logic_error&)
                {
                    std::cout << "[exception caught]\n";
                }
            }
            //printw("%c", ch);
        }
    }while((ch = getch()) != '*');
    
    work = false;
}



int send_DAC(std::queue<char> & letters, std::mutex & mutexLetters,
             std::atomic<bool> & work, 
             ALPHABET* & alph, DEVICE* & dev)
{   
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    struct actuator current_actuator = dev->getActuator("rf2");
    int channel = current_actuator.chan;
    
    double  durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int     durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    
    ad.execute_trajectory(alph->getneutral(), durationRefresh_ns);
    
    std::queue<char> letters_in;
    float incr = 2*M_PI/((float)alph->getFreqRefresh_mHz()*1000);   
    int i = 0;
    int f = 1;
    int a = 1;
    int u = 2048;
    
    int ret;
    unsigned long long missed = 0;
    int overruns = 0;
    struct timespec ts = {
        .tv_sec = 0,
                .tv_nsec = durationRefresh_ns
    };
    struct itimerspec its = {
        .it_interval = ts,
                .it_value = ts
    };
    int _timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    if(_timer_fd == -1)
    {
        perror("execute_trajectory/timerfd_create");
        _timer_fd = 0;
        return overruns;
    }
    if(timerfd_settime(_timer_fd, 0, &its, NULL) == -1)
    {
        perror("execute_trajectory/timerfd_settime");
        close(_timer_fd);
        return overruns;
    }
    
    ret = read(_timer_fd, &missed, sizeof(missed));
    if (ret == -1)
    {
        perror("execute_single_channel/read_first");
        close(_timer_fd);
        return overruns;
    }
    
    
    while(work.load())
    {
        //printw("1");
        //refresh();
        ret = read(_timer_fd, &missed, sizeof(missed));
        if (ret == -1)
        {
            perror("execute_single_channel/read");
            close(_timer_fd);
            return overruns;
        }
        overruns += missed - 1;

        try
        {// using a local lock_guard to lock mtx guarantees unlocking on destruction / exception:
            std::lock_guard<std::mutex> lk(mutexLetters);
            if (!letters.empty())
            {
                letters_in.push(letters.front());
                letters.pop();
            }
        }
        catch (std::logic_error&)
        {
            std::cout << "[exception caught]\n";
        }
        
        if (!letters_in.empty()) 
        {
            if (letters_in.front() == 'u')
            {
                u = execute_up(ad, channel, durationRefresh_ns);
            }
            else
            {
                changeVariables(letters_in.front(), &f, &a, &u);
            }
            
            clear();
            print_instructions();
            print_fau(&f, &a, &u);
            letters_in.pop();
        }
        else
        {
            ad.execute_single_channel((uint16_t) floor(a * sin(i*incr*f) + u), channel);
        }

        i = (i+1)%durationRefresh_ns;
    }
    
    
    close(_timer_fd);
    
    return 1;
}




static void 
parseCmdLineArgs(
        int argc, char ** argv, 
        const char *& cfgSource, const char *& scope, int & nmessage_sec)
{
    int i;

    cfgSource = "";
    scope = "";
    nmessage_sec = 2000;

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
            } else if (strcmp(argv[i], "-nmessage") == 0) {
                    if (i == argc-1) { usage(); }
                    nmessage_sec = atoi(argv[i+1]);
                    i++;
            } else {
                    fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);
                    usage();
            }
    }
}



void 
print_fau(int * f, int * a, int * u)
{
    
    printw("\n");
    printw("f=%i, ", *f);
    printw("a=%i, ", *a);
    printw("u=%i", *u);
    refresh();
}

void 
print_instructions()
{
    printw("---------------------------------------\n");
    printw("\tSine function generator\n");
    printw("---------------------------------------\n");
    
    printw("[Modify amplitude]\n");
    printw("\t+/-1   : 'q'=decrease, 'w'=increase\n");
    printw("\t+/-10  : 'a'=decrease, 's'=increase\n");
    printw("\t+/-100 : 'z'=decrease, 'x'=increase\n");
    
    printw("[Modify frequency]\n");
    printw("\t+/-1   : 'e'=decrease, 'r'=increase\n");
    printw("\t+/-10  : 'd'=decrease, 'f'=increase\n");
    printw("\t+/-100 : 'c'=decrease, 'v'=increase\n");
    
    printw("[Modify statement(offset)]\n");
    printw("\tUp      : 'u'\n");
    printw("\tNeutral : 'n'\n");
    
    printw("--- When you are done, press '*' to Exit ---\n");
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

    