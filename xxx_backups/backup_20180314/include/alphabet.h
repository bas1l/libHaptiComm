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
#include "alphabet_defines.h"


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
    void configure(DEVICE * _dev, WAVEFORM * _wf, double _appMotionActCovering);
    
    std::string getlist_alphabet();
    std::vector<std::vector<uint16_t>> getneutral();
    
    /*
     * @brief return the array according to the letter
     * @param letter letter requested
     */
    std::vector<std::vector<uint16_t>> getl(char l);
    
    std::vector<std::vector<uint16_t>> make_app_letter(std::vector<std::vector<std::string>> a_names);
    
private:
    std::vector<std::vector<uint16_t>> make_tap_letter(std::vector<std::string> a_names);
    std::vector<std::vector<uint16_t>> make_letter(char l);
    bool configure_letters();
    void configure_neutral();
    
    DEVICE * dev;
    WAVEFORM * wf;
    
    std::string list_alphabet;
    std::vector<std::vector<uint16_t>> neutral_statement;
    std::map<char, std::vector<std::vector<uint16_t>>> letters;
    std::map<char, std::vector<std::vector<uint16_t>>>::iterator it_letter;
    
    double  appMotionActCovering;
    int     defaultNeutral;
    int     nbChannel;
    
};


#endif //H_ALPHABET_H

