#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <atomic>

//counts every number that is added to the queue
static long long producer_count = 0;
//counts every number that is taken out of the queue
static long long consumer_count = 0;

void generateNumbers(std::queue<int> & numbers, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    while(!workdone.load())
    {
        std::unique_lock<std::mutex> lk(m);
        int rndNum = rand() % 100;
        std::cout << "-----------------------------[1/5]" << std::endl;
        numbers.push(rndNum);
        std::cout << "-----------------------------[2/5]" << std::endl;
        producer_count++;
        std::cout << "-----------------------------[3/5]" << std::endl;
        cv.notify_one(); // Notify worker
        std::cout << "-----------------------------[4/5]" << std::endl;
        cv.wait(lk); // Wait for worker to complete
        std::cout << "-----------------------------[5/5]" << std::endl; 
    }
}

void work(std::queue<int> & numbers, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone)
{
    while(!workdone.load() or !numbers.empty())
    {
        std::unique_lock<std::mutex> lk(m);
        cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
        if (numbers.empty())
            cv.wait(lk); // Wait for the generator to complete
        if (numbers.empty())
            continue;
        std::cout << numbers.front() << std::endl;
        numbers.pop();
        consumer_count++;
     }
}

int main() {
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<int> numbers;

    //start threads
    std::cout << "thread producer :" << std::endl;
    std::thread producer(generateNumbers, std::ref(numbers), std::ref(cv), std::ref(m), std::ref(workdone));
    std::cout << "thread consumer :" << std::endl;
    std::thread consumer(work, std::ref(numbers), std::ref(cv), std::ref(m), std::ref(workdone));


    //wait for 3 seconds, then join the threads
    std::cout << "sleep :" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "workdone = true" << std::endl;
    workdone = true;
    std::cout << "notify_all" << std::endl;
    cv.notify_all(); // To prevent dead-lock
    
    std::cout << "producer.join()" << std::endl;
    producer.join();
    std::cout << "consumer.join()" << std::endl;
    consumer.join();

    //output the counters
    std::cout << producer_count << std::endl;
    std::cout << consumer_count << std::endl;

    return 0;
}
