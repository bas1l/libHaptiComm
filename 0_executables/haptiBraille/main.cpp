/*
 * subitation.cpp
 *
 *  Created on: 05 Apr. 2019
 *      Author: Basil Duvernoy
 */

#include <algorithm>
#include <chrono> 		// std::clock, std::chrono::high_resolution_clock::now
#include <getopt.h>
#include <iomanip>      // std::setprecision
#include <iostream>		// std::cout, std::fixed
#include <math.h>       // pow
#include <map>
#include <string>
#include <string.h>
#include <time.h> 		// time
#include <unistd.h> 	// sleep, usleep
#include <vector>
	
#include "haptiBrailleDriver.h"


//#define NB_ACCS 6

int main(int argc, char *argv[])
{
	//std::cout<<std::fixed<<std::setprecision(2);
	const char * spidev_fd = (argc<=1 || strlen(argv[1]) == 0)?"/dev/spidev0.0":argv[1];
	
	struct timespec tim_model, tim_rem, handled_rem;
	std::vector<ADXL345> accs;
	std::vector<int> actuators_pins = {12, 13, 14, 16, 18, 22};
	std::vector<bool> taps;
	int nb_accs;
	
	tim_model.tv_sec = 0;
	tim_model.tv_nsec = 1*pow(10,6); // msec*10^6
	tim_rem = tim_model;
	nb_accs = actuators_pins.size();
	accs.reserve(nb_accs);
	taps.reserve(nb_accs);
	for (int i=0; i<nb_accs; i++)
	{
		std::cout<<"accelerometer["<<i<<"] "<<std::endl;
		std::cout<<"\tgpio["<<actuators_pins[i]<<"]\t"<<std::flush;
		accs.push_back(ADXL345());
		if (!accs[i].spi_open(actuators_pins[i])) return 0;
		else std::cout<<"spi_open[OK]\t"<<std::flush;
		if (!accs[i].configure()) return 0;
		else std::cout<<"configure[OK]\t"<<std::flush;
		std::cout<<std::endl;
		// clean previous taps which have been registered
		accs[i].int_tap_detected_reg();
		
		taps.push_back(false);
	}
	std::cout<<"accs are opened and configured."<<std::endl;
	std::cout<<"--------------------"<<std::endl<<std::endl;
	
	std::chrono::high_resolution_clock::time_point tp_start, tp_end;
	double time_used;
	int ttw_left;
	
	while (std::any_of(taps.begin(), taps.end(), [](bool v) { return !v; }))
	{
		tp_start = std::chrono::high_resolution_clock::now();
		for (int i=0; i<accs.size(); i++)
		{
			if (!taps[i] && accs[i].int_tap_detected_reg())
			{
				taps[i] = true;
				std::cout<<"acc["<<i<<"]: tap detected!"<<std::endl;
			}
		}
		tp_end = std::chrono::high_resolution_clock::now();
		time_used = std::chrono::duration_cast<std::chrono::duration<double>>(tp_end - tp_start).count();
		std::cout<<"used: \t"<<time_used*pow(10,3)<<"(ms)"<<std::endl;
				
		tim_rem.tv_nsec = tim_rem.tv_nsec-time_used*pow(10,9);
		if (tim_rem.tv_nsec>0)
		{
			std::cout<<"left: \t"<<tim_rem.tv_nsec/(double)pow(10,6)<<"(ms)"<<std::endl;
			nanosleep(&tim_rem, &handled_rem);
		}
		else
		{
			std::cout<<"overruns: \t"<<tim_rem.tv_nsec/(double)pow(10,6)<<"(ms)"<<std::endl;
		}
		tim_rem = tim_model;
		std::cout<<std::endl;
	}
	
	return 0;
}







