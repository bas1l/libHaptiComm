/*
 * Experiment.cpp
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil
 */

#include "Experiment.h"

Experiment::Experiment()
{
}

Experiment::~Experiment() // Destroy the current Experiment object
{
	if(this->t_record.joinable()) this->t_record.join();
}

bool Experiment::create()
{
	std::cout << "[experiment] create::start..." << std::endl;
    
    // thread and shared memory's variables initialisation for voice recording
	this->workdone = false; 													// check if the experiment is done
	this->is_recording = false; 												// allow t_record to start or stop the recording
	
	std::cout << "[experiment] create::...end" << std::endl;
    return 0;
}

bool Experiment::launchThread()
{
	// This will start the thread. Notice move semantics!
	std::cout<<"thread>>"<<std::endl;
	this->t_record 	= std::thread(&Experiment::record_from_microphone, this); 	// link the thread to its function

	return false;
}




bool Experiment::executeActuator(){
	std::system("clear");
	for(int i=0; i<3; i++) // for the sequence i
	{
		std::cout<<"[main] New sequence:"<<std::endl;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout<<"[main] slept for:1000ms"<<std::endl;
		
		start_recording(); // change is_recording value to true
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout<<"[main] slept for:1000ms again"<<std::endl;
		
		stop_recording(); // change is_recording value to false
	}
	stop_experiment();
	
    return false;
}


// record function for the thread
void Experiment::record_from_microphone()
{
	while(true)
	{
		std::cout<<"Do Some Handshaking"<<std::endl;
		// Acquire the lock
		if(!signal4recording()) 
		{
			break;
		}

		std::cout<<"is recording...start"<<std::endl;
		while(isrecording())
		{
			int i=1;
		}
		std::cout<<"is recording...done"<<std::endl;
		std::lock_guard<std::mutex> mlock(this->m_mutex);
		// Start waiting for the Condition Variable to get signaled
		// Wait() will internally release the lock and make the thread to block
		// As soon as condition variable get signaled, resume the thread and
		// again acquire the lock. Then check if condition is met or not
		// If condition is met then continue else again go in wait.
		std::cout<<"Do Processing On loaded Data"<<std::endl;
	}
}


bool Experiment::signal4recording()
{
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	// Start waiting for the Condition Variable to get signaled
	// Wait() will internally release the lock and make the thread to block
	// As soon as condition variable get signaled, resume the thread and
	// again acquire the lock. Then check if condition is met or not
	// If condition is met then continue else again go in wait.
	this->m_condVar.wait(mlock, std::bind(&Experiment::isrecordingorworkdone, this));

	if (isworkdone()) {return false;}
	else return true;
}

void Experiment::start_recording()
{	
	std::cout << "[main][start_recording] in." << std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	this->is_recording = true;								// start of the microphone recording boolean
	this->m_condVar.notify_one(); 									// waiting thread is notified 
	std::cout << "[main][start_recording] out. ---> "<<std::to_string(this->is_recording)<<std::endl;
}

void Experiment::stop_recording()
{
	std::cout << "[main][stop_recording] in." << std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	this->is_recording = false;								// stop of the microphone recording boolean
	this->m_condVar.notify_one(); 									// waiting thread is notified 
	std::cout << "[main][stop_recording] out." << std::endl;
}

void Experiment::stop_experiment()
{	
	std::cout<<"[stop_experiment] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	this->workdone = true;										// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	std::cout<<"[stop_experiment] out."<<std::endl;
}


bool Experiment::isrecording()
{
	return this->is_recording;
}

bool Experiment::isworkdone()
{
	return this->workdone;
}

bool Experiment::isrecordingorworkdone()
{
	return (isworkdone() || isrecording());
}














