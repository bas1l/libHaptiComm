/*
 * waveform.cpp
 *
 *  Created on: 5 april. 2016
 *  Author: basilou
 */

#include "waveform.h"


WAVEFORM::WAVEFORM() {}

WAVEFORM::~WAVEFORM() {}


void 
WAVEFORM::configure(int nmessage_Hz)
{
    freqRefresh_mHz = nmessage_Hz * (1/(double)sec2ms);
}


bool 
WAVEFORM::insertMotion(struct motion m)
{
    std::pair<std::map<std::string, motion>::iterator,bool> ret;
    std::pair<std::string, motion> result;
    
    extractWAV(&m);
    create_dataWAV(&m);
    
    result  = std::make_pair (m.id, m);
    ret     = motionsMap.insert(result);
    
    return ret.second;
}

struct motion
WAVEFORM::getMotion(std::string id)
{
    it_motionsMap = motionsMap.find(id);
    if (it_motionsMap == motionsMap.end())
    {
        std::cout<<"\nCan't get the motion with id=" << id << std::endl;
        exit(-1);
        
    }
    
    return it_motionsMap->second;
}


/*  PRIVATE:
 * 
 * 
 * 
 * 
 */
void 
WAVEFORM::create_dataWAV(struct motion * m)
{
    int numChannels     = m->fileWAV->getNumChannels();
    int numSamples      = m->fileWAV->getNumSamplesPerChannel();
    // Compensate the difference between WAV and DAC frequencies
    int      fs_mHz     = (int)(m->fileWAV->getSampleRate()*(1/(double)sec2ms));
    double   incr       = fs_mHz / (double) freqRefresh_mHz;
    int      nbValue    = (int) (numSamples/incr);
    //std::vector<double> tmpVec;
    
    m->data.clear();
    m->data.reserve(numChannels);
    
    std::vector<double> tmpVec;
    for (int c = 0; c < numChannels; c++)
    {
        tmpVec.reserve(nbValue);
        tmpVec.clear();
        for (int i = 0; i < nbValue; i++)
        {
            tmpVec.push_back((m->fileWAV->samples[c][(int)(i*incr)]));
        }
        tmpVec.push_back(0);
        
        tmpVec.shrink_to_fit();                  // #2
        m->data.emplace_back(std::move(tmpVec)); // #3
    }
}



float * 
WAVEFORM::create_envelope_sin(int length, int ampl)
{
    float * s;
    s = (float*) malloc(length * sizeof(float));

    float incr = M_PI/(float)length;

    for (int i=0; i<length; i++){
            s[i] = ampl *( sin(i * incr)*0.5 + 0.5);
    }

    return s;
}

float * 
WAVEFORM::create_envelope_asc(int length)
{
    float * s;
    s = (float*) malloc(length * sizeof(float));
    
    float incr = M_PI/(float)(2*length);

    for (int i=0; i<length; i++){
            s[i] = sin(i * incr);
    }

    return s;
}





uint16_t * 
WAVEFORM::create_sin(int freq, int ampl, int phase, int nsample, int offset)
{
    uint16_t * s;
    s = (uint16_t*) malloc(nsample * sizeof(uint16_t));

    // Suivant le nombre d'échantillons voulus :
    float incr = 2*M_PI/((float)nsample);
    for (int i=0; i<nsample; i++){
            s[i] = (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset);
    }
    return s;
}


float * 
WAVEFORM::create_random_dots(int nsample, int a, int b)
{
    float * r;
    r = (float*) malloc(nsample*sizeof(float));

    for (int t=0; t<nsample; t++){
            r[t] = rand_a_b(a, b);
    }

    return r;
}

double 
WAVEFORM::getFreqRefresh_mHz()
{
    return freqRefresh_mHz;
}

bool 
WAVEFORM::extractWAV(struct motion * m)
{
    m->fileWAV = new AudioFile<double>();
    
    if (!(m->fileWAV->load(m->wavPath)))
    {
        std::cout<<"\nCan't load wav file:"<< m->wavPath << std::endl;
        exit(-1);
    }
    
    return true;
}

void
WAVEFORM::informations()
{
    std::cout << "\n[WAVEFORM::Informations]" << std::endl;
    for(auto mMap=motionsMap.begin(); mMap!=motionsMap.end(); mMap++)
    {
        informationsMotion(&(mMap->second));
    }
}

void
WAVEFORM::informationsMotion(struct motion * m)
{
    std::cout   << "Motion:" 
                << "\n    id: " << m->id  
                << "\n    name: " << m->name
                << "\n    wavPath: " << m->wavPath
                << "\n    WAV::numChannel: " << m->data.size()
                << std::endl;
}

void
WAVEFORM::printData(struct motion m)
{
    int numChan = m.data.size(), numSample;
    int c, i;
    
    std::cout.precision(15);
    informationsMotion(&m);
    for(c=0; c<numChan; c++)
    {
        numSample = m.data[c].size();
        std::cout << "channel(" << c+1 << "/" << numChan << "),numSample= " << numSample << std::endl; 
        for(i=0; i<numSample; i++)
        {
            std::cout << std::fixed << m.data[c][i] << "; "; 
        }
        std::cout << std::endl;
    }
}

void
WAVEFORM::printData(std::string id)
{
    struct motion m = getMotion(id);
    printData(m);
}


void
WAVEFORM::printWAVData(struct motion m)
{
    int numChan = m.fileWAV->samples.size(), numSample;
    int c, i;
    
    std::cout.precision(15);
    informationsMotion(&m);
    for(c=0; c<numChan; c++)
    {
        numSample = m.fileWAV->samples[c].size();
        std::cout << "channel(" << c+1 << "/" << numChan << "),numSample= " << numSample << std::endl; 
        for(i=0; i<numSample; i++)
        {
            std::cout << std::fixed << m.fileWAV->samples[c][i] << "; "; 
        }
        std::cout << std::endl;
    }
}

void
WAVEFORM::printWAVData(std::string id)
{
    struct motion m = getMotion(id);
    printWAVData(m);
}
