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
    if (BrailleDevTemp == this->expToExec || BuzzerTemp == this->expToExec)
    {
        initactid4temp();
    }
    
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
    /*
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
    */
    if (c->getLangage().compare("en-us") ==0)
    {
        int fakeargc = 4;
        char jsgf[8], jsgfval[128];
        strcpy(jsgf, "-jsgf");
        strcpy(jsgfval, fdd.c_str());
        strcat(jsgfval, ".gram");
        char eInfo[8], eInfoval[128];
        strcpy(eInfo, "-logfn");
        strcpy(eInfoval, "/dev/null");

        char *fakeargv[fakeargc];
        fakeargv[0] = jsgf;
        fakeargv[1] = jsgfval;
        fakeargv[2] = eInfo;
        fakeargv[3] = eInfoval; 
        
        /*
        for(int i=0; i<fakeargc; i++)
        {
            cout << "fakeargv[" << i << "] = '" << fakeargv[i] << "'" << endl;
        }
        */

        this->vr_cfg = cmd_ln_parse_r(NULL, cont_args_def, fakeargc, fakeargv, TRUE);
    }
    else // french
    {
        int fakeargc = 10;
        char lm[8], lmval[128];
        strcpy(lm, "-lm");
        strcpy(lmval, fdd.c_str());
        strcat(lmval, ".lm.bin");
        char dict[8], dictval[128];
        strcpy(dict, "-dict");
        strcpy(dictval, fdd.c_str());
        strcat(dictval, ".dic");
        char hmm[8], hmmval[128];
        strcpy(hmm, "-hmm");
        strcpy(hmmval, fdd.c_str());
        strcat(hmmval, "/");
        strcpy(hmmval, fdd.c_str());
        char jsgf[8], jsgfval[128];
        strcpy(jsgf, "-jsgf");
        strcpy(jsgfval, fdd.c_str());
        strcat(jsgfval, ".gram");
        char eInfo[8], eInfoval[128];
        strcpy(eInfo, "-logfn");
        strcpy(eInfoval, "/dev/null");

        char *fakeargv[fakeargc];
        fakeargv[0] = lm;
        fakeargv[1] = lmval;
        fakeargv[2] = dict;
        fakeargv[3] = dictval;
        fakeargv[4] = hmm;
        fakeargv[5] = hmmval;
        fakeargv[6] = jsgf;
        fakeargv[7] = jsgfval;
        fakeargv[8] = eInfo;
        fakeargv[9] = eInfoval;
        
        /*
        for(int i=0; i<fakeargc; i++)
        {
            cout << "fakeargv[" << i << "] = '" << fakeargv[i] << "'" << endl;
        }
        */

        this->vr_cfg = cmd_ln_parse_r(NULL, cont_args_def, fakeargc, fakeargv, TRUE);
    }
    
    
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

    if (BrailleDevSpace == this->expToExec || BrailleDevTemp == this->expToExec ||
        BuzzerSpace == this->expToExec     || BuzzerTemp == this->expToExec)
    {
        return this->executeActuator(&durationRefresh_ns);
    }
    else if (FingersSpace == this->expToExec || FingersTemp == this->expToExec)
    {
        return this->executeF(&durationRefresh_ns);
    }
    else
    {
        cout << "[experiment][execute] NO more experiment available: id(expToExec)=" << to_string(expToExec) << endl;
        bool err = true;
        return err;
    }
}




bool Experiment::executeF(int * durationRefresh_ns){
    
    return false;
}



bool Experiment::executeActuator(int * durationRefresh_ns){
    cout << "[experiment][executeActuator] start..." << endl;
    // environmental variables
    int i;
    int overruns=0;
    /* input variables */
    char letter;
    if (BrailleDevSpace == this->expToExec || BrailleDevTemp == this->expToExec)
    {
        letter = 'a'; // change the signal that is used for actuator hapticomm
    }
    else
    {
        letter = 'b';
    }
    waveformLetter values = alph->getl(letter);
    
    /* output variables */
    //array<td_msec, 3> vhrc;
    //int answeri;

    td_msecarray vhrc;
    td_msecarray timerDebug;
    int answeri;
    
    cout << "+----------------------------------------------+" << endl;
    cout << "+...                                        ...+" << endl;
    cout << "     Experiment: \t" << c->expstring(this->expToExec) << endl;
    cout << "+    Press [ENTER] to start the experiment     +" << endl;
    cout << "+...                                        ...+" << endl;
    cout << "+----------------------------------------------+" << endl;
    cin.get();
    
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
    

    //cout << "------------------------------------------------" << endl;
    //cout << "------------------------------------------------" << endl;
    //cout << "Ready to start the experiment." << endl;
    //cout << "wait until a character is entered........." << endl;
    //cin.get();
    //cout << "------------------------------------------------" << endl;
    //cout << "------------------------------------------------" << endl;
    
    /* work */
    for(i=this->seq_start; i<this->seq.size(); i++) // for the sequence i
    {
	// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
	//c_start = chrono::high_resolution_clock::now();
	//overruns += executeSequence(&i, values, durationRefresh_ns, &c_start, &vhrc); // copy of values is important (erase)
	//cout << "It's time to answer:" << endl;
	//answeri = recognize_from_microphone(vr_ps, ad, adbuf, hyp, &utt_started, &k, &vhrc);
	//vhrc[2] = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-c_start);
	//std::cout << "t1: " << vhrc[0].count() 
	//	    << "ms, t2: " << vhrc[1].count() 
	//	    << "ms, t3: " << vhrc[2].count() << "ms" << endl;
	
	// save the result
	//vvtimer.push_back(vhrc);
	//vanswer.push_back(answeri);
      //}
      //ad_close(ad);
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        overruns += executeSequence(&i, values, durationRefresh_ns, &vhrc); // copy of variable[values]  is important (erase)
        answeri = recognize_from_microphone(ad, &vhrc, &timerDebug);
        
        pasteTimers(i, answeri, vhrc, timerDebug);
        if (rectifyAnswer(&answeri)) // if exit the experiment...
        {
            cout << "[EXIT] The experiment will be saved to i=" << i-1 << "/" << this->seq.size() << endl;
            break; // go to save
        }
        pasteTimers(i, answeri, vhrc, timerDebug);
        
        // save the result
        vvtimer.push_back(vhrc);
        vanswer.push_back(answeri);
    }
    
    this->seq_end = i;
    
    
    if (ad_close(ad) < 0) E_FATAL("Failed to close the device\n");
    
    cout << "[experiment][executeActuator] ...end" << endl;
    return false;
}



<<<<<<< Updated upstream
int Experiment::executeSequence(int * i, waveformLetter values, int * dr_ns, td_highresclock * c_start, array<td_msec, 3> * vhrc){
    /* Transforms the values vector corresponding to the sequence */
=======
// values has to be a non-pointer because modified inside the function (erase)
int Experiment::executeSequence(int * currSeq, waveformLetter values_copy, int * dr_ns, td_msecarray * vhrc){
    int ovr=0;
    if (BrailleDevSpace == this->expToExec || BuzzerSpace == this->expToExec)
    {
        ovr = executeSequenceSpace(currSeq, &values_copy, dr_ns, vhrc);
    }
    else
    {
        ovr = executeSequenceTemp(currSeq, &values_copy, dr_ns, vhrc);
    }
    
    if (ovr) { cout << "Overruns: " << ovr << endl; }
	
    return ovr;
}


int Experiment::executeSequenceSpace(int * currSeq, waveformLetter *values, int * dr_ns, td_msecarray * vhrc)
{
>>>>>>> Stashed changes
    int j=0, ovr=0, timeleft=0; // ready to check each actuator availability of the sequence
    int nbAct = values->size();
    waveformLetter::iterator it;
    int randomttw = 250 + rand()%2750; // randomize the time to wait between 1000-3000ms
    cout << "randomttw= " << to_string(randomttw) << "ms" << endl;
    
    std::vector<uint8_t> rel_id_chan = {10, 9, 11, 14, 18, 19};
    
    /* Transforms the values vector corresponding to the sequence */
    for (int a=0; a!=nbAct; a++)
    {
        if (this->seq[*currSeq][a]==0) 
        {
            it = values->find(rel_id_chan[a]);
            values->erase(it);
        }
    }
	
    /* Time to wait before executing the sequence */
    auto timespent = nowSeq().count(); // now - (beginning of the loop for this sequence)
    timeleft = randomttw-timespent;
    if (timeleft>0)
        usleep(timeleft*1000);
    else
    	ovr -= timeleft;
    
    /* Execution of the sequence */
    (*vhrc)[0] = nowSeq();
    ovr = this->ad->execute_selective_trajectory(*values, *dr_ns); // execute the trajectories
    this->ad->execute_trajectory(this->alph->getneutral(), *dr_ns);
    (*vhrc)[1] = nowSeq();
    
    return ovr;
}


<<<<<<< Updated upstream

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
=======
int Experiment::executeSequenceTemp(int * currSeq, waveformLetter *values, int * dr_ns, td_msecarray * vhrc)
{
    int j=0, ovr=0, timeleft=0; // ready to check each actuator availability of the sequence
    int nbAct = values->size();
    waveformLetter::iterator it;
    std::vector<uint8_t> rel_id_chan = {10, 9, 11, 14, 18, 19};
    int nbiteration = std::accumulate(this->seq[*currSeq].begin(),this->seq[*currSeq].end(),0);
    int randomttw = 250 + rand()%2750; // randomize the time to wait between 1000-3000ms
    
    cout << "[executeSequenceTemp] randomttw= " << to_string(randomttw) << "ms" << endl;
    /* Transforms the values vector corresponding to the sequence */
    for (int a=0; a!=nbAct; a++)
    {
        if (a != this->actchannelID[*currSeq]) 
        {
            it = values->find(rel_id_chan[a]);
            values->erase(it);
        }
    }
    
    cout << "[executeSequenceTemp] number of iteration='" << nbiteration << "'" << endl;
    cout << "[executeSequenceTemp] size of values='" << values->size() << "'" << endl;
    /* Time to wait before executing the sequence */
    auto timespent = nowSeq().count(); // now - (beginning of the loop for this sequence)
    timeleft = randomttw-timespent;
    if (timeleft>0)
        usleep(timeleft*1000);
    else
    	ovr -= timeleft;
    
    /* Execution of the sequence */
    (*vhrc)[0] = nowSeq();
    for (j=0;j!=nbiteration; j++)
    {
        ovr = this->ad->execute_selective_trajectory(*values, *dr_ns); // execute the trajectories
        //cout << "[executeSequenceTemp] do it! '" << j << "'x times" << endl;
        this->ad->execute_trajectory(this->alph->getneutral(), *dr_ns);
        //usleep(5*1000);
    }
    (*vhrc)[1] = nowSeq();
    
    return ovr;
}

int Experiment::recognize_from_microphone(ad_rec_t *ad, td_msecarray * vhrc, td_msecarray * timerDebug)
{
    bool job_done, utt_started, in_speech, noHypothesis;
    int answer;
    int32_t k;
    char const *hyp;
    int16_t adbuf[2048];
    
    //empty the recorded sounds from mic
    ad_read(ad, adbuf, 20480);
    //while (k>0) { k = ad_read(ad, adbuf, INT_MAX); }
    
    if (ad_start_rec(ad) < 0) E_FATAL("Failed to start recording\n");
    cout << "Answer?" << endl;
    (*timerDebug)[0] = nowSeq();
    
    job_done = false;
    // if it was a positive false 
    while (!job_done)
    {
        // Detection of the voice
        in_speech = false;
        while (!in_speech) 
        {
            (*vhrc)[2] = nowSeq();
            if ((k = ad_read(ad, adbuf, 128)) < 0)
                E_FATAL("Failed to read audio\n");
            ps_process_raw(vr_ps, adbuf, k, FALSE, FALSE);
            in_speech = ps_get_in_speech(vr_ps);
            (*timerDebug)[1] = nowSeq();
        }
        
        // Recognition of the answer, while there is no hypothesis
        utt_started = false;
        noHypothesis = true;
        while (noHypothesis) 
        {
            if ((k = ad_read(ad, adbuf, 2048)) < 0)
                E_FATAL("Failed to read audio\n");
            ps_process_raw(vr_ps, adbuf, k, FALSE, FALSE);
            in_speech = ps_get_in_speech(vr_ps);
            
            if (in_speech && !utt_started) 
            { 
                utt_started = true;
                E_INFO("Listening...\n");
            }
        
            if (!in_speech && utt_started) 
            {
                // speech -> silence transition, time to start new utterance
                ps_end_utt(vr_ps);
                hyp = ps_get_hyp(vr_ps, NULL );
                if (hyp != NULL) 
                {
                    noHypothesis = false;
                    printf("%s\n", hyp);
                    fflush(stdout);
                    if (!wordtonumb(hyp, &answer)) // if is a number
                    {
                        job_done = true;
                    }
                    else
                    {
                        cout << "Was it an answer?" << endl;
                        string answerchange;
                        cin >> answerchange;
                        if (isdigit(answerchange[0]))
                        {
                            cout << "'" << answerchange << "' is a digit -> '" << atoi(answerchange.c_str()) << "'" << endl;
                            answer = atoi(answerchange.c_str());
                            job_done = true;
                        }
                    }
                }

                if (ps_start_utt(vr_ps) < 0)
                    E_FATAL("Failed to start utterance\n");
                utt_started = false;
            }
>>>>>>> Stashed changes
        }
    }
<<<<<<< Updated upstream
=======
    
    
    k = ad_read(ad, adbuf, INT_MAX);
    if (ad_stop_rec(ad) < 0) E_FATAL("Failed to stop recording\n");
    //empty the recorded sounds from mic
    k = ad_read(ad, adbuf, INT_MAX);
    //while (k>0) { k = ad_read(ad, adbuf, INT_MAX); }
    (*timerDebug)[2] = nowSeq();
>>>>>>> Stashed changes
    
    return answer;
}


td_msec Experiment::nowSeq()
{
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now()-this->c_start);
}


bool Experiment::rectifyAnswer(int * answeri)
{
    bool ret=false;
	string answerchange;
    if (1)
    {
        cout << "Rectify the answer?" << endl;
        cin >> answerchange;
        cout << "answerchange='" << answerchange << "'" << endl;
        if (isdigit(answerchange[0]))
        {
            cout << "'" << answerchange << "' is a digit -> '" << atoi(answerchange.c_str()) << "'" << endl;
            *answeri = atoi(answerchange.c_str());
        }
        else if (0 == answerchange.compare("s") ||
                0 == answerchange.compare("\x18")||
                0 == answerchange.compare("\x18s")) // if has to stop/pause the experiment
        {
            ret = true;
        }
    }
    
    return ret;
}

void Experiment::pasteTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug)
{
    std::cout << "[" << numSeq << "][";
    for (int i=0; i!=seq[numSeq].size(); i++){
        std::cout << seq[numSeq][i];
    }
    std::cout << "] "
            << "V=" << answeri << " | "
            << "start[stim]: " << (int)vhrc[0].count() << " | "
            << "end[stim]: " << (int)vhrc[1].count() << " | "
            << "(tmp:" << (int)timerDebug[0].count() << ")| "
            << "detection: " << (int)vhrc[2].count() << " | "
            << "aft_detection: " << (int)timerDebug[1].count() << " | "
            << "aft_recognition: " << (int)timerDebug[2].count() << "(ms)" << endl;
}

bool Experiment::wordtonumb(const char * hyp, int * hypint)
{
    int ret= false;
    
<<<<<<< Updated upstream
    if (strcmp(hyp, "ONE")==0 || strcmp(hyp, "un")==0)
=======
    if (strncmp(hyp, "one", 3)==0 || strncmp(hyp, "ONE", 3)==0 || strncmp(hyp, "un", 2)==0)
>>>>>>> Stashed changes
    {
        *hypint = 1;
    }
<<<<<<< Updated upstream
    else if (strcmp(hyp, "TWO")==0 || strcmp(hyp, "deux")==0)
=======
    else if (strncmp(hyp, "two", 3)==0 || strncmp(hyp, "TWO", 3)==0 || strncmp(hyp, "deux", 4)==0)
>>>>>>> Stashed changes
    {
        *hypint = 2;
    }
<<<<<<< Updated upstream
    else if (strcmp(hyp, "THREE")==0 || strcmp(hyp, "trois")==0)
=======
    else if (strncmp(hyp, "three", 5)==0 || strncmp(hyp, "THREE", 5)==0 || strncmp(hyp, "trois", 5)==0)
>>>>>>> Stashed changes
    {
        *hypint = 3;
    }
<<<<<<< Updated upstream
    else if (strcmp(hyp, "FOUR")==0 || strcmp(hyp, "quatre")==0)
=======
    else if (strncmp(hyp, "four", 4)==0 || strncmp(hyp, "FOUR", 4)==0 || strncmp(hyp, "quatre", 6)==0)
>>>>>>> Stashed changes
    {
        *hypint = 4;
    }
<<<<<<< Updated upstream
    else if (strcmp(hyp, "FIVE")==0 || strcmp(hyp, "cinq")==0)
=======
    else if (strncmp(hyp, "five", 4)==0 || strncmp(hyp, "FIVE", 4)==0 || strncmp(hyp, "cinq", 4)==0)
>>>>>>> Stashed changes
    {
        *hypint = 5;
    }
<<<<<<< Updated upstream
    else if (strcmp(hyp, "SIX")==0 || strcmp(hyp, "six")==0)
=======
    else if (strncmp(hyp, "six", 3)==0 || strncmp(hyp, "SIX", 3)==0 || strncmp(hyp, "six", 3)==0)
>>>>>>> Stashed changes
    {
        *hypint = 6;
    }
    else
    {
<<<<<<< Updated upstream
	*hypint = -1;
	ret = true;
=======
        *hypint = -1;
        ret = true;
>>>>>>> Stashed changes
    }
    
    return ret;
}

<<<<<<< Updated upstream
=======



/*******************************/
/**                           **/
/**         GETTERS           **/
/**                           **/
/*******************************/
vector<td_msecarray> Experiment::getTimer(){ return this->vvtimer; }
vector<int>         Experiment::getAnswer(){ return this->vanswer; }
int              Experiment::getSeq_start(){ return this->seq_start; }
int                Experiment::getSeq_end(){ return this->seq_end; }










>>>>>>> Stashed changes
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




void Experiment::initactid4temp(){
    // pour chaque effecteur
    int actmin = (BrailleDevTemp == this->expToExec)?0:1;
    int actmax = 6;
    
    cout << "[initactid4temp] actmin=" << actmin << endl;
    //for each sequence
    for(int i=0; i<seq.size(); i++)
    {
        actchannelID.push_back(rand()%actmax+actmin);
    }
    
    for(int i=0; i<actchannelID.size(); i++)
    {
        cout << "[initactid4temp] actchannelID[" << i << "]=" << actchannelID[i] << endl;
    }
}



