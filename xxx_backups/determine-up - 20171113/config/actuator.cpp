/*
 * movement.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 
#include "actuator.h"


Actuator::Actuator(int _id) : id(_id), 
													v_up(DATA_NEUTRAL), 
													v_neutral(DATA_NEUTRAL),
													timer(0),
													state(rest)
{}

Actuator::Actuator(int _id, int _v_up, int _v_neutral) : 
													id(_id),
													v_up(_v_up), 
													v_neutral(_v_neutral), 
													timer(0),
													state(rest)
{}

	
// access and modif
void Actuator::setv_up(int _v_up){
	this->v_up = _v_up;
}
void Actuator::setv_neutral(int _v_neutral){
	this->v_neutral = _v_neutral;
}
void Actuator::addnext_states(action a){
	(this->next_states).push(a);
}

int Actuator::getv_up(){
	return this->v_up;
}
int Actuator::getv_neutral(){
	return this->v_neutral;
}
int Actuator::getstate(){
	return this->state;
}


// other functions
void Actuator::begin_action(action a){
	switch(a){
		case rest : 
			initialise_rest(); 
		break;
		
		case apparent :
			initialise_apparent_asc();
		break;
		
		case tap :
			initialise_tap();
		break;
	}
	
	do_action();
}


void Actuator::do_action(){
	if (this->state != rest)
	{
		switch(this->state){
		case apparent_asc :
			movement_apparent_asc(this->v_up, this->id, this->timer);
			check_end();
		break;
		
		case apparent_move :
			movement_apparent_move(this->v_up, this->id, this->timer);
			check_end();
		break;
		
		case tap :
			movement_tap(this->v_up, this->id);
			check_end();
		break;
		
		default:
		break;
		}
	}
	else if (!this->next_states.empty())
	{
		this->state = this->next_states.front();
		this->next_states.pop();
		do_action();
	}
	
	
}

bool Actuator::action_in_progress(){
	if (this->state == rest){
		return false;
	}
		
	return true;
}


// private functions
void Actuator::settimer(int _timer){
	this->timer = _timer;
}

void Actuator::timer_decrease(){
	this->timer--;
}

void Actuator::initialise_rest(){
	this->state = rest;
	settimer(0);
}

void Actuator::initialise_apparent_asc(){
	this->state = apparent_asc;
	settimer(movement_get_apparent_asc_dur());
}

void Actuator::initialise_apparent_move(){
	this->state = apparent_move;
	settimer(waveform_get_apparent_move_dur());
}

void Actuator::initialise_tap(){
	this->state = tap;
	settimer(movement_get_tap_dur());
}


void Actuator::check_end(){
	if (this->timer > 1)
	{
		timer_decrease();
	}
	else
	{
		switch(this->state){
		case apparent_asc :
			initialise_apparent_move();
		break;
		
		default:
			initialise_rest();
		break;
		}
	}
	
}


std::map<int, Actuator> initialise_actuators(){
	std::map<int, Actuator> m;
	Actuator	a = Actuator(ACT_THUMB1, VUP_THUMB1, VN_THUMB1);
	m.insert( std::pair<int, Actuator>(ACT_THUMB1, a) );
	a = Actuator(ACT_THUMB2, VUP_THUMB2, VN_THUMB2);
	m.insert( std::pair<int, Actuator>(ACT_THUMB2, a) );
	
	a = Actuator(ACT_FOREFINGER1, VUP_FOREFINGER1, VN_FOREFINGER1);
	m.insert( std::pair<int, Actuator>(ACT_FOREFINGER1, a) );
	a = Actuator(ACT_FOREFINGER2, VUP_FOREFINGER2, VN_FOREFINGER2);
	m.insert( std::pair<int, Actuator>(ACT_FOREFINGER2, a) );
	a = Actuator(ACT_FOREFINGER3, VUP_FOREFINGER3, VN_FOREFINGER3);
	m.insert( std::pair<int, Actuator>(ACT_FOREFINGER3, a) );
	
	a = Actuator(ACT_MIDFINGER1, VUP_MIDFINGER1, VN_MIDFINGER1);
	m.insert( std::pair<int, Actuator>(ACT_MIDFINGER1, a) );
	a = Actuator(ACT_MIDFINGER2, VUP_MIDFINGER2, VN_MIDFINGER2);
	m.insert( std::pair<int, Actuator>(ACT_MIDFINGER2, a) );
	a = Actuator(ACT_MIDFINGER3, VUP_MIDFINGER3, VN_MIDFINGER3);
	m.insert( std::pair<int, Actuator>(ACT_MIDFINGER3, a) );
	
	
	a = Actuator(ACT_RINGFINGER1, VUP_RINGFINGER1, VN_RINGFINGER1);
	m.insert( std::pair<int, Actuator>(ACT_RINGFINGER1, a) );
	a = Actuator(ACT_RINGFINGER2, VUP_RINGFINGER2, VN_RINGFINGER2);
	m.insert( std::pair<int, Actuator>(ACT_RINGFINGER2, a) );
	a = Actuator(ACT_RINGFINGER3, VUP_RINGFINGER3, VN_RINGFINGER3);
	m.insert( std::pair<int, Actuator>(ACT_RINGFINGER3, a) );
	
	a = Actuator(ACT_PINKY1, VUP_PINKY1, VN_PINKY1);
	m.insert( std::pair<int, Actuator>(ACT_PINKY1, a) );
	a = Actuator(ACT_PINKY2, VUP_PINKY2, VN_PINKY2);
	m.insert( std::pair<int, Actuator>(ACT_PINKY2, a) );
	
	a = Actuator(ACT_PALMLEFT, VUP_PALMLEFT, VN_PALMLEFT);
	m.insert( std::pair<int, Actuator>(ACT_PALMLEFT, a) );
	a = Actuator(ACT_PALMBOT, VUP_PALMBOT, VN_PALMBOT);
	m.insert( std::pair<int, Actuator>(ACT_PALMBOT, a) );
	
	a = Actuator(ACT_PALM11, VUP_PALM11, VN_PALM11);
	m.insert( std::pair<int, Actuator>(ACT_PALM11, a) );
	a = Actuator(ACT_PALM12, VUP_PALM12, VN_PALM12);
	m.insert( std::pair<int, Actuator>(ACT_PALM12, a) );
	a = Actuator(ACT_PALM13, VUP_PALM13, VN_PALM13);
	m.insert( std::pair<int, Actuator>(ACT_PALM13, a) );
	
	a = Actuator(ACT_PALM21, VUP_PALM21, VN_PALM21);
	m.insert( std::pair<int, Actuator>(ACT_PALM21, a) );
	a = Actuator(ACT_PALM22, VUP_PALM22, VN_PALM22);
	m.insert( std::pair<int, Actuator>(ACT_PALM22, a) );
	a = Actuator(ACT_PALM23, VUP_PALM23, VN_PALM23);
	m.insert( std::pair<int, Actuator>(ACT_PALM23, a) );
	
	a = Actuator(ACT_PALM31, VUP_PALM31, VN_PALM31);
	m.insert( std::pair<int, Actuator>(ACT_PALM31, a) );
	a = Actuator(ACT_PALM32, VUP_PALM32, VN_PALM32);
	m.insert( std::pair<int, Actuator>(ACT_PALM32, a) );
	a = Actuator(ACT_PALM33, VUP_PALM33, VN_PALM33);
	m.insert( std::pair<int, Actuator>(ACT_PALM33, a) );
	
	
	
	return m;
}






