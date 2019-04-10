/*
 * Experiment.h
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil Duvernoy
 */

/* STANDARD LIBRARIES */
#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#ifndef EXPERIMENT_H_
#define EXPERIMENT_H_


/************************************************ 
 * 
 *                 CLASS EXPERIMENT
 * 
 * 
 ************************************************/
class Experiment 
{

	std::mutex 				m_mutex;
	std::condition_variable m_condVar;
	bool 					workdone;
	bool 					is_recording;
	std::thread 			t_record;
	
public:
	Experiment();// Create the structure of the Experiment
	~Experiment();
	
	/* Classic functions */
	bool create();
	bool launchThread();
	bool executeActuator();
	void record_from_microphone();
	
	bool signal4recording();
	void start_recording();
	void stop_recording();
	void stop_experiment();
	
	bool isrecording();
	bool isworkdone();
	bool isrecordingorworkdone();
};

#endif /* Experiment_H_ */





























