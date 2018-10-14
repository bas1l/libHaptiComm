#include <iostream>
#include <sys/mman.h>
#include "ad5383.h"

#include <thread>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <atomic>
#include <sys/ioctl.h>


using namespace std;

//counts every number that is added to the queue
static long long producer_count = 0;
//counts every number that is taken out of the queue
static long long consumer_count = 0;

static int ns2ms = 1000000;



#include<ncurses.h>

void checkArrow(){
    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    printw("Press q to Exit\t");

    do{
        if (ch != ERR) {
            switch(ch){
                case KEY_UP: {
                    printw("\n\nUp Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_DOWN:{
                    printw("\n\nDown Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_LEFT:{
                    printw("\n\nLeft Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_RIGHT:{
                    printw("\n\nRight Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                default:{
                    printw("\n\n %c\n", ch);
                    printw("Press q to Exit\t");
                    break;
                }
            }   
        }
        
        
    }while((ch = getch()) != 'q');

    printw("\n\n\t\t\t-=GoodBye=-!\n");
    refresh();
    endwin();
}


void letter_a_push(std::vector<std::vector<uint16_t> > values){
    int k;
    
    for(int j = 0; j < 31; ++j)
    {
        k=0;
        if (j==4){
            for(; k<500; k++){
                values[j].push_back(0);
            }
            for(;k<999; k++){
                values[j].push_back(2048);
            }        
        }
        else
        {
            for(; k<999; k++){
                values[j].push_back(2048);
            }
        }
    }
   std::cout << "print the letter a" << std::endl; 
}


void init_neutral(AD5383 ad, std::vector<std::vector<uint16_t> >& values)
{
    for(int j = 0; j < 31; ++j)
    {
        values[j].push_back(2048);
        values[j].push_back(2048);          
    }
    
    ad.execute_trajectory(values, 1000000);
}

void generateSentences(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    printw("Press '*' to Exit :  \t");

    do{
        if (ch != ERR) {
            switch(ch){
                case KEY_UP: {
                    printw("\n\nUp Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_DOWN:{
                    printw("\n\nDown Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_LEFT:{
                    printw("\n\nLeft Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_RIGHT:{
                    printw("\n\nRight Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                default:{
                    std::unique_lock<std::mutex> lk(m);
                    printw("%c", ch);
                    //printw("[GS][for] sentences.push(c)\n");
                    sentences.push(ch);
                    //printw("[GS][for] producer_count++()  %c\n", ch);

                    producer_count++;
                    //printw("[GS][for] cv.notify_one()\n");
                    cv.notify_one(); // Notify worker
                    //printw("[GS][for] cv.wait(lk)\n");
                    cv.wait(lk); // Wait for worker to complete
                    //printw("[GS][for] cv.wait(lk) :: FREE\n");
                    //printw("Press q to Exit\t");
                    break;
                }
            }
            // eoeowowej
          }
    }while((ch = getch()) != '*');

    printw("\n\n\t\t\t-=GoodBye=-!\n");
    refresh();
    endwin();
    workdone = true;
    cv.notify_one(); // Notify worker
}

void workSymbols(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    
    printw("\t{WS} Begin\n");
    // init drive electronics
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    init_neutral(ad, values);
    
    std::queue<char> personalSentences;
    
    // Initialisation complete.
    //printw("\t{WS} notify_one()\n");
    cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
    
    // The goal of this function is to use the letters putted by the other
    // thread, one by one, and play them consecutively.
    //printw("\t{WS} while...()\n");
    while(!workdone.load() or !sentences.empty())
    {
        //printw("{WS}{while} Begin\n");
        std::unique_lock<std::mutex> lk(m);
        
        //printw("{WS}{while} sentences.EMPTY ACHTOUNG()\n");
        if (sentences.empty())
        {
            //printw("\t{WS}{while} cv.wait(lk)\n");
            cv.wait(lk); // Wait for the generator to complete
            //printw("\t{WS}{while} cv.wait(lk) :: FREE\n");
        }
        //printw("\t{WS}{while} ..sentences.empty() :: 2\n");
        if (sentences.empty())
            continue;
        // free the common value asap
        //printw("{WS}{while} ..push moi ca !\n");
        personalSentences.push(sentences.front());
        //printw("\t{WS}{while} ..Catch symbol = %c\n", sentences.front());
        
        /*** move ACTUATORS ***/
        letter_a_push(values);
        ad.execute_trajectory(values, 999*ns2ms);
        /*** move ACTUATORS ***/
        
        
        
        sentences.pop();
        consumer_count++;
        cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
        
        // if last char is a space, then a word is finished
        if (personalSentences.front() == ' ')
        {
            if (personalSentences.front() == ' ')// is part of the alphabet){
            {
                //play(personalSentences);
                personalSentences.pop();
            }
        }
     }
    ad.spi_close();    
    printw("\t{WS} End\n");
}


int main(int argc, char *argv[])
{
    std::cout << "Starting9..." << std::endl;

    struct timespec t;
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    
    /**************** C'EST AVEC CETTE OPTION QUE TOUT BUG *********************
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }
    ***************************************************************************/
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }

    
    
    // PARTIE THREADS
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;

    std::cout << "Starting...threads1" << std::endl;
    std::thread producer(generateSentences, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));
    std::thread consumer(workSymbols, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));
    
    //wait for 3 seconds, then join the threads
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //workdone = true;
    //cv.notify_all(); // To prevent dead-lock

    producer.join();
    consumer.join();
  
    //checkArrow();

    //output the counters
    std::cout << producer_count << std::endl;
    std::cout << consumer_count << std::endl;   
    /*
    
    int i;

    for (i=0; i < argc; i++)
    {
        std::cout << "Argument " << i+1 << " : " << argv[i] << std::endl;
    }
    
    int val = atoi(argv[1]);
    i = atoi(argv[2]);
    //for(int i = 0; i < 30; i++)
    {
        for(int j = 0; j < 31; ++j)
        {
            if ((j == i)){
                values[j].push_back(4095);
                values[j].push_back(2048);
                std::cout << "Done tap : " << j << std::endl;            
            }
            else{
                values[j].push_back(2048);
                values[j].push_back(2048);          
            }
            
        }
        
        
    }
    
    long ms = 900;
    std::cout << "overruns : " << std::dec << ad.execute_trajectory(values, ms *1000000) << std::endl;
   */
    std::cout << "Done." << std::endl;

    //ad.spi_close();
  
    return 0;
}
