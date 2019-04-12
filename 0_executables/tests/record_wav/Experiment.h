/*
 * Experiment.h
 *
 *  Created on: 10 Apr. 2019
 *      Author: Basil Duvernoy
 */

/* STANDARD LIBRARIES */
#include <atomic>
#include <array> 				// std::array
#include <chrono> 				// std::clock, std::chrono::high_resolution_clock::now
#include <condition_variable>
#include <iostream>
#include <functional>			// std::bind
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

/* SPEECH RECOGNITION LIBRARIES PocketSphinx and SphinxBase */
#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <pocketsphinx.h>

/* Reading and writing wav's library */
#include "AudioFile.h"




#include <pthread.h>
#include <sched.h>


#ifndef EXPERIMENT_H_
#define EXPERIMENT_H_



/** Configuration for the voice recognition
 * (to be modified after the deadline) */
static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     "plughw:1,0",
     "Name of audio device to use for input."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    {"-inmic",
     ARG_BOOLEAN,
     "no",
     "Transcribe audio from microphone."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION
};
/************************************************ 
 * 
 *                 CLASS EXPERIMENT
 * 
 * 
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
		 * (2) Same for the destructor 
		 * (3) This implementation fixes issues with template classes. 
		 * (3b)https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
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
	vector<td_msecarray> getTimer();
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
	
	
	/* d. voice recognition variables (sphinx) */
	ps_decoder_t * 	vr_ps; 					// decoder for voice recognition
	cmd_ln_t * 		vr_cfg; 				// config for voice recognition
	
	/* e. Candidat variables */
	Candidat * 			c;
	vector<vector<int>> seq;
	vector<int> 		actchannelID;
	expEnum 			expToExec;
	
	/* f. output/result variables */
	td_highresclock 		c_start;
	vector<td_msecarray> 	vvtimer;
	vector<int> 			vanswer;
	AudioFile<double> * 	af;
	
	
/* 2. FUNCTIONS */
	/* a. main tasks experiment */
	static void * static_record_from_microphone(void * c);
	static void * static_executeStimuli(void * c);
	void record_from_microphone();
	void executeStimuli();
	
	bool executeActuator(int * durationRefresh_ns);
	bool executeF(int * durationRefresh_ns);
	int  executeSequence(int * 		currSeq, waveformLetter values_copy, int * 	dr_ns, td_msecarray * vhrc);
	int  executeSequenceSpace(int * currSeq, waveformLetter *values, int * 		dr_ns, td_msecarray * vhrc);
	int  executeSequenceTemp(int * 	currSeq, waveformLetter *values, int * 		dr_ns, td_msecarray * vhrc);
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
	td_msec nowSeq();
	void dispTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug);
};

#endif /* Experiment_H_ */





























