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


void print_fau(int * f, int * a, int * u);
void print_instructions();
void usage();
uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset);
std::vector<uint16_t> createsine_vector(int freq, int ampl, int offset, double phase, int nsample);
std::vector<std::vector<uint16_t>> creatematrix(int nbsample, int value);