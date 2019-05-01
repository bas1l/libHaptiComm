/*
 * actuator.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 

#ifndef H_DEVICE_H_
#define H_DEVICE_H_

#include <iostream>
#include <map>
#include <queue>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <vector>


struct actuator
{
	static constexpr int ANTICLOCKWISE = 1;
	static constexpr int CLOCKWISE = -1;

    std::string name;
    
    int8_t windingDirection;
    uint8_t chan;
    
    uint16_t vmax;
    uint16_t vmin; 
    uint16_t vneutral; 
    uint16_t vup;
};

typedef std::map<std::string, actuator> actuatorsMap;



class DEVICE
{
public :
    // constructors
    DEVICE();

    ~DEVICE();

    /*
     * @brief configure the actuators values and the letters
     *        depending on the actuator_define
     */
    void configure();
    
    
    /*
     * @brief return the values according to the actuator's name
     * @param n name of the actuator requested
     */
    struct actuator getActuator(std::string n);
    void            setActuator(struct actuator _ac, std::string _idname);
    
    int getnbActuator();
    void setnbActuator(int nba);
    
    int getactMaxValue();
    void setactMaxValue(int actmv);
    

    /*
     * @brief return the map of actuators
     */
    actuatorsMap getActuatorMap();
    
    
    /*
     * @brief say if actuators is empty
     */
    bool empty();
    
    /*
     * @brief Set an actuator to the map actuators
     */
    void setActuator(std::string _idname, std::string _name,
                     uint8_t _chan, uint16_t _vup,
                     uint16_t _vneutral, uint16_t _vpush);
    
private:
    actuator make_actuator(std::string n, uint8_t _chan, uint16_t _vup, 
                           uint16_t _vneutral, uint16_t _vpush);
    
    actuatorsMap actuators;
    std::map<std::string, struct actuator>::iterator it_act;
    int nbActuator;
    int actMaxValue;
};
#endif //H_DEVICE_H




