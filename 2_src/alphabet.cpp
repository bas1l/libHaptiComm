/*
 * alphabet.cpp
 *
 *  Created on: 7 apr. 2016
 *      Author: basilou
 */ 

#include "alphabet.h"
#include<ncurses.h>

using namespace std;




ALPHABET::ALPHABET(){}

ALPHABET::ALPHABET(DEVICE * _dev, WAVEFORM * _wf) :
        dev(_dev), 
        wf( _wf){}

ALPHABET::~ALPHABET() {}



void 
ALPHABET::configure() 
{
    configure_neutral();
    dictionnary.clear();
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


symbol *
ALPHABET::getsymbol(std::string l) 
{
  if ((it_dictionnary = dictionnary.find(l)) == dictionnary.end())
  {
    return NULL;
  }
  
  return &(it_dictionnary->second);
}

bool
ALPHABET::getword(std::string l, waveformLetter * wf) 
{
  symbol * s;
  if ( (s=getsymbol(l)) == NULL)
  {
    return false;
  }
  
  *wf = s->data;

  return true;
}

waveformLetter
ALPHABET::getl(char l) 
{
  waveformLetter ret;
  std::string lstring(1, l);
  getword(lstring, &ret);
  
  return ret;  
}


std::vector<std::string>
ALPHABET::getActList_name(string word) 
{
  symbol * s;
  
  if ( (s=getsymbol(word)) == NULL)
  {
    std::vector<std::string> output;
    return output;
  }
  
  return s->actList;
}

std::vector<uint8_t>
ALPHABET::getActList_chan(string s) 
{
  struct actuator * act;
  std::vector<std::string> actnames;
  std::vector<uint8_t> actIDs;
  int cpt, nb_act;
  
  act = new actuator();
  actnames = getActList_name(s);
  nb_act = actnames.size();
  actIDs.resize(nb_act);
  
  for(cpt=0; cpt<nb_act; ++cpt)
  {
    *act = dev->getActuator(actnames[cpt]); //current actuator
    actIDs[cpt] = act->chan;
  }

  return actIDs;
}



double 
ALPHABET::getFreqRefresh_mHz() 
{
    return wf->getFreqRefresh_mHz();
}


/* Private :
 * 
 * create the adequat motion for the letter/symbol
 */
bool 
ALPHABET::insertSymbol(struct symbol s)
{
    std::pair<std::map<std::string, symbol>::iterator,bool> ret;
    struct motion       m = wf->getMotion(s.motion);
    struct actuator *   act = new actuator();
    waveformLetter          wfLetter;
    std::vector<uint16_t>   mv;
    int dir;
    int amplmax;
    int amplmin;
    int neutral;
    
    for(int cpt=0; cpt<s.actList.size(); ++cpt)
    {
        *act = dev->getActuator(s.actList[cpt]); //current actuator
        dir = act->windingDirection;
        neutral = act->vneutral; //neutral value
        amplmax = act->vmax - neutral; //amplitude max value
        amplmin = neutral - act->vmin;  //amplitude min value
        mv.resize(m.data[cpt].size());
        std::transform(m.data[cpt].begin(), m.data[cpt].end(), mv.begin(), //transform the signal from
            [neutral, amplmax, amplmin, dir](double i){ //+1/-1 to +/-amplitude
                int ampl = (i<0)?amplmin:amplmax;
                return (uint16_t)((i*dir)*ampl+neutral);});
        
        wfLetter.insert(waveformLetterPair(act->chan, mv)); //add vector to the map
        mv.clear(); //clear the temporary vector
        
        /*
         * 
        std::cout   << "motion name='" << s.id
                    << "'|| actuator name='" << s.actList[cpt]
                    << "'; neutral=" << neutral
                    << "; amplmax=" << amplmax
                    << "; amplmin=" << amplmin 
                    << std::endl;
                    
         */
    }
    
    s.data = wfLetter;
    ret = dictionnary.insert(std::pair<std::string, struct symbol>(s.id, s)); //add symbol to the map
    
    
    listSymbols.append(s.id);
    return ret.second;
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



void 
ALPHABET::informationsSymbol(std::string id)
{
    symbol s = dictionnary[id];
    std::cout << "Motion:" 
            << "\n    id: " << s.id  
            << "\n    motion: " << s.motion
            << "\n    actOverlap: " << std::fixed << s.actOverlap
            << "\n    actuators' name: " ;
    for(auto it=s.actList.begin(); it != s.actList.end(); ++it)
    {
        std::cout << *it << ", ";
    }
    std::cout << std::endl;
       
}

void 
ALPHABET::printData(std::string id)
{
    waveformLetter wfl;
    std::vector<uint16_t> data;
    int numSample, c, i;
    
    
    std::cout.precision(5);
    getword(id, &wfl);
    informationsSymbol(id);
    for(auto it=wfl.begin(); it!=wfl.end(); ++it)
    {
        c = it->first;
        data.insert(data.begin(), it->second.begin(), it->second.end());
        
        numSample = data.size();
        std::cout << "channel nº" << c << "/32, numSample= " << numSample << std::endl; 
        for(i=0; i<numSample; i++)
        {
            std::cout << std::fixed << data[i] << "; "; 
        }
        std::cout << std::endl;
        data.clear();
    }   
}


