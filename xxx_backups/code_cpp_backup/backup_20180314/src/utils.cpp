/*
 * movement.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 
#include "utils.h"


float rand_a_b(int a, int b){
	//srand((unsigned) time(NULL));
	return (float)(rand() / (float)RAND_MAX) * (b-a) + a; 
}

timespec difftimer(timespec start, timespec end){
	timespec temp;
	
	if((end.tv_nsec-start.tv_nsec)<0){
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	}
	else
	{
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	
	return temp;
}

void mssleep(int time){
	usleep(time*1000);
}
 
void secsleep(int time){
	mssleep(time*1000);
}
 
