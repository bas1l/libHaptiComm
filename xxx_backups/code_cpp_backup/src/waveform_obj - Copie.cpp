
    /*
 * waveform.cpp
 *
 *  Created on: 5 april. 2016
 *  Author: basilou
 */

#include "waveform_obj.h"

WAVEFORM::WAVEFORM(int _tapmove_dur, int _appasc_dur, int _appmove_dur,
                int _appmove_ampl, int _app_nb_act_simult) : 
    tapmove_dur(_tapmove_dur),
    appasc_dur(_appasc_dur),
    appmove_dur(_appmove_dur),
    appmove_ampl(_appmove_ampl),
    app_nb_act_simult(_app_nb_act_simult)
{
    app_tot_dur = appasc_dur + appmove_dur;
}


WAVEFORM::WAVEFORM() :
    tapmove_dur(TAP_MOVE_DURATION),
    appasc_dur(APPARENT_ASC_DURATION),
    appmove_dur(APPARENT_MOVE_DURATION),
    appmove_ampl(APPARENT_MOVE_AMPLITUDE),
    app_nb_act_simult(APPARENT_NB_ACT_SUPERPOSED)
{
    app_tot_dur = appasc_dur + appmove_dur;
}

        
WAVEFORM::~WAVEFORM()
{    
}

void WAVEFORM::configure(int _tapmove_dur, int _appasc_dur, int _appmove_dur,
                int _appmove_ampl, int _app_nb_act_simult)
{
    tapmove_dur     = _tapmove_dur;
    appasc_dur      = _appasc_dur;
    appmove_dur     = _appmove_dur;
    appmove_ampl    = _appmove_ampl;
    app_nb_act_simult = _app_nb_act_simult;
    app_tot_dur     = appasc_dur + appmove_dur;
    
    configure();
}

void WAVEFORM::configure()
{
    create_app_move_standard();
}

uint16_t * WAVEFORM::create_sin(int freq, int ampl, int phase, int nsample, int offset)
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

float * WAVEFORM::create_random_dots(int nsample, int a, int b)
{
    float * r;
    r = (float*) malloc(nsample*sizeof(float));

    for (int t=0; t<nsample; t++){
            r[t] = rand_a_b(a, b);
    }

    return r;
}

void WAVEFORM::print_appmoves()
{
    for (std::vector<uint16_t>::const_iterator i = apparent_move.begin(); i != apparent_move.end(); ++i)
    {
        std::cout << "app_move : " << *i << std::endl;
    }
}



int WAVEFORM::get_app_move_size()
{
    return apparent_move.size();
}


void WAVEFORM::create_app_move(actuator a, int start_at, std::vector<std::vector<uint16_t>>& result)
{// actuator a est a utiliser quand on voudra affiner le lever de la tige
    
    result[a.chan].insert(result[a.chan].begin()+start_at, 
                          apparent_move.begin(), apparent_move.end());

    
    
    
    /*
    int i;
    if (line_number == -1)
    {// if not used
        std::cout << "\t[create_app_move] if" << std::endl;
        for(i=0; i<total_time; i++)
        {
            std::cout << "\t[create_app_move] if(vneutral 1)" << std::endl;
            result[a.chan].push_back(vneutral);
            std::cout << "\t[create_app_move] " << i << std::endl;
        }
    }
    else
    {
        std::cout << "\t[create_app_move] else" << std::endl;
        int max = lag_inter_line*line_number;
        for(i=0; i<max; i++)
        {
            std::cout << "\t[create_app_move] else(vneutral 1) " << std::endl;
            result[a.chan].push_back(vneutral);
            std::cout << "\t[create_app_move] i=" << i << "/"  << max << std::endl;
        }
        
        std::cout << "\t[create_app_move] insert" << std::endl;
        result[a.chan].insert(std::end(result[a.chan]), std::begin(apparent_move), std::end(apparent_move));
        // more linre number is close to nb_range, less we have to fill the end
        max = lag_inter_line*(nb_range -(line_number+1)) +1;
        std::cout << "\t[create_app_move] insert end" << std::endl;
        for(i=0; i<max; i++)
        {
            std::cout << "\t[create_app_move] else(vneutral 2) " << max << std::endl;
            result[a.chan].push_back(vneutral);
            std::cout << "\t[create_app_move] " << i << std::endl;
        }
    }
    */
    std::cout << "\t[create_app_move] end" << std::endl;
}


void WAVEFORM::create_tap_move(actuator a, bool push, std::vector<std::vector<uint16_t>>& result)
{
    if (TAP_MOVE_DURATION < 2)
    {
        perror("TAP_MOVE_DURATION < 2 forbidden");
        //return result;
    }
    
    int mid = TAP_MOVE_DURATION-2;
    uint16_t vneutral = (uint16_t) ~((unsigned int) a.vneutral);
    int ms;
    
    if (push)
    {// if the movement is to push
        uint16_t vpush = (uint16_t) ~((unsigned int) a.vpush);
        uint16_t vpushb = (uint16_t) ~((unsigned int) ACTUATOR_MAXVALUE-a.vpush);
        
        // push the actuator
        for(ms=0; ms<mid;ms++)
        {
            result[a.chan].push_back(vpush);
        }
        // recall the actuator
        for(; ms<TAP_MOVE_DURATION-1;ms++)
        {
            result[a.chan].push_back(vpushb);
        }
        
        // put the neutral value at the end
        result[a.chan].push_back(vneutral);
    }
    else
    {// if the actuator is not a part of the movement
        for(ms=0; ms<TAP_MOVE_DURATION;ms++)
        {
            result[a.chan].push_back(vneutral);
        }
    }
}

void WAVEFORM::create_tap_move_bis(actuator a, bool push, std::vector<uint16_t>& result)
{
    if (TAP_MOVE_DURATION < 2)
    {
        perror("TAP_MOVE_DURATION < 3 forbidden");
        //return result;
    }
    
    int mid = TAP_MOVE_DURATION-2;
    uint16_t vneutral = (uint16_t) ~((unsigned int) a.vneutral);
    int ms;
    
    if (push)
    {// if the movement is to push
        uint16_t vpush = (uint16_t) ~((unsigned int) a.vpush);
        uint16_t vpushb = (uint16_t) ~((unsigned int) ACTUATOR_MAXVALUE-a.vpush);
        
        // push the actuator
        for(ms=0; ms<mid;ms++){
            result.push_back(vpush);
        }
        // recall the actuator
        for(; ms<TAP_MOVE_DURATION-1;ms++){
            result.push_back(vpushb);
        }
        
        // put the neutral value at the end
        result.push_back(vneutral);
    }
    else
    {//if the actuator is not a part of the movement
        for(ms=0; ms<TAP_MOVE_DURATION;ms++){
            result.push_back(vneutral);
        }
    }
}

/*
 *  private :
 */
void WAVEFORM::create_app_move_standard()
{
    int t;
    uint16_t inv = 4095;
    apparent_move.clear();
 
    // part 1/2 : ascension 
    float * asc = create_envelope_asc();

    for (t=0; t<appasc_dur; t++)
    {
            apparent_move.push_back(inv - (uint16_t) (2000+1000*asc[t]));
            //printw("asc=%.f", asc[t]);
    }


    // part 2/2 : pink noise 
    float * shape = create_envelope_sin(appmove_dur, appmove_ampl);
    float * r = create_random_dots(appmove_dur, -1, 1);

    for (t=0; t<appmove_dur; t++){
            apparent_move.push_back(inv - (uint16_t)(3200+ shape[t] * r[t]));
            //printw("move=%f", shape[t] * r[t]);
    }

    free(asc);
    free(shape);
    free(r);
}

float * WAVEFORM::create_envelope_sin(int length, int ampl)
{
    float * s;
    s = (float*) malloc(length * sizeof(float));

    float incr = M_PI/(float)length;

    for (int i=0; i<length; i++){
            s[i] = ampl *( sin(i * incr)*0.5 + 0.5);
    }

    return s;
}

float * WAVEFORM::create_envelope_asc()
{
    float * s;
    s = (float*) malloc(appasc_dur * sizeof(float));

    float incr = M_PI/(float)(2*appasc_dur);

    for (int i=0; i<appasc_dur; i++){
            s[i] = sin(i * incr);
    }

    return s;
}


