#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <queue>
#include <random>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

#include "ctype.h"
// hapticomm headers
#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "haptiBrailleDriver.h"
#include "utils.h"
#include "alphabet.h"

#define GPIO_OUT 1
#define GPIO_LOW  0
#define GPIO_HIGH 1

#define PROSODY_WITHOUT	 0
#define PROSODY_LETTER 	 1
#define PROSODY_WORD   	 2
#define PROSODY_SENTENCE 3

// all in milliseconds
#define PROSODY_LETTER_DELAY 	100
#define PROSODY_WORD_DELAY   	200
#define PROSODY_SENTENCE_DELAY  500
#define DURATION_SYMBOL  	50

std::mutex m_mutex;
std::condition_variable m_condVar;
std::atomic<bool> workdone;
std::queue<char> sentences;
bool sentencesEmpty;

void generateSentences(std::atomic<bool> & workdone, std::string str_alph);
void workSymbols(std::atomic<bool> & workdone, ALPHABET* & alph, DEVICE * dev, int prosody);
bool getWordActIDs(ALPHABET *& alph, std::deque<char> * letters, std::vector<uint8_t> * output);
bool getWordValue(ALPHABET *& alph, std::deque<char> * letters, waveformLetter * output);
  
int  setOpt(int *argc, char *argv[], int * prosody, const char *& cfgSource, const char *& scope);
static void usage();

bool signal4keypressed();
void copySentences(std::deque<char> * letters);
void addto_sentence(char ch);
bool is_sentencesEmpty();
void jobdone();
bool is_jobdone();


using namespace std;


int main(int argc, char *argv[])
{
    /* CREATE VARIABLES */
    const char * cfgSource;
    const char * scope;
    int exitStatus = 0, prosody;
    std::thread extract_text;
    std::thread send_to_dac;
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    DEVICE 	 * dev  = new DEVICE();
    WAVEFORM * wf   = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();

    /*INITIALISE GLOBAL VARIABLES
     * 
     */
    workdone = false;
    sentencesEmpty = true;
    /* INITIALISE VARIABLES
     * 
     */
    /* time of prosody (letter, word or sentence rythms) */
    setOpt(&argc, argv, &prosody, cfgSource, scope);
    cfg->configure(cfgSource, dev, wf, alph);
    std::cout<<"[initialisation][options] prosody=";
    switch (prosody)
    {
      case PROSODY_WITHOUT:
          std::cout<<"PROSODY_WITHOUT"<<std::endl;
          break;
      case PROSODY_LETTER:
          std::cout<<"PROSODY_LETTER"<<std::endl;
          break;
      case PROSODY_WORD:
          std::cout<<"PROSODY_WORD"<<std::endl;
          break;
      case PROSODY_SENTENCE:
          std::cout<<"PROSODY_SENTENCE"<<std::endl;
          break;
    }
    std::cout<<"[initialisation][options] cfgSource="<<cfgSource<<std::endl;
    std::cout<<"[initialisation] ...done."<<std::endl;
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
    setlocale(LC_ALL, "");

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    /* INITIALISE THREAD
     * 
     */
    extract_text = std::thread(workSymbols, std::ref(workdone), std::ref(alph), std::ref(dev), prosody);
    send_to_dac = std::thread( generateSentences, std::ref(workdone), alph->getlistSymbols());
    
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


void generateSentences(std::atomic<bool> & workdone, std::string str_alph)
{
  std::string s = "the quick brown fox jumps over the lazy dog";
  std::string str_prosody = " .";
  std::string str_ponc = "',;:!?-";
  //printw("alphabet:%s", str_alph.c_str());
  
  int ch;
  
  printw("You can start to write a letter, a word, a sentence \n");
  printw(" --- When you are done, press '*' to Exit ---\n");
  do{
    if (ch != ERR)
    {
      // if part of the alphabet
      if (  str_alph.find(ch) != std::string::npos ||
            str_prosody.find(ch) != std::string::npos)
      {
        printw("%c", ch);
        addto_sentence(ch);
      }// if part of the ponctuation
      else if (str_ponc.find(ch) != std::string::npos)
      {
        printw("%c", ch);
      }
      else if (ch == '0')
      {
        // printw("%s. ", s);
        for(int i=0; i<s.length();++i)
        {
          printw("%c", s[i]);
          addto_sentence(s[i]);
        }
      }
      else
      {
        //printw("\n<Key not implemented> Need to Exit ? Press '*'.\n");
      }
    }
  }while((ch = tolower(getch())) != '*');
  
  
  printw("\tWHC::create_sentences::End\n");
  refresh();
  endwin();
  
  jobdone();
}



void workSymbols(std::atomic<bool> & workdone, ALPHABET *& alph, DEVICE * dev, int prosody)
{
  // init drive electronics
  HAPTIBRAILLEDRIVER hb_driver;
  waveformLetter values;
  std::deque<char> letters;
  int duration_ms;
  actuatorsMap act_map = dev->getActuatorMap();
  std::vector<int> actuators_pins;
  
  for (actuatorsMap::iterator it=act_map.begin(); it!=act_map.end(); ++it)
  {
    actuators_pins.push_back(it->second.chan);
  }
  hb_driver.configure(actuators_pins);
  duration_ms  = DURATION_SYMBOL;
  
  // The goal of this function is to use the letters putted by the other
  // thread, one by one, and play them consecutively.
  while(!is_jobdone())
  {
    signal4keypressed();
    copySentences(&letters);
    
    // if last char is a space, then a word is finished
    if ((letters.front() == '+') || (letters.front() == '-'))
    {
      letters.pop_front();
      continue;
    }
    else if (prosody == PROSODY_SENTENCE && letters.end() != std::find(letters.begin(), letters.end(), '.'))
    {
      while (!letters.empty())
      {
        if (letters.front() == ' ')
        {
          letters.pop_front();
          std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_WORD_DELAY));
        }
        else if (letters.front() == '.')
        {
          letters.pop_front();
          std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_SENTENCE_DELAY));
        }
        else
        {
          values = alph->getl(letters.front());
          hb_driver.executeSymbol(values, duration_ms);
          values.clear();
          letters.pop_front();
          std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_LETTER_DELAY));
        }
      }
    }
    else if (prosody == PROSODY_WORD && letters.end() != std::find(letters.begin(), letters.end(), ' '))
    {
      while (!letters.empty())
      {
        if (letters.front() == ' ')
        {
          letters.pop_front();
          std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_WORD_DELAY));
        }
        else if (letters.front() == '.')
        {
          letters.pop_front();
        }
        else
        {
          static std::vector<uint8_t> v;
          do {
            bool found = getWordActIDs(alph, &letters, &v);
            
            if (found)
            {
              hb_driver.executeSymbol6by6(v, duration_ms, PROSODY_LETTER_DELAY/1.5);
              v.clear();
              std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_LETTER_DELAY));
            }
            else
            {
              letters.clear();
              std::cout<<"?"<<std::flush;
            }
            
          } while (false);// (letters.size() != 0);
        }
      }
    }
    else if (prosody == PROSODY_LETTER || prosody == PROSODY_WITHOUT)
    {
      if (letters.front() == ' ' || letters.front() == '.')
      {
        letters.pop_front();
      }
      else
      {
        while (!letters.empty())
        {
          values = alph->getl(letters.front());
          hb_driver.executeSymbol(values, duration_ms);
          values.clear();
          letters.pop_front();
          std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_LETTER_DELAY));
        }
      }
    }
  }
}


bool getWordActIDs(ALPHABET *& alph, std::deque<char> * letters, std::vector<uint8_t> * output)
{
  string input = "";
  bool found = false;
  
  // fill with all the input characters 
  for (auto it = letters->begin(); it != letters->end(); it++)
  {
    input += *it;
  }
  
  // work
  while (input.size()>0 && !found)
  {
    *output = alph->getActList_chan(input);
    if (output->size() == 0)
      input.pop_back();
    else
      found = true;
  }
  
  // remove the letters
  letters->erase(letters->begin(), letters->begin()+input.size());
  
  return found;
}


bool getWordValue(ALPHABET *& alph, std::deque<char> * letters, waveformLetter * output)
{
  string input = "";
  bool found;
  
  // fill the full sentence
  std::cout << std::endl;
  for (auto it = letters->begin(); it != letters->end(); it++)
  {
    input += *it;
    //std::cout << *it << std::endl;
  }
  //std::cout << input << std::endl;
  //std::cout << "input, size=" << input.size() << ", length="<<input.length() << std::endl;
  
  // work
  while (input.size() > 0)
  {
    found = alph->getword(input, output);
    if (found == false)
      input.pop_back();
    else
      break;
  }
  
  // remove the letters
  //std::cout << "remove the letters..." << std::endl;
  letters->erase(letters->begin(), letters->begin()+input.size());
  /*std::cout << "letters remaining..." << std::endl;
  for (auto it = letters->begin(); it != letters->end(); it++)
  {
    std::cout << *it << std::endl;
  }
  */
  
  return found;
}

static void
usage()
{
	fprintf(stderr,
            "\n"
	    "usage: demo <options>\n"
            "\n"
	    "The <options> can be:\n"
	    "  -h, --help\n"
		"\tPrint this usage statement\n"
	    "  -c, --cfg <source>\n"
		"\tParse the specified configuration file\n"
	    "  --scope <name>\n"
		"\tApplication scope in the configuration source\n"
		"You can set up one of the following no argument options:\n"
		"  -l, --letter\n"
		"\tApply instantaneously symbols when a key is pressed.\n"
		"  -w, --word\n"
		"\tApply the all symbols when a 'space' appears.\n"
		"  -s, --sentence\n"
		"\tApply the all symbols when a 'dot' appears.\n"
	    "\n"
	    "A configuration <source> can be one of the following:\n"
	    "  file.cfg       A configuration file\n"
	    "  file#file.cfg  A configuration file\n"
	    "  exec#<command> Output from executing the specified command\n\n");
	exit(1);
}


/* 
 * getopt_long() refers to '-' hyphen for a single alphanumeric symbol option, '--' for a long option 
 * getopt_long_only() check '-' for long option first, if not found check a single alphanumeric symbol option
 */
int setOpt(int *argc, char *argv[], int * prosody, const char *& cfgSource, const char *& scope)
{
	string help 	 	 = "help";
	string letter_long 	 = "letter";
	string word_long 	 = "word";
	string sentence_long = "sentence";
	string cfg_long 	 = "cfg";
	string scope_long 	 = "scope";
	
	static struct option long_options[] =
	{
	    {help.c_str(), 		no_argument, NULL, 'h'},
	    {letter_long.c_str(), 	no_argument, NULL, 'l'},
	    {word_long.c_str(),  	no_argument, NULL, 'w'},
	    {sentence_long.c_str(), no_argument, NULL, 's'},
	    {cfg_long.c_str(), required_argument, NULL, 'c'},
	    {scope_long.c_str(), optional_argument, NULL, 'p'},
	    {NULL, 0, NULL, 0}
	};

	// character of the option
	int c;
	
	*prosody=0;
	// Detect the end of the options. 
	while ((c = getopt_long(*argc, argv, "hlwscp", long_options, NULL)) != -1)
	{
	    switch (c)
	    {
		case 'l':
		    *prosody = PROSODY_LETTER;
		    break;

		case 'w':
		    *prosody =  PROSODY_WORD;
		    break;
		    
		case 's':
		    *prosody =  PROSODY_SENTENCE;
		    break;

		case 'c':
		    cfgSource = optarg;
		    break;

		case 'p':
		    scope = optarg;
		    break;
		    
		case 'h':
		    usage();
		    return -1;
		    
		case '?':
		    /* getopt_long already printed an error message. */
		    *prosody =  -1;
		    break;
	    
		default:
		    abort();
	    }
	}
	
	
	return 0;
}


/*
 * SHARED VARIABLES MODIFIERS
 */
void addto_sentence(char ch)
{
	std::lock_guard<std::mutex> lk(m_mutex); // locker to access shared variables
	sentences.push(ch);
	sentencesEmpty = false;
	m_condVar.notify_one(); // waiting thread is notified 
}

void copySentences(std::deque<char> * letters)
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	while (!sentences.empty()){
		letters->push_back(sentences.front());
		sentences.pop();
	}
	sentencesEmpty = true;
	m_condVar.notify_one(); // waiting thread is notified 
}

void jobdone()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	workdone = true;
	sentencesEmpty = false;
	m_condVar.notify_one(); // waiting thread is notified 
}

/*
 * DAEMON'S FUNCTIONS
 */
bool signal4keypressed()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	m_condVar.wait(mlock, []{return !is_sentencesEmpty();} );
	return true;
}

/*
 * SHARED VARIABLES CHECKERS
 */
bool is_sentencesEmpty()
{
	return sentencesEmpty;
}

bool is_jobdone()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	return (workdone.load() && is_sentencesEmpty());
}
























