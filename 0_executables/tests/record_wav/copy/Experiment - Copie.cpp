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
    
    
	// init wav writer (AudioFile library)
    cout << "AudioFile" << endl;
    this->af = new AudioFile<double>();
    int sampleRate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate");
    this->af_i = 0;										// iterator for audioFile
    this->af_max = sampleRate * 10;						// size of audioFile buffer
    this->af->setAudioBufferSize(1, this->af_max);		// init audioFile buffer
    this->af->setSampleRate(sampleRate);				// set sample rate
    this->af->setBitDepth(16);							// set bit depth
    
    // thread and shared memory's variables initialisation for voice recording
	this->workdone = false; 													// check if the experiment is done
	this->is_recording = false; 												// allow t_record to start or stop the recording
	
	cout << "[experiment] create::...end" << endl;
    return 0;
}

bool Experiment::launchExp()
{
	// This will start the thread. Notice move semantics!
	std::cout<<"thread>>"<<std::endl;
	this->t_tap 	= std::thread(&Experiment::execute, this); 					// link the thread to its function
	this->t_record 	= std::thread(&Experiment::record_from_microphone, this); 	// link the thread to its function
	
	return false;
}

bool Experiment::execute()
{
    // get SPI refresh time (millisecond)
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    cout << std::to_string(durationRefresh_ns) << "dddddddddddddddd"<<endl;
    
    // init drive electronics
    this->ad->spi_open();
    this->ad->configure();
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
	std::system("clear");
	for(int i=0; i<3; i++) // for the sequence i
	{
		std::cout<<"[main] New sequence:"<<std::endl;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout<<"[main] sleept for:1000ms"<<std::endl;
		
		start_recording(); // change is_recording value to true
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout<<"[main] slept for:1000ms again"<<std::endl;
		
		stop_recording(); // change is_recording value to false
	}
	stop_experiment();
	
	
    return false;
}



// record function for the thread
void Experiment::record_from_microphone()
{
	while(true)
	{
		std::cout<<"Do Some Handshaking"<<std::endl;
		// Acquire the lock
		if(!signal4recording()) 
		{
			break;
		}

		std::cout<<"is recording...start"<<std::endl;
		while(isrecording())
		{
		}
		std::cout<<"is recording...done"<<std::endl;
		std::lock_guard<std::mutex> mlock(m_mutex);
		// Start waiting for the Condition Variable to get signaled
		// Wait() will internally release the lock and make the thread to block
		// As soon as condition variable get signaled, resume the thread and
		// again acquire the lock. Then check if condition is met or not
		// If condition is met then continue else again go in wait.
		std::cout<<"Do Processing On loaded Data"<<std::endl;
	}
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
    return 1;
}




void Experiment::start_recording()
{	
	cout << "[main][start_recording] in." << endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	this->is_recording = true;								// start of the microphone recording boolean
	this->cv.notify_one(); 									// waiting thread is notified 
	cout << "[main][start_recording] out. ---> "<<std::to_string(this->is_recording)<<std::endl;
}

void Experiment::stop_recording()
{
	cout << "[main][stop_recording] in." << endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	this->is_recording = false;								// stop of the microphone recording boolean
	this->cv.notify_one(); 									// waiting thread is notified 
	cout << "[main][stop_recording] out." << endl;
}

void Experiment::stop_experiment()
{	
	std::cout<<"[stop_experiment] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	workdone = true;										// start of the microphone recording boolean
	this->cv.notify_one(); 								// waiting thread is notified 
	std::cout<<"[stop_experiment] out."<<std::endl;
}
	
void Experiment::save_audio(int id_seq)
{
	std::cout<<"[main][save_audio] in...["<<std::flush;
	string path(c->getPathDirectory());  					// create the name of the .wav with full path
	path += std::to_string(c->getId()) + "/wav/";			// id of the candidate
	
	string name(std::to_string(c->getId()) + "_");  		// create the name of the .wav with full path
	name += c->expstring(this->expToExec) + "_";			// current experiment's name
	name += std::to_string(id_seq) + ".wav";				// id of the sequence
	
	std::cout<<path+name<<"]..."<<std::flush;
	std::lock_guard<std::mutex> lk(this->m_mutex);			// locker to access shared variables
	af->save(path+name);									// save wav file of the sequence's answer
	this->af_i = 0;											// reset iterator
	for(int i=0; i<this->af_max; i++){						// clean the entire buffer
		this->af->samples[0][i] = 0;						// clean 
	}
	std::cout<<"out."<<std::endl;
}





/********************************/
/*           Tools 				*/
/********************************/
bool Experiment::signal4recording()
{
	std::cout<<"[thread] signal4recording in..."<<std::endl;
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	std::cout<<"[thread] signal4recording mid..."<<std::endl;
	// Start waiting for the Condition Variable to get signaled
	// Wait() will internally release the lock and make the thread to block
	// As soon as condition variable get signaled, resume the thread and
	// again acquire the lock. Then check if condition is met or not
	// If condition is met then continue else again go in wait.
	this->cv.wait(mlock, std::bind(&Experiment::isrecordingorworkdone, this));

	std::cout<<"[thread] signal4recording out."<<std::endl;
	if (isworkdone()) {return false;}
	else return true;
}
bool Experiment::isrecording()
{
	return this->is_recording;
}

bool Experiment::isworkdone()
{
	return this->workdone;
}

bool Experiment::isrecordingorworkdone()
{
	return (isworkdone() || isrecording());
}

td_msec Experiment::nowSeq()
{
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now()-this->c_start);
}


bool Experiment::rectifyAnswer(int * answeri)
{
    bool ret=false;
	string ans;
    if (1)
    {
    	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
        cout << "Rectify the answer? [type 's' to save and quit]" << endl;
        cin >> ans;
        if (isdigit(ans[0]))
        {
            cout << "'" << ans << "' is a digit -> '" << atoi(ans.c_str()) << "'" << endl;
            *answeri = atoi(ans.c_str());
        }
        else if (0 == ans.compare("s") || 0 == ans.compare("\x18")|| 0 == ans.compare("\x18s")) // if has to stop/pause the experiment
        {
            ret = true;
        }
    }
    cout << "The new answer is='" << ans << "'" << endl;
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
    /*
    for(int i=0; i<actchannelID.size(); i++)
    {
        cout << "[initactid4temp] actchannelID[" << i << "]=" << actchannelID[i] << endl;
    }
    */
}


bool Experiment::executeActuator2(int * durationRefresh_ns){
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
    	cout<<endl <<"[main] New sequence:"<<endl;
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        start_recording(); // change is_recording value to true
        //overruns += executeSequence(&i, values, durationRefresh_ns, &vhrc); // copy of variable[values]  is important (erase)
        
        dispTimers(i, answeri, vhrc, timerDebug);
        bool rect = rectifyAnswer(&answeri);
        stop_recording(); // change is_recording value to false
        
        if (rect) // if 's' answer, exit
        {
            cout << "[EXIT] The experiment will be saved to i=" << i-1 << "/" << this->seq.size() << endl;
            break; // go to save
        }
        dispTimers(i, answeri, vhrc, timerDebug);
        
        // save the result
        save_audio(i);
        vvtimer.push_back(vhrc);
        vanswer.push_back(answeri);
    }
    this->seq_end = i;
    
    this->workdone = true;
    this->cv.notify_all(); 						// waiting threads are notified 
    	
    cout << "[main][experiment][executeActuator] ...end" << endl;
    return false;
}

// record function for the thread
void Experiment::record_from_microphone2()
{
    ad_rec_t *adrec;
	int16_t * adBuf, * fullBuf;
	int32_t k;	
	int samprate, sizeBuff, sizefullBuff, sizeRead;
	bool disp, dispLoop, is_recording_local;
	
    samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 		// sampling rate for 'ad'
    sizeBuff 		= 2048;
    sizeRead 		= 128;
    sizefullBuff 	= 10*samprate;
	adBuf 	= new int16_t[sizeBuff];									// audio buffer of the audio device 
	fullBuf = new int16_t[sizefullBuff];								// full audio buffer of the audio device for 10 seconds 

	disp 				= true;
	dispLoop 			= false;
	is_recording_local 	= false;
	
	if ((adrec = ad_open_dev("plughw:0,1", samprate)) == NULL) 			// open the audio device (microphone)
	    	E_FATAL("Failed to open audio device\n"); 
	
    cout << "[thread] READY..." << endl;
    while(true){
		if(!signal4recording())	
		{
			break;
		}
		
		std::cout<<"[thread] Open microphone buffer..."<<std::endl;
		if (ad_start_rec(adrec) < 0) 								// start recording
			E_FATAL("Failed to start recording\n");
		do { k = ad_read(adrec, adBuf, INT_MAX); } while(k>0);		// empty the microphone buffer from mic before recording -- previously[k = ad_read(ad, adBuf, INT_MAX);]

		
		std::cout<<"[thread] Start recording..."<<std::endl;
		while(isrecording())
		{
			//std::cout<<"[thread] is recording..."<<std::endl;
			if ((k = ad_read(adrec, adBuf, sizeRead)) < 0)				// record...
				E_FATAL("Failed to read audio\n");
			for(int i=0; i<k && this->af_i<sizefullBuff; i++)			// store the records into the big buffer object
			{
				fullBuf[this->af_i++] = adBuf[i];
			}
		}
		std::cout<<"[thread] Stop recording..."<<std::endl;
		if (ad_stop_rec(adrec) < 0) 									// answer has been given, stop recording
			E_FATAL("Failed to stop recording\n");

		std::lock_guard<std::mutex> lg(this->m_mutex);
		this->af->setAudioBufferSize(1, this->af_i);
		for(int i=0; i<this->af_i; i++)									// store the records into the big buffer object
		{
			this->af->samples[0][i] = fullBuf[i];
		}
	}
	
    if (ad_close(adrec) < 0) 											// experiment is done, close microphone
    	E_FATAL("Failed to close the device\n");
    delete [] adBuf;
    delete [] fullBuf;
	cout << "[thread] end of the function.." << endl;
}


/*****************/
/* vestiges      */
/*
 * 
 * 
 * // record function for the thread
void Experiment::record_from_microphone()
{
    ad_rec_t *adrec;
	int16_t * adBuf, * fullBuf;
	int32_t k;	
	int samprate, sizeBuff, sizefullBuff, sizeRead;
	bool disp, dispLoop, is_recording_local;
	
    samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 			// sampling rate for 'ad'
    sizeBuff 		= 2048;
    sizeRead 		= 128;
    sizefullBuff 	= 10*samprate;
	adBuf 	= new int16_t[sizeBuff];										// audio buffer of the audio device 
	fullBuf = new int16_t[sizefullBuff];									// full audio buffer of the audio device for 10 seconds 

	disp 				= true;
	dispLoop 			= false;
	is_recording_local 	= false;
	
	if ((adrec = ad_open_dev("plughw:0,1", samprate)) == NULL) 				// open the audio device (microphone)
	    	E_FATAL("Failed to open audio device\n"); 
	
    if(disp) cout << "[thread] READY..." << endl;
	while(true){ 															// while experiment is not done
		{ 	
			// wait to be notified before starting
			std::unique_lock<std::mutex> lg(this->m_mutex);						// locker to access shared variables
			if(disp) cout << "[thread] [this->cv.wait(lg)] in..." << endl;
			
			this->cv.wait(lg, [&]{return (this->is_recording.load()||this->workdone.load());});
			if (this->workdone.load())
			{
				if(disp) cout << "[thread] ------------------------- WORK IS DONE SO EXIT." << endl;
				break;
			}
			lg.unlock();
			if(disp) cout << "[thread] notify_one in..." << flush;
			this->cv.notify_one();
			if(disp) cout << "out." << endl;
			/*
			this->cv.wait(lg);												// wait for a new sequence (WF notify_one in the current 'execute' function)
			if(disp) cout << "[thread] [this->cv.wait(lg)] out.." << endl;
			if ()
			{
				if(disp) cout << "[thread] ------------------------- WORK IS DONE SO EXIT." << endl;
				break;
			}
			else if(!this->is_recording.load()){							// not a new sequence, go back to wait
				if(disp) cout << "[thread] +++++++++++++++++++++++++ NO RECORDING SO CONTINUE." << endl;
				continue;													// go to wait
			}
			*
		}
		
		if(disp) cout << "[thread] ad_start_rec in..." << flush;
		if (ad_start_rec(adrec) < 0) 										// start recording
			E_FATAL("Failed to start recording\n");
		if(disp) cout << "out." << endl;
		if(disp) cout << "[thread] ad_read in..." << flush;
		do { k = ad_read(adrec, adBuf, INT_MAX); } while (k>0); 			// empty the microphone buffer from mic before recording -- previously[k = ad_read(ad, adBuf, INT_MAX);]
		if(disp) cout << "out." << endl;
		
		is_recording_local = true;
		while(is_recording_local){ 											// while the answer is not given
			if(dispLoop) cout << "[thread] into the while..." << flush;
			if(dispLoop) cout << "k_prev=" << std::to_string(k) << "/ sizefullBuff="<< std::to_string(sizefullBuff) << "..."<< flush;
			
	        if ((k = ad_read(adrec, adBuf, sizeRead)) < 0)					// record...
	        	E_FATAL("Failed to read audio\n");
	        
	        for(int i=0; i<k && this->af_i<sizefullBuff; i++)				// store the records into the big buffer object
	        {
	        	if(dispLoop) cout << "[thread] into the for..." << std::to_string(this->af_i)<< endl;
	        	fullBuf[this->af_i++] = adBuf[i];
	        }
	        
	        {	// locker for is_recording access
				std::lock_guard<std::mutex> lg(this->m_mutex);					// locker to access shared variables
				if(dispLoop) cout << "lock_guard." << endl;
				is_recording_local = this->is_recording.load();
			}
		}
		if(disp) cout << "[thread] ad_stop_rec in..." << flush;
	    if (ad_stop_rec(adrec) < 0) 										// answer has been given, stop recording
	    	E_FATAL("Failed to stop recording\n");
	    
	    {
	    	std::lock_guard<std::mutex> lg(this->m_mutex);						// locker to access shared variables
	    	for(int i=0; i<this->af_i; i++)									// store the records into the big buffer object
			{
				this->af->samples[0][this->af_i++] = fullBuf[i];
			}
	    }
	    if(disp) cout << "out." << endl;
	}
	
    if (ad_close(adrec) < 0) 												// experiment is done, close microphone
    	E_FATAL("Failed to close the device\n");
    delete [] adBuf;
    delete [] fullBuf;
	cout << "[thread] end of the function.." << endl;
}
 */
