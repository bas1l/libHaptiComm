/*
 * waveform.cpp
 *
 *  Created on: 5 april. 2016
 *  Author: basilou
 */
#include <vector>

#include "waveform.h"




void waveform_begin(){
	waveform_create_apparent();
}


void waveform_end(){
	//if (apparent_asc_fun != NULL) free(apparent_asc_fun);
	//if (apparent_move_fun != NULL) free(apparent_move_fun);
}

double * create_envelope_sin(int length, int ampl){
	double * s;
	s = (double*) malloc(length * sizeof(double));
	
	double incr = M_PI/(double)length;
	 
	for (int i=0; i<length; i++){
		s[i] = ampl *( sin(i * incr)*0.5 + 0.5);
	}
	
	return s;
}

double * create_sin(int freq, int ampl, int phase, int nsample, int offset){
	double * s;
	s = (double*) malloc(nsample * sizeof(double));
	
	double incr = freq*2*M_PI/((double)nsample);
	 
	for (int i=0; i<nsample; i++){
		s[i] = ampl * sin(i*incr +phase) + offset;
	}
	
	return s;
}


double * create_envelope_asc(){
	double * s;
	s = (double*) malloc(APPARENT_ASC_DURATION * sizeof(double));
	
	double incr = M_PI/(double)(2*APPARENT_ASC_DURATION);
	 
	for (int i=0; i<APPARENT_ASC_DURATION; i++){
		s[i] = sin(i * incr);
	}
	
	return s;
}

double * create_random_dots(int length, int a, int b){
	double * r;
	r = (double*) malloc(length*sizeof(double));
	
	for (int t=0; t<length; t++){
		r[t] = rand_a_b(a, b);
	}
	
	return r;
}


std::vector<int> waveform_create_apparent(){
        std::vector<int> apparent_movement;
	int t;
	int inv = 4095;
	// ascension part 1/2
        
	double * asc = create_envelope_asc();
	
	for (t=0; t<APPARENT_ASC_DURATION; t++)
        {
		apparent_movement.push_back(inv - (int) asc[t]);
	}
	
	
	// pink noise part 2/2
	double * shape = create_envelope_sin(APPARENT_MOVE_DURATION, APPARENT_MOVE_AMPLITUDE);
	double * r = create_random_dots(APPARENT_MOVE_DURATION, -1, 1);
	
	for (t=0; t<APPARENT_MOVE_DURATION; t++){
		apparent_movement.push_back(inv - (int)(shape[t] * r[t]));
	}
	
	
	free(asc);
	free(shape);
	free(r);
        
        return apparent_movement;
}

// setter and getter


int waveform_get_apparent_asc_dur(){
	return APPARENT_ASC_DURATION;
}
int waveform_get_apparent_move_dur(){
	return APPARENT_MOVE_DURATION;
}
int waveform_get_apparent_dur(){
	return APPARENT_DURATION;
}
int waveform_get_tap_dur(){
	return TAP_MOVE_DURATION;
}

double waveform_get_apparent_asc(int idx){
	return apparent_asc_fun[idx];
}

int waveform_get_apparent_move(int idx){
	return apparent_move_fun[idx];
}

void print_moves(){
	for (int i = 0; i<APPARENT_ASC_DURATION; i++){
		std::cout << "apparent_asc_fun[" << i << "] = " << apparent_asc_fun[i] << std::endl;
	}
	
	
	for (int i = 0; i<APPARENT_MOVE_DURATION; i++){
		std::cout << "apparent_move_fun[" << i << "] = " << apparent_move_fun[i] << std::endl;
	}
	
}
