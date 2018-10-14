/*
 * movement.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 
 
#ifndef H_ACTUATOR_H_
#define H_ACTUATOR_H_

#include <stdio.h>
#include <queue>
#include <map>

#include "waveform.h"
//#include "comm_ad5383.h"

#include "actuator_defines.h"


class Actuator
{
public :
	// constructors
	Actuator(int _id);
	Actuator(int _id, int _v_neutral, int _v_up);
	
	// access and modif
	void setv_up(int _v_up);
	void setv_neutral(int _v_neutral);
	void addnext_states(action a);
	int getv_up();
	int getv_neutral();
	int getstate();
	
	// other functions
	void begin_action(action a);
	void do_action();
	bool action_in_progress();
	
private:
	int v_up;
	int v_neutral;
	int timer;
	int id;
	action state; 
	std::queue<action> next_states; 
	
	void settimer(int _timer);
	void timer_decrease();
	void initialise_rest();
	void initialise_apparent_asc();
	void initialise_apparent_move();
	void initialise_tap();
	void check_end();
	
};

std::map<int, Actuator> initialise_actuators();

#endif //H_ACTUATOR_H

