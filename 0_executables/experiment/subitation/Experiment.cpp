#include "Experiment.h"

/********************************************/
/*                 1. PUBLIC                    */
/*                     all                     */
/*                                              */
/********************************************/

Experiment::~Experiment() {
    delete this->cfg;
    delete this->dev;
    delete this->wf;
    delete this->alph;
    delete this->ad;
}

/********************************************/
/*                 1. PUBLIC                    */
/*                    a. Classic functions        */
/*                                              */
/********************************************/
bool Experiment::create() {
    std::cout << "[experiment] create::start..." << std::endl;
    std::cout << std::fixed;
    std::cout << std::setprecision(2);

    int err = 0;

    /* (0/6)[DISPLAY] SETUP ENVIRONEMENT: */
    //printw and timer */
    srand (time(NULL));         // initialize random seed
    struct timespec    t;       // get timers for actuators control
    struct sched_param param;

    /* (1/6)[MICROPHONE] Prepare PCM for use. */
    if ((err = tool_setup_captureHandle()) < 0) {
        fprintf(stderr, "tool_setup_captureHandle failed (%s)\n",
                snd_strerror(err));
        exit(1);
    }

    /* (2/6)[WAV] wav writer (AudioFile library) */
    std::cout << "AudioFile" << std::endl;
    this->af = new AudioFile<double>();
    this->af_i = 0;                                    // iterator for audioFile
    this->af_max = this->rate_mic * 10;              // size of audioFile buffer
    this->af->setAudioBufferSize(1, this->af_max);      // init audioFile buffer
    this->af->setSampleRate(rate_mic);                // set sample rate
    this->af->setBitDepth(16);                            // set bit depth

    /* (3/6)[THREADS] thread and shared memory's variables initialisation for voice recording */
    /* Share the attribute for all created threads */
    pthread_attr_init (&attr);
    if (pthread_attr_setinheritsched(&(this->attr), PTHREAD_INHERIT_SCHED)
            != 0) {
        perror("pthread_attr_setinheritsched() failed");
        exit(-1);
    }
    /* Add attribute to the parameter schedule for the threads */
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (pthread_attr_setschedparam(&(this->attr), &param) == 0) {
        perror("pthread_attr_setschedparam() failed");
        exit(-2);
    }
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-4);
    }
    setlocale(LC_ALL, "");
    this->workdone = false;                   // check if the experiment is done
    this->recording = false; // allow t_record to start or stop the recording
    this->audioBufferReady = false;

    /* (4/6)[SPI COMMUNICATION] period for the SPI communication */
    this->cfg->configure(cfgSource, dev, wf, alph); // hapticomm configuration file
    double durationRefresh_ms = 1 / (double) alph->getFreqRefresh_mHz();
    this->period_spi_ns = durationRefresh_ms * ms2ns; // * ns

    /* (5/6)[SEQUENCES ORDERING] */
    std::vector < char > haptiCommActuatorletter, 
            ERMActuatorletter, ERMCalibrationLetter;
    haptiCommActuatorletter.push_back('1'); // configuration file's value for the hapticomm actuators
    haptiCommActuatorletter.push_back('2'); // configuration file's value for the hapticomm actuators
    haptiCommActuatorletter.push_back('3'); // configuration file's value for the hapticomm actuators
    haptiCommActuatorletter.push_back('4'); // configuration file's value for the hapticomm actuators
    ERMActuatorletter.push_back('5'); // configuration file's value for the ERM actuators
    ERMActuatorletter.push_back('6'); // configuration file's value for the ERM actuators
    ERMActuatorletter.push_back('7'); // configuration file's value for the ERM actuators
    ERMActuatorletter.push_back('8'); // configuration file's value for the ERM actuators
    ERMCalibrationLetter.push_back('a');
    ERMCalibrationLetter.push_back('b');
    ERMCalibrationLetter.push_back('c');
    // decide which experiment has to be executed
    this->expToExec = this->c->get_nextExp(); // define current experiment
    switch (this->expToExec) {
    case BrailleDevSpace:
        tool_setup_waveformIDsAndWF_map(haptiCommActuatorletter);
        tool_setup_actuatorsAndWaveformIDs_sequences(this->c->get_sequence(),
                haptiCommActuatorletter);
        break;
    case BuzzerSpace:
        tool_setup_waveformIDsAndWF_map(ERMActuatorletter);
        tool_setup_actuatorsAndWaveformIDs_sequences(this->c->get_sequence(),
                ERMActuatorletter);
        break;
    case CalibrationWord:
        haptiCommActuatorletter.erase(haptiCommActuatorletter.begin()+1, haptiCommActuatorletter.end());
        tool_setup_waveformIDsAndWF_map(haptiCommActuatorletter);
        tool_setup_actuatorsAndWaveformIDs_sequences(this->c->get_sequence(),
                haptiCommActuatorletter);
        break;
    case CalibrationERM:
        tool_setup_waveformIDsAndWF_map(ERMCalibrationLetter);
        tool_setup_actuatorsAndWaveformIDs_sequences(this->c->get_sequence(),
                ERMCalibrationLetter);
        break;
    case FingersSpace:
        break;
    default:
        err = 61;
        fprintf(stderr, "No experiment available : (%s)\n", snd_strerror(err));
        exit(1);
        break;
    }
    
    /* (6/6)[OUTPUTS] */
    this->answer_timers.reserve(this->actuatorsAndWaveformIDs_sequences.size()); // vector of timers (beginning and end of the sequence, and RT)
    this->answer_values.reserve(this->actuatorsAndWaveformIDs_sequences.size()); // vector of answers (between 1 to 6)

    std::cout << "[experiment] create::...end" << std::endl;
    return err;
}

bool Experiment::execute() {
    int err;
    err = pthread_create(&(this->t_record), &(this->attr),
            &Experiment::static_record_from_microphone, this);
    err = pthread_create(&(this->t_tap), &(this->attr),
            &Experiment::static_execute_stimuli, this);
    err = pthread_create(&(this->t_orchestration), &(this->attr),
            &Experiment::static_start_experiment, this);

    err = pthread_join(this->t_record, NULL);
    err = pthread_join(this->t_tap, NULL);
    err = pthread_join(this->t_orchestration, NULL);

    return false;
}

/********************************************/
/*                 1. PUBLIC                    */
/*                    b. Getters                    */
/*                                              */
/********************************************/
vector<msec_array_t> Experiment::get_answer_timers() {
    return this->answer_timers;
}
vector<int> Experiment::get_answer_values() {
    return this->answer_values;
}
vector<int> Experiment::get_answer_confidences() {
    return this->answer_confidences;
}
std::vector<vector<int>> Experiment::get_actuators_sequences() {
    std::vector<std::vector<int>> actuators_sequences;
    
    std::transform(begin(actuatorsAndWaveformIDs_sequences), end(actuatorsAndWaveformIDs_sequences),
                   std::back_inserter(actuators_sequences),
                   get_first());
    
    return actuators_sequences;
}
std::vector<char> Experiment::get_waveforms_sequences() {
    std::vector<char> waveforms_sequences;
     
    std::transform(begin(actuatorsAndWaveformIDs_sequences), end(actuatorsAndWaveformIDs_sequences),
                   std::back_inserter(waveforms_sequences),
                   get_second());
    
    return waveforms_sequences;
}
vector<int> Experiment::get_ERMCalibrationID() {
    std::vector<int> waveforms_sequences;
    return waveforms_sequences;
}
int Experiment::get_seq_start() {
    return this->seq_start;
}
int Experiment::get_seq_end() {
    return this->seq_end;
}

/*------------------------------------------*/
/*                 2. PRIVATE               */
/*                 a. main tasks experiment */
/*                                          */
/*------------------------------------------*/

void * Experiment::static_record_from_microphone(void * c) {
    ((Experiment *) c)->record_from_microphone();
    return NULL;
}

void * Experiment::static_execute_stimuli(void * c) {
    ((Experiment *) c)->execute_stimuli();
    return NULL;
}

void * Experiment::static_start_experiment(void * c) {
    ((Experiment *) c)->start_experiment();
    return NULL;
}

void Experiment::record_from_microphone() {
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
    durRecMax_sec = 10;
    sizeBuf = 2048;
    ttw = pow(10, 3) * sizeBuf / this->rate_mic; // in millisecond. 
    nbItPerSec = (this->rate_mic / sizeBuf) + 1;
    nbReadMax = durRecMax_sec * nbItPerSec;
    sizefullBuff = durRecMax_sec * this->rate_mic;

    buf = (short**) calloc(nbReadMax, sizeof(short *));
    for (ii = 0; ii < nbReadMax; ii++) {
        buf[ii] = (short*) calloc(sizeBuf, sizeof(short));
    }
    buffer.resize(1);
    buffer[0].resize(sizefullBuff);

    /* work */
    std::cout << "[record_from_microphone] READY..." << std::endl;
    while (!signal4stop_experiment())             // check if experiment is done
    {
        i = 0;
        signal4recording();  // wait for messaging the thread to start to record

        // open the microphone
        if ((err = snd_pcm_prepare(this->capture_handle)) < 0) {
            fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                    snd_strerror(err));
            exit(1);
        }
        if ((err = snd_pcm_start(this->capture_handle)) < 0) {
            fprintf(stderr, "start recording failed (%s)\n", snd_strerror(err));
            exit(1);
        }
        sleep_for(milliseconds(ttw)); // let some time for alsa to feed the buffer

        // record until the answer is given
        while (!signal4stop_recording() && i != nbReadMax) {
            clk_read_before = chrono::high_resolution_clock::now(); // get timer when the mic is opened    
            if ((err = snd_pcm_readi(this->capture_handle, buf[i++], sizeBuf))
                    < 0) {
                fprintf(stderr, "read from audio interface failed (%s)\n",
                        snd_strerror(err));
                exit(1);
            }

            clk_read_after = chrono::high_resolution_clock::now(); // get timer when the mic is opened    
            time_span = duration_cast<duration<double>>(
                    clk_read_after - clk_read_before).count();
            ttw_left = ttw - time_span * pow(10, 3);

            if (ttw_left > 0)
                sleep_for(milliseconds(ttw_left));
            else
                overruns += ttw_left;

        }
        // waiting for the answer if the answer has not been given previously
        if (i == nbReadMax) {
            std::cout << "[WARNING] Buffer full without asking to stop.."
                    << std::endl;
            while (!signal4stop_recording()) {
            }
        }

        // close the microphone
        if ((err = snd_pcm_drop(this->capture_handle)) < 0) {
            fprintf(stderr, "drop the microphone failed (%s)\n",
                    snd_strerror(err));
            exit(1);
        }

        // save the recorded sound
        buffer[0].resize(i * sizeBuf); // keep only the data that has been recorded
        for (ii = 0; ii < i; ii++) {
            for (j = 0; j < sizeBuf; j++) { // store the records into the big buffer object
                buffer[0][this->af_i++] = buf[ii][j];
            }
        }
        std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(),
                std::bind(std::divides<double>(), std::placeholders::_1,
                        SHRT_MAX));
        set_audioBuffer(buffer);            // fill Audiobuffer with tmp buffer 
    }

    /* leave */
    snd_pcm_close(this->capture_handle);
    std::cout << "[record_from_microphone] end of the function..." << std::endl;
    std::cout << "[record_from_microphone][OVERRUNS] Total overrruns="
            << overruns << " samples." << std::endl;
}

void Experiment::execute_stimuli() {

}

void Experiment::start_experiment() {

    // INITIALISATION
    this->ad->spi_open();            // open SPI port: init drive electronics
    this->ad->configure();            // configure actuators values of the pcb
    this->ad->execute_trajectory(alph->getneutral(), this->period_spi_ns); // reset to 0 the actuators

    // decide which experiment has to be executed
    switch (this->expToExec) {
    case BrailleDevSpace:
        execute_space_actuator();
        break;
    case BuzzerSpace:
        execute_space_actuator();
        break;
    case FingersSpace:
        execute_space_finger();
        break;
    case CalibrationWord:
        execute_calibration_word();
        break;
    case CalibrationERM:
        execute_calibration_ERM();
        break;
    default:
        std::cout
                << "[experiment][execute] NO more experiment available: id(expToExec)="
                << to_string(this->expToExec) << std::endl;
        break;
    }

    std::cout << "[main][experiment][executeActuator] end of the function..."
            << std::endl;
}

bool Experiment::execute_calibration_word() {
    using namespace std::this_thread;
    using namespace std::chrono;
    std::cout << "[experiment][executeActuator] start..." << std::endl;
    // environmental variables
    bool rect;            // 
    int i, j, nb_sequences, timeleft, randomttw, overruns;
    waveformLetter vvalues;
    /* output variables */
    msec_array_t vhrc;            // vector of high resolution clock
    msec_array_t timerDebug;    // vector of high resolution clock for debugging
    int answeri, confidencei;
    
    rect = false;
    overruns = 0;
    i = 0;
    timeleft = 0;
    nb_sequences = this->actuatorsAndWaveformIDs_sequences.size();
    std::vector<int> answerOrder = { 1, 2, 3, 4, 5, 6};    // id of the answers
    std::random_shuffle(answerOrder.begin(), answerOrder.end()); // shuffle the answers
    
    for (i = 0; i != vhrc.size(); i++) {
        vhrc[i] = tool_now(chrono::high_resolution_clock::now());
    }

    vvalues = tool_get_sequenceCalibration_space();

    /* work */
    sleep_for(milliseconds(50));                // let some time to open the mic
    tool_dispHeader(c->expstring(this->expToExec));
    cin.get();
    for (i = this->seq_start; i < nb_sequences; i++) {  // for the sequence i
        std::cout << "Push [ENTER] to start the next sequence. Say the number <"
                << answerOrder[i / 10] << ">" << std::endl;
        if (i != this->seq_start) std::cin.ignore();
        std::cin.get();
        std::cout << std::endl << "[main][Calibration] New sequence: [" << i
                << "/" << nb_sequences << "]" << std::endl;
        
        // initialisation
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        tool_randomWaiting(); // random waiting time to avoir any adapting rythm behavior during the exp

        // work
        start_recording();                    // start to record from microphone
        vhrc[0] = tool_now(this->c_start);          // get timing before stimuli
        //std::cout<<"[ACTUATOR] timer since start="<<vhrc[0].count()<<" ms"<<std::endl;
        overruns += this->ad->execute_selective_trajectory(vvalues,
                this->period_spi_ns); // execute the sequence
        this->ad->execute_trajectory(this->alph->getneutral(),
                this->period_spi_ns);        // all actuators to rest (security)
        
        vhrc[1] = tool_now(this->c_start);           // get timing after stimuli
        sleep_for(milliseconds(1));    // let some time to record the mic
        
        rect = read_answer(&answeri); // write the answer given by the participant
        sleep_for(milliseconds(1000));        // let some time to record the mic
        
        stop_recording();                      // stop to record from microphone
        
        // store and check the work
        if (rect)                                         // if 's' answer, exit
        {
            std::cout << "[EXIT] The experiment will be saved to i=" << i - 1
                    << "/" << nb_sequences << std::endl;
            break; // go to save
        }
        save_audio(i);                      // save audio file
        answer_timers.push_back(vhrc);      // save timers
        answer_values.push_back(answeri);   // save answer
        answer_confidences.push_back(4);    // save confidences

    }

    this->seq_end = i;
    stop_experiment();
    return false;
}

bool Experiment::execute_calibration_ERM() {
    using namespace std::this_thread;
    using namespace std::chrono;
    std::cout << "[experiment][executeActuator] start..." << std::endl;
    // environmental variables
    waveformLetter valuestmp;
    std::vector<int> waveformIDs;
    bool rect;
    int i, j, overruns, nb_sequences;
    /* output variables */
    msec_array_t vhrc;
    msec_array_t timerDebug;
    int answeri, confidencei;

    rect = false;
    i = 0;
    j = 0;
    overruns = 0;
    nb_sequences = this->actuatorsAndWaveformIDs_sequences.size();

    /* work */
    sleep_for(milliseconds(50)); // let some time to create, initialize, and open the mic
    tool_dispHeader(c->expstring(this->expToExec));

    cin.get();
    for (i = 0; i < nb_sequences; i++) {            // for the sequence i
        if (i == nb_sequences/4 || i == 2 * (nb_sequences/4)
                || i == 3 * (nb_sequences/4)) {
            std::cout << "------ BREAK IS NEEDED. Press [ENTER] to start again:"
                    << std::endl;
            cin.get();
        }
        // initialisation
        std::cout << std::endl << "[main] New sequence: [" << i << "/"
                << nb_sequences << "]" << std::endl;
        valuestmp = tool_get_sequenceWaveform_space(i); // set up the waveform corresponding to the sequence
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        tool_randomWaiting(); // random waiting time to avoir any adapting rythm behavior during the exp

        // work
        start_recording();                    // start to record from microphone
        vhrc[0] = tool_now(this->c_start);          // get timing before stimuli
        std::cout << "[ACTUATOR] timer since start=" << vhrc[0].count() << " ms"
                << std::endl;
        overruns += this->ad->execute_selective_trajectory(valuestmp,
                this->period_spi_ns);                    // execute the sequence
        this->ad->execute_trajectory(this->alph->getneutral(),
                this->period_spi_ns);        // all actuators to rest (security)
        vhrc[1] = tool_now(this->c_start);           // get timing after stimuli
        sleep_for(milliseconds(1));           // let some time to record the mic
        rect = read_answer(&answeri); // read input keyboard of the participant answer
        sleep_for(milliseconds(1000));        // let some time to record the mic
        stop_recording();                      // stop to record from microphone

        // store and check the work
        if (rect)                                         // if 's' answer, exit
        {
            std::cout << "[EXIT] The experiment will be saved to i=" << i - 1
                    << "/" << nb_sequences << std::endl;
            break; // go to save
        }
        read_confidence(&confidencei); // read input keyboard of the participant confidence

        save_audio(i);                              // save audio file
        answer_timers.push_back(vhrc);                    // save timers
        answer_values.push_back(answeri);                 // save answer
        answer_confidences.push_back(confidencei);         // save confidence
    }

    this->seq_end = i;
    stop_experiment();
    return false;
}

bool Experiment::execute_space_actuator() {
    using namespace std::this_thread;
    using namespace std::chrono;
    std::cout << "[experiment][executeActuator] start..." << std::endl;

    /* (1/5)[MEMORY ALLOCATION] */
    // environmental variables
    waveformLetter valuestmp;
    bool rect;
    int i, overruns, nb_sequences;
    // output variables
    msec_array_t vhrc;
    msec_array_t timerDebug;
    int answeri, confidencei;

    /* (2/5)[INITIALISATION] */
    rect = false;
    overruns = 0;
    i = 0;
    nb_sequences = this->actuatorsAndWaveformIDs_sequences.size();

    /* (3/5)[PRE-WORK] */
    sleep_for(milliseconds(50)); // let some time to open the mic
    tool_dispHeader(c->expstring(this->expToExec));
    cin.get();

    /* (4/5)[WORK] */
    for (i = this->seq_start; i < nb_sequences; i++) { // for the sequence i
        // display current sequence
        std::cout << std::endl;
        std::cout << "[main] New sequence: [" << i << "/" << nb_sequences
                << "]" << std::endl;

        // initialisation
        valuestmp = tool_get_sequenceWaveform_space(i); // set up the waveform corresponding to the sequence
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        tool_randomWaiting(); // random waiting time to avoir any adapting rythm behavior during the exp

        // work
        start_recording();                    // start to record from microphone
        vhrc[0] = tool_now(this->c_start);          // get timing before stimuli
        std::cout << "[ACTUATOR] timer since start=" << vhrc[0].count() << " ms"
                << std::endl;
        overruns += this->ad->execute_selective_trajectory(valuestmp,
                this->period_spi_ns); // execute the sequence
        this->ad->execute_trajectory(this->alph->getneutral(),
                this->period_spi_ns);        // all actuators to rest (security)
        vhrc[1] = tool_now(this->c_start);           // get timing after stimuli
        sleep_for(milliseconds(1));    // let some time to record the mic
        rect = read_answer(&answeri); // write the answer given by the participant
        sleep_for(milliseconds(1000));        // let some time to record the mic
        stop_recording();                      // stop to record from microphone

        // store and check the work
        if (rect)                                         // if 's' answer, exit
        {
            std::cout << "[EXIT] The experiment will be saved to i=" << i - 1
                    << "/" << nb_sequences << std::endl;
            break; // go to save
        }
        read_confidence(&confidencei);

        save_audio(i);                    // save audio file
        answer_timers.push_back(vhrc);        // save timers
        answer_values.push_back(answeri);        // save answer
        answer_confidences.push_back(confidencei); // save answer
    }

    /* (5/5)[POST-WORK] */
    this->seq_end = i;
    stop_experiment();
    return false;
}

bool Experiment::execute_space_finger() {
    using namespace std::this_thread;
    using namespace std::chrono;
    std::cout << "[experiment][executeActuator] start..." << std::endl;
    // environmental variables
    waveformLetter valuestmp;
    bool rect;
    int i, overruns, nb_sequences;
    /* output variables */
    msec_array_t vhrc;
    msec_array_t timerDebug;
    int answeri, confidencei;

    rect = false;
    overruns = 0;
    i = 0;
    nb_sequences = this->actuatorsAndWaveformIDs_sequences.size();

    /* work */
    sleep_for(milliseconds(50)); // let some time to open the mic
    tool_dispHeader(c->expstring(this->expToExec));
    cin.get();
    for (i = this->seq_start; i < nb_sequences; i++)   // for the sequence i
            {
        // initialisation
        std::cout << std::endl << "[main] New sequence: [" << i << "/"
                << nb_sequences << "]" << std::endl;
        //valuestmp = tool_get_sequenceWaveform_space(&i, values, &vhrc);            // set up the waveform corresponding to the sequence
        this->c_start = chrono::high_resolution_clock::now(); // https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
        tool_randomWaiting(); // random waiting time to avoir any adapting rythm behavior during the exp

        // work
        start_recording();                    // start to record from microphone
        vhrc[0] = tool_now(this->c_start);          // get timing before stimuli
        std::cout << "[ACTUATOR] timer since start=" << vhrc[0].count() << " ms"
                << std::endl;
        overruns += this->ad->execute_selective_trajectory(valuestmp,
                this->period_spi_ns); // execute the sequence
        this->ad->execute_trajectory(this->alph->getneutral(),
                this->period_spi_ns);        // all actuators to rest (security)
        vhrc[1] = tool_now(this->c_start);           // get timing after stimuli
        sleep_for(milliseconds(1));    // let some time to record the mic
        rect = read_answer(&answeri); // write the answer given by the participant
        sleep_for(milliseconds(1000));        // let some time to record the mic
        stop_recording();                      // stop to record from microphone

        // store and check the work
        if (rect)                                         // if 's' answer, exit
        {
            std::cout << "[EXIT] The experiment will be saved to i=" << i - 1
                    << "/" << nb_sequences << std::endl;
            break; // go to save
        }
        read_confidence(&confidencei);

        save_audio(i);                    // save audio file
        answer_timers.push_back(vhrc);        // save timers
        answer_values.push_back(answeri);        // save answer
        answer_confidences.push_back(confidencei); // save answer
    }

    this->seq_end = i;
    stop_experiment();
    return false;
}

/*------------------------------------------*/
/*                 2. PRIVATE               */
/*                 b. threads related       */
/*                                          */
/*------------------------------------------*/
bool Experiment::signal4recording() {
    std::unique_lock < std::mutex > mlock(this->m_mutex);
    // Start waiting for the Condition Variable to get signaled
    // Wait() will internally release the lock and make the thread to block
    // As soon as condition variable get signaled, resume the thread and
    // again acquire the lock. Then check if condition is met or not
    // If condition is met then continue else again go in waithis->t_record.
    this->m_condVar.wait(mlock,
            std::bind(&Experiment::is_recordingorworkdone, this));
    if (is_workdone()) {
        return false;
    } else
        return true;
}

bool Experiment::signal4stoprecording() {
    std::unique_lock < std::mutex > mlock(this->m_mutex);
    // Start waiting for the Condition Variable to get signaled
    // Wait() will internally release the lock and make the thread to block
    // As soon as condition variable get signaled, resume the thread and
    // again acquire the lock. Then check if condition is met or not
    // If condition is met then continue else again go in waithis->t_record.
    this->m_condVar.wait(mlock,
            std::bind(&Experiment::is_stopedrecording, this));
    return true;
}

bool Experiment::signal4stop_recording() {
    //std::cout<<"[stop_recording] in."<<std::endl;
    std::lock_guard < std::mutex > lk(this->m_mutex); // locker to access shared variables
    return !(this->recording);
}

bool Experiment::signal4stop_experiment() {
    //std::cout<<"[stop_recording] in."<<std::endl;
    std::lock_guard < std::mutex > lk(this->m_mutex); // locker to access shared variables
    return this->workdone;
}

void Experiment::start_recording() {
    //std::cout<<"[start_recording] in."<<std::endl;
    std::lock_guard < std::mutex > lk(this->m_mutex); // locker to access shared variables
    this->recording = true;      // start of the microphone recording boolean
    this->m_condVar.notify_one();                 // waiting thread is notified 
    //std::cout<<"[start_recording] out."<<std::endl;
}

void Experiment::stop_recording() {
    //std::cout<<"[stop_recording] in."<<std::endl;
    std::lock_guard < std::mutex > lk(this->m_mutex); // locker to access shared variables
    this->recording = false;     // start of the microphone recording boolean
    this->m_condVar.notify_one();                 // waiting thread is notified 
    //std::cout<<"[stop_recording] out."<<std::endl;
}

void Experiment::stop_experiment() {
    //std::cout<<"[stop_experiment] in."<<std::endl;
    std::lock_guard < std::mutex > lk(this->m_mutex); // locker to access shared variables
    this->workdone = true;          // start of the microphone recording boolean
    this->m_condVar.notify_one();                 // waiting thread is notified 
    //std::cout<<"[stop_experiment] out."<<std::endl;
}

/*------------------------------------------*/
/*                 2. PRIVATE               */
/*                 c. Answer related        */
/*                                          */
/*------------------------------------------*/
bool Experiment::read_answer(int * answeri) {
    bool ret = false;
    string ans;
    std::cout << "What was the answer? ['s' = save and quit]" << std::endl;
    cin >> ans;
    if (isdigit(ans[0])) {
        //std::cout<<"'"<<ans<<"' is a digit -> '"<<atoi(ans.c_str())<<"'"<<std::endl;
        *answeri = atoi(ans.c_str());
    } else if (0 == ans.compare("s") || 0 == ans.compare("\x18")
            || 0 == ans.compare("\x18s")) // if has to stop/pause the experiment
                    {
        ret = true;
    }

    //std::cout<<"The new answer is='"<<ans<<"'"<<std::endl;
    return ret;
}

//answer_confidences
bool Experiment::read_confidence(int * confidencei) {
    bool workdone = false;
    string ans;

    do {
        std::cout << "What is the confidence? (between 0-4)>" << std::endl;
        cin >> ans;
        // if it is a int 
        if (isdigit(ans[0])) {
            *confidencei = atoi(ans.c_str());
            // if respect the 0 to 4 value, return
            if (*confidencei >= 0 && *confidencei <= 4) {
                workdone = true;
            }
        }
    } while (!workdone);

    return false;
}
void Experiment::set_audioBuffer(AudioFile<double>::AudioBuffer buffer) {
    std::lock_guard < std::mutex > mlock(this->m_mutex);
    buffer[0].resize(this->af_i);   // keep only the data that has been recorded
    this->af->setNumSamplesPerChannel(buffer[0].size()); // resize the audioFile object
    this->af->setAudioBuffer(buffer);                      // add the new buffer
    this->af_i = 0;                            // reset the audioBuffer iterator
    this->audioBufferReady = true;
    this->m_condVar.notify_one();
}

void Experiment::save_audio(int id_seq) {
    std::unique_lock < std::mutex > mlock(this->m_mutex);
    this->m_condVar.wait(mlock,
            std::bind(&Experiment::is_audioBufferReady, this));
    string path(c->get_pathDirectory()); // create the name of the .wav with full path
    path += std::to_string(c->get_id()) + "/wav/";        // id of the candidate

    string name(std::to_string(c->get_id()) + "_"); // create the name of the .wav with full path
    name += c->expstring(this->expToExec) + "_";    // current experiment's name
    name += std::to_string(id_seq) + ".wav";               // id of the sequence
    this->af->save(path + name);       // save wav file of the sequence's answer
    this->audioBufferReady = false;
}

/*------------------------------------------*/
/*                 2. PRIVATE               */
/*                 d. checkers              */
/*                                          */
/*------------------------------------------*/
bool Experiment::is_recording() {
    return this->recording;
}
bool Experiment::is_stopedrecording() {
    return !(this->recording);
}
bool Experiment::is_workdone() {
    return this->workdone;
}
bool Experiment::is_recordingorworkdone() {
    return (is_workdone() || is_recording());
}
bool Experiment::is_audioBufferReady() {
    return this->audioBufferReady;
}

/*------------------------------------------*/
/*                 2. PRIVATE               */
/*                 e. Tools                 */
/*                                          */
/*------------------------------------------*/
waveformLetter Experiment::tool_get_sequenceWaveform_space(int sequence_id) {
    // output variable
    waveformLetter values_copy;
    // temporary variables
    std::pair<std::vector<int>,char> p;
    std::vector<int> actuator_on_off;
    std::vector<uint8_t> actuators_channel;
    waveformLetter::iterator it;
    int nb_actuators;

    p = this->actuatorsAndWaveformIDs_sequences[sequence_id];
    actuator_on_off = p.first;
    actuators_channel = {10, 9, 11, 14, 18, 19};  // actuator's ID (AD5383)
    nb_actuators = actuator_on_off.size();         // get the number of actuator

    values_copy = waveformIDsAndWF_map.find(p.second)->second;
    for (int a = 0; a != nb_actuators; a++) { // Transforms the values vector corresponding to the sequence
        if (actuator_on_off[a] == 0) {
            it = values_copy.find(actuators_channel[a]);
            values_copy.erase(it);
        }
    }

    return values_copy;
}


waveformLetter Experiment::tool_get_sequenceCalibration_space() {
    // output variable
    waveformLetter values_copy;
    // temporary variables
    std::pair<std::vector<int>,char> p;
    std::vector<uint8_t> actuators_channel;
    waveformLetter::iterator it;
    int nb_actuators;

    p = this->actuatorsAndWaveformIDs_sequences[0];
    actuators_channel = {10, 9, 11, 14, 18, 19};// actuator's ID (AD5383)
    nb_actuators = actuators_channel.size();      // get the number of actuator

    values_copy = waveformIDsAndWF_map.find(p.second)->second;
    for (int a = 1; a != nb_actuators; a++) { // Transforms the values vector corresponding to the sequence
        it = values_copy.find(actuators_channel[a]);
        values_copy.erase(it);
    }

    return values_copy;
}

int Experiment::tool_setup_captureHandle() {

    rate_mic = EXPERIMENT_SAMPLING_RATE;
    snd_pcm_hw_params_t *hw_params;
    int err = 0;

    if ((err = snd_pcm_open(&this->capture_handle, "default",
            SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "cannot open audio device %s (%s)\n", "default",
                snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror(err));
        return err;
    }
    // Fill params with a full configuration space for a PCM.
    if ((err = snd_pcm_hw_params_any(this->capture_handle, hw_params)) < 0) {
        fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror(err));
        return err;
    }
    // Mode de lecture sur le pulse code modulation
    if ((err = snd_pcm_hw_params_set_access(this->capture_handle, hw_params,
            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
        return err;
    }
    if ((err = snd_pcm_hw_params_set_format(this->capture_handle, hw_params,
            SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
        return err;
    }
    // snd_pcm_hw_params_set_rate_near
    if ((err = snd_pcm_hw_params_set_rate_near(this->capture_handle, hw_params,
            &rate_mic, 0)) < 0) {
        fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
        return err;
    }
    // Restrict a configuration space to contain only its minimum channels count.
    if ((err = snd_pcm_hw_params_set_channels(this->capture_handle, hw_params,
            1)) < 0) {
        fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
        return err;
    }
    // Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it.
    if ((err = snd_pcm_hw_params(this->capture_handle, hw_params)) < 0) {
        fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
        return err;
    }

    if (snd_pcm_hw_params_can_pause(hw_params) == 0) {
        printf("pause is not supported!!!\n");
    }

    snd_pcm_hw_params_free(hw_params);

    return err;
}

void Experiment::tool_randomWaiting() {
    /* Time to wait before executing the sequence */
    int timespent, timeleft, randomttw;
    randomttw = 250 + rand() % 2000; // randomize the time to wait between 1000-3000ms
    timespent = (int) (tool_now(this->c_start).count()); // now - (beginning of the loop for this sequence)
    timeleft = randomttw - timespent;
    std::cout << "wait for: " << timeleft << "(ms)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(timeleft)); // let some time for alsa to feed the buffer
}

msec_t Experiment::tool_now(highresclock_t start) {
    return chrono::duration<double, milli>(
            chrono::high_resolution_clock::now() - start);
}

void Experiment::tool_dispHeader(string s) {
    std::cout << "+----------------------------------------------+"
            << std::endl;
    std::cout << "+...                                        ...+"
            << std::endl;
    std::cout << "     Experiment: \t" << s << std::endl;
    std::cout << "+    Press [ENTER] to start the experiment     +"
            << std::endl;
    std::cout << "+...                                        ...+"
            << std::endl;
    std::cout << "+----------------------------------------------+"
            << std::endl;
}

void Experiment::tool_dispTimers(int sequence_id, int answeri, msec_array_t vhrc,
        msec_array_t timerDebug) {
    std::vector<int> actuators_on_off = this->actuatorsAndWaveformIDs_sequences[sequence_id].first; 
    
    std::cout << "[" << sequence_id << "][";
    for (int i = 0; i != actuators_on_off.size(); i++) {
        std::cout << actuators_on_off[i];
    }
    std::cout << "] " << "V=" << answeri << " | " << "start[stim]: "
            << (int) vhrc[0].count() << " | " << "end[stim]: "
            << (int) vhrc[1].count() << " | " << "(tmp:"
            << (int) timerDebug[0].count() << ")| " << "detection: "
            << (int) vhrc[2].count() << " | " << "aft_detection: "
            << (int) timerDebug[1].count() << " | " << "aft_recognition: "
            << (int) timerDebug[2].count() << "(ms)" << std::endl;
}

/* init/setup the actuatorsAndWaveformIDs_sequences variable before launching an experiment*/
void Experiment::tool_setup_actuatorsAndWaveformIDs_sequences(
        vector<vector<int>> actuatorIDs, vector<char> waveforms) {
    std::pair<std::vector<int>, char> p;
    int i, j;
    // vector of (actuators array, vvalues ID )

    this->actuatorsAndWaveformIDs_sequences.clear();
    this->actuatorsAndWaveformIDs_sequences.reserve(
            waveforms.size() * actuatorIDs.size());

    // create pairs of stimulis and waveforms sequences
    for (i = 0; i < waveforms.size(); i++) {
        for (j = 0; j < actuatorIDs.size(); j++) {
            p = std::make_pair(actuatorIDs[j], waveforms[i]);
            actuatorsAndWaveformIDs_sequences.push_back(p);
        }
    }
    // randomize the sequence's order
    std::random_shuffle(actuatorsAndWaveformIDs_sequences.begin(),
            actuatorsAndWaveformIDs_sequences.end());
}



void Experiment::tool_setup_waveformIDsAndWF_map(vector<char> waveforms) {
    this->waveformIDsAndWF_map.clear();
    for (int j = 0; j < waveforms.size(); j++) {
        this->waveformIDsAndWF_map[waveforms[j]] = this->alph->getl(waveforms[j]);
    }
}


