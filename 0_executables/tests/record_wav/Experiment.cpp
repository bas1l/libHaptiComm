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
    this->seq = this->c->getSequence();				// get the sequence for the experiment
    this->expToExec = this->c->nextExp();			// define current experiment
    if (BrailleDevTemp == this->expToExec || BuzzerTemp == this->expToExec)
    {
        initactid4temp();							// redefine actuator array if it's a temporal experiment
    }
    
    // output variables
    this->vvtimer.reserve(this->seq.size());		// vector of timers (beginning and end of the sequence, and RT)
    this->vanswer.reserve(this->seq.size());		// vector of answers (between 1 to 6)
    
    
    /* SETUP ENVIRONEMENT: printw and timer */
    srand (time(NULL));								// initialize random seed
    struct timespec t;								// get timers for actuators control
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

    cout << "cfg->configure>>" << endl;
    this->cfg->configure(cfgSource, dev, wf, alph);	// hapticomm configuration file
    
    // preparing the command line arguments 'argv' variable for CMU Sphinx VR
    cout << "fakeargs>>" << endl;
    string fdd(c->getPathDict()+c->getLangage()+"/"+c->getLangage()); // folder of the CMU Sphinx database
    if (c->getLangage().compare("en-us") ==0) // english VR
    {
        char jsgfval[128], eInfoval[128], adcdevval[128];
        strcpy(jsgfval, fdd.c_str());
        strcat(jsgfval, ".gram");
        strcpy(eInfoval, 	"/dev/null");
        strcpy(adcdevval, 	"plughw:1,0");
        
    	this->vr_cfg = cmd_ln_init(NULL, ps_args(), TRUE,   	// Load the configuration structure - ps_args() passes the default values
					"-logfn", 	eInfoval,                   	// suppress log info from being sent to screen
					"-jsgf", 	jsgfval,
					//"-adcdev", 	adcdevval,
					 NULL);
    } 
    else // french VR
    {
        char lmval[128], dictval[128], hmmval[128], jsgfval[128], eInfoval[128], adcdevval[128];
        strcpy(lmval, 	fdd.c_str());
        strcat(lmval, 	".lm.bin");
        strcpy(dictval, fdd.c_str());
        strcat(dictval, ".dic");
        strcpy(hmmval, 	fdd.c_str());
        strcat(hmmval, 	"/");
        strcpy(hmmval, 	fdd.c_str());
        strcpy(jsgfval, fdd.c_str());
        strcat(jsgfval, ".gram");
        strcpy(eInfoval, 	"/dev/null");
        strcpy(adcdevval, 	"plughw:1,0");
        
    	this->vr_cfg = cmd_ln_init(NULL, ps_args(), TRUE,   	// Load the configuration structure - ps_args() passes the default values
					"-hmm", 	lmval,  						// path to the standard english language model
					"-lm", 		lmval,                  		// custom language model (file must be present)
					"-dict", 	dictval,                   		// custom dictionary (file must be present)
					"-logfn", 	eInfoval,                   	// suppress log info from being sent to screen
					"-jsgf", 	jsgfval,
					//"-adcdev", 	adcdevval,
					 NULL);
    }
    
    ps_default_search_args(this->vr_cfg);			// fill non-defined variables with default values
    /*
    this->vr_ps = ps_init(this->vr_cfg);			// initialise voice recognition variables
    if (this->vr_ps == NULL) {						// if failed, exit
        cmd_ln_free_r(this->vr_cfg);				// free memory
        return 1;									// exit
    }
    */
    
    // thread and shared memory's variables initialisation for voice recording
    cout << "thread>>" << endl;
    this->workdone = false; 													// check if the experiment is done
    this->is_recording = false; 												// allow t_record to start or stop the recording
    // This will start the thread. Notice move semantics!
	this->t_record = std::thread(&Experiment::record_from_microphone, this); 	// link the thread to its function

	// init wav writer (AudioFile library)
    cout << "AudioFile" << endl;
    this->af = new AudioFile<double>();
    int sampleRate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate");
    this->af_i = 0;										// iterator for audioFile
    this->af_max = sampleRate * 10;						// size of audioFile buffer
    this->af->setAudioBufferSize(1, this->af_max);		// init audioFile buffer
    this->af->setSampleRate(sampleRate);				// set sample rate
    this->af->setBitDepth(16);							// set bit depth
    
    cout << "[experiment] create::...end" << endl;
    return 0;
}


bool Experiment::execute()
{
    // init drive electronics
    this->ad->spi_open();
    this->ad->configure();
    // get SPI refresh time (millisecond)
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    
    // init all the actuators to zero
    this->ad->execute_trajectory(alph->getneutral(), durationRefresh_ns);
    
    // decide which experiment has to be executed
    if (BrailleDevSpace == this->expToExec || BrailleDevTemp == this->expToExec)
    {
        return this->executeActuator(&durationRefresh_ns);
    }
    else if (BuzzerSpace == this->expToExec || BuzzerTemp == this->expToExec)
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
    
    /* work */
    for(i=this->seq_start; i<this->seq.size(); i++) // for the sequence i
    {
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        start_recording(); // change is_recording value to true
        overruns += executeSequence(&i, values, durationRefresh_ns, &vhrc); // copy of variable[values]  is important (erase)
        
        dispTimers(i, answeri, vhrc, timerDebug);
        bool rect = rectifyAnswer(&answeri);
        stop_recording(); // change is_recording value to false
        save_recording(i);
        if (rect) // if -1 has been writen, exit
        {
            cout << "[EXIT] The experiment will be saved to i=" << i-1 << "/" << this->seq.size() << endl;
            break; // go to save
        }
        dispTimers(i, answeri, vhrc, timerDebug);
        
        // save the result
        vvtimer.push_back(vhrc);
        vanswer.push_back(answeri);
    }
    
    this->seq_end = i;
    
    
    
    cout << "[experiment][executeActuator] ...end" << endl;
    return false;
}




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
                    if (!str2num(hyp, &answer)) // if is a number
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
        }
    }
    
    k = ad_read(ad, adbuf, INT_MAX);
    if (ad_stop_rec(ad) < 0) E_FATAL("Failed to stop recording\n");
    //empty the recorded sounds from mic
    k = ad_read(ad, adbuf, INT_MAX);
    //while (k>0) { k = ad_read(ad, adbuf, INT_MAX); }
    (*timerDebug)[2] = nowSeq();
    
    return answer;
}


// record function for the thread
void Experiment::record_from_microphone()
{
    /* voice recognition variables */
    ad_rec_t *adrec;													// audio device
    //const char *adcdev = cmd_ln_str_r(this->vr_cfg, "-adcdev"); 		// audio device informations for 'ad'
    int samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 	// sampling rate for 'ad'
    if ((adrec = ad_open_dev("plughw:1,0", samprate)) == NULL) 				// open the audio device (microphone)
    	E_FATAL("Failed to open audio device\n"); 
	int16_t adbuf[2048];												// audio buffer of the audio device 
    int32_t k;															// returned value of reading microphone
	std::unique_lock<std::mutex> lk(this->m);							// locker to access shared variables
	
	cout << "while1" << endl;
	while(!this->workdone.load()){ 										// while experiment is not done
		this->cv.wait(lk); 												// wait for a new sequence (WF notify_one in the current 'execute' function)
		if (!this->is_recording.load()){								// not a new sequence, go back to wait
			lk.unlock();												// unlock mutex before waiting
			continue;													// go to wait
		}
		cout << "after continue." << endl;
		if (ad_start_rec(adrec) < 0) 									// start recording
			E_FATAL("Failed to start recording\n");
		cout << "after ad_start_rec." << endl;
		do { k = ad_read(adrec, adbuf, INT_MAX); } while (k>0); 		// empty the microphone buffer from mic before recording -- previously[k = ad_read(ad, adbuf, INT_MAX);]
		cout << "after ad_read. emptier" << endl;
		while(this->is_recording.load()){ 								// while the answer is not given
	        cout << "start while2" << endl;
	        lk.unlock();												// return the access for is_recording
	        this->cv.notify_one(); 										// waiting thread is notified 
	        if ((k = ad_read(adrec, adbuf, 10)) < 0)					// record...
	        	E_FATAL("Failed to read audio\n");
	        for(int i=0; i<10; i++,this->af_i++)						// store the records into audioFile object
	        {
	        	this->af->samples[0][this->af_i] = adbuf[i];
	        }
	        lk.lock();													// locker for is_recording access
		}
		cout << "after while2." << endl;
		lk.unlock();													// return the access for is_recording
	    if (ad_stop_rec(adrec) < 0) 									// answer has been given, stop recording
	    	E_FATAL("Failed to stop recording\n");
	}
	
	// experiment is done, close microphone
    if (ad_close(adrec) < 0) E_FATAL("Failed to close the device\n");
}




void Experiment::start_recording()
{	
	cout << "[start_recording] in." << endl;
	{
		std::lock_guard<std::mutex> lk(this->m);			// locker to access shared variables
		this->is_recording = true;							// start of the microphone recording boolean
	}
	this->cv.notify_one(); 									// waiting thread is notified 
	cout << "[start_recording] out." << endl;
}

void Experiment::stop_recording()
{
	cout << "[stop_recording] in." << endl;
	{
		std::lock_guard<std::mutex> lk(this->m);			// locker to access shared variables
		this->is_recording = false;							// stop of the microphone recording boolean
	}
	this->cv.notify_one(); 									// waiting thread is notified
	cout << "[stop_recording] out." << endl;
}

void Experiment::save_recording(int id_seq)
{
	cout << "[save_recording] in." << endl;
	string name(c->getPathDirectory());  					// create the name of the .wav with full path
	name += c->getId() + "_";								// id of the candidate
	name += c->expstring(this->expToExec) + "_";			// current experiment's name
	name += std::to_string(id_seq) + ".wav";				// id of the sequence
	{
		std::lock_guard<std::mutex> lk(this->m);			// locker to access shared variables
		af->save(name);										// save wav file of the sequence's answer
		this->af_i = 0;										// reset iterator
		cout << "[save_recording] before for." << endl;
		for(int i=0; i<this->af_max; i++){					// clean the entire buffer
			this->af->samples[0][i] = 0;					// clean 
		}
	}
	this->cv.notify_one(); 									// waiting thread is notified
	cout << "[save_recording] out." << endl;
}

/********************************/
/*           Tools 				*/
/********************************/
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

void Experiment::dispTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug)
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

bool Experiment::str2num(const char * hyp, int * hypint)
{
    int ret= false;
    

    if (strncmp(hyp, "one", 3)==0 || strncmp(hyp, "ONE", 3)==0 || strncmp(hyp, "un", 2)==0)
    {
        *hypint = 1;
    }
    else if (strncmp(hyp, "two", 3)==0 || strncmp(hyp, "TWO", 3)==0 || strncmp(hyp, "deux", 4)==0)
    {
        *hypint = 2;
    }
    else if (strncmp(hyp, "three", 5)==0 || strncmp(hyp, "THREE", 5)==0 || strncmp(hyp, "trois", 5)==0)
    {
        *hypint = 3;
    }
    else if (strncmp(hyp, "four", 4)==0 || strncmp(hyp, "FOUR", 4)==0 || strncmp(hyp, "quatre", 6)==0)

    {
        *hypint = 4;
    }
    else if (strncmp(hyp, "five", 4)==0 || strncmp(hyp, "FIVE", 4)==0 || strncmp(hyp, "cinq", 4)==0)

    {
        *hypint = 5;
    }
    else if (strncmp(hyp, "six", 3)==0 || strncmp(hyp, "SIX", 3)==0 || strncmp(hyp, "six", 3)==0)

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




/*******************************/
/**                           **/
/**         GETTERS           **/
/**                           **/
/*******************************/
vector<td_msecarray> Experiment::getTimer(){ return this->vvtimer; }
vector<int>         Experiment::getAnswer(){ return this->vanswer; }
int              Experiment::getSeq_start(){ return this->seq_start; }
int                Experiment::getSeq_end(){ return this->seq_end; }











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



