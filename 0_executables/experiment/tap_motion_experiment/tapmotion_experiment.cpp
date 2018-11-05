#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <fstream>
#include <math.h>
#include <mutex>
#include <ncurses.h>
#include <queue>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <time.h>
#include <thread>
#include <vector> 

// haptiComm library
#include "HaptiCommConfiguration.h"
#include "waveform.h"
#include "ad5383.h"
#include "utils.h"
#include "alphabet.h"

// personal tools
#include "toolstap.h"

using namespace std;
using namespace std::chrono;

#ifdef _WIN32
#include <windows.h>
#define SYSERROR()  GetLastError()
#else
#include <errno.h>
#define SYSERROR()  errno
#endif


void draw_variable(struct variableMove * vam);
void draw(struct moveWF * mwf);
waveformLetter getAppmove(struct appMove * am, ALPHABET* alph);
waveformLetter getTapmove(struct appMove * am, ALPHABET* alph);
struct variableMove * getVariableam(struct appMove *am, char * c);
bool modifyVariable(struct variableMove * vam, int v);
void resetVariable(struct variableMove * vam);
void initAppMoveVariables(struct appMove * am);
void initTapMoveVariables(struct moveWF * am);
static void parseCmdLineArgs(int argc, char ** argv, const char *& cfgSource, const char *& scope, int & nmessage_sec);



int main(int argc, char *argv[])
{
    /*** Initialisation VARIABLES ***/
    HaptiCommConfiguration * cfg = new HaptiCommConfiguration();
    AD5383 * ad = new AD5383();
    DEVICE *    dev = new DEVICE();
    WAVEFORM *  wf  = new WAVEFORM();
    ALPHABET * alph = new ALPHABET();
    struct appMove * am = new appMove();
    struct moveWF * tapmc = new moveWF();
    struct variableMove * tapvm = new variableMove();
    std::vector<std::vector<uint16_t> > values(AD5383::num_channels);
    waveformLetter wfLetter;
    
    int nbAppmove=0;
    const char * cfgSource;
    const char * scope;
    std::string changeVariables = "";
    char cCurrentVariable = ' ';
    int ch = ERR;
    int nmessage_sec;
    int exitStatus = 0;
    int overruns = 0;
    
    /*** Initialisation ENVIRONMENT ***/
    setlocale(LC_ALL, "");
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
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    
    /*** Configuration VARIABLES ***/
    parseCmdLineArgs(argc, argv, cfgSource, scope, nmessage_sec);
    
    double timePmessage_ns = sec2ns/(double)nmessage_sec; // 2 messages per millisecond
    cfg->parse(cfgSource, "HaptiComm");
    
    ad->spi_open();
    ad->configure();
    cfg->configureDevice(dev);
    cfg->configureWaveform(wf);
    alph->configure(dev, wf);
    initAppMoveVariables(am);
    initTapMoveVariables(tapmc);
    (*tapvm) = {"null", ' ', -1, -1, -1, -1};
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    int refreshRate_Hz      = alph->getFreqRefresh_mHz()*1000;
    
    
    /*** work ***/
    wf->printCharacteristics();
    print_instructions();
    //values = alph->getneutral();
    ad->execute_trajectory(alph->getneutral(), timePmessage_ns);
    
    do {
        if (ERR != ch) {
            if (std::string::npos != changeVariables.find(ch)) {
                //cCurrentVariable = ch;
                //tapvm = getVariableam(am, &cCurrentVariable);
            }
            else if (KEY_RIGHT == ch) {  
                modifyVariable(&tapmc->amplitude, +1);
            }
            else if (KEY_UP == ch) {
                modifyVariable(&tapmc->amplitude, +100);
            }
            else if (KEY_LEFT == ch) {
                modifyVariable(&tapmc->amplitude, -1);
            }
            else if (KEY_DOWN == ch) {
				modifyVariable(&tapmc->amplitude, -100);
            }
            else if ('v' == ch) {
                //write_file(am);
            }
            else if ('j' == ch) {
                resetVariable(&tapmc->amplitude);
            }
            else if ('l' == ch) {
                //initAppMoveVariables(am);
            }
            else if ('\n' == ch) {
                
                tapHoldMove tapholdmc = wf->getTapHoldMoveC();
                wf->configure(tapholdmc, *tapmc, *am, refreshRate_Hz, 1);
                
                //wf->configure(*tapmc, *am, alph->getFreqRefresh_mHz()*1000, 1);
                alph->configure(dev, wf);
                
                wfLetter = getTapmove(am, alph);
                int ovr = ad->execute_selective_trajectory(wfLetter, durationRefresh_ns);
				if (ovr)
				{
					overruns += ovr;
				}
				
				wfLetter.clear();
                nbAppmove++;
            }
        }
        
        draw(tapmc);
        //draw_variable(tapvm);
        printw("\n");
        //printw("ID apparent move :%i\n", nbAppmove);
		printw("transferFrequency=%iHz, overruns=%iBits", (int)alph->getFreqRefresh_mHz()*1000, overruns);
        
    }while((ch = getch()) != '*');
    
    
    /*** Free the environment and memory ***/
    refresh();
    endwin();
    delete dev;
    delete wf;
    delete alph;
    
    return exitStatus;
}

void draw_variable(struct variableMove * vam) {
    printw("\n");
    //printw("%s values:\n", vam->name.c_str());
    //printw("min=<%i> v=<%i> max=<%i>\n", vam->min, vam->value, vam->max);
}

void 
draw(struct moveWF * mwf) {    
    clear();
    print_instructions();
    printw("Global variables:\n");
    //printw("(%c) Number of actuators = <%i>\n", am->nbAct.key, am->nbAct.value);
    //printw("(%c) Overlap between actuators = <%i%>\n", am->actOverlap.key, am->actOverlap.value);
    
    //printw("\n");
    //printw("Ascension :\n");
    //printw("(%c) Type of signal = <%i>\n", am->asc.typeSignal.key, am->asc.typeSignal.value);
    printw("(%c) Amplitude = <%i>\n", mwf->amplitude.key, mwf->amplitude.value);
    //printw("(%c) Duration = <%i>\n", am->asc.duration.key, am->asc.duration.value);
    
    //printw("\n");
    //printw("Action :\n");
    //printw("(%c) Type of signal = <%i>\n", am->action.typeSignal.key, am->action.typeSignal.value);
    //printw("(%c) Amplitude = <%i>\n", am->action.amplitude.key, am->action.amplitude.value);
    //printw("(%c) Duration = <%i>\n", am->action.duration.key, am->action.duration.value);
}

waveformLetter 
getAppmove(struct appMove * am, ALPHABET* alph)
{
    //reconfigure the app motion
    std::vector<std::vector<std::string>> names(am->nbAct.max, std::vector<std::string>(1));
    names[0][0] = "palm32";
    names[1][0] = "palm22";
    names[2][0] = "palm12";
    names[3][0] = "mf1";
    names[4][0] = "mf2";
    names[5][0] = "mf3";
    
    
    std::vector<std::vector<std::string>> actIDs(am->nbAct.value, std::vector<std::string>(1));
    for(int i=0; i<am->nbAct.value; i++)
    {
        actIDs[i][0] = names[(am->nbAct.max-am->nbAct.value)+i][0];
    }
    //std::vector<std::vector<uint16_t>> result = ;
    
    return alph->make_appLetter(actIDs);
}   



waveformLetter 
getTapmove(struct appMove * am, ALPHABET* alph)
{
    //reconfigure the app motion
    std::vector<std::string> names(am->nbAct.max);
    names[0] = "palm32";
    names[1] = "palm22";
    names[2] = "palm12";
    names[3] = "mf1";
    names[4] = "mf2";
    names[5] = "mf3";
    
    
    std::vector<std::string> actIDs(am->nbAct.value);
    for(int i=0; i<am->nbAct.value; i++)
    {
        actIDs[i] = names[(am->nbAct.max-am->nbAct.value)+i];
    }
    //std::vector<std::vector<uint16_t>> result = ;
    
    return alph->make_tapLetter(actIDs);
}   


struct variableMove * getVariableam(struct appMove *am, char * c)
{
    if (*c == am->asc.typeSignal.key){
        return &(am->asc.typeSignal);
    }
    else if (*c == am->asc.amplitude.key){
        return &(am->asc.amplitude);
    }
    else if (*c == am->asc.duration.key){
        return &(am->asc.duration);
    }
    else if (*c == am->action.typeSignal.key){
        return &(am->action.typeSignal);
    }
    else if (*c == am->action.amplitude.key){
        return &(am->action.amplitude);
    }
    else if (*c == am->action.duration.key){
        return &(am->action.duration);
    }
    else if (*c == am->nbAct.key){
        return &(am->nbAct);
    }
    else if (*c == am->actOverlap.key){
        return &(am->actOverlap);
    }
    else{
        return &(am->asc.typeSignal);
    }
}


bool modifyVariable(struct variableMove * vam, int v) {
    bool modified = false;
    int tmp = vam->value + v;
    
    if (tmp<=vam->max && tmp>=vam->min)
    {
        vam->value = tmp;
        modified = true;
    }
    
    return modified;
}

void resetVariable(struct variableMove * vam) {
    vam->value =  vam->valueDefault;
}


void initAppMoveVariables(struct appMove * am) {
	am->asc.name = "Apparent Asc";
    am->asc.wav = "apparentAsc.wav";
    
    am->asc.typeSignal.key = 'q';
    am->asc.typeSignal.name = "ascension type";
    am->asc.typeSignal.value = 1;
    am->asc.typeSignal.valueDefault = 1;
    am->asc.typeSignal.min = 1;
    am->asc.typeSignal.max = 3;
    
    am->asc.amplitude.key = 'w';
    am->asc.amplitude.name = "ascension amplitude";
    am->asc.amplitude.value = 2700;
    am->asc.amplitude.valueDefault = 2700;
    am->asc.amplitude.min = 0;
    am->asc.amplitude.max = 4095;
    
    am->asc.duration.key = 'e';
    am->asc.duration.name = "ascension duration";
    am->asc.duration.value = 100;
    am->asc.duration.valueDefault = 100;
    am->asc.duration.min = 0;
    am->asc.duration.max = 2000;//ms
    
    
    am->action.name = "Apparent Action";
    am->action.wav = "../libtacom/Movement.wav";
    
    am->action.typeSignal.key = 'a';
    am->action.typeSignal.name = "action type";
    am->action.typeSignal.value = 1;
    am->action.typeSignal.valueDefault = 1;
    am->action.typeSignal.min = 1;
    am->action.typeSignal.max = 3;
    
    am->action.amplitude.key = 's';
    am->action.amplitude.name = "action amplitude";
    am->action.amplitude.value = 700;
    am->action.amplitude.valueDefault = 700;
    am->action.amplitude.min = 0;
    am->action.amplitude.max = 4095;
    
    am->action.duration.key = 'd';
    am->action.duration.name = "action duration";
    am->action.duration.value = 400;
    am->action.duration.valueDefault = 400;
    am->action.duration.min = 0;
    am->action.duration.max = 2000;//ms
    
    
    am->nbAct.key = 'z';
    am->nbAct.name = "Number of actuators";
    am->nbAct.value = 1;
    am->nbAct.valueDefault = 1;
    am->nbAct.min = 1;
    am->nbAct.max = 6;//ms
    
    am->actOverlap.key = 'x';
    am->actOverlap.name = "Overlap between actuators";
    am->actOverlap.value = 0;
    am->actOverlap.valueDefault = 0;
    am->actOverlap.min = 0;
    am->actOverlap.max = 100;//ms
}

void initTapMoveVariables(struct moveWF * am) {
    am->name = "Tap motion";
    am->wav = "../libtacom/Tap.wav";
    
    am->amplitude.key = 's';
    am->amplitude.name = "Tap amplitude";
    am->amplitude.value = 2048;
    am->amplitude.valueDefault = 2048;
    am->amplitude.min = 0;
    am->amplitude.max = 4095;
    
    am->duration.value = 15;
}

static void 
parseCmdLineArgs(int argc, char ** argv, const char *& cfgSource, const char *& scope, int & nmessage_sec)
{
    int i;

    cfgSource = "";
    scope = "";
    nmessage_sec = 2000;

    for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0) {
                    usage();
            } else if (strcmp(argv[i], "-cfg") == 0) {
                    if (i == argc-1) { usage(); }
                    cfgSource = argv[i+1];
                    i++;
            } else if (strcmp(argv[i], "-scope") == 0) {
                    if (i == argc-1) { usage(); }
                    scope = argv[i+1];
                    i++;
            } else if (strcmp(argv[i], "-nmessage") == 0) {
                    if (i == argc-1) { usage(); }
                    nmessage_sec = atoi(argv[i+1]);
                    i++;
            } else {
                    fprintf(stderr, "Unrecognised option '%s'\n\n", argv[i]);
                    usage();
            }
    }
}


    































    
