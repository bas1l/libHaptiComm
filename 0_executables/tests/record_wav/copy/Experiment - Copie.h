/*
 * Experiment.h
 *
 *  Created on: 14 janv. 2019
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

using namespace std;

#ifndef EXPERIMENT_H_
#define EXPERIMENT_H_




/** Configuration for the voice recognition
 * (to be modified after the deadline) */
static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-adcdev", // about the microphone device (has to be default)
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    CMDLN_EMPTY_OPTION
};


/************************************************ 
 * 
 *                 CLASS EXPERIMENT
 * 
 * 
 ************************************************/
class Experiment {
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
	
	virtual ~Experiment(){ // Destroy the current Experiment object
	    delete this->cfg;
	    delete this->dev;
	    delete this->wf;
	    delete this->alph;
	    delete this->ad;
	    
		//if(vr_ps!=NULL) ps_free(vr_ps);
		if(vr_cfg!=NULL) cmd_ln_free_r(vr_cfg);
		if(this->t_record.joinable()) this->t_record.join();
		if(this->t_tap.joinable()) this->t_tap.join();
	}
	
	/* Classic functions */
	bool create();
	bool launchExp();
	
	bool execute();
	void record_from_microphone();
	bool executeActuator(int * durationRefresh_ns);
		
	
	/* Getters */
	vector<td_msecarray> getTimer();
	vector<int> getAnswer();
	int getSeq_start();
	int getSeq_end();
	
	
private:
/* VARIABLES */
	// HaptiComm variables
	HaptiCommConfiguration * 	cfg;
	DEVICE * 					dev;
	WAVEFORM * 					wf;
	ALPHABET * 					alph;
	AD5383 * 					ad;
	int 			exitStatus;
	const char * 	cfgSource;
	const char * 	scope;
	
	int seq_start;
	int seq_end;
	
	// Thread for executing tap and record voice and shared variables
	std::condition_variable cv;
	std::mutex 				m_mutex;
	bool 					workdone;
	bool 					is_recording;
	std::thread 			t_record;
	std::thread 			t_tap;
	
	// Object to write wav files
	AudioFile<double> * af;
	int 				af_i;
	int					af_max;
	
	// voice recognition variables (sphinx)
	ps_decoder_t * 	vr_ps; 					// decoder for voice recognition
	cmd_ln_t * 		vr_cfg; 				// config for voice recognition
	
	
	// Candidat variables
	Candidat * c;
	vector<vector<int>> seq;
	vector<int> actchannelID;
	expEnum expToExec;
	
	// output/result variables
	td_highresclock c_start;
	vector<td_msecarray> vvtimer;
	vector<int> vanswer;

/* FUNCTIONS */
	/* function to execute depending on expToExec */
	bool executeF(int * durationRefresh_ns);
	int  executeSequence(int * 		currSeq, waveformLetter values_copy, int * 	dr_ns, td_msecarray * vhrc);
	int  executeSequenceSpace(int * currSeq, waveformLetter *values, int * 		dr_ns, td_msecarray * vhrc);
	int  executeSequenceTemp(int * 	currSeq, waveformLetter *values, int * 		dr_ns, td_msecarray * vhrc);
	
	int  recognize_from_microphone(ad_rec_t *ad, td_msecarray * vhrc, td_msecarray * timerDebug);
	
	void start_recording();
	void stop_recording();
	void stop_experiment();
	void save_audio(int id_seq);
	
	bool signal4recording();
	bool isrecording();
	bool isworkdone();
	bool isrecordingorworkdone();
	td_msec nowSeq();
	bool rectifyAnswer(int * answeri);
	void dispTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug);
	
	bool str2num(const char * hyp, int * answer);
	
	void recognize_from_microphoneBckpup();
	
	/* SETTERS */
	void initactid4temp(); 
	

	void record_from_microphone2();
	bool executeActuator2(int * durationRefresh_ns);
};






#endif /* Experiment_H_ */







