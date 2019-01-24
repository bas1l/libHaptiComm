/*
 * Experiment.h
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil Duvernoy
 */

#include <array> // std::array
#include <chrono> // std::clock, std::chrono::high_resolution_clock::now
#include <stdlib.h> // srand, rand
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

using namespace std;

#ifndef EXPERIMENT_H_
#define EXPERIMENT_H_

typedef std::chrono::time_point<std::chrono::high_resolution_clock> td_highresclock;
typedef std::chrono::duration<double, milli> td_msec;


class Experiment {
public:
	// this implementation fixes issues with template classes. 
	// https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
	Experiment(const char * _cfgSource, vector<vector<int>> _seq, expEnum _ee)// Create the structure of the Experiment
	{
		cout << "[experiment] new::start..." << endl;
		/* Environmental variables */
		this->cfg = new HaptiCommConfiguration();
		this->dev = new DEVICE();
		this->wf  = new WAVEFORM();
		this->alph = new ALPHABET();
		this->ad = new AD5383();
		this->exitStatus = 0;
		this->cfgSource = _cfgSource;
		
		// input variables
		this->seq = _seq;
		this->expToExec = _ee;

		// output variables
	    this->vvtimer.reserve(this->seq.size());
	    this->vanswer.reserve(this->seq.size());
		
		/* initialize random seed: */
		srand (time(NULL));

		cout << "[experiment] new::...end" << endl;
	}
	virtual ~Experiment(){ // Destroy the Experiment object
	    /* CLEAN */
	    delete cfg;
	    delete dev;
	    delete wf;
	    delete alph;
	}
	
	/* Classic functions */
	bool create();
	bool execute();

	/* Getters */
    vector<std::array<td_msec, 3>> getTimer();
    vector<int> getAnswer();
	
	
private:
	// environmental variables
	HaptiCommConfiguration * cfg;
	DEVICE * dev;
	WAVEFORM * wf;
	ALPHABET * alph;
    AD5383 * ad;
	int exitStatus;
	const char * cfgSource;
	const char * scope;
	
	// input variables to execute the current experiment
	vector<vector<int>> seq;
	expEnum expToExec;
	
	// output/result variables
    vector<std::array<td_msec, 3>> vvtimer;
    vector<int> vanswer;
	
	/* function to execute depending on expToExec */
	bool executeF(int * durationRefresh_ns);
	bool executeBD(int * durationRefresh_ns);
	bool executeBuz(int * durationRefresh_ns);
	bool executeActuator(char letter, int * durationRefresh_ns);
	int executeSequence(int * i, waveformLetter values, int * dr_ns, td_highresclock * c_start, array<td_msec, 3> * vhrc);
	
	
	

};






#endif /* Experiment_H_ */







