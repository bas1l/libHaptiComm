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
#include<ncurses.h>
#include <math.h>

#include "waveform.h"
#include "ad5383.h"
#include "utils.h"

#include "alphabet.h"

using namespace std;



void push_actuator(std::vector<std::vector<uint16_t> >& values, int sens){
    static int v = 2048;
    std::vector<int> channels { 9 };//nothing, just a break.

    if (1==sens){
        v+=50;
    }
    else if (0==sens){
        v-=50;
    }
    
    for(int j = 0; j < 31; ++j)
    {
        if (9==j){
            for(int i=0;i < 500; i++){
                values[j].push_back(v);
            }
            values[j].push_back(2048);
        }
        else {
            for(int i=0;i < 500; i++){
                values[j].push_back(2048);
            }
            values[j].push_back(2048);
        }
    }
    
    printw("\nact=9,value=%i\t", v);
    //push_channel(values, channels);
}

void push_sine_wave(std::vector<std::vector<uint16_t> >& values, int freq, int ampl, int offset){
    int chan = 9;
    
    int phase = 0;
    int nsample = 2000;
    std::vector<int> channels{ chan };//nothing, just a break.
    
    
    uint16_t * s = create_sin(freq, ampl, phase, nsample, offset);
    
    
    //printw("\nact=9,freq=%i,ampl=%i\n", freq, ampl);
    for(int j = 0; j < 31; ++j)
    {
        if (9==j){
            for(int i=0;i < nsample; i++){
                values[j].push_back(s[i]);
                //printw("%i, ", s[i]);
            }
            values[j].push_back(2048);
        }
        else {
            for(int i=0;i < nsample; i++){
                values[j].push_back(2048);
            }
            values[j].push_back(2048);
        }
    }
    
}


std::vector<uint16_t> push_sine_wave_ret(int freq, int ampl, int offset){
    int phase = 0;
    int nsample = 2000;
    std::vector<int> channels{nsample+1};//nothing, just a break.
    
    
    uint16_t * s = create_sin(freq, ampl, phase, nsample, offset);
    
    
    //printw("\nact=9,freq=%i,ampl=%i\n", freq, ampl);
    for(int i=0;i < nsample; i++){
        channels[j] = s[i];
        //printw("%i, ", s[i]);
    }
    channels[j] = 2048;
    
    return channels;
}

void push_letters(std::vector<std::vector<uint16_t> >& values, const char c)
{
    static int ampl = 50;
    static int upto = 2048;
    static int freq = 0;
    switch(c){
        case '1' :     
        {
            freq -= 5;
            printw("freq=%i, ", freq);
            //printw("Donnez une frequence: ");
            //scanw("%d", &freq);
            //printw("%i", freq);
            //push_sine_wave(values, 100, ampl);
            break;
        }
        case '2' :
        {
            freq += 5;
            printw("freq=%i, ", freq);
            //printw("Donnez l'offset: ");
            //scanw("%d", &upto);
            //printw("%i", upto);
            //push_sine_wave(values, 200, ampl);
            break;
        }
        case '3' :    
        { 
            upto -= 25;
            printw("offset=%i, ", upto);
            //printw("Donnez une frequence: ");
            //scanw("%d", &freq);
            //printw("%i", freq);
            //push_sine_wave(values, 100, ampl);
            break;
        }
        case '4' :
        {
            upto += 25;
            printw("offset=%i, ", upto);
            //printw("Donnez l'offset: ");
            //scanw("%d", &upto);
            //printw("%i", upto);
            //push_sine_wave(values, 200, ampl);
            break;
        }
        case '5' :
        {
            //push_actuator(values, 0);
            ampl -= 25;
            printw("ampl=%i, ", ampl);
            break;
        }
        case '6' :
        {
            //push_actuator(values, 1);
            ampl += 25;
            printw("ampl=%i, ", ampl);
            break;
        }
        case '+' :
        {
            printw("\n(freq=%i, ampl=%i,  offset=%i) \t",  freq, ampl, upto);
            push_sine_wave(values, freq, ampl, upto);
            //push_actuator(values, -1);
            break;
        }
        default :
            break;
    }
    
}

void generateSentences(std::queue<char> & sentences, std::condition_variable & cv,
            std::mutex & m, std::atomic<bool> & workdone, std::string str_alph)
{

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    std::string str_ponc = " .,;:!?-";
    
    int ch;
    printw("You can start to write a letter, a word, a sentence \n --- When you are done, press '*' to Exit ---\n");
    do{
        if (ch != ERR) 
        {
            if (str_alph.find(ch) != std::string::npos)
            {// if part of the alphabet
                printw("%c", ch);
                
                std::unique_lock<std::mutex> lk(m);
                sentences.push(ch);
                
                cv.notify_one(); // Notify worker
                cv.wait(lk); // Wait for worker to complete
            }
            else if (str_ponc.find(ch) != std::string::npos)
            {// if part of the ponctuation
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
            values = alph.getl(letters.front());
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


int main(int argc, char *argv[])
{   
    /* 1/2 init */
    // a l'heure actuelle les valeurs sont : 10 20 40 800 4
    /*
    TAP_MOVE_DURATION = atoi(argv[1])*2;//10
    APPARENT_ASC_DURATION = atoi(argv[2])*2;//20
    APPARENT_MOVE_DURATION = atoi(argv[3])*2;//40
    APPARENT_MOVE_AMPLITUDE = atoi(argv[4]);//800
    APPARENT_NB_ACT_SUPERPOSED = atoi(argv[5]);//4
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
    
    
    
    int freq[] = {1, 2, 5, 10, 20, 50, 100, 200, 300, 500, 700};
    int freq_max = sizeof(freq);
    
    int ampl[] = {50, 100, 150, 200, 250, 300, 400, 600, 800, 1000};
    int ampl_max = sizeof(ampl);
    
    int upto[] = {1000, 1500, 2048};
    int upto_max = sizeof(upto);
    
    for (freq_id=0;freq_id<freq_max; freq_id++)
    {
        for (ampl_id=0;ampl_id<ampl_max; ampl_id++)
        {
            for (upto_id=0;upto_id<upto_max; upto_id++)
            {
                std::vector<uint16_t> values = push_sine_wave_ret(
                                                freq[freq_id],
                                                ampl[ampl_id],
                                                upto[upto_id]);
                write_file(values,freq, ampl, upto[upto_id]);
            }
        }
    }
    
    
    /****** PAS BESOIN DE CE CODE A CE STADE *******/
    /*
    DEVICE dev;
    dev.configure();
    WAVEFORM wf;
    wf.configure();
    ALPHABET alph(dev, wf);//, AD5383::num_channels);
    alph.configure();
    
    
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;
    std::thread extract_text;
    extract_text = std::thread(workSymbols, std::ref(sentences), std::ref(cv), 
                           std::ref(m), std::ref(workdone), std::ref(alph));
    std::thread send_to_dac;
    send_to_dac = std::thread(generateSentences, std::ref(sentences), std::ref(cv),
                           std::ref(m), std::ref(workdone), alph.getlist_alphabet());
    
    extract_text.join();
    send_to_dac.join();
    */
    
    return 0;
}
