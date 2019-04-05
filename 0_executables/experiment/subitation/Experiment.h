/*
 * Experiment.h
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil Duvernoy
 */

#include <array> // std::array
#include <chrono> // std::clock, std::chrono::high_resolution_clock::now
#include <numeric> // accumulate
#include <stdlib.h> // srand, rand
#include <string.h> // strncmp
#include <sys/mman.h>
#include <time.h> // time
#include <unistd.h> // sleep, usleep
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
	
	virtual ~Experiment(){ // Destroy the Experiment object
	    delete this->cfg;
	    delete this->dev;
	    delete this->wf;
	    delete this->alph;
	    delete this->ad;
	    
		ps_free(vr_ps);
		cmd_ln_free_r(vr_cfg);
	}
	
	/* Classic functions */
	bool create();
	bool execute();

	/* Getters */
	//vector<std::array<td_msec, 3>> getTimer();
	vector<td_msecarray> getTimer();
	vector<int> getAnswer();
	int getSeq_start();
	int getSeq_end();
	
	
private:
	// HaptiComm variables
	HaptiCommConfiguration * cfg;
	DEVICE * dev;
	WAVEFORM * wf;
	ALPHABET * alph;
	AD5383 * ad;
	int exitStatus;
	const char * cfgSource;
	const char * scope;
	
	int seq_start;
	int seq_end;
	
	// voice recognition variables (sphinx)
	ps_decoder_t * vr_ps; // decoder for voice recognition
	cmd_ln_t * vr_cfg; // config for voice recognition

	
	// Candidat variables
	Candidat * c;
	vector<vector<int>> seq;
	vector<int> actchannelID;
	expEnum expToExec;
	
	// output/result variables
	  //    vector<std::array<td_msec, 3>> vvtimer;
	td_highresclock c_start;
	vector<td_msecarray> vvtimer;
	vector<int> vanswer;
	
	/* function to execute depending on expToExec */
	bool executeF(int * durationRefresh_ns);
	//bool executeBD(int * durationRefresh_ns);
	//bool executeBuz(int * durationRefresh_ns);
	//bool executeActuator(char letter, int * durationRefresh_ns);
	//int executeSequence(int * i, waveformLetter values, int * dr_ns, td_highresclock * c_start, array<td_msec, 3> * vhrc);
	bool executeActuator(int * durationRefresh_ns);
	int  executeSequence(int * currSeq, waveformLetter values_copy, int * dr_ns, td_msecarray * vhrc);
	int executeSequenceSpace(int * currSeq, waveformLetter *values, int * dr_ns, td_msecarray * vhrc);
	int  executeSequenceTemp(int * currSeq, waveformLetter *values, int * dr_ns, td_msecarray * vhrc);
	int recognize_from_microphone(ad_rec_t *ad, td_msecarray * vhrc, td_msecarray * timerDebug);
	
	
	//int recognize_from_microphone(ps_decoder_t * vr_ps, ad_rec_t *ad,
	//							int16_t * adbuf, char const *hyp,
	//							bool * utt_started, int32_t * k,
	//							array<td_msec, 3> * vhrc);
	td_msec nowSeq();
	bool rectifyAnswer(int * answeri);
	void pasteTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug);

	bool wordtonumb(const char * hyp, int * answer);
	void recognize_from_microphoneBckpup();
	
	/* SETTERS */
	void initactid4temp(); 
	

};






#endif /* Experiment_H_ */







