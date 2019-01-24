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
    configure_actuators();
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
    std::pair<std::map<std::string, struct actuator>::iterator, bool> ret;
    ret = actuators.insert(std::make_pair(_idname, _ac));
    if (ret.second==false) {
        std::cerr <<    "element '" << _idname << "' already existed"  << '\n';
    }
}

std::map<std::string, struct actuator> DEVICE::getActuatorMap()
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


void DEVICE::configure_actuators()
{
    //std::map<std::string, struct actuator> actuators;
    if (actuators.empty())
    {
        actuators.clear();
    }
    std::string name;
    
    name = "t1";
    actuators[name] = make_actuator(name, ACT_THUMB1, VUP_THUMB1, 
                                    VN_THUMB1, VPUSH_THUMB1);
    name = "t2";
    actuators[name] = make_actuator(name, ACT_THUMB2, VUP_THUMB2, 
                                    VN_THUMB2, VPUSH_THUMB2);
    name = "ff1";
    actuators[name] = make_actuator(name, ACT_FOREFINGER1, VUP_FOREFINGER1, 
                                    VN_FOREFINGER1, VPUSH_FOREFINGER1);
    name = "ff2";
    actuators[name] = make_actuator(name, ACT_FOREFINGER2, VUP_FOREFINGER2, 
                                    VN_FOREFINGER2, VPUSH_FOREFINGER2);
    name = "ff3";
    actuators[name] = make_actuator(name, ACT_FOREFINGER3, VUP_FOREFINGER3, 
                                    VN_FOREFINGER3, VPUSH_FOREFINGER3);
    name = "mf1";
    actuators[name] = make_actuator(name, ACT_MIDFINGER1, VUP_MIDFINGER1, 
                                    VN_MIDFINGER1, VPUSH_MIDFINGER1);
    name = "mf2";
    actuators[name] = make_actuator(name, ACT_MIDFINGER2, VUP_MIDFINGER2, 
                                    VN_MIDFINGER2, VPUSH_MIDFINGER2);
    name = "mf3";
    actuators[name] = make_actuator(name, ACT_MIDFINGER3, VUP_MIDFINGER3, 
                                    VN_MIDFINGER3, VPUSH_MIDFINGER3);
    name = "rf1";
    actuators[name] = make_actuator(name, ACT_RINGFINGER1, VUP_RINGFINGER1, 
                                    VN_RINGFINGER1, VPUSH_RINGFINGER1);
    name = "rf2";
    actuators[name] = make_actuator(name, ACT_RINGFINGER2, VUP_RINGFINGER2, 
                                    VN_RINGFINGER2, VPUSH_RINGFINGER2);
    name = "rf3";
    actuators[name] = make_actuator(name, ACT_RINGFINGER3, VUP_RINGFINGER3, 
                                    VN_RINGFINGER3, VPUSH_RINGFINGER3);
    name = "p1";
    actuators[name] = make_actuator(name, ACT_PINKY1, VUP_PINKY1, 
                                    VN_PINKY1, VPUSH_PINKY1);
    name = "p2";
    actuators[name] = make_actuator(name, ACT_PINKY2, VUP_PINKY2, 
                                    VN_PINKY2, VPUSH_PINKY2);
    name = "palmleft";
    actuators[name] = make_actuator(name, ACT_PALMLEFT, VUP_PALMLEFT, 
                                    VN_PALMLEFT, VPUSH_PALMLEFT);
    name = "palmbot";
    actuators[name] = make_actuator(name, ACT_PALMBOT, VUP_PALMBOT, 
                                    VN_PALMBOT, VPUSH_PALMBOT);
    name = "palm11";
    actuators[name] = make_actuator(name, ACT_PALM11, VUP_PALM11, 
                                    VN_PALM11, VPUSH_PALM11);
    name = "palm12";
    actuators[name] = make_actuator(name, ACT_PALM12, VUP_PALM12, 
                                    VN_PALM12, VPUSH_PALM12);
    name = "palm13";
    actuators[name] = make_actuator(name, ACT_PALM13, VUP_PALM13, 
                                    VN_PALM13, VPUSH_PALM13);
    name = "palm21";
    actuators[name] = make_actuator(name, ACT_PALM21, VUP_PALM21, 
                                    VN_PALM21, VPUSH_PALM21);
    name = "palm22";
    actuators[name] = make_actuator(name, ACT_PALM22, VUP_PALM22, 
                                    VN_PALM22, VPUSH_PALM22);
    name = "palm23";
    actuators[name] = make_actuator(name, ACT_PALM23, VUP_PALM23, 
                                    VN_PALM23, VPUSH_PALM23);
    name = "palm31";
    actuators[name] = make_actuator(name, ACT_PALM31, VUP_PALM31, 
                                    VN_PALM31, VPUSH_PALM31);
    name = "palm32";
    actuators[name] = make_actuator(name, ACT_PALM32, VUP_PALM32, 
                                    VN_PALM32, VPUSH_PALM32);
    name = "palm33";
    actuators[name] = make_actuator(name, ACT_PALM33, VUP_PALM33, 
                                    VN_PALM33, VPUSH_PALM33);
}
