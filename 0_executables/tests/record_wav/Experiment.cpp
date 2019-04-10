
#include "Experiment.h"



/********************************************/
/* 				1. PUBLIC					*/
/* 				    all 					*/
/* 					     					*/
/********************************************/

Experiment::~Experiment()
{
    delete this->cfg;
    delete this->dev;
    delete this->wf;
    delete this->alph;
    delete this->ad;
    
	//if(vr_ps!=NULL) ps_free(vr_ps);
	if(vr_cfg!=NULL) cmd_ln_free_r(vr_cfg);
}


/********************************************/
/* 				1. PUBLIC					*/
/* 			   	b. Classic functions		*/
/* 					     					*/
/********************************************/
bool Experiment::create()
{
	std::cout<<"[experiment] create::start..."<<std::endl;
	   
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
	
	/* Share the attribute for all created threads */
	pthread_attr_init(&attr);
	if(pthread_attr_setinheritsched(&(this->attr), PTHREAD_INHERIT_SCHED) != 0) {
		perror("pthread_attr_setinheritsched() failed");
		exit(-1);
	}
	
	/* Add attribute to the parameter schedule for the threads */
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(pthread_attr_setschedparam(&(this->attr), &param) == 0) {
		perror("pthread_attr_setschedparam() failed");
		exit(-2);
	}
	if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
		perror("mlockall failed");
		exit(-4);
	}
	setlocale(LC_ALL, "");
	
	std::cout<<"cfg->configure>>"<<std::endl;
	this->cfg->configure(cfgSource, dev, wf, alph);	// hapticomm configuration file

	
	
	// preparing the command line arguments 'argv' variable for CMU Sphinx VR
	std::cout<<"fakeargs>>"<<std::endl;
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
  
	// init wav writer (AudioFile library)
	std::cout<<"AudioFile"<<std::endl;
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
	
	std::cout<<"[experiment] create::...end"<<std::endl;
    return 0;
}


bool Experiment::execute(){
	int err;
	err = pthread_create(&(this->t_record), &(this->attr), &Experiment::static_record_from_microphone, this);
	err = pthread_create(&(this->t_tap), 	&(this->attr), &Experiment::static_executeStimuli, this);

	err = pthread_join(this->t_record, NULL);
	err = pthread_join(this->t_tap, NULL);
	
	
	return false;
}
	

/********************************************/
/* 				1. PUBLIC					*/
/* 			   	b. Getters					*/
/* 					     					*/
/********************************************/
vector<td_msecarray> Experiment::getTimer(){ return this->vvtimer; }
vector<int>         Experiment::getAnswer(){ return this->vanswer; }
int              Experiment::getSeq_start(){ return this->seq_start; }
int                Experiment::getSeq_end(){ return this->seq_end; }






/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				a. main tasks experiment	*/
/* 					     					*/
/*------------------------------------------*/

void * Experiment::static_record_from_microphone(void * c)
{
    ((Experiment *) c)->record_from_microphone();
    return NULL;
}

void * Experiment::static_executeStimuli(void * c)
{
    ((Experiment *) c)->executeStimuli();
	return NULL;
}

void Experiment::record_from_microphone()
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
	
	if ((adrec = ad_open_dev("plughw:1,0", samprate)) == NULL) 			// open the audio device (microphone)
			E_FATAL("Failed to open audio device\n"); 
	
	std::cout<<"[thread] READY..."<<std::endl;
	while(true)
	{
		// Acquire the lock
		if(!signal4recording()) 
		{
			break;
		}
		std::cout<<"[thread] Open microphone buffer..."<<std::endl;
		if (ad_start_rec(adrec) < 0) 								// start recording
			E_FATAL("Failed to start recording\n");
		do { k = ad_read(adrec, adBuf, INT_MAX); } while(k>0);		// empty the microphone buffer from mic before recording -- previously[k = ad_read(ad, adBuf, INT_MAX);]

		
		std::cout<<">>>>>>>>>>>>>>>>>>>>>>"<<std::endl;
		while(true)
		{
			std::lock_guard<std::mutex> lk(this->m_mutex);// locker to access shared variables
			if (!isrecording()) break;
			if ((k = ad_read(adrec, adBuf, sizeRead)) < 0)				// record...
				E_FATAL("Failed to read audio\n");
			for(int i=0; i<k && this->af_i<sizefullBuff; i++)			// store the records into the big buffer object
			{
				fullBuf[this->af_i++] = adBuf[i];
			}
		}
		std::cout<<"<<<<<<<<<<<<<<<<<<<<<<"<<std::endl;
		if (ad_stop_rec(adrec) < 0) 									// answer has been given, stop recording
			E_FATAL("Failed to stop recording\n");

		std::lock_guard<std::mutex> mlock(this->m_mutex);
		std::cout<<"Do Processing On loaded Data"<<std::endl;
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
	std::cout<<"[thread] end of the function.."<<std::endl;
}

void Experiment::executeStimuli()
{
    // get SPI refresh time (millisecond)
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    int durationRefresh_ns  = durationRefresh_ms * ms2ns; // * ns
    
    // init drive electronics
    this->ad->spi_open();
    this->ad->configure();
    // init all the actuators to zero
    this->ad->execute_trajectory(alph->getneutral(), durationRefresh_ns);
    
    // decide which experiment has to be executed
    if (BrailleDevSpace == this->expToExec || BrailleDevTemp == this->expToExec)
    {
        executeActuator(&durationRefresh_ns);
    }
    else if (BuzzerSpace == this->expToExec || BuzzerTemp == this->expToExec)
	{
        executeActuator(&durationRefresh_ns);
	}
	else if (FingersSpace == this->expToExec || FingersTemp == this->expToExec)
	{
		executeF(&durationRefresh_ns);
    }
    else
    {
        std::cout<<"[experiment][execute] NO more experiment available: id(expToExec)="<<to_string(expToExec)<<std::endl;
    }
}

bool Experiment::executeActuator(int * durationRefresh_ns)
{
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	bool rect;
	int i, overruns;
	/* input variables */
	char letter;
	waveformLetter values;
	/* output variables */
	td_msecarray vhrc;
	td_msecarray timerDebug;
	int answeri;
	
	/* Initialisation */
	if (BrailleDevSpace == this->expToExec || BrailleDevTemp == this->expToExec)
	{
		letter = 'a'; // change the signal that is used for actuator hapticomm
	}
	else
	{
		letter = 'b';
	}
	values = alph->getl(letter);
	rect = false;
	overruns = 0;
	i = 0;

	/* work */
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the experiment     +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	cin.get();
	//for(i=this->seq_start; i<this->seq.size(); i++) // for the sequence i
	for(i=this->seq_start; i<3; i++) // for the sequence i
	{
		std::cout<<std::endl<<"[main] New sequence:"<<std::endl;
		//this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		start_recording(); // change is_recording value to true
		//overruns += executeSequence(&i, values, durationRefresh_ns, &vhrc); 	// copy of variable[values]  is important (erase)
		
		//dispTimers(i, answeri, vhrc, timerDebug);
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); 			// let some time to open the mic
		rect = writeAnswer(&answeri);
		stop_recording(); // change is_recording value to false
		
		if (rect) // if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		dispTimers(i, answeri, vhrc, timerDebug);
		
		// save the result
		save_audio(i);
		vvtimer.push_back(vhrc);
		vanswer.push_back(answeri);
	}
	this->seq_end = i;
	
	stop_experiment();
	
	std::cout<<"[main][experiment][executeActuator] ...end"<<std::endl;
	return false;
}


bool Experiment::executeF(int * durationRefresh_ns){
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
    
    if (ovr) { std::cout<<"Overruns: "<<ovr<<std::endl; }
	
    return ovr;
}


int Experiment::executeSequenceSpace(int * currSeq, waveformLetter *values, int * dr_ns, td_msecarray * vhrc)
{
    int j=0, ovr=0, timeleft=0; 		// ready to check each actuator availability of the sequence
    int nbAct = values->size();
    waveformLetter::iterator it;
    int randomttw = 250 + rand()%2750; 	// randomize the time to wait between 1000-3000ms
    std::cout<<"randomttw= "<<to_string(randomttw)<<"ms"<<std::endl;
    
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
    
    std::cout<<"[executeSequenceTemp] randomttw= "<<to_string(randomttw)<<"ms"<<std::endl;
    /* Transforms the values vector corresponding to the sequence */
    for (int a=0; a!=nbAct; a++)
    {
        if (a != this->actchannelID[*currSeq]) 
        {
            it = values->find(rel_id_chan[a]);
            values->erase(it);
        }
    }
    
    std::cout<<"[executeSequenceTemp] number of iteration='"<<nbiteration<<"'"<<std::endl;
    std::cout<<"[executeSequenceTemp] size of values='"<<values->size()<<"'"<<std::endl;
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
        //std::cout<<"[executeSequenceTemp] do it! '"<<j<<"'x times"<<std::endl;
        this->ad->execute_trajectory(this->alph->getneutral(), *dr_ns);
        //usleep(5*1000);
    }
    (*vhrc)[1] = nowSeq();
    
    return ovr;
}

void Experiment::initactid4temp(){
    // pour chaque effecteur
    int actmin = (BrailleDevTemp == this->expToExec)?0:1;
    int actmax = 6;
    
    //for each sequence
    for(int i=0; i<seq.size(); i++)
    {
        actchannelID.push_back(rand()%actmax+actmin);
    }
}


/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				b. threads related			*/
/* 					     					*/
/*------------------------------------------*/
bool Experiment::signal4recording()
{
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	// Start waiting for the Condition Variable to get signaled
	// Wait() will internally release the lock and make the thread to block
	// As soon as condition variable get signaled, resume the thread and
	// again acquire the lock. Then check if condition is met or not
	// If condition is met then continue else again go in waithis->t_record.
	this->m_condVar.wait(mlock, std::bind(&Experiment::isrecordingorworkdone, this));
	if (isworkdone()) {return false;}
	else return true;
}

void Experiment::start_recording()
{	
	//std::cout<<"[start_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->is_recording = true;									// start of the microphone recording boolean
	this->m_condVar.notify_all(); 								// waiting thread is notified 
	//std::cout<<"[start_recording] out."<<std::endl;
}

void Experiment::stop_recording()
{	
	//std::cout<<"[stop_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->is_recording = false;									// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	//std::cout<<"[stop_recording] out."<<std::endl;
}

void Experiment::stop_experiment()
{	
	std::cout<<"[stop_experiment] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->workdone = true;										// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	std::cout<<"[stop_experiment] out."<<std::endl;
}


/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				c. Answer related			*/
/* 					     					*/
/*------------------------------------------*/
bool Experiment::writeAnswer(int * answeri)
{
    bool ret=false;
	string ans;
    if (1)
    {
    	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
        std::cout<<"Rectify the answer? [type 's' to save and quit]"<<std::endl;
        cin >> ans;
        if (isdigit(ans[0]))
        {
            std::cout<<"'"<<ans<<"' is a digit -> '"<<atoi(ans.c_str())<<"'"<<std::endl;
            *answeri = atoi(ans.c_str());
        }
        else if (0 == ans.compare("s") || 0 == ans.compare("\x18")|| 0 == ans.compare("\x18s")) // if has to stop/pause the experiment
        {
            ret = true;
        }
    }
    std::cout<<"The new answer is='"<<ans<<"'"<<std::endl;
    return ret;
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

/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				d. checkers					*/
/* 					     					*/
/*------------------------------------------*/
bool Experiment::isrecording()	{ return this->is_recording; }
bool Experiment::isworkdone()	{ return this->workdone; }
bool Experiment::isrecordingorworkdone()	{ return (isworkdone() || isrecording()); }







/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				e. Tools					*/
/* 					     					*/
/*------------------------------------------*/

td_msec Experiment::nowSeq()
{
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now()-this->c_start);
}

void Experiment::dispTimers(int numSeq, int answeri, td_msecarray vhrc, td_msecarray timerDebug)
{
    std::cout<<"["<<numSeq<<"][";
    for (int i=0; i!=seq[numSeq].size(); i++){
        std::cout<<seq[numSeq][i];
    }
    std::cout<<"] "
           <<"V="<<answeri<<" | "
           <<"start[stim]: "<<(int)vhrc[0].count()<<" | "
           <<"end[stim]: "<<(int)vhrc[1].count()<<" | "
           <<"(tmp:"<<(int)timerDebug[0].count()<<")| "
           <<"detection: "<<(int)vhrc[2].count()<<" | "
           <<"aft_detection: "<<(int)timerDebug[1].count()<<" | "
           <<"aft_recognition: "<<(int)timerDebug[2].count()<<"(ms)"<<std::endl;
}


