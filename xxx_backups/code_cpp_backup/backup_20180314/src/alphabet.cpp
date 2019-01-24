/*
 * alphabet.cpp
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 

#include "alphabet.h"
#include<ncurses.h>




ALPHABET::ALPHABET() {}


ALPHABET::ALPHABET(DEVICE * _dev, WAVEFORM * _wf) :
        dev(_dev), 
        wf( _wf),
        list_alphabet("abcdefghijklmnopqrstuvwxyz~") {}


ALPHABET::~ALPHABET() {}




void 
ALPHABET::configure(DEVICE * _dev, WAVEFORM * _wf) {
    dev = _dev;
    wf = _wf;
    
    appMotionActCovering = APPARENT_PERCENTAGE_COVERING;
    nbChannel = AD5383::num_channels;
    defaultNeutral = AD5383_DEFAULT_NEUTRAL;
    
    configure_letters();
    configure_neutral();
}


void 
ALPHABET::configure(DEVICE * _dev, WAVEFORM * _wf, double _appMotionActCovering) {
    dev = _dev;
    wf = _wf;
    
    appMotionActCovering = _appMotionActCovering;
    nbChannel = AD5383::num_channels;
    defaultNeutral = AD5383_DEFAULT_NEUTRAL;
    
    configure_letters();
    configure_neutral();
}


void 
ALPHABET::configure() {
    configure_letters();
    configure_neutral();
}


std::string 
ALPHABET::getlist_alphabet() {
    return list_alphabet;
}


std::vector<std::vector<uint16_t>> 
ALPHABET::getneutral() {
    return neutral_statement;
}


std::vector<std::vector<uint16_t>> 
ALPHABET::getl(char l) {
    // searching for the letter l
    it_letter = letters.find(l);
    if (it_letter != letters.end())
    {
        return it_letter->second;
    }
    else
    {// if not found, return an empty vector 
        return letters.find('~')->second;
    }
}


/*
 *  private :
 */

/*
 * create the adequat tap move for the letter
 */
std::vector<std::vector<uint16_t>> 
ALPHABET::make_tap_letter(std::vector<std::string> a_names) {
    bool push = false;
    std::vector<std::vector<uint16_t>>      result(nbChannel);
    std::map<std::string, struct actuator>  actuators = dev->getActuatorMap();
    
    // For each actuator :
    for(auto it=actuators.begin() ; it!=actuators.end() ; ++it)
    {
        std::string curr_name = it->first;
        struct actuator curr_act = it->second;
        int curr_chan = curr_act.chan;
        
        // check if the current actuator has to be a part of the move
        if (std::find(a_names.begin(), a_names.end(), curr_name) != a_names.end())
        {// if yes
            push = true;
        }
        else
        {// if not
            push = false;
        }
        
        // put the corresponding tap move into the result vector
        wf->create_tap_move(curr_act, push, result);
        // to make it faster : work on it
        //act_names.erase(std::remove(act_names.begin(), act_names.end(), j), act_names.end());
    }

    return result;
}


std::vector<std::vector<uint16_t>> 
ALPHABET::make_app_letter(std::vector<std::vector<std::string>> a_names) {
    int nb_range = a_names.size();
    int amsize   = wf->get_app_move_size();
    // shift in time/ms/value between 2 actuators in series into the app move
    int lag_inter_line = amsize*appMotionActCovering;
    int total_time = amsize* (1+ appMotionActCovering*(nb_range-1)) +1;//+1 for neutral statement
    
    std::vector<uint16_t> ttv;
    ttv.reserve(total_time);
    
    bool find = false;
    std::vector<std::vector<uint16_t>>      result(nbChannel, ttv);
    std::map<std::string, struct actuator>  actuators = dev->getActuatorMap();
    
    
    // For each actuator :
    for(auto it=actuators.begin() ; it!=actuators.end() ; ++it)
    {
        std::string curr_name = it->first;
        struct actuator curr_act = it->second;
        uint16_t vneutral = (uint16_t) ~((unsigned int) curr_act.vneutral);

        // put vneutral for all the time of the app move
        result[curr_act.chan].insert(result[curr_act.chan].begin(),
                                     total_time, vneutral);
        
        for(int line=0; line<a_names.size() && !find; line++)
        {// check if the current actuator is a part of the movement, for each line
            auto out = std::find(a_names[line].begin(), a_names[line].end(), curr_name);
            if (out != a_names[line].end())
            {// if yes
                int start_at = lag_inter_line*line;
                wf->create_app_move(curr_act, start_at, result);
                find = true;
            }
        }
        if (find)
        {// so next actuator
            find = false;
        }
    }
    
    return result;
}


std::vector<std::vector<uint16_t>> 
ALPHABET::make_letter(char l) {
    std::vector<std::vector<uint16_t>> result;
    switch (l){
        case 'a' :
        {
            std::vector<std::string> names {"t2"};
            result = make_tap_letter(names);
            break;
        }
        case 'b' :
        {
            std::vector<std::string> names {"palm11", "palm12",
                                            "palm21"};
            result = make_tap_letter(names);
            break;
        }
        case 'c' :
        {
            
            std::vector<std::vector<std::string>> names(7,std::vector<std::string>(1));
            names[0][0] = "t1";
            names[1][0] = "t2";
            names[2][0] = "palm23";
            names[3][0] = "palm13";
            names[4][0] = "ff1";
            names[5][0] = "ff2";
            names[6][0] = "ff3";
            
            result = make_app_letter(names);
            break;
        }
        case 'd' :
        {
            std::vector<std::string> names {"ff3",
                                            "ff2"};
            result = make_tap_letter(names);
            break;
        }
        case 'e' :
        {
            std::vector<std::string> names {"ff3"};
            result = make_tap_letter(names);
            break;
        }
        case 'f' :
        {
            std::vector<std::string> names {"mf3", "ff3", 
                                            "mf2", "ff2"};
            result = make_tap_letter(names);
            break;
        }
        case 'g' :
        {
            std::vector<std::string> names {"palm12", "palm13", 
                                            "palm22", "palm23"};
            result = make_tap_letter(names);
            break;
        }
        case 'h' :
        {
            std::vector<std::vector<std::string>> names(6,std::vector<std::string>(4));
            // first line.
            names[0][0] = "~";
            names[0][1] = "palm31";
            names[0][2] = "palm32";
            names[0][3] = "palm33";
            // second line..
            names[1][0] = "palmleft";
            names[1][1] = "palm21";
            names[1][2] = "palm22";
            names[1][3] = "palm23";
            // third line...
            names[2][0] = "~";
            names[2][1] = "palm11";
            names[2][2] = "palm12";
            names[2][3] = "palm13";
            
            names[3][0] = "p1";
            names[3][1] = "rf1";
            names[3][2] = "mf1";
            names[3][3] = "ff1";
            
            names[4][0] = "p2";
            names[4][1] = "rf2";
            names[4][2] = "mf2";
            names[4][3] = "ff2";
            
            names[5][0] = "~";
            names[5][1] = "rf3";
            names[5][2] = "mf3";
            names[5][3] = "ff3";
            
            result = make_app_letter(names);
            break;
        }
        case 'i' :
        {
            std::vector<std::string> names {"mf3"};
            result = make_tap_letter(names);
            break;
        }
        case 'j' :
        {
            std::vector<std::vector<std::string>> names(9,std::vector<std::string>(1));
            names[0][0] = "mf3";
            names[1][0] = "mf2";
            names[2][0] = "mf1";
            names[3][0] = "palm12";
            names[4][0] = "palm22";
            names[5][0] = "palm32";
            names[6][0] = "palm33";
            names[7][0] = "t1";
            names[8][0] = "t2";
            result = make_app_letter(names);
            break;
        }
        case 'k' :
        {
            std::vector<std::string> names {"ff2",
                                            "ff1"};
            result = make_tap_letter(names);
            break;
        }
        case 'l' :
        {
            std::vector<std::string> names {"palm21", "palm22", "palm23"};
            result = make_tap_letter(names);
            break;
        }
        case 'm' :
        {
            std::vector<std::string> names {"palm11", "palm12", "palm13",
                                            "palm21", "palm22", "palm23",
                                            "palm31", "palm32", "palm33"};
            result = make_tap_letter(names);
            break;
        }
        case 'n' :
        {
            std::vector<std::string> names {"palm21", "palm22", "palm23",
                                            "palm31", "palm32", "palm33"};
            result = make_tap_letter(names);
            break;
        }
        case 'o' :
        {
            std::vector<std::string> names {"rf3"};
            result = make_tap_letter(names);
            break;
        }
        case 'p' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(1));
            names[0][0] = "ff3";
            result = make_app_letter(names);
            break;
        }
        case 'q' :
        {
            std::vector<std::string> names {         "palm23", 
                                            "palm32"};
            result = make_tap_letter(names);
            break;
        }
        case 'r' :
        {
            std::vector<std::string> names {"palmleft", "palm21", 
                                            "palm31"};
            result = make_tap_letter(names);
            break;
        }
        case 's' :
        {
            std::vector<std::string> names {"p1"};
            result = make_tap_letter(names);
            break;
        }
        case 't' :
        {
            std::vector<std::string> names {"palmleft"};
            result = make_tap_letter(names);
            break;
        }
        case 'u' :
        {
            std::vector<std::string> names {"p2"};
            result = make_tap_letter(names);
            break;
        }
        case 'v' :
        {
            std::vector<std::string> names {            "palm11","palm12",
                                            "palmleft",
                                                        "palm31","palm32",};
            result = make_tap_letter(names);
            break;
        }
        case 'w' :
        {
            std::vector<std::string> names {"rf3", "mf3", "ff3", 
                                            "rf2", "mf2", "ff2" 
                                            "rf1", "mf1", "ff1"};
            result = make_tap_letter(names);
            break;
        }
        case 'x' :
        {
            std::vector<std::string> names {"ff2"};
            result = make_tap_letter(names);
            break;
        }
        case 'y' :
        {
            std::vector<std::vector<std::string>> names(3,std::vector<std::string>(1));
            names[0][0] = "palm13";
            names[1][0] = "palm22";
            names[2][0] = "palm31";
            result = make_app_letter(names);
            break;
        }
        case 'z' :
        {
            std::vector<std::vector<std::string>> names(7,std::vector<std::string>(1));
            names[0][0] = "palm13";
            names[1][0] = "palm12";
            names[2][0] = "palm11";
            names[3][0] = "palm22";
            names[4][0] = "palm33";
            names[5][0] = "palm32";
            names[6][0] = "palm31";
            result = make_app_letter(names);
            break;
        }

        default :
            break;
    }
    
    return result;
}


bool 
ALPHABET::configure_letters() {
    //std::map<char, std::vector<std::vector<uint16_t>>> letters;
    if (dev->empty())
    {
        perror("dev->actuators is empty");
        return false;
    }
    if (!letters.empty())
    {
        letters.clear();
    }
    
    //std::cout << "configure_letters()" << std::endl;
    char l;
    for(std::string::size_type i = 0; i < list_alphabet.length(); ++i)
    {
        //std::cout << "l = " << alph[i] << std::endl;
        l = list_alphabet[i];
        letters[l] = make_letter(l);
    }
    
    //std::cout << "configure_letters().end" << std::endl;
    return true;
}


void 
ALPHABET::configure_neutral() {
    std::vector<uint16_t> temp; 
    temp.push_back(defaultNeutral);
    
    for(int i=0; i<nbChannel; i++)
    {
        neutral_statement.push_back(temp);
    }
}

    

