/*
 * alphabet.h
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 
 
#ifndef H_ALPHABET_H_
#define H_ALPHABET_H_

#include <algorithm>
#include <map>
#include <queue>
#include <stdio.h>
#include <string>
#include <vector>

#include "ad5383.h"
#include "waveform.h"
#include "device.h"

#include <iostream>

using namespace std;


/**********************************************/
/***            @typedef alphabet           ***/
/**********************************************/
/* @typedef actuatorsList & actuatorArrangement
 * @brief Arrangement of the actuator during a symbol
 *        according to the wav file channels
 *
 */
typedef std::vector<std::string> actuatorsList;

/* waveformLetter:
 * First -> Identification Number for ad5383 channels
 * Second -> Corresponding waveform values 
 */
typedef std::pair<uint8_t, std::vector<uint16_t>> waveformLetterPair;
typedef std::multimap<uint8_t, std::vector<uint16_t>> waveformLetter;


/**********************************************/
/***            @struct alphabet            ***/
/**********************************************/
/* @struct symbol
 * @brief Value of a symbol : 
 *          - its name
 *          - its timer of the used actuators
 *          - its type of motion
 */

struct symbol
{
    waveformLetter data;
    
    std::string id;
    std::string motion;
    double actOverlap;
    actuatorsList actList;
};




/**********************************************/
/***            @class ALPHABET             ***/
/**********************************************/
class ALPHABET
{
public :
    // constructors
    ALPHABET();
    ALPHABET(DEVICE * _dev, WAVEFORM * _wf);

    ~ALPHABET();
    
    /*
     * @brief configure the actuators values and the letters
     *        depending on the alphabet_define
     */
    void configure();
    void configure(DEVICE * _dev, WAVEFORM * _wf);
    
    
    std::string getlistSymbols();
    double getFreqRefresh_mHz();
    waveformLetter getl(std::string l);
    waveformLetter getl(char l);
    std::vector<std::vector<uint16_t>> getneutral();
        
    void informationsSymbol(std::string id);
    void printData(std::string id);
    
    bool insertSymbol(struct symbol s);
    
private:
    bool configure_letters();
    void configure_neutral();
    
    DEVICE * dev;
    WAVEFORM * wf;
    
    std::string listSymbols;
    std::vector<std::vector<uint16_t>> neutral_statement;
    std::map<std::string, struct symbol> dictionnary;
    std::map<std::string, symbol >::iterator it_dictionnary;
};
#endif //H_ALPHABET_H

 