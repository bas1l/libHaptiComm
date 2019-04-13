
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
	std::cout << std::fixed;
	std::cout << std::setprecision(2);
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
	if(1)
	{
		std::cout<<"fakeargs>>pass..."<<std::endl;
	}
	else if(c->getLangage().compare("en-us") ==0) // english VR
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

	std::cout<<"cmd_ln_parse_r"<<std::endl;
	int argc = 0;
	char** argv = (char**)malloc(32 * sizeof(char)); // chars
	this->vr_cfg = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);	// default values for cfg
	free(argv);
	//ps_default_search_args(this->vr_cfg);			// fill non-defined variables with default values
		
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
	this->audioBufferReady = false;
	
	// period for the SPI communication
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    this->period_spi_ns  = durationRefresh_ms * ms2ns; // * ns
    
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
	/* create */
	ad_rec_t * adrec;
	AudioFile<double>::AudioBuffer buffer;								// intermediate buffer between audioFile buffer and
    int16 micBuf[2048];
	int32_t k;	
	int i, samprate, sizeBuff, sizefullBuff, ttw, nbS;
	float nbSperMS;

	/* initialize */
	const char * adcdev = cmd_ln_str_r(this->vr_cfg, "-adcdev");
	samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 		// sampling rate for 'ad'
	sizeBuff 		= 2048;
	sizefullBuff 	= 10*samprate;
	nbSperMS 		= samprate/(float)1000;
	ttw 			= 50; // milliseconds
	nbS 			= nbSperMS*ttw; // nb sample / ttw
	buffer.resize (1);
	buffer[0].resize (10*samprate);
	if ((adrec = ad_open_dev(adcdev, samprate)) == NULL) 				// open the audio device (microphone)
		E_FATAL("Failed to open audio device\n");
	
	/* work */
	std::cout<<"[record_from_microphone] READY..."<<std::endl;
	while(!signal4stop_experiment())									// check if experiment is done
	{
		signal4recording();												// wait for messaging the thread to start to record
		if (ad_start_rec(adrec) < 0) 									// start recording
			E_FATAL("Failed to start recording\n");
		std::cout<<"[MICROPHONE] timer since start="<<nowLocal(this->c_start).count()<<" ms"<<std::endl;
		auto clk_start = chrono::high_resolution_clock::now();			// get timer when the mic is opened
		do { k = ad_read(adrec, micBuf, sizeBuff); } while(k<1);		// wait until signal is found
		int latency_ms = (int)(nowLocal(clk_start).count());			// calculate the latency between the mic opening and the first datas
			
		for(i=0; i<k && this->af_i<sizefullBuff; i++){					// store the first record into the big buffer object
			buffer[0][this->af_i++] = micBuf[i];
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(ttw));	// let some time for alsa to feed the buffer
		while(!signal4stop_recording())
		{
			if ((k = ad_read(adrec, micBuf, nbS)) < 0)					// record...
				E_FATAL("Failed to read audio\n");
			for(i=0; i<k && this->af_i<sizefullBuff; i++){				// store the records into the big buffer object
				buffer[0][this->af_i++] = micBuf[i];
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(ttw));// let some time for alsa to feed the buffer
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));	// wait for the latency due to the mic driver
		do {
			if ((k = ad_read(adrec, micBuf, sizeBuff)) < 0)				// record...
				E_FATAL("Failed to read audio\n");
			for(i=0; i<k && this->af_i<sizefullBuff; i++){				// store the records into the big buffer object
				buffer[0][this->af_i++] = micBuf[i];
			}
		} while(k>0);
		if (ad_stop_rec(adrec) < 0) 									// answer has been given, stop recording
					E_FATAL("Failed to stop recording\n");
		
		std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), std::bind(std::divides<double>(), std::placeholders::_1, 
		            		   SHRT_MAX));								// Normalisation from uint16_t into -1/+1
		fillAudioBuffer(buffer);										// fill Audiobuffer with tmp buffer 
	}
	
	/* leave */
	if (ad_close(adrec) < 0) 											// experiment is done, close microphone
		E_FATAL("Failed to close the device\n");
	std::cout<<"[record_from_microphone] end of the function..."<<std::endl;
}

void Experiment::executeStimuli()
{   
	// CREATION input variables
	waveformLetter 	values;				// vector based on the wav in the config file
	char 			haptiCommActuatorletter, ERMActuatorletter;
	
	// INITIALISATION
    this->ad->spi_open();			// open SPI port: init drive electronics
    this->ad->configure();			// configure actuators values of the pcb
    this->ad->execute_trajectory(alph->getneutral(), this->period_spi_ns); // reset to 0 the actuators
    haptiCommActuatorletter = '1';	// configuration file's value for the hapticomm actuators
    ERMActuatorletter 		= '2';	// configuration file's value for the ERM actuators
	
    // decide which experiment has to be executed
    if (BrailleDevSpace == this->expToExec)						// BRAILLE_DEV AND SPACE
    {
    	values = alph->getl(haptiCommActuatorletter);
        executeActuatorSpace(values);
    }
    else if (BrailleDevTemp == this->expToExec)					// BRAILLE_DEV AND TEMPORAL
    {
    	values = alph->getl(haptiCommActuatorletter);
        executeActuatorTemp(values);
    }
    else if (BuzzerSpace == this->expToExec)					// ERM_DEV AND SPACE
	{
    	values = alph->getl(ERMActuatorletter);
        executeActuatorSpace(values);
	}
    else if (BuzzerTemp == this->expToExec)						// ERM_DEV AND TEMPORAL
    {
    	values = alph->getl(ERMActuatorletter);
        executeActuatorTemp(values);
    }
	else if (FingersSpace == this->expToExec || FingersTemp == this->expToExec) // FINGERS SPACE AND TEMPORAL
	{
		executeF();
    }
	else if (Calibration == this->expToExec)					// TR WORDS CALIBRATION
	{
    	values = alph->getl(haptiCommActuatorletter);
		executeCalibration(values);
    }
    else
    {
        std::cout<<"[experiment][execute] NO more experiment available: id(expToExec)="<<to_string(expToExec)<<std::endl;
    }
    
	std::cout<<"[main][experiment][executeActuator] end of the function..."<<std::endl;
}

bool Experiment::executeCalibration(waveformLetter values)
{
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	bool 			rect;			// 
	int 			i, j, nbAct, timeleft, randomttw, overruns;

	/* output variables */
	td_msecarray 	vhrc;			// vector of high resolution clock
	td_msecarray 	timerDebug;		// vector of high resolution clock for debugging
	int 			answeri;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	timeleft	= 0;
	nbAct 		= values.size();								// nb total of actuator
	std::vector<uint8_t> rel_id_chan = {10, 9, 11, 14, 18, 19};	// id of the actuators
    
	for (int a=1; a!=nbAct; a++)
	{
		values.erase(values.find(rel_id_chan[a])); 			// keep only the first actuator to do the stimulus
	}

	/* work */
	std::this_thread::sleep_for(std::chrono::milliseconds(50)); 						// let some time to open the mic
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the Calibration    +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	cin.get();
	for(i=this->seq_start; i<this->seq.size(); i++) 			// for the sequence i
	{
		// initialisation
		std::cout<<std::endl<<"[main][Calibration] New sequence: ["<<i<<"/"<<this->seq.size()<<"]"<<std::endl;
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
		overruns += this->ad->execute_selective_trajectory(values, this->period_spi_ns); // execute the sequence
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		std::this_thread::sleep_for(std::chrono::milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}

bool Experiment::executeActuatorSpace(waveformLetter values)
{
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	bool 			rect;
	int 			i, overruns;
	/* output variables */
	td_msecarray 	vhrc;
	td_msecarray 	timerDebug;
	int 			answeri;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	/* work */
	std::this_thread::sleep_for(std::chrono::milliseconds(50)); // let some time to open the mic
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the experiment     +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	cin.get();
	for(i=this->seq_start; i<this->seq.size(); i++) 			// for the sequence i
	{
		// initialisation
		std::cout<<std::endl<<"[main] New sequence: ["<<i<<"/"<<this->seq.size()<<"]"<<std::endl;
		valuestmp = setupWaveformSpace(&i, values, &vhrc);			// set up the waveform corresponding to the sequence
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
		overruns += this->ad->execute_selective_trajectory(valuestmp, this->period_spi_ns); // execute the sequence
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		std::this_thread::sleep_for(std::chrono::milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}


bool Experiment::executeActuatorTemp(waveformLetter values)
{
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	bool 			rect;
	int 			i, j, nbIt, overruns;
	/* output variables */
	td_msecarray 	vhrc;
	td_msecarray 	timerDebug;
	int 			answeri;
	
	/* Initialisation */
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	/* work */
	std::this_thread::sleep_for(std::chrono::milliseconds(50)); 			// let some time to open the mic
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the experiment     +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	cin.get();
	for(i=this->seq_start; i<this->seq.size(); i++) 			// for the sequence i
	{
		// initialisation
		std::cout<<std::endl<<"[main] New sequence: ["<<i<<"/"<<this->seq.size()<<"]"<<std::endl;
		valuestmp = setupWaveformTemp(&i, values, &vhrc);			// set up the waveform corresponding to the sequence
		nbIt = std::accumulate(this->seq[i].begin(),this->seq[i].end(),0);
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		for (j=0;j!=nbIt; j++)
		{
			overruns += this->ad->execute_selective_trajectory(valuestmp, this->period_spi_ns); // execute the trajectories
		    this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);
		   // std::this_thread::sleep_for(std::chrono::milliseconds(5));						// wait between stimuli?
		}
		
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		std::this_thread::sleep_for(std::chrono::milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
	}
	this->seq_end = i;
	stop_experiment();
	return false;
}

bool Experiment::executeF(){
    return false;
}

waveformLetter Experiment::setupWaveformSpace(int * currSeq, waveformLetter values_copy, td_msecarray * vhrc)
{
    waveformLetter::iterator 	it;
    std::vector<uint8_t> 		rel_id_chan;
    int nbAct;
    
    rel_id_chan = {10, 9, 11, 14, 18, 19};		// actuator's ID (AD5383)
    nbAct 		= values_copy.size();			// get the number of actuator
    for (int a=0; a!=nbAct; a++) 				// Transforms the values vector corresponding to the sequence
    {
        if (this->seq[*currSeq][a]==0) 
        {
            it = values_copy.find(rel_id_chan[a]);
            values_copy.erase(it);
        }
    }
	
    return values_copy;
}


waveformLetter Experiment::setupWaveformTemp(int * currSeq, waveformLetter values_copy, td_msecarray * vhrc)
{
    waveformLetter::iterator 	it;
    std::vector<uint8_t> 		rel_id_chan;
    int a, nbAct;
    
    nbAct 		= values_copy.size();
    rel_id_chan = {10, 9, 11, 14, 18, 19};
    for (a=0; a!=nbAct; a++)
    {
        if (a != this->actchannelID[*currSeq]) 
        {
            it = values_copy.find(rel_id_chan[a]);
            values_copy.erase(it);
        }
    }
    return values_copy;
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

bool Experiment::signal4stoprecording()
{
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	// Start waiting for the Condition Variable to get signaled
	// Wait() will internally release the lock and make the thread to block
	// As soon as condition variable get signaled, resume the thread and
	// again acquire the lock. Then check if condition is met or not
	// If condition is met then continue else again go in waithis->t_record.
	this->m_condVar.wait(mlock, std::bind(&Experiment::isstopedrecording, this));
	return true;
}

bool Experiment::signal4stop_recording()
{	
	//std::cout<<"[stop_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	return !(this->is_recording);
}

bool Experiment::signal4stop_experiment()
{	
	//std::cout<<"[stop_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	return this->workdone;
}

void Experiment::start_recording()
{	
	//std::cout<<"[start_recording] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->is_recording = true;									// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
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
	//std::cout<<"[stop_experiment] in."<<std::endl;
	std::lock_guard<std::mutex> lk(this->m_mutex);				// locker to access shared variables
	this->workdone = true;										// start of the microphone recording boolean
	this->m_condVar.notify_one(); 								// waiting thread is notified 
	//std::cout<<"[stop_experiment] out."<<std::endl;
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
	std::cout<<"Rectify the answer? [type 's' to save and quit]"<<std::endl;
	cin >> ans;
	if (isdigit(ans[0]))
	{
		//std::cout<<"'"<<ans<<"' is a digit -> '"<<atoi(ans.c_str())<<"'"<<std::endl;
		*answeri = atoi(ans.c_str());
	}
	else if (0 == ans.compare("s") || 0 == ans.compare("\x18")|| 0 == ans.compare("\x18s")) // if has to stop/pause the experiment
	{
		ret = true;
	}
    
    //std::cout<<"The new answer is='"<<ans<<"'"<<std::endl;
    return ret;
}

void Experiment::fillAudioBuffer(AudioFile<double>::AudioBuffer buffer)
{
	std::lock_guard<std::mutex> mlock(this->m_mutex);
	buffer[0].resize(this->af_i);									// keep only the data that has been recorded
	this->af->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	this->af->setAudioBuffer(buffer);								// add the new buffer
	this->af_i = 0;													// reset the audioBuffer iterator
	this->audioBufferReady = true;
	this->m_condVar.notify_one();
}

void Experiment::save_audio(int id_seq)
{
	std::unique_lock<std::mutex> mlock(this->m_mutex);
	this->m_condVar.wait(mlock, std::bind(&Experiment::isAudioBufferReady, this));
	string path(c->getPathDirectory());  					// create the name of the .wav with full path
	path += std::to_string(c->getId()) + "/wav/";			// id of the candidate
	
	string name(std::to_string(c->getId()) + "_");  		// create the name of the .wav with full path
	name += c->expstring(this->expToExec) + "_";			// current experiment's name
	name += std::to_string(id_seq) + ".wav";				// id of the sequence
	//std::cout<<"[main][save_audio] in...["<<path+name<<"]..."<<std::endl;
	//this->af->printSummary();
	this->af->save(path+name);								// save wav file of the sequence's answer
	this->audioBufferReady = false;
}

/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				d. checkers					*/
/* 					     					*/
/*------------------------------------------*/
bool Experiment::isrecording()	{ return this->is_recording; }
bool Experiment::isstopedrecording()	{ return !(this->is_recording); }
bool Experiment::isworkdone()	{ return this->workdone; }
bool Experiment::isrecordingorworkdone()	{ return (isworkdone() || isrecording()); }
bool Experiment::isAudioBufferReady()		{ return this->audioBufferReady;}





/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				e. Tools					*/
/* 					     					*/
/*------------------------------------------*/

void Experiment::randomWaiting()
{
    /* Time to wait before executing the sequence */
    int timespent, timeleft, randomttw;
	randomttw 	= 250 + rand()%2750; 								// randomize the time to wait between 1000-3000ms
	timespent 	= (int)(nowLocal(this->c_start).count()); 			// now - (beginning of the loop for this sequence)
    timeleft 	= randomttw-timespent;
	std::cout<<"wait for: "<<timeleft<<"(ms)"<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(timeleft));// let some time for alsa to feed the buffer
}

td_msec Experiment::nowLocal(td_highresclock start)
{
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now()-start);
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

/*------------------------------------------*/
/* 				3. VESTIGE					*/
/* 				a. ALL						*/
/* 					     					*/
/*------------------------------------------*

void record_from_microphone_bak()
{
	ad_rec_t * adrec;
	AudioFile<double>::AudioBuffer buffer;						// intermediate buffer between audioFile buffer and
    int16 micBuf[2048];
	int32_t k;	
	int i, samprate, sizeBuff, sizefullBuff, ttw, nbS;
	float nbSperMS;
	bool disp, dispLoop, is_recording_local;

	const char * adcdev = cmd_ln_str_r(this->vr_cfg, "-adcdev");
	samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 			// sampling rate for 'ad'
	sizeBuff 		= 2048;
	sizefullBuff 	= 10*samprate;
	nbSperMS 		= samprate/(float)1000;
	ttw 			= 50; // milliseconds
	nbS 			= nbSperMS*ttw; // nb sample / ttw
	buffer.resize (1);
	buffer[0].resize (10*samprate);
	disp 				= true;
	dispLoop 			= false;
	is_recording_local 	= false;
	
	if ((adrec = ad_open_dev(adcdev, samprate)) == NULL) 				// open the audio device (microphone)
		E_FATAL("Failed to open audio device\n");

	std::cout<<"samprate="<<samprate<<", "<<std::flush;
	std::cout<<"sizefullBuff="<<sizefullBuff<<", "<<std::endl;
	std::cout<<"[record_from_microphone] READY..."<<std::endl;
	
	while(!signal4stop_experiment())
	{
		signal4recording();	// wait for messaging the thread to start to record
		//std::cout<<"[record_from_microphone] Open microphone buffer..."<<std::endl;
		auto c_open1 = nowLocal(this->c_start);
		if (ad_start_rec(adrec) < 0) 									// start recording
			E_FATAL("Failed to start recording\n");
		auto c_open2 = nowLocal(this->c_start);
		
//		std::cout<<">>>>>>>>>>>>>>>>>>>>>> ["<<(this->af_i/(float)sizefullBuff)*100<<"%]"<<std::endl;
				
		do { k = ad_read(adrec, micBuf, sizeBuff); } while(k<1);	// wait until signal is found
		
		auto c_work1 = nowLocal(this->c_start);
		for(i=0; i<k && this->af_i<sizefullBuff; i++){			// store the records into the big buffer object
			buffer[0][this->af_i++] = micBuf[i];
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));// let some time for alsa to feed the buffer
		while(!signal4stop_recording())
		{
			if ((k = ad_read(adrec, micBuf, nbS)) < 0)				// record...
				E_FATAL("Failed to read audio\n");
			for(i=0; i<k && this->af_i<sizefullBuff; i++){			// store the records into the big buffer object
				buffer[0][this->af_i++] = micBuf[i];
			}
			//std::cout<<"[NB K ="<<k)<<"] ["<<"]"<<std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(ttw));// let some time for alsa to feed the buffer
		}
		auto c_work2 = nowLocal(this->c_start);
		auto lenBuff2_ms = this->af_i*1000/(float)samprate;
		
		do {
			if ((k = ad_read(adrec, micBuf, sizeBuff)) < 0)				// record...
				E_FATAL("Failed to read audio\n");
			for(i=0; i<k && this->af_i<sizefullBuff; i++){			// store the records into the big buffer object
				buffer[0][this->af_i++] = micBuf[i];
			}
		} while(k>0);
		
		auto c_close1 = nowLocal(this->c_start);
		if (ad_stop_rec(adrec) < 0) 									// answer has been given, stop recording
					E_FATAL("Failed to stop recording\n");
		auto c_close2 = nowLocal(this->c_start);
		
		
		
		
		auto c_diffOpen 	= c_open2.count()  - c_open1.count();
		auto c_diffClose 	= c_close2.count() - c_close1.count();
		auto c_diffWork 	= c_work2.count()  - c_work1.count();
		auto c_diffWorkmore = c_close1.count() - c_work1.count();
		auto c_norec	 	= c_work1.count()  - c_open1.count();

		auto c_diff2 		= c_close2.count() - c_open2.count();
		auto c_diff3 		= c_close1.count() - c_open2.count();

		auto c_bigdiff 		= c_close2.count() - c_open1.count();
		auto c_litdiff 		= c_close1.count() - c_open2.count();
		
		auto lenBuff_ms		= this->af_i*1000/(float)samprate;
		std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
		std::cout<<"[c_diffOpen=\t"<<c_diffOpen<<"], \t"<<std::flush;
		std::cout<<"[c_diffClose=\t"<<c_diffClose<<"]"<<std::flush;
		std::cout<<std::endl;
		std::cout<<"[Big Diff=\t"<<c_bigdiff<<"], \t"<<std::flush;
		std::cout<<"[Little Diff=\t"<<c_litdiff<<"]"<<std::flush;
		std::cout<<std::endl;
		std::cout<<"[close2-start2=\t"<<c_diff2<<"], \t"<<std::flush;
		std::cout<<"[close1-start2=\t"<<c_diff3<<"]"<<std::flush;
		std::cout<<std::endl<<std::endl;
		
		std::cout<<"[Full time=\t"<<c_bigdiff<<"], \t"<<std::flush;
		std::cout<<"[while Work=\t"<<c_diffWork<<"]"<<std::flush;
		std::cout<<std::endl;

		std::cout<<"[No rec=\t"<<c_norec<<"], \t"<<std::flush;
		std::cout<<std::endl;

		std::cout<<"[while Work=\t"<<c_diffWork<<"], \t"<<std::flush;
		std::cout<<"[full Work=\t"<<c_diffWorkmore<<"]"<<std::flush;
		std::cout<<std::endl;
		std::cout<<"[Buffer While=\t"<<lenBuff2_ms<<"], \t"<<std::flush;
		std::cout<<"[Buffer Full=\t"<<lenBuff_ms<<"]"<<std::flush;
		std::cout<<std::endl;
		std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
		
//		std::cout<<"<<<<<<<<<<<<<<<<<<<<<< ["<<(this->af_i/(float)sizefullBuff)*100<<"%]"<<std::endl;

		// Normalisation from uint16_t into -1/+1
		std::vector<double>::iterator result = std::max_element(buffer[0].begin(), buffer[0].end());
		std::cout<<"MAX OF VECTOR="<<*result<<std::endl;
		std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(),
		               std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));// *result));
		fillAudioBuffer(buffer);
	}
	
	if (ad_close(adrec) < 0) 						// experiment is done, close microphone
		E_FATAL("Failed to close the device\n");
	std::cout<<"[record_from_microphone] end of the function..."<<std::endl;
}
*/



























