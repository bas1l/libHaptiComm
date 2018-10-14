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






void        generateSentences(  std::queue<char> & sentences, 
                                std::condition_variable & cv,
                                std::mutex & m, std::atomic<bool> & workdone, 
                                std::string str_alph);
void        workSymbols(std::queue<char> & sentences, 
                        std::condition_variable & cv, 
                        std::mutex & m, std::atomic<bool> & workdone, 
                        ALPHABET* & alph);
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
    
    //std::vector<std::string> idMotions = {"tap", "apparent", "tapSync", "tapHoldSync"};
    //wf->printWAVData(idMotions[0]);
    //wf->printData(idMotions[0]);
    //alph->printData("h");
    
    cout << "END" << endl;
    
    /* INITIALISE THREAD
     * 
     */
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;
    
    std::thread extract_text;
    extract_text = std::thread( workSymbols, 
                                std::ref(sentences), std::ref(cv), 
                                std::ref(m), std::ref(workdone), 
                                std::ref(alph));
    std::thread send_to_dac;
    send_to_dac = std::thread(  generateSentences, 
                                std::ref(sentences), std::ref(cv),
                                std::ref(m), std::ref(workdone), 
                                alph->getlistSymbols());
    
    
    /* WORK
     * 
     */
    extract_text.join();
    send_to_dac.join();
    
    
    /* CLEAN
     * 
     */
    delete cfg;
    delete dev;
    delete wf;
    delete alph;
    
    return exitStatus;
}


void generateSentences(std::queue<char> & sentences, std::condition_variable & cv,
            std::mutex & m, std::atomic<bool> & workdone, std::string str_alph)
{

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    std::string str_ponc = " '.,;:!?-";
    //printw("alphabet:%s", str_alph.c_str());
    
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
                //printw("\n<Key not implemented> Need to Exit ? Press '*'.\n");
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
                std::mutex & m, std::atomic<bool> & workdone, ALPHABET *& alph)
{
    // init drive electronics
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    
    int timerInterLetterMs = 20;
    
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    
    ad.execute_trajectory(alph->getneutral(), durationRefresh_ns);
    
    waveformLetter values;
    std::queue<char> letters;
    int overruns = 0;
    
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
        if (letters.front() == '+')// is part of the alphabet){
        {
            timerInterLetterMs += 20;
        }
        else if (letters.front() == '-' && timerInterLetterMs>20)
        {
            timerInterLetterMs -= 20;
        }
        else
        {
            values = alph->getl(letters.front());
            int ovr = ad.execute_selective_trajectory(values, durationRefresh_ns);
            if (ovr)
            {
                    overruns += ovr;
                    //cout << "Overruns: " << overruns << endl;
            }
            values.clear();
            usleep(timerInterLetterMs);
        }
        letters.pop();
     }
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

    
