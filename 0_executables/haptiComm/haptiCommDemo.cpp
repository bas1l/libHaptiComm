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
#include <signal.h> // for SIGWINCH signal
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

// RPI3 gpios values
#define GPIO_OUT 1
#define GPIO_LOW  0
#define GPIO_HIGH 1

// prosody arrays separator index and values
#define PROSODY_WITHOUT	 0
#define PROSODY_LETTER 	 1
#define PROSODY_WORD   	 2
#define PROSODY_SENTENCE 3
int PROSODY_SEPARATORS[] = { '\0', '\0', ' ', '.' }; //without, letter, word, and sentence

// all in milliseconds
#define PROSODY_LETTER_DELAY 	700
#define PROSODY_WORD_DELAY   	1000
#define PROSODY_SENTENCE_DELAY  500
#define DURATION_SYMBOL  	700

// keyboard keys
#define KEY_ESC 27 // escape key
#define KEY_ERASE 263 // erase key
#define KEY_CTRLC 0x03 // CTRL+C
#define KEY_Fmin 265
#define KEY_Fmax 276
#define KEY_F1 265
#define KEY_F2 266
#define KEY_F3 267
#define KEY_F4 268
#define KEY_F5 269
#define KEY_F6 270
#define KEY_F7 271
#define KEY_F8 272
#define F_KEYPRESS(k) (k>=KEY_Fmin && k<=KEY_Fmax)
#define EXIT_KEYPRESS(k) (k==KEY_ESC || k==KEY_CTRLC)

// global variables
int STATE_PROGRAM;
// global variables threads
std::mutex m_mutex;
std::condition_variable m_condVar;
std::atomic<bool> workdone;
std::deque<char> sentences;
bool sentencesEmpty;
bool sentencesReady;

// thread functions
void generateSentences(std::atomic<bool> & workdone, std::string str_alph,
    char separator);
void workSymbols(std::atomic<bool> & workdone, ALPHABET* & alph, DEVICE * dev);

// checkers
bool signal4keypressed();
bool is_sentencesEmpty();
bool is_sentencesReady();
bool is_jobdone();

// setters and getters
void set_sentencesReady();
void set_jobdone();
void addfrom_sentences(std::deque<char> * letters);
void addto_sentence(char ch);
bool rmto_sentence();

// configuration functions
void create_window();
void init_window();
void rmto_window();
void close_window();
void resize_handler(int sig);
int setOpt(int *argc, char *argv[], int * prosody, const char *& cfgSource,
    const char *& scope);
static void usage();

using namespace std;

int main(int argc, char *argv[]) {
  /* SETUP ENVIRONEMENT */
  /*  printw and timer  */
  std::cout << "[initialisation] ...done." << std::endl;
  struct timespec t;
  struct sched_param param;
  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    perror("sched_setscheduler failed");
    exit(-1);
  }
  if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
    perror("mlockall failed");
    exit(-2);
  }
  setlocale(LC_ALL, "");

  /* CREATE VARIABLES */
  const char * cfgSource;
  const char * scope;
  int exitStatus = 0, prosody;
  char prosody_separator;
  std::thread extract_text;
  std::thread send_to_dac;

  // time of prosody (letter, word or sentence rythms)  
  switch (prosody) {
  case PROSODY_WITHOUT:
    std::cout << "PROSODY_WITHOUT" << std::endl;
    break;
  case PROSODY_LETTER:
    std::cout << "PROSODY_LETTER" << std::endl;
    break;
  case PROSODY_WORD:
    std::cout << "PROSODY_WORD" << std::endl;
    break;
  case PROSODY_SENTENCE:
    std::cout << "PROSODY_SENTENCE" << std::endl;
    break;
  }

  STATE_PROGRAM = 0;
  create_window();
  do {

    init_window();

    /* INITIALISE VARIABLES */
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    DEVICE * dev = new DEVICE();
    WAVEFORM * wf = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();

    /* INITIALISE GLOBAL VARIABLES */
    workdone = false;
    sentencesEmpty = true;
    sentencesReady = false;

    switch (STATE_PROGRAM) {
    case 0:
      // initialise the first config file
      setOpt(&argc, argv, &prosody, cfgSource, scope);
      std::cout << "[initialisation][options] cfgSource=" << cfgSource
          << std::endl;
      break;
    case KEY_F1: // Japanese Braille
      prosody = PROSODY_WORD;
      cfgSource = "libHaptiComm/4_configuration/config_JapaneseBraille.cfg";
      break;
    case KEY_F5: // English Braille
      prosody = PROSODY_LETTER;
      cfgSource =
          "libHaptiComm/4_configuration/configHaptiBraille_newDriver.cfg";
      break;
    case KEY_F6: // English Braille
      prosody = PROSODY_WORD;
      cfgSource =
          "libHaptiComm/4_configuration/configHaptiBraille_newDriver.cfg";
      break;
    case KEY_F7: // English Braille
      prosody = PROSODY_SENTENCE;
      cfgSource =
          "libHaptiComm/4_configuration/configHaptiBraille_newDriver.cfg";
      break;
    default:
      break;
    }
    prosody_separator = PROSODY_SEPARATORS[prosody];

    /* INITIALISE */
    cfg->configure(cfgSource, dev, wf, alph);
    send_to_dac = std::thread(workSymbols, std::ref(workdone), std::ref(alph),
        std::ref(dev));
    extract_text = std::thread(generateSentences, std::ref(workdone),
        alph->getlistSymbols(), prosody_separator);

    /* WORK */
    extract_text.join();
    send_to_dac.join();

    /* CLEAN */
    //close_window();
    delete cfg;
    delete dev;
    delete wf;
    delete alph;

  } while (!EXIT_KEYPRESS(STATE_PROGRAM));

  return exitStatus;
}

void generateSentences(std::atomic<bool> & workdone, std::string str_alph,
    char separator) {
  std::string s = "the quick brown fox jumps over the lazy dog";
  std::string str_prosody = " .";
  std::string str_ponc = "',;:!?-";
  bool lastis_pod; // part of dictionnary
  int ch;

  lastis_pod = false;
  do {
    ch = getch();
    /*
     for (int x=0x000; x<=0xfff; x++)
     {
     if (ch == x)
     {
     printw("<%i>", x);
     }
     }
     */

    if (ch == KEY_ERASE) {
      rmto_sentence();
      rmto_window();
    } else if (str_alph.find(tolower(ch)) != std::string::npos
        || str_prosody.find(ch) != std::string::npos) {
      printw("%c", ch);
      addto_sentence(ch);
    } else if (str_ponc.find(ch) != std::string::npos) {
      printw("%c", ch);
    }

    if (ch == separator || separator == '\0') {
      set_sentencesReady();
    }
  } while (!(EXIT_KEYPRESS(ch) || F_KEYPRESS(ch)));

  STATE_PROGRAM = ch;
  close_window();
  set_jobdone();
  set_sentencesReady();
}

void workSymbols(std::atomic<bool> & workdone, ALPHABET *& alph, DEVICE * dev) {
  // create variables
  AD5383 ad;
  waveformLetter values;
  std::deque<char> letters;

  int overruns, durationRefresh_ns;
  double durationRefresh_ms;

  overruns = 0;
  durationRefresh_ms = 1 / (double) alph->getFreqRefresh_mHz();
  durationRefresh_ns = durationRefresh_ms * ms2ns; // * ns
  if (!ad.spi_open())
    exit;
  ad.configure();
  ad.execute_trajectory(alph->getneutral(), durationRefresh_ns);

  // The goal of this function is to use the letters putted by the other
  // thread, one by one, and play them consecutively.
  while (!is_jobdone()) {
    signal4keypressed();
    addfrom_sentences(&letters);

    // + or - for further work on 'tap' intensy 
    if ((letters.front() == '+') || (letters.front() == '-')) {
      letters.pop_front();
      continue;
    }

    // do the job.
    while (!letters.empty()) {
      if (letters.front() == '.') {
        letters.pop_front();
        std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_SENTENCE_DELAY));
      } 
      else if (letters.front() == ' ') {
        letters.pop_front();
        std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_WORD_DELAY));
      } 
      else {
        values = alph->getl(letters.front());
        int ovr = ad.execute_selective_trajectory(values, durationRefresh_ns);
        if (ovr) {
          overruns += ovr;
        }
        values.clear();
        letters.pop_front();
        std::this_thread::sleep_for(std::chrono::milliseconds(PROSODY_LETTER_DELAY));
      }
    }

  }
}




/*
 * SHARED VARIABLES MODIFIERS
 */

void set_sentencesReady() {
  std::unique_lock < std::mutex > mlock(m_mutex);
  sentencesReady = true;
  m_condVar.notify_one(); // waiting thread is notified 
}

void addto_sentence(char ch) {
  std::lock_guard < std::mutex > lk(m_mutex); // locker to access shared variables
  sentences.push_back(ch);
  sentencesEmpty = false;
  m_condVar.notify_one(); // waiting thread is notified 
}

bool rmto_sentence() {
  bool success = true;

  std::lock_guard < std::mutex > lk(m_mutex); // locker to access shared variables
  if (is_sentencesEmpty()) {
    success = false;
  } else if (sentences.size() == 1) {
    sentences.pop_back();
    sentencesEmpty = true;
    success = true;
  } else {
    sentences.pop_back();
    success = true;
  }
  m_condVar.notify_one(); // waiting thread is notified 

  return success;
}

void addfrom_sentences(std::deque<char> * letters) {
  std::unique_lock < std::mutex > mlock(m_mutex);
  while (!sentences.empty()) {
    letters->push_back(sentences.front());
    sentences.pop_front();
  }
  sentencesEmpty = true;
  sentencesReady = false;
  m_condVar.notify_one(); // waiting thread is notified 
}

void set_jobdone() {
  std::unique_lock < std::mutex > mlock(m_mutex);
  workdone = true;
  m_condVar.notify_one(); // waiting thread is notified 
}

/*
 * DAEMON'S FUNCTIONS
 */
bool signal4keypressed() {
  std::unique_lock < std::mutex > mlock(m_mutex);
  m_condVar.wait(mlock, [] {return is_sentencesReady();});
  return true;
}

/*
 * SHARED VARIABLES CHECKERS
 */
bool is_sentencesReady() {
  return sentencesReady;
}

bool is_sentencesEmpty() {
  return sentencesEmpty;
}

bool is_jobdone() {
  //std::unique_lock<std::mutex> mlock(m_mutex);
  return (workdone.load() && is_sentencesEmpty());
}

/**
 *  CONFIGURATIONS
 */
void create_window() {
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
}

void init_window() {
  // init empty screen
  erase();
  signal(SIGWINCH, resize_handler);

  printw("You can start to write a letter, a word, a sentence \n");
  printw(" --- When you are done, press <ESCAPE> or <CTRL+C> to Exit ---\n");
  printw(
      "<F1> = Japanese Braille | <F5> = English Braille (letter rythm) | <F6> = English Braille (word rythm) | <F7> = English Braille (sentence rythm)\n");

}

// remove the last character of the window
void rmto_window() {
  int x = 0, y = 0;
  getyx(stdscr, y, x);

  if (x > 1) {
    move(y, x - 1);
    delch();
  }

  refresh();
}

void close_window() {
  refresh();
  endwin();
}

void resize_handler(int sig) {
  int nh, nw;

  getmaxyx(stdscr, nh, nw); // get the new screen size
}

static void usage() {
  fprintf(stderr, "\n"
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
int setOpt(int *argc, char *argv[], int * prosody, const char *& cfgSource,
    const char *& scope) {
  string help = "help";
  string letter_long = "letter";
  string word_long = "word";
  string sentence_long = "sentence";
  string cfg_long = "cfg";
  string scope_long = "scope";

  static struct option long_options[] = {
      { help.c_str(), no_argument, NULL, 'h' }, { letter_long.c_str(),
          no_argument, NULL, 'l' },
      { word_long.c_str(), no_argument, NULL, 'w' }, { sentence_long.c_str(),
          no_argument, NULL, 's' }, { cfg_long.c_str(), required_argument, NULL,
          'c' }, { scope_long.c_str(), optional_argument, NULL, 'p' }, { NULL,
          0, NULL, 0 } };

  // character of the option
  int c;

  *prosody = 0;
  // Detect the end of the options. 
  while ((c = getopt_long(*argc, argv, "hlwscp", long_options, NULL)) != -1) {
    switch (c) {
    case 'l':
      *prosody = PROSODY_LETTER;
      break;

    case 'w':
      *prosody = PROSODY_WORD;
      break;

    case 's':
      *prosody = PROSODY_SENTENCE;
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
      *prosody = -1;
      break;

    default:
      abort();
    }
  }

  return 0;
}

