#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <atomic>
#include <sys/ioctl.h>

//counts every number that is added to the queue
static long long producer_count = 0;
//counts every number that is taken out of the queue
static long long consumer_count = 0;

int kbhit()
{
    int i;
    ioctl(0, FIONREAD, &i);
    return i;
    
}

void generateSentences(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    
    while(!workdone.load())
    {
        std::unique_lock<std::mutex> lk(m);
        
        int i = 0;
        int c = ' ';
        
        cv.wait(lk); // Wait for worker to complete
        std::cout << "Please enter '*' to quit\n" << std::endl;
        for(;c !='*'; i++)
        {
            // if a char is detected, sent it to the other thread :
            if (kbhit())
            {
                c=getchar();
                sentences.push(c);
                producer_count++;
                cv.notify_one(); // Notify worker
                cv.wait(lk); // Wait for worker to complete
            }
        }
        system("stty raw -echo");
        
        // if * is pressed :
        workdone = true;
     }
}

void workSymbols(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    std::unique_lock<std::mutex> lk(m);
    
    //Alphabet al = createAlphabet();
    std::queue<char> personalSentences;
    
    // Initialisation complete.
    cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
    
    // The goal of this function is to use the letters putted by the other
    // thread, one by one, and play them consecutively.
    while(!workdone.load() or !sentences.empty())
    {
        std::unique_lock<std::mutex> lk(m);
        
        if (sentences.empty())
            cv.wait(lk); // Wait for the generator to complete
        if (sentences.empty())
            continue;
        // free the common value asap
        personalSentences.push(sentences.front());
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
}

int main() {
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;

    //start threads     
    std::thread producer(generateSentences, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));
    std::thread consumer(workSymbols, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));


    //wait for 3 seconds, then join the threads
    std::this_thread::sleep_for(std::chrono::seconds(1));
    workdone = true;
    cv.notify_all(); // To prevent dead-lock

    producer.join();
    consumer.join();

    //output the counters
    std::cout << producer_count << std::endl;
    std::cout << consumer_count << std::endl;

    return 0;
}