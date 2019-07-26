/*
 * Experiment.h
 *
 *  Created on: 10 Apr. 2019
 *      Author: Basil Duvernoy
 */

/* STANDARD LIBRARIES */
#include <algorithm>			// for max_element, transform, random_shuffle
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
#include "adxl345.h"
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

struct get_first
{
    template<typename T, typename U>
    T const& operator()(std::pair<T, U> const& pair)
    {
        return pair.first;
    }
};
struct get_second
{
    template<typename T, typename U>
    U const& operator()(std::pair<T, U> const& pair)
    {
        return pair.second;
    }
};

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
	std::vector<msec_array_t> get_answer_timers();
	std::vector<int> 		  get_answer_values();
	std::vector<int> 		  get_answer_confidences();
	std::vector<vector<int>>  get_actuators_sequences();
	std::vector<char>         get_waveforms_sequences();
	std::vector<int>          get_ERMCalibrationID();
	int get_seq_start();
	int get_seq_end();
	
	
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
    bool                    recording;
    bool                    executeMotion;
    bool                    audioBufferReady;
    bool                    waveformReady;
    waveformLetter          current_waveform;
    
	pthread_attr_t     		attr;
	pthread_t  				t_record;
    pthread_t               t_tap;
    pthread_t               t_orchestration;

	
	
	/* d. Alsa voice recording variables */
	snd_pcm_t *capture_handle;
	unsigned int rate_mic;
	
	/* e. Candidat variables */
	Candidat * 			c;
	vector<int> 		actchannelID;
	expEnum 			expToExec;

    /* f. input experiment variables */
    // pair's vector of <ON/OFF actuator's value, waveforms> => ie. <[0 0 1 0 1 1], '1'>, <[1 1 1 0 1 0], '3'>, ...
    std::vector< std::pair<std::vector<int>, char> > actuatorsAndWaveformIDs_sequences;
    // map of <waveformIDs, waveforms> => ie. <'1', waveformLetter1>, <'2', waveformLetter2>, ...
    std::map<char, waveformLetter> waveformIDsAndWF_map;
    
    /* g. output/result variables */
	highresclock_t 			c_start;
	vector<msec_array_t> 	answer_timers;
	vector<int> 			answer_values;
	vector<int> 			answer_confidences;
	AudioFile<double> * 	af;
	
	
/* 2. FUNCTIONS */
    /* a.1 main tasks experiment:: void threads functions */
	static void * static_record_from_microphone(void * c);
    static void * static_execute_stimuli(void * c);
    static void * static_start_experiment(void * c);
    
    /* a.2 main tasks experiment:: effective functions */
	void record_from_microphone();
    void execute_stimuli();
    void start_experiment();

    /* a.3 main tasks experiment:: sub-effective functions */
	bool start_calibration_word();
	bool start_calibration_ERM();
	bool start_space_finger();
	bool start_space_actuator();
	
	
	/* b. threads related */
    bool signal4waveform_ready();
    bool signal4motion();
	bool signal4stopmotion();
	bool signal4recording();
	bool signal4stoprecording();
	bool signal4stop_recording();
	bool signal4stop_experiment();
	void start_recording();
	void stop_recording();
	void stop_experiment();
    bool start_motion();
    bool stop_motion();
    bool set_current_waveform(int sequence_id);
    waveformLetter get_current_waveform();
	
	/* c. answers related */
	bool read_answer(int * answeri);
	bool read_confidence(int * confidencei);
	void set_audioBuffer(AudioFile<double>::AudioBuffer buffer);
	void save_audio(int id_seq);
		
	/* d. checkers */
    bool is_executeMotion();
    bool is_executeMotion_stoped();
	bool is_recording();
	bool is_recording_stoped();
	bool is_workdone();
	bool is_recordingorworkdone();
    bool is_audioBufferReady();
    bool is_waveformReady();
	
	/* e. inner tools */
    void tool_setup_waveformIDsAndWF_map(vector<char> waveformIDs);
    void tool_setup_actuatorsAndWaveformIDs_sequences(vector<vector<int>> actuatorIDs, vector<char> waveformIDs);
	int  tool_setup_captureHandle();
    waveformLetter tool_get_sequenceWaveform_space(int sequence_id);
    waveformLetter tool_get_sequenceCalibration_space();
    
    msec_t tool_now(highresclock_t start);
	void   tool_randomWaiting();
    void   tool_dispHeader(string s);
    void   tool_dispTimers(int sequence_id, int answeri, msec_array_t vhrc, msec_array_t timerDebug);
};

#endif /* Experiment_H_ */





























