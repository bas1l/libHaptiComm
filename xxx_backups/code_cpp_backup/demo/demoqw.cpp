#include <iostream>
#include <sys/mman.h>
#include <algorithm>
#include <thread>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <random>
#include <atomic>
#include <sys/ioctl.h>
#include<ncurses.h>
#include <math.h>

#include "waveform.h"
#include "ad5383.h"
#include "utils.h"



using namespace std;


/*
int TAP_MOVE_DURATION;// 50; // ms

int APPARENT_ASC_DURATION;// 300 // ms
int APPARENT_MOVE_DURATION;// 50 // ms
#define APPARENT_DURATION   (APPARENT_ASC_DURATION+APPARENT_MOVE_DURATION)

int APPARENT_MOVE_AMPLITUDE;// 700
int APPARENT_NB_ACT_SUPERPOSED; //a tester entre 3 et 12
*/
//counts every number that is added to the queue
static long long producer_count = 0;
//counts every number that is taken out of the queue
static long long consumer_count = 0;

static int ms2ns = 1000000;
static int hz_static = 1000; // ne doit jamais etre en dessous de 2.
static int ms_per_symbols = 1;


static const uint8_t a1 = 5;
static const uint8_t a2 = 4;
static const uint8_t a3 = 3;
static const uint8_t a4 = 17;
static const uint8_t a5 = 14;
static const uint8_t a6 = 22;
static const uint8_t a7 = 12;
static const uint8_t a8 = 9;
static const uint8_t a9 = 15;
static const uint8_t a10 = 2;
static const uint8_t a11 = 21;
static const uint8_t a12 = 18;
static const uint8_t a13 = 19;
static const uint8_t a14 = 13;
static const uint8_t a15 = 8;
static const uint8_t a16 = 16;
static const uint8_t a17 = 1;
static const uint8_t a18 = 20;
static const uint8_t a19 = 7;
static const uint8_t a20 = 23;
static const uint8_t a21 = 11;
static const uint8_t a22 = 10;
static const uint8_t a23 = 6;
static const uint8_t a24 = 0;


float * create_envelope_sin(int length, int ampl){
	float * s;
	s = (float*) malloc(length * sizeof(float));
	
	float incr = M_PI/(float)length;
	 
	for (int i=0; i<length; i++){
		s[i] = ampl *( sin(i * incr)*0.5 + 0.5);
	}
	
	return s;
}
//float * create_sin(int freq, int ampl, int phase, int nsample, int offset){
//	float * s;
//	s = (float*) malloc(nsample * sizeof(float));
//	
//        // Suivant le nombre d'échantillons voulus :
//	float incr = 2*M_PI/((float)nsample);
//	printw("incr= %.6f\n", incr);
//	for (int i=0; i<nsample; i++){
//                //s[i] = sin(i*incr*freq +phase);
//		s[i] = ampl * sin(i*incr*freq +phase) + offset;
//                printw("%.6f,", s[i]);
//	}
//	
//	return s;
//}

uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset){
	uint16_t * s;
	s = (uint16_t*) malloc(nsample * sizeof(uint16_t));
	
        // Suivant le nombre d'échantillons voulus :
	float incr = 2*M_PI/((float)nsample);
	printw("incr= %.6f\n", incr);
	for (int i=0; i<nsample; i++){
                //s[i] = sin(i*incr*freq +phase);
		s[i] = (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset);
                //printw("%i,", s[i]);
	}
	return s;
}


float * create_envelope_asc(){
	float * s;
	s = (float*) malloc(APPARENT_ASC_DURATION * sizeof(float));
	
	float incr = M_PI/(float)(2*APPARENT_ASC_DURATION);
	 
	for (int i=0; i<APPARENT_ASC_DURATION; i++){
		s[i] = sin(i * incr);
	}
	
	return s;
}
float * create_random_dots(int length, int a, int b){
	float * r;
	r = (float*) malloc(length*sizeof(float));
	
	for (int t=0; t<length; t++){
		r[t] = rand_a_b(a, b);
	}
	
	return r;
}
std::vector<uint16_t> waveform_create_apparent(){
        std::vector<uint16_t> apparent_movement;
	int t;
	uint16_t inv = 4095;
	// ascension part 1/2
        
	float * asc = create_envelope_asc();
	
	for (t=0; t<APPARENT_ASC_DURATION; t++)
        {
		apparent_movement.push_back(inv - (uint16_t) (2000+1000*asc[t]));
                //printw("asc=%.f", asc[t]);
	}
	
	
	// pink noise part 2/2
	float * shape = create_envelope_sin(APPARENT_MOVE_DURATION, APPARENT_MOVE_AMPLITUDE);
	float * r = create_random_dots(APPARENT_MOVE_DURATION, -1, 1);
	
	for (t=0; t<APPARENT_MOVE_DURATION; t++){
		apparent_movement.push_back(inv - (uint16_t)(3200+ shape[t] * r[t]));
                //printw("move=%f", shape[t] * r[t]);
	}
	
	
	free(asc);
	free(shape);
	free(r);
        
        return apparent_movement;
}

void push_channel(std::vector<std::vector<uint16_t> >& values, std::vector<int> channels){
    int ms_max = TAP_MOVE_DURATION;//50;
    int mid = ms_max-2;
    
    
    for(int j = 0; j < 31; ++j)
    {
        if (std::find(channels.begin(), channels.end(), j) != channels.end())
        {
            //printw("yolo\n");
            int ms;
            for(ms=0; ms<mid;ms++){
                values[j].push_back(0);
            }
            for(; ms<ms_max-1;ms++){
                values[j].push_back(4000);
            }
            values[j].push_back(2048);
            channels.erase(std::remove(channels.begin(), channels.end(), j), channels.end());
        }
        else
        {
            int ms;
            for(ms=0; ms<ms_max;ms++){
                values[j].push_back(2048);
            }
        }
    }
    
}
void letter_a_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a2};
    push_channel(values, channels);
}
void letter_b_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a8, a12, a13};
    push_channel(values, channels);
}
void letter_c_push(std::vector<std::vector<uint16_t> >& values, std::vector<uint16_t>& am){
    int number_act_used = 8;
    int size = am.size();
    int time_beforenext = size/APPARENT_NB_ACT_SUPERPOSED;
    //int tbeforenext = size/6;;
    int total_time = (number_act_used+(APPARENT_NB_ACT_SUPERPOSED-1))*time_beforenext+1;
    
    
    for(int j = 0; j < 31; ++j)
    {
        switch(j){
            case a2 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a1 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a4 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a10 :
                for(int i=0; i<3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a11 :
                for(int i=0; i<4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a17 :
                for(int i=0; i<5*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-5*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a18 :
                for(int i=0; i<6*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-6*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a24 :
                for(int i=0; i<7*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                //for(int i=0; i<total_time-size-7*time_beforenext;i++){
                values[j].push_back(2048);
                //}
                break;
                
            default :
                for(int i=0; i<total_time;i++){
                    values[j].push_back(2048);
                }
                break;
        }
    }

    
    std::vector<int> channels {a1, a2};
    push_channel(values, channels);
}
void letter_d_push(std::vector<std::vector<uint16_t> >& values){
    
    
    std::vector<int> channels {a17, a24};
    push_channel(values, channels);
}
void letter_e_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a24};
    push_channel(values, channels);
}
void letter_f_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a18, a19, a23, a24};
    push_channel(values, channels);
}
void letter_g_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a11, a12, a9, a10};
    push_channel(values, channels);
}
void letter_h_push(std::vector<std::vector<uint16_t> >& values, std::vector<uint16_t>& am){
    int number_act_used = 6; // in the same time. the longer of the direction
    int size = am.size();
    int time_beforenext = size/APPARENT_NB_ACT_SUPERPOSED;
    //int tbeforenext = size/6;;
    int total_time = (number_act_used+(APPARENT_NB_ACT_SUPERPOSED-1))*time_beforenext+1;
    
    
    for(int j = 0; j < 31; ++j)
    {
        switch(j){
            case a4 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
            break;
            case a5 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
            break;
            case a6 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
            break;
                
            case a8 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
            break;
            case a9 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
            break;
            case a10 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
            break;
                
            case a11 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a12 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a13 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a15 :
                for(int i=0; i<3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a16 :
                for(int i=0; i<3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a17 :
                for(int i=0; i<3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a18 :
                for(int i=0; i<4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a19 :
                for(int i=0; i<4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a20 :
                for(int i=0; i<4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a22 :
                for(int i=0; i<total_time-size-1;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                break;   
            case a23 :
                for(int i=0; i<total_time-size-1;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                break;   
            case a24 :
                for(int i=0; i<total_time-size-1;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                break;   
                
            default :
                for(int i=0; i<total_time;i++){
                    values[j].push_back(2048);
                }
                break;
        }
    }
    


    
}
void letter_i_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a23};
    push_channel(values, channels);
}
void letter_j_push(std::vector<std::vector<uint16_t> >& values, std::vector<uint16_t>& am){
    int number_act_used = 9;
    int size = am.size();
    int time_beforenext = size/APPARENT_NB_ACT_SUPERPOSED;
    //int tbeforenext = size/6;;
    int total_time = (number_act_used+(APPARENT_NB_ACT_SUPERPOSED-1))*time_beforenext+1;
    
    
    for(int j = 0; j < 31; ++j)
    {
        switch(j){
            case a23 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a19 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a16 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a12 :
                for(int i=0; i<3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-3*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a9 :
                for(int i=0; i<4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-4*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a5 :
                for(int i=0; i<5*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-5*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a4 :
                for(int i=0; i<6*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-6*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
            case a1 :
                for(int i=0; i<7*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-7*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a2 :
                for(int i=0; i<total_time-size-1;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                break;   
                
            default :
                for(int i=0; i<total_time;i++){
                    values[j].push_back(2048);
                }
                break;
        }
    }
}
void letter_k_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a17, a18};
    push_channel(values, channels);
}
void letter_l_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a8, a9, a10 };
    push_channel(values, channels);
}
void letter_m_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a4, a5, a6, a8, a9, a10, a11, a12, a13 };
    push_channel(values, channels);
}
void letter_n_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a4, a5, a6, a8, a9, a10 };
    push_channel(values, channels);
}
void letter_o_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a22};
    push_channel(values, channels);
}
void letter_p_push(std::vector<std::vector<uint16_t> >& values, std::vector<uint16_t>& am){
    int number_act_used = 6;
    int size = am.size();
    int time_beforenext = size/APPARENT_NB_ACT_SUPERPOSED;
    //int tbeforenext = size/6;;
    int total_time = (number_act_used+(APPARENT_NB_ACT_SUPERPOSED-1))*time_beforenext+1;
    
    for(int j = 0; j < 31; ++j)
    {
        switch(j){
            case a24 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                break;
                
            default :
                for(int i=0; i<total_time;i++){
                    values[j].push_back(2048);
                }
                break;
        }
    }
    //std::vector<int> channels {};
    //push_channel(values, channels);
}
void letter_q_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a10, a5 };
    push_channel(values, channels);
}
void letter_r_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a6, a7, a8 };
    push_channel(values, channels);
}
void letter_s_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a14 };
    push_channel(values, channels);
}
void letter_t_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a7 };
    push_channel(values, channels);
}
void letter_u_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a21 };
    push_channel(values, channels);
}
void letter_v_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a5, a7, a12 };
    push_channel(values, channels);
}
void letter_w_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a15, a16, a17, a18, a19, a20, a22, a24 };
    push_channel(values, channels);
}
void letter_x_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a18 };
    push_channel(values, channels);
}
void letter_y_push(std::vector<std::vector<uint16_t> >& values, std::vector<uint16_t>& am){
    int number_act_used = 3;
    int size = am.size();
    int time_beforenext = size/APPARENT_NB_ACT_SUPERPOSED;
    //int tbeforenext = size/6;;
    int total_time = (number_act_used+(APPARENT_NB_ACT_SUPERPOSED-1))*time_beforenext+1;
    
    
    for(int j = 0; j < 31; ++j)
    {
        switch(j){
            case a11 :
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a9 :
                for(int i=0; i<time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                for(int i=0; i<total_time-size-time_beforenext;i++){
                    values[j].push_back(2048);
                }
                break;
                
            case a6 :
                for(int i=0; i<2*time_beforenext;i++){
                    values[j].push_back(2048);
                }
                values[j].insert(std::end(values[j]), std::begin(am), std::end(am));
                values[j].push_back(2048);
                
                break;
                
            default :
                for(int i=0; i<total_time;i++){
                    values[j].push_back(2048);
                }
                break;
        }
    }
}
void letter_z_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels {a8, a9, a10};
    push_channel(values, channels);
}
void letter_space_push(std::vector<std::vector<uint16_t> >& values){
    std::vector<int> channels { 30 };//nothing, juste a break.
    push_channel(values, channels);
}
void push_actuator(std::vector<std::vector<uint16_t> >& values, int sens){
    static int v = 2048;
    std::vector<int> channels { 9 };//nothing, just a break.

    if (1==sens){
        v+=50;
    }
    else if (0==sens){
        v-=50;
    }
    
    for(int j = 0; j < 31; ++j)
    {
        if (9==j){
            for(int i=0;i < 500; i++){
                values[j].push_back(v);
            }
            values[j].push_back(2048);
        }
        else {
            for(int i=0;i < 500; i++){
                values[j].push_back(2048);
            }
            values[j].push_back(2048);
        }
    }
    
    printw("\nact=9,value=%i\t", v);
    //push_channel(values, channels);
}
void push_sine_wave(std::vector<std::vector<uint16_t> >& values, int freq, int ampl, int offset){
    int chan = 9;
    
    int phase = 0;
    int nsample = 2000;
    std::vector<int> channels{ chan };//nothing, just a break.
    
    
    uint16_t * s = create_sin(freq, ampl, phase, nsample, offset);
    
    
    //printw("\nact=9,freq=%i,ampl=%i\n", freq, ampl);
    for(int j = 0; j < 31; ++j)
    {
        if (9==j){
            for(int i=0;i < nsample; i++){
                values[j].push_back(s[i]);
                //printw("%i, ", s[i]);
            }
            values[j].push_back(2048);
        }
        else {
            for(int i=0;i < nsample; i++){
                values[j].push_back(2048);
            }
            values[j].push_back(2048);
        }
    }
    
}


void push_letters(std::vector<std::vector<uint16_t> >& values, const char c, std::vector<uint16_t>& am, double * freq_message){
    
    static int ampl = 50;
    static int upto = 2048;
    static int freq = 0;
    switch(c){
        case 'a' :
            letter_a_push(values);
            break;
            
        case 'b' :
            letter_b_push(values);
            break;
            
        case 'c' :
            letter_c_push(values, am);
            break;
            
        case 'd' :
            letter_d_push(values);
            break;
            
        case 'e' :
            letter_e_push(values);
            break;
            
        case 'f' :
            letter_f_push(values);
            break;
            
        case 'g' :
            letter_g_push(values);
            break;
            
        case 'h' :
            letter_h_push(values, am);
            break;
            
        case 'i' :
            letter_i_push(values);
            break;
            
        case 'j' :
            letter_j_push(values, am);
            break;
            
        case 'k' :
            letter_k_push(values);
            break;
            
        case 'l' :
            letter_l_push(values);
            break;
            
        case 'm' :
            letter_m_push(values);
            break;
            
        case 'n' :
            letter_n_push(values);
            break;
            
        case 'o' :
            letter_o_push(values);
            break;
            
        case 'p' :
            letter_p_push(values, am);
            break;
            
        case 'q' :
            letter_q_push(values);
            break;
            
        case 'r' :
            letter_r_push(values);
            break;
            
        case 's' :
            letter_s_push(values);
            break;
            
        case 't' :
            letter_t_push(values);
            break;
            
        case 'u' :
            letter_u_push(values);
            break;
            
        case 'v' :
            letter_v_push(values);
            break;
            
        case 'w' :
            letter_w_push(values);
            break;
            
        case 'x' :
            letter_x_push(values);
            break;
            
        case 'y' :
            letter_y_push(values, am);
            break;
            
        case 'z' :
            letter_z_push(values);
            break;
            
        case ' ' :
            letter_space_push(values);
        break;
        case '1' :     
            freq -= 5;
            printw("freq=%i, ", freq);
            //printw("Donnez une frequence: ");
            //scanw("%d", &freq);
            //printw("%i", freq);
            //push_sine_wave(values, 100, ampl);
        break;
        case '2' :
            freq += 5;
            printw("freq=%i, ", freq);
            //printw("Donnez l'offset: ");
            //scanw("%d", &upto);
            //printw("%i", upto);
            //push_sine_wave(values, 200, ampl);
        break;
        case '3' :     
            upto -= 25;
            printw("offset=%i, ", upto);
            //printw("Donnez une frequence: ");
            //scanw("%d", &freq);
            //printw("%i", freq);
            //push_sine_wave(values, 100, ampl);
        break;
        case '4' :
            upto += 25;
            printw("offset=%i, ", upto);
            //printw("Donnez l'offset: ");
            //scanw("%d", &upto);
            //printw("%i", upto);
            //push_sine_wave(values, 200, ampl);
        break;
        case '5' :
            //push_actuator(values, 0);
            ampl -= 25;
            printw("ampl=%i, ", ampl);
        break;
        case '6' :
            //push_actuator(values, 1);
            ampl += 25;
            printw("ampl=%i, ", ampl);
        break;
        case '+' :
            printw("\n(freq=%i, ampl=%i,  offset=%i) \t",  freq, ampl, upto);
            push_sine_wave(values, freq, ampl, upto);
            //push_actuator(values, -1);
        break;
//        case '1' :            
//            printw("Donnez une frequence: ");
//            scanw("%d", &freq);
//            printw("%i", freq);
//            //push_sine_wave(values, 100, ampl);
//        break;
//        case '2' :
//            printw("Donnez l'offset: ");
//            scanw("%d", &upto);
//            printw("%i", upto);
//            //push_sine_wave(values, 200, ampl);
//        break;
        default :
            
            break;
    }
    
}
void init_neutral(AD5383 ad, std::vector<std::vector<uint16_t> >& values){
    for(int j = 0; j < 31; ++j)
    {
        values[j].push_back(2048);
        values[j].push_back(2048);
    }
    
    ad.execute_trajectory(values, 1000000);
}

void generateSentences(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone){
    int p = 0;
    p==1 && std::cout <<  "c0" << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    p==1 && std::cout <<  "c1" << std::endl;
    int ch;

    initscr();
    p==1 && std::cout <<  "c2" << std::endl;
    raw();
    p==1 && std::cout <<  "c3" << std::endl;
    keypad(stdscr, TRUE);
    p==1 && std::cout <<  "c4" << std::endl;
    noecho();
    
    p==1 && std::cout <<  "c5" << std::endl;
    printw("WHC::create_sentences::Begin  (Press '*' to Exit)\n");

    do{
        if (ch != ERR) {
            switch(ch){
                case KEY_UP: {
                    printw("\n\nUp Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_DOWN:{
                    printw("\n\nDown Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_LEFT:{
                    printw("\n\nLeft Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                case KEY_RIGHT:{
                    printw("\n\nRight Arrow\n");
                    printw("Press q to Exit\t");
                    break;
                }
                default:{
                    std::unique_lock<std::mutex> lk(m);
                    printw("%c", ch);
                    //printw("[GS][for] sentences.push(c)\n");
                    sentences.push(ch);
                    //printw("[GS][for] producer_count()++  %c\n", ch);

                    producer_count++;
                    //printw("[GS][for] cv.notify_one()\n");
                    cv.notify_one(); // Notify worker
                    //printw("[GS][for] cv.wait(lk)\n");
                    cv.wait(lk); // Wait for worker to complete
                    //printw("[GS][for] cv.wait(lk) :: FREE\n");
                    //printw("Press q to Exit\t");
                    break;
                }
            }
            // eoeowowej
          }
    }while((ch = getch()) != '*');
    
    
    printw("\tWHC::create_sentences::End\n");
    refresh();
    endwin();
    workdone = true;
    cv.notify_one(); // Notify worker
}
void workSymbols(std::queue<char> & sentences, std::condition_variable & cv, std::mutex & m, std::atomic<bool> & workdone){
    //initscr();
    int p =0;
    
    p && printw("\tWHC::workSymbols::Begin\n");
    // init drive electronics
    AD5383 ad;
    ad.spi_open();
    ad.configure();
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    p && printw("\tWHC::workSymbols::neutral\n");
    init_neutral(ad, values);
    
    // fréquence maximale pour les sinus utilisées
    double hz_max = 1000; //Hz=1/s
    // th. de Nyquist implique :
    double freq_message = hz_max*2; // 2000 message / secondes (par chan)
    // un peu bizarre. Mais on souhaite faire 2 envoies de messages par millisec
    double timePmessage_ns = hz_max/freq_message * ms2ns; // * ns
    
    std::vector<uint16_t> apparent_movement = waveform_create_apparent();
    std::queue<char> personalSentences;
    
    // Initialisation complete.
    //printw("\t{WS} notify_one()\n");
    cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
    
    // The goal of this function is to use the letters putted by the other
    // thread, one by one, and play them consecutively.
    //printw("\t{WS} while...()\n");
    while(!workdone.load() or !sentences.empty())
    {
        //printw("{WS}{while} Begin\n");
        std::unique_lock<std::mutex> lk(m);

        //printw("{WS}{while} sentences.EMPTY ACHTOUNG()\n");
        if (sentences.empty())
        {
            //printw("\t{WS}{while} cv.wait(lk)\n");
            cv.wait(lk); // Wait for the generator to complete
            //printw("\t{WS}{while} cv.wait(lk) :: FREE\n");
        }
        //printw("\t{WS}{while} ..sentences.empty() :: 2\n");
        if (sentences.empty())
            continue;
        // free the common value asap
        personalSentences.push(sentences.front());
        
        sentences.pop();
        consumer_count++;
        cv.notify_one(); // Notify generator (placed here to avoid waiting for the lock)
        
        // if last char is a space, then a word is finished
        if (personalSentences.front() != ' ')// is part of the alphabet){
        {
            values = alph.getl(letters.front());
            ad.execute_trajectory(values, timePmessage_ns);
            // for all channels, clear.
            for (int w=0; w<values.size(); ++w)
            {
                values[w].clear();
            }
        }
        personalSentences.pop();
     }
    
    p && printw("\tWS::workSymbols::End\n");
    //endwin();
}


int main(int argc, char *argv[])
{   
    /* 1/2 init */
    // a l'heure actuelle les valeurs sont : 10 20 40 800 4
    /*
    TAP_MOVE_DURATION = atoi(argv[1])*2;//10
    APPARENT_ASC_DURATION = atoi(argv[2])*2;//20
    APPARENT_MOVE_DURATION = atoi(argv[3])*2;//40
    APPARENT_MOVE_AMPLITUDE = atoi(argv[4]);//800
    APPARENT_NB_ACT_SUPERPOSED = atoi(argv[5]);//4
    */
    struct timespec t;
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
            perror("sched_setscheduler failed");
            exit(-1);
    }
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            perror("mlockall failed");
            exit(-2);
    }
    
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> workdone(false);
    std::queue<char> sentences;
    std::thread consumer;
    consumer = std::thread(workSymbols, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));
    std::thread producer;
    producer = std::thread(generateSentences, std::ref(sentences), std::ref(cv), std::ref(m), std::ref(workdone));
    
    
    /* 2/2 job */
    consumer.join();
    producer.join();

    return 0;
}
