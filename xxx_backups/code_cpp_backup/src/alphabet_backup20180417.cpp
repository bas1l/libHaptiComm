/*
 * alphabet.cpp
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 

#include "alphabet.h"
#include<ncurses.h>

using namespace std;




ALPHABET::ALPHABET() : listSymbols("abcdefghijklmnopqrstuvwxyz~") {}

ALPHABET::ALPHABET(DEVICE * _dev, WAVEFORM * _wf) :
        dev(_dev), 
        wf( _wf),
        listSymbols("abcdefghijklmnopqrstuvwxyz~") {}

ALPHABET::~ALPHABET() {}



void 
ALPHABET::configure() 
{
    //configure_letters();
    configure_neutral();
    
    dictionnary.clear();
    std::string nan = "~";
    dictionnary[nan] = make_letter(nan.at(0));
}

void 
ALPHABET::configure(DEVICE * _dev, WAVEFORM * _wf) 
{
    dev = _dev;
    wf = _wf;
    
    configure();
}






std::string 
ALPHABET::getlistSymbols() 
{
    return listSymbols;
}

std::vector<std::vector<uint16_t>> 
ALPHABET::getneutral() 
{
    return neutral_statement;
}


waveformLetter
ALPHABET::getl(std::string l) 
{
    // searching for the letter l
    it_dictionnary = dictionnary.find(l);
    if (it_dictionnary != dictionnary.end())
    {
        return it_dictionnary->second;
    }
    else
    {// if not found, return an empty vector 
        return dictionnary.find("~")->second;
    }
}

waveformLetter
ALPHABET::getl(char l) 
{
    std::string lstring(1, l);
    return getl(lstring);
}


double 
ALPHABET::getFreqRefresh_mHz() 
{
    return wf->getFreqRefresh_mHz();
}

/*
 *  private :
 */

/*
 * create the adequat tap move for the letter
 */

waveformLetter
ALPHABET::make_tapHoldLetter(std::vector<std::vector<std::string>> a_names) 
{
    std::map<std::string, struct actuator>  actuators = dev->getActuatorMap();
    
    double actOverlap   = 0.20; //0.25
    bool find           = false;
    motion                tapHoldMotion = wf->getMotion("tapHold");
    std::vector<uint16_t> tapHoldvec    = tapHoldMotion.motionVec;
    int  tapholdsize    = tapHoldvec.size();
    // shift in time/ms/value between 2 actuators in series into the app move
    int  lag_inter_line = tapholdsize*actOverlap;
    int  nb_range       = a_names.size();
    int  total_time     = tapholdsize *(1+ actOverlap*(nb_range-1)) +1;//+1 for neutral statement
    
    std::vector<uint16_t> tmp;
    waveformLetter result;
    
    
    // For each actuator :
    for(auto it=actuators.begin() ; it!=actuators.end() ; ++it)
    {
        std::string curr_name = it->first;

        for(int line=0; line<a_names.size() && !find; line++)
        {// check if the current actuator is a part of the movement, for each line
            auto out = std::find(a_names[line].begin(), a_names[line].end(), curr_name);
            if (out != a_names[line].end())
            {// if yes
                actuator *  act      = &(it->second);
                int         start_at = lag_inter_line*line;
                uint16_t    vneutral = (uint16_t) ~((unsigned int) act->vneutral);

                std::vector<uint16_t> tmp;
                tmp.reserve(total_time+300);
                // (1/3) before the movement:
                tmp.insert(tmp.end(), start_at, vneutral);
                // (2/3) the movement:
                tmp.insert(tmp.end(), tapHoldvec.begin(), tapHoldvec.end());
                // (3/3) after the movement + inter-letters procrastination:
                tmp.insert(tmp.end(), total_time-tmp.size(), vneutral);
                tmp.insert(tmp.end(), 300, vneutral);
                
                
                result.insert(std::pair<uint8_t,std::vector<uint16_t>>(act->chan,tmp));
                
                tmp.clear();
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


waveformLetter
ALPHABET::make_tapLetter(std::vector<std::string> a_names) 
{
    
    waveformLetter result;//(a_names.size());
    actuatorsMap  actuators = dev->getActuatorMap();
    
    motion                tapMotion = wf->getMotion("tap");
    std::vector<uint16_t> tapmove   = tapMotion.motionVec;
    
    // For each actuator :
    //cout << "\tFor each actuator: " << endl;
    for(auto it=actuators.begin() ; it!=actuators.end() ; ++it)
    {
        std::string curr_name = it->first;
        
        // check if the current actuator has to be a part of the move
        if (std::find(a_names.begin(), a_names.end(), curr_name) != a_names.end())
        {
//            cout << "\tcurr_name= " << curr_name << endl; 
            actuator * act = &(it->second);
            
            std::vector<uint16_t> tm;
            tm.reserve(tapmove.size()+300);
            tm.insert(tm.end(), tapmove.begin(), tapmove.end());
            tm.insert(tm.end(), 300, act->vneutral); // inter-letters procrastination
            
            result.insert(std::pair<uint8_t,std::vector<uint16_t>>(act->chan,tm));
        }
        
        // to make it faster : work on it
        //act_names.erase(std::remove(act_names.begin(), act_names.end(), j), act_names.end());
    }
    
    
    return result;
}


waveformLetter
ALPHABET::make_appLetter(std::vector<std::vector<std::string>> a_names) 
{
    actuatorsMap            actuators   = dev->getActuatorMap();
    motion                  appMotion   = wf->getMotion("apparent");
    std::vector<uint16_t>   amvec       = appMotion.motionVec;
    
    double  appMotionActOverlap = 0.30; //0.25
    bool    find                = false;
    // shift in time/ms/value between 2 actuators in series into the app move
    int  lag_inter_line = amvec.size()*appMotionActOverlap;
    int  nb_range       = a_names.size();
    int  total_time     = amvec.size() *(1+ appMotionActOverlap*(nb_range-1)) +1;//+1 for neutral statement
    
    std::vector<uint16_t> tmp;
    waveformLetter result;
    
    
    // For each actuator :
    for(auto it=actuators.begin() ; it!=actuators.end() ; ++it)
    {
        std::string curr_name = it->first;
		
        for(int line=0; line<a_names.size() && !find; line++)
        {// check if the current actuator is a part of the movement, for each line
            auto out = std::find(a_names[line].begin(), a_names[line].end(), curr_name);
            if (out != a_names[line].end())
            {// if yes
                actuator *  act      = &(it->second);
                int         start_at = lag_inter_line*line;
                uint16_t    vneutral = (uint16_t) ~((unsigned int) act->vneutral);
                
                tmp.reserve(total_time);
                // (1/3) before the movement:
                tmp.insert(tmp.end(), start_at, vneutral);
                // (2/3) the movement:
                tmp.insert(tmp.end(), amvec.begin(), amvec.end());
                // (3/3) after the movement + inter-letters procrastination:
                tmp.insert(tmp.end(), total_time-tmp.size(), vneutral);
                //tmp.insert(tmp.end(), 300, vneutral);
                
                
                result.insert(std::pair<uint8_t,std::vector<uint16_t>>(act->chan,tmp));
                
                tmp.clear();
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


bool 
ALPHABET::insertSymbol(struct symbol s)
{
    waveformLetter wfLetter;
    std::pair<std::map<std::string ,waveformLetter>::iterator,bool> ret;
    struct motion m     = wf->getMotion(s.motion);
    int lagInterLine    = m.motionVec.size()*s.actOverlap; // time to wait between two lines
    int totDuration     = m.motionVec.size()+ lagInterLine*(getMaxStartLineID(&(s.actArr))-1) +1;//+1 for end neutral statement
    /*cout    << "symbol name:" << s.name 
            << ". motion:" << s.motion 
            << " (size=" << m.motionVec.size()
            << ". totDuration:" << totDuration 
            << ". lagInterLine:" << lagInterLine 
            << "). overlap:" << s.actOverlap 
            << std::endl;
    */
    actuator *              act = new actuator();
    std::vector<uint16_t>   totMotion;
    std::string             actID;
    int                     actStartLineID;
    int                     timeToWait;
    uint16_t                actVNeutral;
    
    totMotion.reserve(totDuration);
    
    for(actuatorArrangement::iterator it = s.actArr.begin(); it != s.actArr.end(); ++it)
    {
        actID           = it->first;
        actStartLineID  = it->second;
        //cout << "actID:" << actID << "\tactStartLineID:" << actStartLineID << std::endl;
        //cout    << "1" << endl;
        (*act)      = dev->getActuator(actID);
        actVNeutral = (uint16_t) ~((unsigned int) act->vneutral);
        timeToWait  = lagInterLine*(actStartLineID-1);
        //cout    << "reserve" << endl;
        // (1/3) before the motion:
        //cout    << "before:" << timeToWait << endl;
        totMotion.insert(totMotion.begin(), timeToWait, actVNeutral);
        // (2/3) the motion:
        //cout    << "motion" << endl;
        totMotion.insert(totMotion.end(), m.motionVec.begin(), m.motionVec.end());
        // (3/3) after the motion:
        //cout    << "after:" << totDuration << ". " << totMotion.size() << endl;
        totMotion.insert(totMotion.end(), totDuration-totMotion.size(), actVNeutral);
        
        //cout    << "insert" << endl;
        wfLetter.insert(std::pair<uint8_t,std::vector<uint16_t>>(act->chan, totMotion));
       
        //cout    << "clear" << endl;
        totMotion.clear();
    }
    
    //std::pair<std::string, waveformLetter> p;
    //p = std::make_pair;
    
    ret = dictionnary.insert(std::pair<std::string, waveformLetter>(s.id, wfLetter));
    
    return ret.second;
}


int 
ALPHABET::getMaxStartLineID(actuatorArrangement * ar)
{
    int startLine = 0;
    
    for(actuatorArrangement::reverse_iterator rit = ar->rbegin(); rit != ar->rend(); ++rit)
    {
        if (startLine < rit->second)
        {
            startLine = rit->second;
        }
    }
    
    return startLine;
}

waveformLetter
ALPHABET::make_letter(char l) {
    waveformLetter result;
    
    
    switch (l){
        case 'a' :
        {
            std::vector<std::string> names {"t2"};
            result = make_tapLetter(names);
            break;
        }
        case 'b' :
        {
            std::vector<std::string> names {"palm11", "palm12",
                                            "palm21"};
            result = make_tapLetter(names);
            break;
        }
        case 'c' :
        {
            
            std::vector<std::vector<std::string>> names(4,std::vector<std::string>(1));
            names[0][0] = "t2";
            //names[1][0] = "t1";
            names[1][0] = "palm33";
            names[2][0] = "palm13";
            //names[4][0] = "ff1";
            //names[5][0] = "ff2";
            names[3][0] = "ff3";
            
            result = make_tapHoldLetter(names);
            //result = make_appLetter(names);
            break;
        }
        case 'd' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(2));
            names[0][0] = "ff3";
            names[0][1] = "ff1";
            
            result = make_tapHoldLetter(names);
            //result = make_tapLetter(names);
            break;
        }
        case 'e' :
        {
            std::vector<std::string> names {"ff3"};
            result = make_tapLetter(names);
            break;
        }
        case 'f' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(4));
            names[0][0] = "mf3";
            names[0][1] = "mf2";
            names[0][2] = "ff3";
            names[0][3] = "ff2";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'g' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(4));
            names[0][0] = "palm12";
            names[0][1] = "palm13";
            names[0][2] = "palm22";
            names[0][3] = "palm23";
            
            result = make_tapHoldLetter(names);
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
            
            result = make_appLetter(names);
            break;
        }
        case 'i' :
        {
            std::vector<std::string> names {"mf3"};
            result = make_tapLetter(names);
            break;
        }
        case 'j' :
        {
            std::vector<std::vector<std::string>> names(4,std::vector<std::string>(1));
            names[0][0] = "mf3";
            //names[1][0] = "mf2";
            names[1][0] = "mf1";
            //names[3][0] = "palm12";
            //names[4][0] = "palm22";
            names[2][0] = "palm32";
            //names[6][0] = "palm33";
            //names[7][0] = "t1";
            names[3][0] = "t2";
            result = make_tapHoldLetter(names);
            //result = make_appLetter(names);
            break;
        }
        case 'k' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(2));
            names[0][0] = "ff2";
            names[0][1] = "ff1";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'l' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(3));
            names[0][0] = "palm11";
            names[0][1] = "palm12";
            names[0][2] = "palm13";
            
            result = make_tapHoldLetter(names);   
            break;
        }
        case 'm' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(9));
            names[0][0] = "palm11";
            names[0][1] = "palm12";
            names[0][2] = "palm13";
            names[0][3] = "palm21";
            names[0][4] = "palm22";
            names[0][5] = "palm23";
            names[0][6] = "palm31";
            names[0][7] = "palm32";
            names[0][8] = "palm33";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'n' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(6));
            names[0][0] = "palm21";
            names[0][1] = "palm22";
            names[0][2] = "palm23";
            names[0][3] = "palm31";
            names[0][4] = "palm32";
            names[0][5] = "palm33";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'o' :
        {
            std::vector<std::string> names {"rf3"};
            result = make_tapLetter(names);
            break;
        }
        case 'p' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(1));
            names[0][0] = "ff3";
            //waveformLetter r = make_tapHoldLetter(names);
            result = make_appLetter(names);
            //result.begin()->second.insert(result.begin()->second.begin(), r.begin()->second.begin(),r.begin()->second.end());
            break;
        }
        case 'q' :
        {
            std::vector<std::string> names {"palm23"};
            result = make_tapLetter(names);
            break;
        }
        case 'r' :
        {
            std::vector<std::string> names {"palmleft", "palm21", 
                                            "palm31"};
            result = make_tapLetter(names);
            break;
        }
        case 's' :
        {
            std::vector<std::string> names {"p1"};
            result = make_tapLetter(names);
            break;
        }
        case 't' :
        {
            std::vector<std::string> names {"palmleft"};
            result = make_tapLetter(names);
            break;
        }
        case 'u' :
        {
            std::vector<std::string> names {"p2"};
            result = make_tapLetter(names);
            break;
        }
        case 'v' :
        {
            
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(5));
            names[0][0] = "palmleft";
            names[0][1] = "palm11";
            names[0][2] = "palm12";
            names[0][3] = "palm31";
            names[0][4] = "palm32";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'w' :
        {
            std::vector<std::vector<std::string>> names(1,std::vector<std::string>(9));
            names[0][0] = "rf3";
            names[0][1] = "rf2";
            names[0][2] = "rf1";
            names[0][3] = "mf3";
            names[0][4] = "mf2";
            names[0][5] = "mf1";
            names[0][6] = "ff3";
            names[0][7] = "ff2";
            names[0][8] = "ff1";
            
            result = make_tapHoldLetter(names);
            break;
        }
        case 'x' :
        {
            std::vector<std::string> names {"ff2"};
            result = make_tapLetter(names);
            break;
        }
        case 'y' :
        {
            std::vector<std::vector<std::string>> names(3,std::vector<std::string>(1));
            names[0][0] = "palm13";
            names[1][0] = "palm22";
            names[2][0] = "palm31";
            result = make_tapHoldLetter(names);
            //result = make_appLetter(names);
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
            result = make_tapHoldLetter(names);
            //result = make_appLetter(names);
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
    if (!dictionnary.empty())
    {
        dictionnary.clear();
    }
    
    //std::cout << "configure_letters()" << std::endl;
    std::string l;
    for(std::string::size_type i = 0; i < listSymbols.length(); ++i)
    {
        l = listSymbols[i];
        //std::cout << "l = " << l << "::begin " <<  std::endl;
        dictionnary[l] = make_letter(l.at(0));
        //std::cout << "l = " << l << "::end " << std::endl;
    }
    
    //std::cout << "configure_letters().end" << std::endl;
    return true;
}


void 
ALPHABET::configure_neutral() {
    std::vector<uint16_t> temp; 
    temp.push_back(AD5383_DEFAULT_NEUTRAL);
    temp.push_back(AD5383_DEFAULT_NEUTRAL);
    
    for(int i=0; i<AD5383::num_channels; i++)
    {
        neutral_statement.push_back(temp);
    }
}

    

