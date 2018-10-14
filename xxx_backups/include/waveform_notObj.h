/*
 * waveform.h
 *
 *  Created on: 5 apr. 2016
 *  Author: basilou
 */

#ifndef H_WAVEFORM_H_
#define H_WAVEFORM_H_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"
#include "waveform_defines.h"


/***********************************************
				GLOBAL VARIABLES
***********************************************/
static std::vector<int> apparent_asc_fun;
static std::vector<int> apparent_move_fun;

// initialise waveforms
void waveform_begin();
void waveform_end();

float * create_envelope_sin(int length, int ampl);
uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset);
float * create_envelope_asc();
float * create_random_dots(int length, int a, int b);
std::vector<uint16_t> waveform_create_apparent();


// setter and getter
int waveform_get_apparent_asc_dur();
int waveform_get_apparent_move_dur();
int waveform_get_apparent_dur();
int waveform_get_tap_dur();
float waveform_get_apparent_asc(int idx);
float waveform_get_apparent_move(int idx);

void print_moves();

#endif /* H_WAVEFORM_H_ */
