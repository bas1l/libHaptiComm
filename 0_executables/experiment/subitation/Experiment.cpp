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
	
	/* INITIALISE VARIABLES */
	setlocale(LC_ALL, "");
	cfg->configure(cfgSource, dev, wf, alph);

	cout << "[experiment] create::...end" << endl;
	
	return true;
}


bool Experiment::execute()
{
	// init drive electronics
    //this->ad->spi_open();
    //this->ad->configure();
    
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
		return false;
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
    // input variables
    waveformLetter values = alph->getl(letter); // get the symbols data with all the actuator in action
    // output variables
    array<td_msec, 3> vhrc;
    string answer;
    
	for(i=0; i<this->seq.size(); i++) // for the sequence i
	{
		// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now
		c_start = chrono::high_resolution_clock::now();
		overruns += executeSequence(&i, values, durationRefresh_ns, &c_start, &vhrc); // copy of values is important (erase)
	    cout << "write the answer:";
		answer = "1";//cin >> answer;
		
		vhrc[2] = chrono::duration<double, milli>(chrono::high_resolution_clock::now()-c_start);
	    std::cout << "t1: " << vhrc[0].count() 
				  << "ms, t2: " << vhrc[1].count() 
				  << "ms, t3: " << vhrc[2].count() << "ms" << endl;
	    
	    // save the result
	    vvtimer.push_back(vhrc);
	    vanswer.push_back(stoi(answer));
	}

	cout << "[experiment][executeActuator] ...end" << endl;
	return true;
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



