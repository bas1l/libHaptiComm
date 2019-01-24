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

float * create_envelope_sin(int length, int ampl){
	float * s;
	s = (float*) malloc(length * sizeof(float));
	
	float incr = M_PI/(float)length;
	 
	for (int i=0; i<length; i++){
		s[i] = ampl *( sin(i * incr)*0.5 + 0.5);
	}
	
	return s;
}
uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset){
	uint16_t * s;
	s = (uint16_t*) malloc(nsample * sizeof(uint16_t));
	
        // Suivant le nombre d'échantillons voulus :
	float incr = 2*M_PI/((float)nsample);
	for (int i=0; i<nsample; i++){
		s[i] = (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset);
	}
	return s;
}
float * create_envelope_asc(){
	float * s;
	s = (float*) malloc(APPARENT_ASC_DURATION * sizeof(float));
	
	float incr = M_PI/(float)(2*APPARENT_ASC_DURATION);
	 
	for (int i=0; i<APPARENT_ASC_DURATION; i++){
		s[i] = sin(i * incr);
	}
	
	return s;
}
float * create_random_dots(int length, int a, int b){
	float * r;
	r = (float*) malloc(length*sizeof(float));
	
	for (int t=0; t<length; t++){
		r[t] = rand_a_b(a, b);
	}
	
	return r;
}
std::vector<uint16_t> waveform_create_apparent(){
        std::vector<uint16_t> apparent_movement;
	int t;
	uint16_t inv = 4095;
	// ascension part 1/2
        
	float * asc = create_envelope_asc();
	
	for (t=0; t<APPARENT_ASC_DURATION; t++)
        {
		apparent_movement.push_back(inv - (uint16_t) (2000+1000*asc[t]));
                //printw("asc=%.f", asc[t]);
	}
	
	
	// pink noise part 2/2
	float * shape = create_envelope_sin(APPARENT_MOVE_DURATION, APPARENT_MOVE_AMPLITUDE);
	float * r = create_random_dots(APPARENT_MOVE_DURATION, -1, 1);
	
	for (t=0; t<APPARENT_MOVE_DURATION; t++){
		apparent_movement.push_back(inv - (uint16_t)(3200+ shape[t] * r[t]));
                //printw("move=%f", shape[t] * r[t]);
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

float waveform_get_apparent_asc(int idx){
	return apparent_asc_fun[idx];
}

float waveform_get_apparent_move(int idx){
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
