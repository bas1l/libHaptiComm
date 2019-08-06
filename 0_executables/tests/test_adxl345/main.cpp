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
	
#include "adxl345.h"


int initialise_accelerometers(std::vector<int> cs_pins, std::vector<ADXL345> accs);
bool get_taps(std::vector<ADXL345> accs);
bool get_axis(std::vector<ADXL345> accs);



//#define NB_ACCS 6
int main(int argc, char *argv[]) {
	std::vector<ADXL345> accs;
	std::vector<int> cs_pins = {6, 13, 24, 23, 25};
	
	int err;
	
	err = 0;
	if ((err = initialise_accelerometers(cs_pins, accs))) {
	  std::cout<<"[MAIN][ERROR] issue during accs initialisation."<<std::endl;
	  return err;
	}
	
	//get_taps(accs);
	get_axis(accs);
	
	return err;
}


bool get_axis(std::vector<ADXL345> accs) {
  std::chrono::high_resolution_clock::time_point tp_start, tp_end;
  struct timespec tim_model, tim_rem, handled_rem;
  double time_used;
  int ttw_left, nb_accs;
  uint16_t value;
  uint8_t axis[3];
  
  tim_model.tv_sec = 0;
  tim_model.tv_nsec = 100*pow(10,6); // msec*10^6
  tim_rem = tim_model;
  value = 0;
  axis[0] = ADXL345_SELECT_X;
  axis[1] = ADXL345_SELECT_Y;
  axis[2] = ADXL345_SELECT_Z;
  
  
  while (true) {
    tp_start = std::chrono::high_resolution_clock::now();
    
    for (int i=0; i<accs.size(); i++) {
      std::cout<<"("<<std::flush;
      for (int ax=0; ax<sizeof(axis); ax++) {
        value = accs[i].get_axis(axis[ax]);
        if (ax) std::cout<<", "<<std::flush;
        std::cout<<value<<std::flush;
      }
      std::cout<<")\t\t"<<std::flush;
    }
    std::cout<<std::endl;
    
    tp_end = std::chrono::high_resolution_clock::now();
    time_used = std::chrono::duration_cast<std::chrono::duration<double>>(tp_end - tp_start).count();
    std::cout<<"used: \t"<<time_used*pow(10,3)<<"(ms)"<<std::endl;
        
    tim_rem.tv_nsec = tim_rem.tv_nsec-time_used*pow(10,9);
    if (tim_rem.tv_nsec>0) {
      std::cout<<"left: \t"<<tim_rem.tv_nsec/(double)pow(10,6)<<"(ms)"<<std::endl;
      nanosleep(&tim_rem, &handled_rem);
    }
    else {
      std::cout<<"overruns: \t"<<tim_rem.tv_nsec/(double)pow(10,6)<<"(ms)"<<std::endl;
    }
    tim_rem = tim_model;
    std::cout<<std::endl;
  }
  
  return true;
}




bool get_taps(std::vector<ADXL345> accs) {
  std::vector<bool> taps;
  std::chrono::high_resolution_clock::time_point tp_start, tp_end;
  struct timespec tim_model, tim_rem, handled_rem;
  double time_used;
  int ttw_left, nb_accs;
  

  tim_model.tv_sec = 0;
  tim_model.tv_nsec = 1*pow(10,6); // msec*10^6
  tim_rem = tim_model;
    
  nb_accs = accs.size();
  taps.reserve(nb_accs);
  for (int i=0; i<nb_accs; i++) {
    taps.push_back(false);
  }
  
  
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
  
  return true;
}








int initialise_accelerometers(std::vector<int> cs_pins, std::vector<ADXL345> accs) {
  int nb_accs;

  nb_accs = cs_pins.size();
  accs.reserve(nb_accs);
  for (int i=0; i<nb_accs; i++) {
    std::cout<<"accelerometer["<<i<<"] "<<std::endl;
    std::cout<<"\tgpio["<<cs_pins[i]<<"]\t"<<std::flush;
    
    accs.push_back(ADXL345());
    if (!accs[i].spi_open(cs_pins[i])) return 1;
    else std::cout<<"spi_open[OK]\t"<<std::flush;
    
    if (!accs[i].configure()) return 1;
    else std::cout<<"configure[OK]\t"<<std::flush;
    std::cout<<std::endl;
    
    // clean previous taps which have been registered
    accs[i].int_tap_detected_reg();
  }

  std::cout<<"accs are opened and configured."<<std::endl;
  std::cout<<"--------------------"<<std::endl<<std::endl;
  
  return 0;
}



