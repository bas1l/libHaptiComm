/*
 * Experiment.cpp
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil
 */

#include "Experiment.h"


bool Experiment::create()
{
    cout << "[experiment] create::start..." << endl;
    
    // input variables
    this->seq = this->c->getSequence();
    this->expToExec = this->c->nextExp();
    
    // output variables
    this->vvtimer.reserve(this->seq.size());
    this->vanswer.reserve(this->seq.size());
    
    /* initialize random seed: */
    srand (time(NULL));

    /* SETUP ENVIRONEMENT: printw and timer */
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
    
    setlocale(LC_ALL, "");
    
    /* configurations */
    this->cfg->configure(cfgSource, dev, wf, alph);
    
    // preparing the fake argv variable
    string fdd(c->getPathDict()+c->getLangage()+"/"+c->getLangage());
    int fakeargc = 4;
    char lm[8], lmval[128], dict[8], dictval[128], *fakeargv[4];;
    strcpy(lm, "-lm");
    strcpy(lmval, fdd.c_str());
    strcat(lmval, ".lm");
    strcpy(dict, "-dict");
    strcpy(dictval, fdd.c_str());
    strcat(dictval, ".dic");
    fakeargv[0] = lm;
    fakeargv[1] = lmval;
    fakeargv[2] = dict;
    fakeargv[3] = dictval;
    
    this->vr_cfg = cmd_ln_parse_r(NULL, cont_args_def, fakeargc, fakeargv, TRUE);
    
    ps_default_search_args(this->vr_cfg);
    this->vr_ps = ps_init(this->vr_cfg);
    if (this->vr_ps == NULL) {
        cmd_ln_free_r(this->vr_cfg);
        return 1;
    }
    
    
    cout << "[experiment] create::...end" << endl;
    
    return 0;
}


bool Experiment::execute()
{
    // init drive electronics
    this->ad->spi_open();
    this->ad->configure();
    
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    
    this->ad->execute_trajectory(alph->getneutral(), durationRefresh_ns);

    if (BrailleDev == this->expToExec)
    {
	return this->executeBD(&durationRefresh_ns);
    }
    else if (Fingers == this->expToExec)
    {
	return this->executeBD(&durationRefresh_ns);
	//return this->executeF(&durationRefresh_ns);
    }
    else if (Buzzer == this->expToExec)
    {
	return this->executeBuz(&durationRefresh_ns);
    }
    else
    {
	cout << "[experiment][execute] NO more experiment available: id(expToExec)=" << to_string(expToExec) << endl;
	bool err = true;
	return err;
    }
}




bool Experiment::executeF(int * durationRefresh_ns){
    
    return true;
}



bool Experiment::executeBD(int * durationRefresh_ns){
    char letter = 'a'; // change the signal that is used for actuator hapticomm
    
    return executeActuator(letter, durationRefresh_ns);
}

bool Experiment::executeBuz(int * durationRefresh_ns){
    char letter = 'b'; // change the signal that is used for buzzer
    
    return executeActuator(letter, durationRefresh_ns);
}



bool Experiment::executeActuator(char letter, int * durationRefresh_ns){
    cout << "[experiment][executeActuator] start..." << endl;
    // environmental variables
    int i;
    int overruns=0;
    td_highresclock c_start;
    /* input variables */
    waveformLetter values = alph->getl(letter); // get the symbols data with all the actuator in action
    /* output variables */
    array<td_msec, 3> vhrc;
    int answeri;
    
    /* voice recognition variables */
    ad_rec_t *ad;
    int16_t adbuf[2048];
    bool utt_started = false, in_speech;
    int32_t k;
    char const *hyp;
    const char *adcdev = cmd_ln_str_r(this->vr_cfg, "-adcdev");
    int samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate");
    if ((ad = ad_open_dev(adcdev, samprate)) == NULL) E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0) E_FATAL("Failed to start recording\n");
    if (ps_start_utt(this->vr_ps) < 0) E_FATAL("Failed to start utterance\n");
    
    cout << "------------------------------------------------" << endl;
    cout << "------------------------------------------------" << endl;
    cout << "Ready to start the experiment." << endl;
    cout << "wait until a character is entered........." << endl;
    cin.get();
    cout << "------------------------------------------------" << endl;
    cout << "------------------------------------------------" << endl;
    
    /* work */
    for(i=0; i<this->seq.size(); i++) // for the sequence i
    {
	// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
	c_start = chrono::high_resolution_clock::now();
	overruns += executeSequence(&i, values, durationRefresh_ns, &c_start, &vhrc); // copy of values is important (erase)
	cout << "It's time to answer:" << endl;
	answeri = recognize_from_microphone(vr_ps, ad, adbuf, hyp, &utt_started, &k, &vhrc);
	vhrc[2] = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-c_start);
	std::cout << "t1: " << vhrc[0].count() 
		    << "ms, t2: " << vhrc[1].count() 
		    << "ms, t3: " << vhrc[2].count() << "ms" << endl;
	
	// save the result
	vvtimer.push_back(vhrc);
	vanswer.push_back(answeri);
    }


    ad_close(ad);
    cout << "[experiment][executeActuator] ...end" << endl;
    return false;
}



int Experiment::executeSequence(int * i, waveformLetter values, int * dr_ns, td_highresclock * c_start, array<td_msec, 3> * vhrc){
    /* Transforms the values vector corresponding to the sequence */
    int j=0, ovr=0, timeleft=0; // ready to check each actuator availability of the sequence
    int randomttw = rand()%500;//2000 + 1000; // randomize the time to wait between 1000-3000ms
    cout << "randomttw= " << to_string(randomttw) << "ms" << endl;
    for(waveformLetter::iterator it=values.begin(); it!=values.end(); ++it) // foreach actuator
    {
	if (this->seq[*i][j]==0) { values.erase(it); } // erase the corresponding actuation of the actuator
	j++; // go to the next actuator of the sequence
    }
	
    /* Time to wait before executing the sequence */
    auto timespent = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-*c_start).count(); // now - (beginning of the loop for this sequence)
    timeleft = randomttw-timespent;
    if (timeleft>0)
	usleep(timeleft*1000);
    else
    	ovr -= timeleft;
    
    /* Execution of the sequence */
    (*vhrc)[0] = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-*c_start);
    //usleep(1*1000);
    ovr = this->ad->execute_selective_trajectory(values, *dr_ns); // execute the trajectories
    (*vhrc)[1] = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-*c_start);
    if (ovr) { cout << "Overruns: " << ovr << endl; }
	
    return ovr;
}



vector<std::array<td_msec, 3>> Experiment::getTimer()
{
    return this->vvtimer;
}

vector<int> Experiment::getAnswer()
{
    return this->vanswer;
}


int Experiment::recognize_from_microphone(
    ps_decoder_t * vr_ps, ad_rec_t * ad, int16_t * adbuf,// intput (1)
    char const *hyp, bool * utt_started, int32_t * k,    // intput (2)
    array<td_msec, 3> * vhrc) // output
{
    bool in_speech, listen = true;
    int answer;
    
    while (listen) {
        if ((*k = ad_read(ad, adbuf, 2048)) < 0)
            E_FATAL("Failed to read audio\n");
        ps_process_raw(vr_ps, adbuf, *k, FALSE, FALSE);
        in_speech = ps_get_in_speech(vr_ps);
        if (in_speech && !*utt_started) {
            *utt_started = true;
            E_INFO("Listening...\n");
        }
	
        if (!in_speech && *utt_started) {
            // speech -> silence transition, time to start new utterance
            ps_end_utt(vr_ps);
            hyp = ps_get_hyp(vr_ps, NULL );
            if (hyp != NULL) {
                printf("%s\n", hyp);
                fflush(stdout);
		if (!wordtonumb(hyp, &answer))
		{
		    listen = false;
		}
            }

            if (ps_start_utt(vr_ps) < 0)
                E_FATAL("Failed to start utterance\n");
            *utt_started = false;
        }
	
    }
    
    return answer;
}



bool Experiment::wordtonumb(const char * hyp, int * hypint){
    int ret= false;
    
    if (strcmp(hyp, "ONE")==0 || strcmp(hyp, "un")==0)
    {
	*hypint = 1;
    }
    else if (strcmp(hyp, "TWO")==0 || strcmp(hyp, "deux")==0)
    {
	*hypint = 2;
    }
    else if (strcmp(hyp, "THREE")==0 || strcmp(hyp, "trois")==0)
    {
	*hypint = 3;
    }
    else if (strcmp(hyp, "FOUR")==0 || strcmp(hyp, "quatre")==0)
    {
	*hypint = 4;
    }
    else if (strcmp(hyp, "FIVE")==0 || strcmp(hyp, "cinq")==0)
    {
	*hypint = 5;
    }
    else if (strcmp(hyp, "SIX")==0 || strcmp(hyp, "six")==0)
    {
	*hypint = 6;
    }
    else
    {
	*hypint = -1;
	ret = true;
    }
    
    return ret;
}

void Experiment::recognize_from_microphoneBckpup()
{
    ad_rec_t *ad;
    int16 adbuf[2048];
    uint8 utt_started, in_speech;
    int32 k;
    char const *hyp;
    
    static ps_decoder_t *ps = vr_ps;
    static cmd_ln_t *config = vr_cfg;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    if (ps_start_utt(ps) < 0)
        E_FATAL("Failed to start utterance\n");
    utt_started = FALSE;
    E_INFO("Ready....\n");

    for (;;) {
        if ((k = ad_read(ad, adbuf, 2048)) < 0)
            E_FATAL("Failed to read audio\n");
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
        if (in_speech && !utt_started) {
            utt_started = TRUE;
            E_INFO("Listening...\n");
        }
        if (!in_speech && utt_started) {
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL );
            if (hyp != NULL) {
                printf("%s\n", hyp);
                fflush(stdout);
            }

            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            utt_started = FALSE;
            E_INFO("Ready....\n");
        }
    }
    ad_close(ad);
}


