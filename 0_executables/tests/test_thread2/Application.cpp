
#include "Application.h"

Application::Application()
{
	this->is_recording = false;
	this->workdone = false;
}

Application::~Application()
{
	if(this->t_record.joinable()) this->t_record.join();
}
		
void Application::launchThread()
{
	this->t_record = std::thread(&Application::record_from_microphone, this);
}

void Application::executeActuator()
{
	for(int i=0; i<3; i++) // for the sequence i
	{
		std::cout<<std::endl<<"[main] New sequence:"<<std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		start_recording(); // change is_recording value to true
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		
		stop_recording(); // change is_recording value to false
	}
	stop_experiment();
}


void Application::record_from_microphone()
{
	while(true)
	{
		std::cout<<"Do Some Handshaking"<<std::endl;
		// Acquire the lock
		if(!signal4recording()) 
		{
			break;
		}
		
		while(isrecording())
		{
			std::cout<<"is recording..."<<std::endl;
		}
		std::lock_guard<std::mutex> mlock(this->m_mutex);
		// Start waiting for the Condition Variable to get signaled
		// Wait() will internally release the lock and make the thread to block
		// As soon as condition variable get signaled, resume the thread and
		// again acquire the lock. Then check if condition is met or not
		// If condition is met then continue else again go in waithis->t_record.
		std::cout<<"Do Processing On loaded Data"<<std::endl;
	}
}

bool Application::signal4recording()
{
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	// Start waiting for the Condition Variable to get signaled
	// Wait() will internally release the lock and make the thread to block
	// As soon as condition variable get signaled, resume the thread and
	// again acquire the lock. Then check if condition is met or not
	// If condition is met then continue else again go in waithis->t_record.
	this->m_condVar.wait(mlock, std::bind(&Application::isrecordingorworkdone, this));
	
	if (isworkdone()) {return false;}
	else return true;
}

void Application::start_recording()
{	
	std::cout<<"[start_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->is_recording = true;									// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	std::cout<<"[start_recording] out."<<std::endl;
}

void Application::stop_recording()
{	
	std::cout<<"[start_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->is_recording = false;									// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	std::cout<<"[start_recording] out."<<std::endl;
}

void Application::stop_experiment()
{	
	std::cout<<"[stop_experiment] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->workdone = true;										// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	std::cout<<"[stop_experiment] out."<<std::endl;
}


bool Application::isrecording()
{
	return this->is_recording;
}

bool Application::isworkdone()
{
	return this->workdone;
}

bool Application::isrecordingorworkdone()
{
	return (isworkdone() || isrecording());
}









