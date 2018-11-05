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

#include "../moves/movement.h"
#include "../comm/ad5383/comm_ad5383.h"

#define ACT_THUMB1 24
#define ACT_THUMB2 25

#define ACT_FOREFINGER1 21
#define ACT_FOREFINGER2 20
#define ACT_FOREFINGER3 29

#define ACT_MIDFINGER1 10
#define ACT_MIDFINGER2 12
#define ACT_MIDFINGER3 9

#define ACT_RINGFINGER1 26
#define ACT_RINGFINGER2 18
#define ACT_RINGFINGER3 15

#define ACT_PINKY1 23
#define ACT_PINKY2 27

#define ACT_PALMLEFT 30
#define ACT_PALMBOT 0

#define ACT_PALM11 19
#define ACT_PALM12 22
#define ACT_PALM13 8

#define ACT_PALM21 3
#define ACT_PALM22 1
#define ACT_PALM23 2

#define ACT_PALM31 28
#define ACT_PALM32 16
#define ACT_PALM33 17


#define VUP_THUMB1 2700
#define VUP_THUMB2 2700

#define VUP_FOREFINGER1 2500
#define VUP_FOREFINGER2 2600
#define VUP_FOREFINGER3 2500

#define VUP_MIDFINGER1 2700
#define VUP_MIDFINGER2 2700
#define VUP_MIDFINGER3 2700

#define VUP_RINGFINGER1 2600
#define VUP_RINGFINGER2 2700
#define VUP_RINGFINGER3 2650

#define VUP_PINKY1 2700
#define VUP_PINKY2 2700

#define VUP_PALMLEFT 2700
#define VUP_PALMBOT 2700

#define VUP_PALM11 2700
#define VUP_PALM12 2700
#define VUP_PALM13 2700

#define VUP_PALM21 2700
#define VUP_PALM22 2700
#define VUP_PALM23 2700

#define VUP_PALM31 2700
#define VUP_PALM32 2700
#define VUP_PALM33 2700


#define VN_THUMB1 2048
#define VN_THUMB2 2048

#define VN_FOREFINGER1 2048
#define VN_FOREFINGER2 2048
#define VN_FOREFINGER3 2048

#define VN_MIDFINGER1 2048
#define VN_MIDFINGER2 2048
#define VN_MIDFINGER3 2048

#define VN_RINGFINGER1 2048
#define VN_RINGFINGER2 2048
#define VN_RINGFINGER3 2048

#define VN_PINKY1 2048
#define VN_PINKY2 2048

#define VN_PALMLEFT 2048
#define VN_PALMBOT 2048

#define VN_PALM11 2048
#define VN_PALM12 2048
#define VN_PALM13 2048

#define VN_PALM21 2048
#define VN_PALM22 2048
#define VN_PALM23 2048

#define VN_PALM31 2048
#define VN_PALM32 2048
#define VN_PALM33 2048


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

