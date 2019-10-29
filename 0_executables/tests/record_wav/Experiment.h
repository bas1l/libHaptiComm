/*
 * Experiment.h
 *
 *  Created on: 10 Apr. 2019
 *      Author: Basil Duvernoy
 */

/* STANDARD LIBRARIES */
#include <algorithm>			// for max_element, transform
#include <atomic>				
#include <array> 				// std::array
#include <chrono> 				// std::clock, std::chrono::high_resolution_clock::now
#include <condition_variable>	
#include <iomanip>				// cout with desired floating precision
#include <iostream>
#include <functional>			// std::bind and placeholders
#include <limits.h>
#include <mutex>
#include <numeric> 				// accumulate
#include <stdlib.h> 			// srand, rand
#include <string.h> 			// strncmp
#include <sys/mman.h>
#include <time.h> 				// time
#include <thread>
#include <unistd.h> 			// sleep, usleep
#include <vector>

/* HAPTICOMM LIBRARIES */
#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "utils.h"
#include "alphabet.h"

/* Candidat library for experiment */
#include "Candidat.h"

/* Alsa library */
#include <alsa/asoundlib.h>


/* Reading and writing wav's library */
#include "AudioFile.h"

#include <pthread.h>
#include <sched.h>


#ifndef EXPERIMENT_H_
#define EXPERIMENT_H_

#define EXPERIMENT_SAMPLING_RATE 16000


/************************************************ 
 * 												*
 *                 CLASS EXPERIMENT				*
 * 												*
 * 												*
 ************************************************/
class Experiment 
{

public:
	Experiment(const char * _cfgSource, Candidat * _c, int _seq_start)// Create the structure of the Experiment
	{
		cout << "[experiment] new::start..." << endl;
		this->cfgSource = _cfgSource;
		this->c = _c;
		this->seq_start = _seq_start;
		/* HaptiComm variables
		 * (1) If not in the .h -> CANNOT COMPILE (?)
		 * (-) This implementation fixes issues with template classes. 
		 * (--)https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
		 * --------------------------------*/
		this->cfg = new HaptiCommConfiguration();
		this->dev = new DEVICE();
		this->wf  = new WAVEFORM();
		this->alph= new ALPHABET();
		this->ad  = new AD5383();
		/* -------------------------------- */
		
		cout << "[experiment] new::...end" << endl;
	}
	~Experiment();
	
	/* a. Classic functions */
	bool create();
	bool execute();
		
	
	/* b. Getters */
	vector<msec_array_t> getTimer();
	vector<int> getAnswer();
	int getSeq_start();
	int getSeq_end();
	
	
private:
/* 1. VARIABLES */
	/* a. HaptiComm variables */
	HaptiCommConfiguration * 	cfg;
	DEVICE * 					dev;
	WAVEFORM * 					wf;
	ALPHABET * 					alph;
	AD5383 * 					ad;
	int 						exitStatus;
	const char * 				cfgSource;
	const char * 				scope;
	int 						period_spi_ns; // period used to refresh datas
	/* b. AudioFile and sequence parameters */
	int af_i;
	int af_max;
	int seq_start;
	int seq_end;
	
	/* c. Thread for executing tap and record voice and shared variables */
	std::mutex 				m_mutex;
	std::condition_variable m_condVar;
	bool 					workdone;
	bool 					is_recording;
	bool	 				audioBufferReady;
	pthread_attr_t     		attr;
	pthread_t  				t_record;
	pthread_t  				t_tap;

	
	
	/* d. Alsa voice recording variables */
	snd_pcm_t *capture_handle;
	unsigned int rate_mic;
	
	/* e. Candidat variables */
	Candidat * 			c;
	vector<vector<int>> seq;
	vector<int> 		actchannelID;
	expEnum 			expToExec;
	
	/* f. output/result variables */
	highresclock_t 			c_start;
	vector<msec_array_t> 	vvtimer;
	vector<int> 			vanswer;
	AudioFile<double> * 	af;
	
	
/* 2. FUNCTIONS */
	/* a. main tasks experiment */
	static void * static_record_from_microphone(void * c);
	static void * static_executeStimuli(void * c);
	void record_from_microphone();
	void executeStimuli();

	bool executeCalibration(waveformLetter values);	
	bool executeActuatorSpace(waveformLetter values);
	bool executeActuatorTemp(waveformLetter values);
	bool executeF();
	waveformLetter  setupWaveformSpace(int * currSeq, waveformLetter values_copy, msec_array_t * vhrc);
	waveformLetter  setupWaveformTemp( int * currSeq, waveformLetter values_copy, msec_array_t * vhrc);
	void initactid4temp(); 
	
	/* b. threads related */
	bool signal4recording();
	bool signal4stoprecording();
	bool signal4stop_recording();
	bool signal4stop_experiment();
	void start_recording();
	void stop_recording();
	void stop_experiment();
	
	/* c. answers related */
	bool writeAnswer(int * answeri);
	void fillAudioBuffer(AudioFile<double>::AudioBuffer buffer);
	void save_audio(int id_seq);
		
	/* d. checkers */
	bool isrecording();
	bool isstopedrecording();
	bool isworkdone();
	bool isrecordingorworkdone();
	bool isAudioBufferReady();
	
	/* e. tools */
	int init_captureHandle(snd_pcm_t ** capture_handle, unsigned int * exact_rate);
	void randomWaiting();
	msec_t nowLocal(highresclock_t start);
	void dispTimers(int numSeq, int answeri, msec_array_t vhrc, msec_array_t timerDebug);
};

#endif /* Experiment_H_ */





























