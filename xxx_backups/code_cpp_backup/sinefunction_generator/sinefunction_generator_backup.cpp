
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

    

static int number_of_the_file = 1;
void write_file(std::vector<uint16_t> values, int freq, int ampl, int upto)
{
    std::string path = "caracterise2018/results/";
    std::string name =  path +
                        "n" + std::to_string(number_of_the_file) +
                        "_up" + std::to_string(upto) + 
                        "f" + std::to_string(freq) +
                        "amp" + std::to_string(ampl) +
                        "_theoric.csv";
    ofstream fichier(name, ios::out | ios::trunc);  //déclaration du flux et ouverture du fichier

    //cout << name << endl;
    if(fichier.is_open())  // si l'ouverture a réussi
    {
        // instructions
        for (int w=0; w<values.size(); ++w)
        {
            fichier << values[w];  // on l'affiche
            fichier << ", ";  // on l'affiche
        }
        fichier.close();  // on referme le fichier
    }
    else  // sinon
    {
        std::cerr<<"Failed to open file : "<<SYSERROR()<<std::endl;
        //cerr << "write_file/Erreur a l'ouverture !" << endl;
    }
    
    number_of_the_file++;
}

/***********************************
 *
 *          Creation of 
 *          arrays
 *          vectors
 *          matrices
 * 
 **********************************/
uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset)
{
	uint16_t * s;
	s = (uint16_t*) malloc(nsample * sizeof(uint16_t));
	
        // Suivant le nombre d'échantillons voulus :
	float incr = 2*M_PI/((float)nsample);
	for (int i=0; i<nsample; i++){
                //s[i] = sin(i*incr*freq +phase);
		s[i] = (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset);
                //printw("%i,", s[i]);
	}
	return s;
}

std::vector<uint16_t> createsine_vector(int freq, int ampl, int offset, double phase, int nsample)
{
    std::vector<uint16_t> sinus;
    float incr = 2*M_PI/((float)nsample);
    
    for(int i=0; i < nsample; i++){
        sinus.push_back((uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset));
    }
    
    return sinus;
}

std::vector<std::vector<uint16_t>> creatematrix(int nbsample, int value)
{
    std::vector<std::vector<uint16_t>> result(AD5383::num_channels);
    std::vector<uint16_t> onechan(nbsample, value);
    
    for(int c=0; c<AD5383::num_channels; c++)
    {
        result[c].assign(onechan.begin(), onechan.end());
    }
    
    return result;
}




/***********************************
 *
 *          Creation of 
 *          patterns
 * 
 **********************************/
void triple_spike(ALPHABET& alph, int chan_current, std::vector<std::vector<uint16_t>>& result)
{
    
    std::vector<std::vector<uint16_t>> spike = creatematrix(20, 0);
    std::vector<std::vector<uint16_t>> waitspike = creatematrix(20, 2048);
    std::vector<std::vector<uint16_t>> wait = creatematrix(2000, 2048);
    
    for (int i=0; i<3; i++)
    {
        for(int c=0; c<AD5383::num_channels; c++)
        {
            result[c].insert(result[c].end(),  wait[c].begin(), wait[c].end());
            if (c == chan_current)
            {
                result[c].insert(result[c].end(), spike[c].begin(), spike[c].end());
            }
            else
            {
                result[c].insert(result[c].end(), waitspike[c].begin(), waitspike[c].end());
            }
            
            
        }
    }
    for(int c=0; c<AD5383::num_channels; c++)
    {
        result[c].insert(result[c].end(),  wait[c].begin(), wait[c].end());
    }
}


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
void getvalues(std::vector<uint16_t> & result, char c, int nsample)
{
    static int f = 1;
    static int a = 1;
    static int u = 2048;
    
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
        case 'u':
        {// up statement
            u = get_up(result, nsample);
            break;
        }
        case 'n' :
        {// neutral statement
            u = 2048;
            result.clear();
            result.push_back(u);
            result.push_back(u);
            break;
        }
        default :
        {
            break;
        }
    }
    
    if ((f+fadd) <= 0)
    {
        printw("f<=0");
    }
    if ((a+aadd) <= 0)
    {
        printw("a<=0");
    }
    
    if (((f+fadd) > 0) && ((a+aadd) > 0) && (c != 'n') )
    {
        f = f+fadd;
        a = a+aadd;
        int phase = 0;
        std::vector<uint16_t> sinus = createsine_vector(f, a, u, phase, nsample);
        
        result.clear();
        result.insert(result.end(), sinus.begin(), sinus.end());
    }
    
    printw("\n");
    printw("f=%i, ", f);
    printw("a=%i, ", a);
    printw("u=%i", u);
    refresh();
     
    
    
}




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
            result.clear();
            result.push_back(*u);
            result.push_back(*u);
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
    printw("\n");
    printw("f=%i, ", *f);
    printw("a=%i, ", *a);
    printw("u=%i", *u);
    refresh();
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


void execute_up(AD5383 & ad, int channel, int nsample)
{
    std::vector<uint16_t> result;
    get_up(result, nsample);
    
    for(int i=0; i<result.size(); i++)
    {
        ad.execute_single_channel(result[i], channel);
    }
}


void print_instructions()
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



int send_DAC(std::queue<char> & letters, std::mutex & mutexLetters, std::atomic<bool> & work, int nmessage_sec)
{   
    DEVICE dev;
    dev.configure();
    WAVEFORM wf;
    wf.configure();
    ALPHABET alph(dev, wf);//, AD5383::num_channels);
    alph.configure();
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    struct actuator current_actuator = dev.getact("rf2");
    
    //int nmessage_sec = 2000; // message/s
    double dur_message_ms = (1/(double)nmessage_sec) *1000; // dur_message_per_sec * sec2ms
    long dur_message_ns = dur_message_ms * ms2ns; // * ns
    ad.execute_trajectory(alph.getneutral(), dur_message_ns);
    
    int channel = current_actuator.chan;
    std::vector<uint16_t> values(10, current_actuator.vneutral);
    
    std::queue<char> letters_in;
        
    int f = 1;
    int a = 1;
    int u = 2048;
    int v_up;
    // get value of the up/offset
    change_variables('u', &f, &a, &v_up);
    
    int ret;
    unsigned long long missed = 0;
    int overruns = 0;
    struct timespec ts = {
        .tv_sec = 0,
                .tv_nsec = dur_message_ns
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
    
    uint16_t current_v = 0;
    std::vector<uint16_t>::iterator valuesit;
    valuesit = values.begin();
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

        //printw("2");
        //refresh();
        
        
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
        
        //printw(".%i", *valuesit);
        //refresh();
        current_v = *valuesit;
        //printw("4");
        //refresh();
        if (!letters_in.empty()) 
        {
            
            if (letters_in.front() == 'u')
            {
                execute_up(ad, channel, nmessage_sec);
                u = v_up;
            }
            changeVariables(letters_in.front(), &f, &a, &u);
            
            /*
            getvalues(values, letters_in.front(), nmessage_sec);
            for(uint16_t v : values)
            {
                printw("%i/", v); refresh();
            }
            */
  
            letters_in.pop();
            
            valuesit = std::find(values.begin(), values.end(), current_v);
        }
        else
        {
            (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset)
            ad.execute_single_channel(current_v, channel);
            //printw(".");
            //refresh();
        }
        
        // check if loop 
        if (valuesit == values.end())
        {
            valuesit = values.begin();
            printw("_");
            refresh();
        }
        ++valuesit;
        //std::advance(valuesit, 1);
    }
    
    
    close(_timer_fd);
    
    return 1;
}



int main(int argc, char *argv[])
{
    if ((argc != 1) && (argc != 2)) 
    {
        fprintf(stderr, "%s need the number of message per second\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int nmessage_sec;
    if (argc == 1) {
        nmessage_sec = 2000;
    } else {
        nmessage_sec = atoi(argv[1]);
    }
    fprintf(stderr, "nmessage_sec = %i\n", nmessage_sec);
    
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
    
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    // global variable
    std::queue<char> letters;
    std::mutex mutexLetters;
    std::atomic<bool> work(true);
    //std::condition_variable cv;
    
    std::thread thread_readLetters(read_letters, std::ref(letters), std::ref(mutexLetters), std::ref(work));
    std::thread thread_sendToDAC(send_DAC, std::ref(letters), std::ref(mutexLetters), std::ref(work), nmessage_sec);

    thread_sendToDAC.join();
    thread_readLetters.join();
    
    
    refresh();
    endwin();
    
    return 0;
}
