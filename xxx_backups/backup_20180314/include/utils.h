/*
 * movement.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 
#ifndef H_UTILS_H_
#define H_UTILS_H_

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "utils_defines.h"

// functions
float rand_a_b(int a, int b);
timespec difftimer(timespec start, timespec end);
void mssleep(int time);
void secsleep(int time);

#endif /* H_UTILS_H_ */
