
    /*
 * waveform.cpp
 *
 *  Created on: 5 april. 2016
 *  Author: basilou
 */


#include "waveform.h"




WAVEFORM::WAVEFORM(
            int _tapDuration, int _appAscDuration, int _appActionDuration,
            int _appActionAmplitude, int _appActSuperposed) : 
    tapDuration(_tapDuration),
    appAscDuration(_appAscDuration),
    appActionDuration(_appActionDuration),
    appActionAmplitude(_appActionAmplitude),
    appActSuperposed(_appActSuperposed) {
    appDuration = appAscDuration + appActionDuration;
}


WAVEFORM::WAVEFORM() {}


WAVEFORM::~WAVEFORM() {}




void 
WAVEFORM::configure(int _tapDuration, int _appActSuperposed, 
                    int _appRatioCover, int _appAscDuration, 
                    int _appActionDuration, int _appActionAmplitude)
{

            
    tapDuration         = _tapDuration;
    
    appActSuperposed    = _appActSuperposed;
    appRatioCover       = _appRatioCover;
    appAscDuration      = _appAscDuration;
    appActionDuration   = _appActionDuration;
    appActionAmplitude  = _appActionAmplitude;
    
    appDuration         = appAscDuration + appActionDuration;
    
    create_app_move_standard();
}


void 
WAVEFORM::configure(struct appMoveCarac _amc)
{
    amc = _amc;
    create_app_move_standard();
}


void 
WAVEFORM::configure()
{
    create_app_move_standard();
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


void 
WAVEFORM::print_appmoves()
{
    for (std::vector<uint16_t>::const_iterator i = appMoveVec.begin(); i != appMoveVec.end(); ++i)
    {
        std::cout << "app_move : " << *i << std::endl;
    }
}


int 
WAVEFORM::get_app_move_size()
{
    return appMoveVec.size();
}


void 
WAVEFORM::create_app_move(actuator a, int start_at, std::vector<std::vector<uint16_t>>& result)
{// actuator a est a utiliser quand on voudra affiner le lever de tige
    result[a.chan].insert(result[a.chan].begin()+start_at, 
                          appMoveVec.begin(), appMoveVec.end());
}


void 
WAVEFORM::create_tap_move(actuator a, bool push, std::vector<std::vector<uint16_t>>& result)
{
    if (tapDuration < 2)
    {
        perror("TAP_MOVE_DURATION < 2 forbidden");
    }
    
    int mid = tapDuration-2;
    uint16_t vneutral = (uint16_t) ~((unsigned int) a.vneutral);
    int ms;
    
    if (push)
    {// if the movement is to push
        uint16_t vpush = (uint16_t) ~((unsigned int) a.vpush);
        uint16_t vpushb = (uint16_t) ~((unsigned int) ACTUATOR_MAXVALUE-a.vpush);
        
        // push the actuator
        for(ms=0; ms<mid; ms++)
        {
            result[a.chan].push_back(vpush);
        }
        // recall the actuator
        for(; ms<tapDuration-1; ms++)
        {
            result[a.chan].push_back(vpushb);
        }
        
        // put the neutral value at the end
        result[a.chan].push_back(vneutral);
    }
    else
    {// if the actuator is not a part of the movement
        for(ms=0; ms<tapDuration; ms++)
        {
            result[a.chan].push_back(vneutral);
        }
    }
}

void 
WAVEFORM::create_tap_move_bis(actuator a, bool push, std::vector<uint16_t>& result)
{
    if (tapDuration < 2)
    {
        perror("TAP_MOVE_DURATION < 3 forbidden");
        //return result;
    }
    
    int mid = tapDuration-2;
    uint16_t vneutral = (uint16_t) ~((unsigned int) a.vneutral);
    int ms;
    
    if (push)
    {// if the movement is to push
        uint16_t vpush = (uint16_t) ~((unsigned int) a.vpush);
        uint16_t vpushb = (uint16_t) ~((unsigned int) ACTUATOR_MAXVALUE-a.vpush);
        
        // push the actuator
        for(ms=0; ms<mid; ms++){
            result.push_back(vpush);
        }
        // recall the actuator
        for(; ms<tapDuration-1; ms++){
            result.push_back(vpushb);
        }
        
        // put the neutral value at the end
        result.push_back(vneutral);
    }
    else
    {//if the actuator is not a part of the movement
        for(ms=0; ms<tapDuration; ms++){
            result.push_back(vneutral);
        }
    }
}

/*
 *  private :
 */
void 
WAVEFORM::create_app_move_standard()
{
    int t;
    uint16_t inv = 4095;
    appMoveVec.clear();
 
    // part 1/2 : ascension 
    float * asc = create_envelope_asc();

    for (t=0; t<appAscDuration; t++)
    {
            appMoveVec.push_back(inv - (uint16_t) (2000+1000*asc[t]));
            //printw("asc=%.f", asc[t]);
    }


    // part 2/2 : pink noise 
    float * shape = create_envelope_sin(appActionDuration, appActionAmplitude);
    float * r = create_random_dots(appActionDuration, -1, 1);

    for (t=0; t<appActionDuration; t++){
            appMoveVec.push_back(inv - (uint16_t)(3200+ shape[t] * r[t]));
            //printw("move=%f", shape[t] * r[t]);
    }

    free(asc);
    free(shape);
    free(r);
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
WAVEFORM::create_envelope_asc()
{
    float * s;
    s = (float*) malloc(appAscDuration * sizeof(float));
    
    float incr = M_PI/(float)(2*appAscDuration);

    for (int i=0; i<appAscDuration; i++){
            s[i] = sin(i * incr);
    }

    return s;
}


