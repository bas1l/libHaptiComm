
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
}


/********************************************/
/* 				1. PUBLIC					*/
/* 			   	a. Classic functions		*/
/* 					     					*/
/********************************************/
bool Experiment::create()
{
	std::cout << std::fixed;
	std::cout << std::setprecision(2);
	std::cout<<"[experiment] create::start..."<<std::endl;
	int err = 0;
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
	
	// Prepare PCM for use.
	rate_mic = EXPERIMENT_SAMPLING_RATE;
	if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", "default", snd_strerror (err));
		exit (1);
	}
	if ((err = init_captureHandle (&this->capture_handle, &rate_mic)) < 0) {
		fprintf (stderr, "init_captureHandle failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	
	// init wav writer (AudioFile library)
	std::cout<<"AudioFile"<<std::endl;
	this->af = new AudioFile<double>();
	this->af_i = 0;										// iterator for audioFile
	this->af_max = rate_mic * 10;						// size of audioFile buffer
	this->af->setAudioBufferSize(1, this->af_max);		// init audioFile buffer
	this->af->setSampleRate(rate_mic);				// set sample rate
	this->af->setBitDepth(16);							// set bit depth
	
	// thread and shared memory's variables initialisation for voice recording
	this->workdone = false; 													// check if the experiment is done
	this->is_recording = false; 												// allow t_record to start or stop the recording
	this->audioBufferReady = false;
	
	// period for the SPI communication
    double durationRefresh_ms = 1/(double) alph->getFreqRefresh_mHz();
    this->period_spi_ns  = durationRefresh_ms * ms2ns; // * ns
    
	std::cout<<"[experiment] create::...end"<<std::endl;
    return err;
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
vector<msec_array_t> Experiment::getTimer(){ return this->vvtimer; }
vector<int>         Experiment::getAnswer(){ return this->vanswer; }
vector<int>     Experiment::getConfidence(){ return this->vconfidence; }
vector<vector<int>> Experiment::getSeq(){ return this->seq; }
vector<int>     Experiment::getERMCalibrationID(){ return this->vermCalibrationID; }
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
	using namespace std::this_thread;
	using namespace std::chrono;
	// MEMORY ALLOCATIONS
	AudioFile<double>::AudioBuffer buffer; // intermediate buffer between audioFile buffer and
	int i, ii, j;
	int overruns, err;
	int sizeBuf, sizefullBuff;
	int ttw, ttw_left;
	int nbReadMax, durRecMax_sec, nbItPerSec;
	short ** buf;
	high_resolution_clock::time_point clk_read_before, clk_read_after;
	double time_span;
	int z;
	
	/* initialize */
	i = 0;
	ii = 0;
	j = 0;
	durRecMax_sec	= 10;
	sizeBuf 		= 2048;
	ttw 			= pow(10,3)*sizeBuf/this->rate_mic; // in millisecond. 
	nbItPerSec		= (this->rate_mic/sizeBuf)+1;
	nbReadMax 		= durRecMax_sec*nbItPerSec;
	sizefullBuff 	= durRecMax_sec*this->rate_mic;
	
	buf = (short**)calloc(nbReadMax, sizeof(short *));
	for(ii = 0; ii < nbReadMax; ii++) { 
		buf[ii] = (short*)calloc(sizeBuf, sizeof(short));
	}
	buffer.resize (1);
	buffer[0].resize (sizefullBuff);
	
	
	/* work */
	std::cout<<"[record_from_microphone] READY..."<<std::endl;
	while(!signal4stop_experiment())									// check if experiment is done
	{
		i = 0;
		signal4recording();												// wait for messaging the thread to start to record
		if ((err = snd_pcm_prepare (this->capture_handle)) < 0) {
			fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
			exit (1);
		}
		if ((err = snd_pcm_start (this->capture_handle)) < 0) {
			fprintf (stderr, "start recording failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		sleep_for(milliseconds(ttw));	// let some time for alsa to feed the buffer
		
		while (!signal4stop_recording() && i!=nbReadMax)
		{
			clk_read_before = chrono::high_resolution_clock::now();	// get timer when the mic is opened	
			if ((err = snd_pcm_readi(this->capture_handle, buf[i++], sizeBuf)) < 0) {
				fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
				exit (1);
			}
			
			clk_read_after 	= chrono::high_resolution_clock::now();	// get timer when the mic is opened	
			time_span 		= duration_cast<duration<double>>(clk_read_after - clk_read_before).count();
			ttw_left 		= ttw-time_span*pow(10,3);	
			
			if (ttw_left>0) sleep_for(milliseconds(ttw_left));
			else overruns += ttw_left;
			
		}
		if (i==nbReadMax)
		{
			std::cout<<"[WARNING] Buffer full without asking to stop.."<<std::endl;
			while (!signal4stop_recording()) {}
		}
		if ((err = snd_pcm_drop(this->capture_handle)) < 0) {
			fprintf (stderr, "drop the microphone failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		z=0;
		buffer[0].resize(i*sizeBuf);				// keep only the data that has been recorded
		for(ii=0; ii<i; ii++){
			for(j=0; j<sizeBuf; j++){				// store the records into the big buffer object
				buffer[0][this->af_i++] = buf[ii][j];
				if (buf[ii][j] == 0)
				{
					z++;
				}
				//std::cout<<","<<buffer[0][this->af_i-1]<<std::flush;
			}
		}
		//std::cout<<"[record_from_microphone] Before, Zeros = "<<z<<"/"<<sizeBuf*i<<std::endl;
		z=0;
		for (ii=0; ii<buffer[0].size();ii++)
		{
			if (buffer[0][ii] == 0)
				{
					z++;
				}
		}
		//std::cout<<"[record_from_microphone] Buffer after resize, before transform, Zeros = "<<z<<"/"<<sizeBuf*i<<std::endl;
		std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), 
					   std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));
		z=0;
		for (ii=0; ii<buffer[0].size();ii++)
		{
			if (buffer[0][ii] == 0)
				{
					z++;
				}
		}
		//std::cout<<"[record_from_microphone] Buffer after transform, Zeros = "<<z<<"/"<<sizeBuf*i<<std::endl;
			
		fillAudioBuffer(buffer);										// fill Audiobuffer with tmp buffer 
	}
	
	/* leave */
	snd_pcm_close(this->capture_handle);
	std::cout<<"[record_from_microphone] end of the function..."<<std::endl;
	std::cout<<"[OVERRUNS] Total overrruns="<<overruns<<" samples."<<std::endl;
}


void Experiment::executeStimuli()
{
	// CREATION input variables
	waveformLetter 	values;				// vector based on the wav in the config file
	char haptiCommActuatorletter, ERMActuatorletter, ERMCalibrationLetter[3];
	
	// INITIALISATION
    this->ad->spi_open();			// open SPI port: init drive electronics
    this->ad->configure();			// configure actuators values of the pcb
    this->ad->execute_trajectory(alph->getneutral(), this->period_spi_ns); // reset to 0 the actuators
    haptiCommActuatorletter = '1';	// configuration file's value for the hapticomm actuators
    ERMActuatorletter 		= '2';	// configuration file's value for the ERM actuators
	ERMCalibrationLetter[0] = '2';
	ERMCalibrationLetter[1] = '3';
	ERMCalibrationLetter[2] = '4';
	
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
	else if (FingersSpace == this->expToExec) // FINGERS SPACE AND TEMPORAL
	{
		executeFingerSpace();
    }
    else if (FingersTemp == this->expToExec)
    {
		std::cout<<"[experiment][execute] Experiment<"<<to_string(expToExec)<<"> not available yet."<<std::endl;
	}
	else if (CalibrationWord == this->expToExec)					// TR WORDS CALIBRATION
	{
    	values = alph->getl(haptiCommActuatorletter);
		executeCalibrationWord(values);
    }
	else if (CalibrationERM == this->expToExec)					// calibration optimization ERM
	{
		std::vector<waveformLetter> vvalues;
		for (int i=0; i<sizeof(ERMCalibrationLetter); i++){
			vvalues.push_back(alph->getl(ERMCalibrationLetter[i]));
		}
		executeCalibrationERM(vvalues);
    }
    else
    {
        std::cout<<"[experiment][execute] NO more experiment available: id(expToExec)="<<to_string(expToExec)<<std::endl;
    }
    
	std::cout<<"[main][experiment][executeActuator] end of the function..."<<std::endl;
}




bool Experiment::executeCalibrationWord(waveformLetter values)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	bool 			rect;			// 
	int 			i, j, nbAct, timeleft, randomttw, overruns;

	/* output variables */
	msec_array_t 	vhrc;			// vector of high resolution clock
	msec_array_t 	timerDebug;		// vector of high resolution clock for debugging
	int 			answeri, confidencei;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	timeleft	= 0;
	nbAct 		= values.size();								// nb total of actuator
	std::vector<int> answerOrder = {1, 2, 3, 4, 5, 6};			// id of the answers
	std::random_shuffle(answerOrder.begin(), answerOrder.end());// shuffle the answers
	std::vector<uint8_t> rel_id_chan = {10, 9, 11, 14, 18, 19};	// id of the actuators
    
    for(i=0; i!=vhrc.size(); i++)
    {
		vhrc[i] = nowLocal(chrono::high_resolution_clock::now());
	}
	
	for (int a=1; a!=nbAct; a++)
	{
		values.erase(values.find(rel_id_chan[a])); 			// keep only the first actuator to do the stimulus
	}

	/* work */
	sleep_for(milliseconds(50)); 						// let some time to open the mic
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the Calibration    +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	
	for(i=this->seq_start; i<this->seq.size(); i++) 			// for the sequence i
	{
		std::cout<<"Push [ENTER] to start the next sequence. Say the number <"<<answerOrder[i/10]<<">"<<std::endl;
		if(i!=this->seq_start) std::cin.ignore();
		std::cin.get();
		
		// initialisation
		std::cout<<std::endl<<"[main][Calibration] New sequence: ["<<i<<"/"<<this->seq.size()<<"]"<<std::endl;
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		//std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
		overruns += this->ad->execute_selective_trajectory(values, this->period_spi_ns); // execute the sequence
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		sleep_for(milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		sleep_for(milliseconds(1000));								// let some time to record the mic
		stop_recording(); 											// stop to record from microphone
		
		confidencei = 4;
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
		vconfidence.push_back(confidencei); // save answer
		
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}



bool Experiment::executeCalibrationERM(std::vector<waveformLetter> vvalues)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	std::vector<int> waveformIDs;
	bool 			rect;
	int 			i, j, overruns;
	/* output variables */
	msec_array_t 	vhrc;
	msec_array_t 	timerDebug;
	int 			answeri, confidencei;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	// vector of (actuators array, vvalues ID )
	std::vector<std::pair<std::vector<int>, int>> seqcalERM;
	
	for (i=0; i<vvalues.size(); i++)
	{
		for (j=0; j<this->seq.size(); j++)
		{
			seqcalERM.push_back(std::make_pair(this->seq[j], i));
		}
	}
	std::random_shuffle(seqcalERM.begin(), seqcalERM.end());
	
	std::cout<<"this->seq.size()>"<<this->seq.size()<<std::endl;
	this->seq.clear();
	std::cout<<"this->seq.size()>"<<this->seq.size()<<std::endl;
	this->seq.reserve(seqcalERM.size());
	std::cout<<"this->seq.size()>"<<this->seq.size()<<std::endl;
	this->vermCalibrationID.reserve(seqcalERM.size());
	for (i=0; i<seqcalERM.size(); i++)
	{
		this->seq.push_back(seqcalERM[i].first);
		this->vermCalibrationID.push_back(seqcalERM[i].second);
		std::cout<<this->vermCalibrationID[i]<<", "<<std::flush;
		for(j=0;j<this->seq[i].size();j++)
		{
			std::cout<<this->seq[i][j]<<std::flush;
		}
		std::cout<<std::endl;
	}
	std::cout<<"this->seq.size()>"<<this->seq.size()<<std::endl;
	std::cout<<"seqcalERM.size()>"<<this->vermCalibrationID.size()<<std::endl;
	
	/* work */
	sleep_for(milliseconds(50)); // let some time to open the mic
	std::cout<<"+----------------------------------------------+"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"     Experiment: \t"<<c->expstring(this->expToExec)<<std::endl;
	std::cout<<"+    Press [ENTER] to start the experiment     +"<<std::endl;
	std::cout<<"+...                                        ...+"<<std::endl;
	std::cout<<"+----------------------------------------------+"<<std::endl;
	cin.get();
	for(i=0; i<this->seq.size(); i++) 			// for the sequence i
	{
		if ( i==this->seq.size()/4 || i==2*(this->seq.size()/4) || i==3*(this->seq.size()/4) )
		{
			std::cout<<"------ BREAK IS NEEDED. Press [ENTER] to start again:"<<std::endl;
			cin.get();
		}
		
		// initialisation
		std::cout<<std::endl<<"[main] New sequence: ["<<i<<"/"<<this->seq.size()<<"]"<<std::endl;
		valuestmp = setupWaveformSpace(&i, vvalues[this->vermCalibrationID[i]], &vhrc);	// set up the waveform corresponding to the sequence
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
		overruns += this->ad->execute_selective_trajectory(valuestmp, this->period_spi_ns); // execute the sequence
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		sleep_for(milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		sleep_for(milliseconds(1000));								// let some time to record the mic
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		writeConfidence(&confidencei);
		
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
		vconfidence.push_back(confidencei); // save confidence
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}

bool Experiment::executeFingerSpace()
{	
	using namespace std::this_thread;
	using namespace std::chrono;
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	bool 			rect;
	int 			i, overruns;
	/* output variables */
	msec_array_t 	vhrc;
	msec_array_t 	timerDebug;
	int 			answeri, confidencei;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	/* work */
	sleep_for(milliseconds(50)); // let some time to open the mic
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
		//valuestmp = setupWaveformSpace(&i, values, &vhrc);			// set up the waveform corresponding to the sequence
		this->c_start = chrono::high_resolution_clock::now(); 		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		randomWaiting();											// random waiting time to avoir any adapting rythm behavior during the exp
		
		// work
		start_recording(); 											// start to record from microphone
		vhrc[0] = nowLocal(this->c_start);							// get timing before stimuli
		std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
		overruns += this->ad->execute_selective_trajectory(valuestmp, this->period_spi_ns); // execute the sequence
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		sleep_for(milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		sleep_for(milliseconds(1000));								// let some time to record the mic
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		writeConfidence(&confidencei);
		
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
		vconfidence.push_back(confidencei); // save answer
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}


bool Experiment::executeActuatorSpace(waveformLetter values)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	bool 			rect;
	int 			i, overruns;
	/* output variables */
	msec_array_t 	vhrc;
	msec_array_t 	timerDebug;
	int 			answeri, confidencei;
	
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	/* work */
	sleep_for(milliseconds(50)); // let some time to open the mic
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
		sleep_for(milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		sleep_for(milliseconds(1000));								// let some time to record the mic
		stop_recording(); 											// stop to record from microphone
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		writeConfidence(&confidencei);
		
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
		vconfidence.push_back(confidencei); // save answer
	}
	
	this->seq_end = i;
	stop_experiment();
	return false;
}



bool Experiment::executeActuatorTemp(waveformLetter values)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	std::cout<<"[experiment][executeActuator] start..."<<std::endl;
	// environmental variables
	waveformLetter	valuestmp;
	bool 			rect;
	int 			i, j, nbIt, overruns;
	/* output variables */
	msec_array_t 	vhrc;
	msec_array_t 	timerDebug;
	int 			answeri, confidencei;
	
	/* Initialisation */
	rect 		= false;
	overruns 	= 0;
	i 			= 0;
	
	/* work */
	sleep_for(milliseconds(50)); 			// let some time to open the mic
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
		   // std::this_thread::sleep_for(milliseconds(5));						// wait between stimuli?
		}
		
		this->ad->execute_trajectory(this->alph->getneutral(), this->period_spi_ns);		// all actuators to rest (security)
		vhrc[1] = nowLocal(this->c_start);							// get timing after stimuli
		sleep_for(milliseconds(1));	// let some time to record the mic
		rect = writeAnswer(&answeri);								// write the answer given by the participant
		stop_recording(); 											// stop to record from microphone
		sleep_for(milliseconds(1000));								// let some time to record the mic
		
		writeConfidence(&confidencei);
		
		// store and check the work
		if (rect) 													// if 's' answer, exit
		{
			std::cout<<"[EXIT] The experiment will be saved to i="<<i-1<<"/"<<this->seq.size()<<std::endl;
			break; // go to save
		}
		save_audio(i);					// save audio file
		vvtimer.push_back(vhrc);		// save timers
		vanswer.push_back(answeri);		// save answer
		vconfidence.push_back(confidencei); // save confidence
	}
	this->seq_end = i;
	stop_experiment();
	return false;
}


waveformLetter Experiment::setupWaveformSpace(int * currSeq, waveformLetter values_copy, msec_array_t * vhrc)
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


waveformLetter Experiment::setupWaveformTemp(int * currSeq, waveformLetter values_copy, msec_array_t * vhrc)
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
	std::cout<<"What was the answer? ['s' = save and quit]"<<std::endl;
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

//vconfidence
bool Experiment::writeConfidence(int * confidencei)
{
    bool workdone=false;
	string ans;
	
	do {
		std::cout<<"What is the confidence? (between 0-4)>"<<std::endl;
		cin >> ans;
		// if it is a int 
		if (isdigit(ans[0]))
		{
			*confidencei = atoi(ans.c_str());
			// if respect the 0 to 4 value, return
			if (*confidencei >= 0 && *confidencei <=4)
			{
				workdone = true;
			}
		}
	} while( !workdone );
	
    return false;
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
bool Experiment::isrecording() { return this->is_recording; }
bool Experiment::isstopedrecording() { return !(this->is_recording); }
bool Experiment::isworkdone() { return this->workdone; }
bool Experiment::isrecordingorworkdone() { return (isworkdone() || isrecording()); }
bool Experiment::isAudioBufferReady() { return this->audioBufferReady;}





/*------------------------------------------*/
/* 				2. PRIVATE					*/
/* 				e. Tools					*/
/* 					     					*/
/*------------------------------------------*/

int Experiment::init_captureHandle(snd_pcm_t ** capture_handle, unsigned int * exact_rate)
{

	snd_pcm_hw_params_t *hw_params;
	int err = 0;
	
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return err;
	}
	// Fill params with a full configuration space for a PCM.
	if ((err = snd_pcm_hw_params_any (*capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return err;
	}
	// Mode de lecture sur le pulse code modulation
	if ((err = snd_pcm_hw_params_set_access (*capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_set_format (*capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
		return err;
	}
	// snd_pcm_hw_params_set_rate_near
	if ((err = snd_pcm_hw_params_set_rate_near (*capture_handle, hw_params, exact_rate, 0)) < 0)
	{
		fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
		return err;
	}
	// Restrict a configuration space to contain only its minimum channels count.
	if ((err = snd_pcm_hw_params_set_channels (*capture_handle, hw_params, 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
		return err;
	}
	// Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it.
	if ((err = snd_pcm_hw_params (*capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
		return err;
	}

	if (snd_pcm_hw_params_can_pause	(hw_params)	 == 0) {
		printf("pause is not supported!!!\n");
	}
	
			
	snd_pcm_hw_params_free (hw_params);
	
	return err;
}


void Experiment::randomWaiting()
{
    /* Time to wait before executing the sequence */
    int timespent, timeleft, randomttw;
	randomttw 	= 250 + rand()%2000; 								// randomize the time to wait between 1000-3000ms
	timespent 	= (int)(nowLocal(this->c_start).count()); 			// now - (beginning of the loop for this sequence)
    timeleft 	= randomttw-timespent;
	std::cout<<"wait for: "<<timeleft<<"(ms)"<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(timeleft));// let some time for alsa to feed the buffer
}

msec_t Experiment::nowLocal(highresclock_t start)
{
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now()-start);
}

void Experiment::dispTimers(int numSeq, int answeri, msec_array_t vhrc, msec_array_t timerDebug)
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
	int i, samprate, sizeBuf, sizefullBuff, ttw, nbS;
	float nbSperMS;
	bool disp, dispLoop, is_recording_local;

	const char * adcdev = cmd_ln_str_r(this->vr_cfg, "-adcdev");
	samprate = (int) cmd_ln_float32_r(this->vr_cfg, "-samprate"); 			// sampling rate for 'ad'
	sizeBuf 		= 2048;
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
				
		do { k = ad_read(adrec, micBuf, sizeBuf); } while(k<1);	// wait until signal is found
		
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
			if ((k = ad_read(adrec, micBuf, sizeBuf)) < 0)				// record...
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



























