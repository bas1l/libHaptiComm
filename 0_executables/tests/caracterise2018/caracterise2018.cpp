#include <fstream>
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
#include <string>
#include <ncurses.h>
#include <math.h>
#include <vector> 

#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "utils.h"
#include "alphabet.h"

using namespace std;

#ifdef _WIN32
#include <windows.h>
#define SYSERROR()  GetLastError()
#else
#include <errno.h>
#define SYSERROR()  errno
#endif

static void parseCmdLineArgs(int argc, char ** argv, 
                             const char *& cfgSource, const char *& scope);
static void usage();
uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset);
void write_file(std::vector<uint16_t> values, int freq, int ampl, int upto);
std::vector<uint16_t> push_sine_wave_ret(int freq, int ampl, int offset);
std::vector<std::vector<uint16_t>> creatematrix(int nbsample, int value);
void triple_spike(ALPHABET& alph, int chan_current, 
                  std::vector<std::vector<uint16_t>>& result);
void get_sinus(int f, int a, int u, int nos, int chan_current, 
               ALPHABET& alph, std::vector<std::vector<uint16_t>>& result);
int get_up(std::vector<std::vector<uint16_t>>& result, int chan_used);
void get_sinesweep(int fbeg, int fend, int amp1, int amp2, int up, int chan_used,
                   int number_of_rep, std::vector<std::vector<uint16_t>>& result);
void getfrequencies(int *fbeg, int *fend);
std::vector<std::vector<uint16_t> > getvalues(char c, ALPHABET& alph);
void generateSentences(std::queue<char> & sentences, std::condition_variable & cv,
                       std::mutex & m, std::atomic<bool> & workdone, std::string str_alph);
void workSymbols(std::queue<char> & sentences, std::condition_variable & cv, 
                 std::mutex & m, std::atomic<bool> & workdone, ALPHABET& alph);


int main(int argc, char ** argv)
{
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    DEVICE *    dev = new DEVICE();
    WAVEFORM *  wf  = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();
    const char * cfgSource;
    const char * scope;
    int exitStatus = 0;

    
    setlocale(LC_ALL, "");
    parseCmdLineArgs(argc, argv, cfgSource, scope);
    
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
    
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;
    
    
    std::thread extract_text;
    extract_text = std::thread(workSymbols, std::ref(sentences), std::ref(cv), 
                           std::ref(m), std::ref(workdone), std::ref(*alph));
    std::thread send_to_dac;
    send_to_dac = std::thread(generateSentences, std::ref(sentences), std::ref(cv),
                           std::ref(m), std::ref(workdone), alph->getlistSymbols());
    
    extract_text.join();
    send_to_dac.join();
    
    
    delete cfg;
    delete dev;
    delete wf;
    delete alph;
    
    return exitStatus;
}


static void parseCmdLineArgs(int argc, char ** argv, const char *& cfgSource, const char *& scope)
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



static void usage()
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

std::vector<uint16_t> push_sine_wave_ret(int freq, int ampl, int offset)
{
    int phase = 0;
    int nsample = 2000;
    std::vector<uint16_t> sinus;//nothing, just a break.
    
    uint16_t * s = create_sin(freq, ampl, phase, nsample, offset);
    
    for(int i=0; i < nsample; i++){
        sinus.push_back(s[i]);
    }
    //sinus.push_back(2048);
    
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

void get_sinus(int f, int a, int u, int nos, int chan_current, ALPHABET& alph, std::vector<std::vector<uint16_t>>& result)
{
    
    std::vector<std::vector<uint16_t>> wait = creatematrix(2000, 2048);
    std::vector<std::vector<uint16_t>> sinus4all(AD5383::num_channels);
    
    
    std::vector<uint16_t> sinus = push_sine_wave_ret(f, a, u);
    std::vector<uint16_t> waitsinus(sinus.size(), 2048); //std::fill(waitsinus.begin(), waitsinus.end(), 2048);
    
    for(int chan=0; chan<AD5383::num_channels; chan++)
    {
        if (chan == chan_current)
        {
            sinus4all[chan].assign(sinus.begin(), sinus.end());
        }
        else
        {
            sinus4all[chan].assign(waitsinus.begin(), waitsinus.end());
        }
    }
    
    for (int i=0; i!=nos; i++)
    {
        for(int c=0; c<AD5383::num_channels; c++)
        {
            result[c].insert(result[c].end(), sinus4all[c].begin(), sinus4all[c].end());
            result[c].insert(result[c].end(),  wait[c].begin(), wait[c].end());   
        }
    }
    
}

static int amp_get_up = 500;
int get_up(std::vector<std::vector<uint16_t>>& result, int chan_used)
{
    int offset = 2048;
    int freq = 1;
    //int amp = 2000;
    
    std::vector<uint16_t> go_up = push_sine_wave_ret(freq, amp_get_up, offset);
    
    int go_up_length = (int)(go_up.size()/4);
    std::vector<uint16_t> waitsinus(go_up_length, 2048);
    
    for(int c=0; c<AD5383::num_channels; c++)
    {
        if (c == chan_used)
        {
            //result[c].push_back(1000);
            result[c].insert(result[c].end(), go_up.begin()+2*go_up_length, go_up.begin()+3*go_up_length);
        }
        else
        {
            //result[c].push_back(2048);
            result[c].insert(result[c].end(), waitsinus.begin(), waitsinus.end());
        }
    }

    
    
    printw("amp_get_up = %i\n", amp_get_up);
    return go_up[3*go_up_length];
}
    

void get_sinesweep(int fbeg, int fend, int amp1, int amp2, int up, int chan_used, int number_of_rep, std::vector<std::vector<uint16_t>>& result)
{
    //std::vector<uint16_t> sinus = push_sine_wave_ret(f, a, u);
    std::vector<uint16_t> waitsinus(2000, 2048);//sinus.size(), 2048); //std::fill(waitsinus.begin(), waitsinus.end(), 2048);
    
    int amp_used = amp1;
    int f = fbeg;
    
    if (fbeg<10)
    {
        for(f=fbeg; f<=5; f++)
        {
            for (int nor=0; nor<number_of_rep; nor++)
            {
                for(int c=0; c<AD5383::num_channels; c++)
                {
                    if (c == chan_used)
                    {
                        std::vector<uint16_t> sinus = push_sine_wave_ret(f, amp_used, up);
                        result[c].insert(result[c].end(), sinus.begin(), sinus.end());
                    }
                    else
                    {
                        result[c].insert(result[c].end(), waitsinus.begin(), waitsinus.end());
                    }
                }
            }
        }
        
        f=10;
    }
    
    for(; f<=fend; f+=5)
    {
        for (int nor=0; nor<number_of_rep; nor++)
        {
            for(int c=0; c<AD5383::num_channels; c++)
            {
                if (c == chan_used)
                {
                    std::vector<uint16_t> sinus = push_sine_wave_ret(f, amp_used, up);
                    result[c].insert(result[c].end(), sinus.begin(), sinus.end());

                }
                else
                {
                    result[c].insert(result[c].end(), waitsinus.begin(), waitsinus.end());
                }
            }
        }
    }

    // for amp2
    if (0)
    {
        int amp_used = amp2;
        for(int f=fbeg; f<=fend; f+=5)
        {
            for (int nor=0; nor<number_of_rep; nor++)
            {
                for(int c=0; c<AD5383::num_channels; c++)
                {
                    if (c == chan_used)
                    {
                        std::vector<uint16_t> sinus = push_sine_wave_ret(f, amp_used, up);
                        result[c].insert(result[c].end(), sinus.begin(), sinus.end());

                    }
                    else
                    {
                        result[c].insert(result[c].end(), waitsinus.begin(), waitsinus.end());
                    }
                }
            }
        }
    }
    
    
}

static int f_state = 1;
void getfrequencies(int *fbeg, int *fend)
{
    
    int fmin = 10;
    int fmax = 500;
    
    if (f_state == 1)
    {
        *fbeg = 1;
        *fend = 99;
    }
    else if (f_state == 2)
    {
        *fbeg = *fend+1;
        *fend = *fend+100;
    }
    else if (f_state == 3)
    {
        *fbeg = *fend+1;
        *fend = *fend+100;
    }
    else if (f_state == 4)
    {
        *fbeg = *fend+1;
        *fend = *fend+100;
    }
    else if (f_state == 5)
    {
        *fbeg = *fend+1;
        *fend = *fend+100;
    }
    
    printw("Frequencies statement = %i\n", f_state);
    
    f_state++;
}

std::vector<std::vector<uint16_t> > getvalues(char c, ALPHABET& alph)
{
    static int a = 0;
    static int u = 0;
    static int f = 0;
    
    static int freq[] = {1, 2, 5, 10, 20, 50, 100, 200, 300, 500, 700};
    static int freq_max = sizeof(freq)/sizeof(int);

    static int ampl[] = {50, 100, 150, 200, 250, 300, 400, 600, 800, 1000};
    static int ampl_max = sizeof(ampl)/sizeof(int);
    
    static int upto[] = {2048};
    //static int upto_max = sizeof(upto)/sizeof(int);
    
    std::vector< std::vector< uint16_t>> result(AD5383::num_channels);
    static int up = 2048;
    
    int chan_used = 23;// ACT_RINGFINGER2
    switch (c)
    {
        
        case 'q':
        {
            f_state = 1;
            printw("f_state is back to 1\n");
        }
            
        case 'k' :
        {
            int ff = freq[f];
            int aa = ampl[a];
            int uu = upto[u];
            int num_of_sinus = 2;
            
            triple_spike(alph, chan_used, result); // to fit between the laser data and the theoric data
            get_sinus(ff, aa, uu, num_of_sinus, chan_used, alph, result);
            
            std::vector<uint16_t> towrite(result[chan_used].begin(), result[chan_used].end());
            write_file(towrite, ff, aa, uu);
            
            printw("::After : go. \n");
            break;
        }
        case 'a' :     
        {
            amp_get_up +=50;
            printw("amp_get_up = %i\n", amp_get_up);
            if (a == ampl_max-1)
            {
                if (f == freq_max-1)
                {
                    //printw("::After(f=%i,a=%i)/Impossible\t", freq[f], ampl[a]);
                }
                else
                {
                    f += 1;
                    a = 0;
                    //printw("::After(f=%i,a=%i).", freq[f], ampl[a]);
                }
            }
            else
            {
                a += 1;
                //printw("::After(f=%i,a=%i).", freq[f], ampl[a]);
            }
            
            
        break;
        }
        case 'b' :
        {   
            amp_get_up -=50;
            printw("amp_get_up = %i\n", amp_get_up);
            if (a == 0)
            {
                if (f == 0)
                {
                    //printw("::After(f=%i,a=%i)/Impossible\t", freq[f], ampl[a]);
                }
                else
                {
                    f -= 1;
                    a = ampl_max-1;
                    //printw("::After(f=%i,a=%i).", freq[f], ampl[a]);
                }
                
            }
            else
            {
                a -= 1;
                //printw("::After(f=%i,a=%i).", freq[f], ampl[a]);
            }
            
        break;
        }
        
        case 'u':
        {
            up = get_up(result, chan_used);
            break;
        }
        case 'f':
        {
            int fbeg;
            int fend;
            int upvalue = 2048-400;
            int amp1 = 1500;
            int amp2 = 500;
            int number_of_rep = 2;
            
            triple_spike(alph, chan_used, result); // to fit between the laser data and the theoric data
            
            // f_state = 1 (1-100Hz)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            
            number_of_rep = 1;
            
            // f_state = 2 (100-200)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            // f_state = 3 (200-300)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            
            std::vector<uint16_t> towrite(result[chan_used].begin(), result[chan_used].end());
            write_file(towrite, 1, amp1, 300);
            
            //printw("f_beg = %i, f_end = %i, size(ms) = %i\n", fbeg, fend, csize/2);
            printw("1hz to 300Hz\n");//, fbeg, fend, csize/2);
            break;
        }
        case 'g':
        {
            int fbeg;
            int fend;
            int upvalue = 2048-400;
            int amp1 = 1500;
            int amp2 = 500;
            int number_of_rep = 1;
            
            triple_spike(alph, chan_used, result); // to fit between the laser data and the theoric data
            
            // f_state = 4 (300-400Hz)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            // f_state = 5 (400-500)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            std::vector<uint16_t> towrite(result[chan_used].begin(), result[chan_used].end());
            write_file(towrite, 300, amp1, 500);
            
            //printw("f_beg = %i, f_end = %i, size(ms) = %i\n", fbeg, fend, csize/2);
            printw("300hz to 500Hz\n");//, fbeg, fend, csize/2);
            break;
        }
        case 'r':
        {
            int fbeg;
            int fend;
            int upvalue = 2048-400;
            int amp1 = 1000;
            int amp2 = 500;
            int number_of_rep = 2;
            
            triple_spike(alph, chan_used, result); // to fit between the laser data and the theoric data
            
            // f_state = 1 (10-100Hz)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            
            number_of_rep = 1;
            
            // f_state = 2 (100-200)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            // f_state = 3 (200-300)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            std::vector<uint16_t> towrite(result[chan_used].begin(), result[chan_used].end());
            write_file(towrite, 10, amp1, 300);
            //printw("f_beg = %i, f_end = %i, size(ms) = %i\n", fbeg, fend, csize/2);
            printw("10hz to 300Hz\n");//, fbeg, fend, csize/2);
            break;
        }
        case 't':
        {
            int fbeg;
            int fend;
            int upvalue = 2048-400;
            int amp1 = 1000;
            int amp2 = 500;
            int number_of_rep = 1;
            
            triple_spike(alph, chan_used, result); // to fit between the laser data and the theoric data
            
            // f_state = 4 (300-400Hz)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            // f_state = 5 (400-500)
            getfrequencies(&fbeg, &fend);
            get_sinesweep(fbeg, fend, amp1, amp2, upvalue, chan_used, number_of_rep, result);
            
            std::vector<uint16_t> towrite(result[chan_used].begin(), result[chan_used].end());
            write_file(towrite, 300, amp1, 500);
            //printw("f_beg = %i, f_end = %i, size(ms) = %i\n", fbeg, fend, csize/2);
            printw("300hz to 500Hz\n");//, fbeg, fend, csize/2);
            break;
        }
        
        case 'n' :
        {
            //result = alph.getneutral();
            for(int c=0; c<AD5383::num_channels; c++)
            {
                result[c].push_back(2048);
                result[c].push_back(2048);
            }
            break;
        }
        
        default :
            
        break;
    }
    
    // CLEANNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
    //for (int w=0; w<result.size(); ++w)
    //{
     //   result[w].clear();
    //}
    
    
    return result;
}


void generateSentences(std::queue<char> & sentences, std::condition_variable & cv,
            std::mutex & m, std::atomic<bool> & workdone, std::string str_alph)
{

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    std::string str_ponc = " .,;:!?-'";
    
    int ch;
    printw("You can start to write a letter, a word, a sentence \n --- When you are done, press '*' to Exit ---\n");
    do{
        if (ch != ERR) 
        {
            // if part of the alphabet
            if (str_alph.find(ch) != std::string::npos)
            {
                printw("%c", ch);
                
                std::unique_lock<std::mutex> lk(m);
                sentences.push(ch);
                
                cv.notify_one();
                cv.wait(lk);
            }// if part of the ponctuation
            else if (str_ponc.find(ch) != std::string::npos)
            {
                printw("%c", ch);
            }
            else
            {
                printw("\n<Key not implemented> Need to Exit ? Press '*'.\n");
            }
          }
    }while((ch = getch()) != '*');
    
    
    printw("\tWHC::create_sentences::End\n");
    refresh();
    endwin();
    workdone = true;
    cv.notify_one(); // Notify worker
}



void workSymbols(std::queue<char> & sentences, std::condition_variable & cv, 
                std::mutex & m, std::atomic<bool> & workdone, ALPHABET& alph)
{
    // init drive electronics
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    // fréquence maximale pour les sinus utilisées
    double hz_max = 1000; //Hz=1/s
    // th. de Nyquist implique :
    double freq_message = hz_max*2; // 2000 message / secondes (par chan)
    // un peu bizarre. Mais on souhaite faire 2 envoies de messages par millisec
    double timePmessage_ns = hz_max/freq_message * ms2ns; // * ns
    
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    values = alph.getneutral();
    ad.execute_trajectory(values, timePmessage_ns);
    for (int w=0; w<values.size(); ++w)
    {
        values[w].clear();
    }
    
    std::queue<char> letters;
    
    // Initialisation complete.
    cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
    
    // The goal of this function is to use the letters putted by the other
    // thread, one by one, and play them consecutively.
    while(!workdone.load() or !sentences.empty())
    {
        std::unique_lock<std::mutex> lk(m);

        if (sentences.empty())
        {
            cv.wait(lk); // Wait for the generator to complete
        }
        if (sentences.empty()) 
            continue;
        
        // free the common value asap
        letters.push(sentences.front());
        sentences.pop();
        cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
        
        // if last char is a space, then a word is finished
        if (letters.front() != ' ')// is part of the alphabet){
        {
            values = getvalues(letters.front(), alph);
            ad.execute_trajectory(values, timePmessage_ns);
            
            // for all channels, clear.
            for (int w=0; w<values.size(); ++w)
            {
                values[w].clear();
            }
        }
        letters.pop();
     }
}



