    /*
 * devicecpp
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 

#include "device.h"

using namespace std;




DEVICE::DEVICE(){
    actuators.clear();
    it_act = actuators.begin();
}

DEVICE::~DEVICE(){}

void DEVICE::configure() {
    
    //nbActuator = ACTUATOR_NB;
    //actMaxValue = ACTUATOR_MAXVALUE;
    if (actuators.empty())
    {
        actuators.clear();
    }
}




struct actuator DEVICE::getActuator(std::string n)
{
    // searching for the name n
    it_act = actuators.find(n);
    
    if (it_act == actuators.end())
    {// if not found, print an error 
        char errorname[100];
        snprintf(errorname, 100, "actuator's name does not exist : %s", n.c_str());
        perror(errorname);
    }
    
    return it_act->second;
}


void DEVICE::setActuator(struct actuator _ac, std::string _idname)
{
    std::pair<actuatorsMap::iterator, bool> ret;
    ret = actuators.insert(std::pair<std::string,actuator>(_idname, _ac));
    if (ret.second==false) {
        std::cerr <<    "element '" << _idname << "' already existed"  << '\n';
    }
}


int DEVICE::getnbActuator() {
    return nbActuator;
}

void DEVICE::setnbActuator(int nba) {
    nbActuator = nba;
}


int DEVICE::getactMaxValue() {
    return actMaxValue;
}

void DEVICE::setactMaxValue(int actmv) {
    actMaxValue = actmv;
}


actuatorsMap DEVICE::getActuatorMap()
{
    return actuators;
}


bool DEVICE::empty()
{
    return actuators.empty();
}




/*
 * private :
 */
actuator DEVICE::make_actuator(std::string n, uint8_t _chan, uint16_t _vup, 
                           uint16_t _vneutral, uint16_t _vpush)
{
    actuator a = {n, +1, _chan, _vup, _vneutral, _vpush};
    return a;
}






