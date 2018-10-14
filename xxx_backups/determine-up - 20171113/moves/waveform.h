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

#include "../config/utils.h"

/***********************************************
				GLOBAL CONSTANTS
***********************************************/
#define deg2rad 3.14159265/180

#define TAP_MOVE_DURATION 50 // ms

#define APPARENT_ASC_DURATION 2000 // ms
#define APPARENT_MOVE_DURATION 750 // ms
#define APPARENT_DURATION   (APPARENT_ASC_DURATION+APPARENT_MOVE_DURATION)

#define APPARENT_MOVE_AMPLITUDE 700 //unit of voltage (0 < v < 4095)
//#define APPARENT_MOVE_FREQUENCY  //?


/***********************************************
				GLOBAL VARIABLES
***********************************************/
static std::vector<int> apparent_asc_fun;
static std::vector<int> apparent_move_fun;


// initialise waveforms
void waveform_begin();
void waveform_end();

double * create_envelope_sin(int length, int ampl);
double * create_sin(int freq, int ampl, int phase, int nsample, int offset);
double * create_envelope_asc();
double * create_random_dots(int length, int a, int b);
std::vector<int> waveform_create_apparent();


// setter and getter
int waveform_get_apparent_asc_dur();
int waveform_get_apparent_move_dur();
int waveform_get_apparent_dur();
int waveform_get_tap_dur();
double waveform_get_apparent_asc(int idx);
int waveform_get_apparent_move(int idx);

void print_moves();

#endif /* H_WAVEFORM_H_ */
