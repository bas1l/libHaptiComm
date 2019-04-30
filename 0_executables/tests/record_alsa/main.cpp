
#include <algorithm>			// for max_element, transform
#include <alsa/asoundlib.h>
#include <chrono> 				// std::clock, std::chrono::high_resolution_clock::now
#include <functional>			// std::bind
#include <float.h> 
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 			// strncmp
#include <unistd.h> 			// sleep, usleep

#include "AudioFile.h"
	  

using namespace std::chrono;


typedef std::chrono::duration<double, std::milli> msec_t;


int init_captureHandle(snd_pcm_t ** capture_handle, unsigned int * exact_rate);

main (int argc, char *argv[])
{
	/* MEMORY ALLOCATION */
	int i, ii, j, err;
	unsigned int rate;
	int sizeBuf = 2048, sizefullBuff;
	snd_pcm_t *capture_handle;
	snd_pcm_status_t * pcm_state;
	snd_pcm_status_alloca(&pcm_state);
	
	int af_i, durationRecord_sec, nbItPerSec, nbRead, ttw, overruns;
	AudioFile<double> * af;
	AudioFile<double>::AudioBuffer buffer;
	
	// INITIALISATION VARIABLES
	// Prepare PCM for use.
	high_resolution_clock::time_point t8 = high_resolution_clock::now();
	if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", "default", snd_strerror (err));
		exit (1);
	}
	if ((err = init_captureHandle (&capture_handle, &rate)) < 0) {
		fprintf (stderr, "init_captureHandle failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	std::string wavname("/home/etudiant/Documents/haptiComm/libHaptiComm/results/test_alsa.wav"); // create the name of the .wav with full path
	durationRecord_sec 	= 2;
	rate 				= 16000;
	// wav variables
	af_i 			= 0;
	sizefullBuff 	= rate*durationRecord_sec;
	af = new AudioFile<double>();
	buffer.resize(1);
	buffer[0].resize (sizefullBuff);
	// loop variables
	nbItPerSec		= (rate/sizeBuf)+1;
	nbRead 			= durationRecord_sec*nbItPerSec;
	ttw 			= pow(10,6)*sizeBuf/rate; // in nanosecond. 
	overruns 		= 0;
	short buf[nbRead][(int)(sizeBuf*1.5)];
	
	printf("[nbItPerSec = %i]\t\t[ttw = %i]\n", nbItPerSec, ttw);
	printf("[rate = %i]\t\t\t\n", rate);
	
	
	// RECORD FROM MICROPHONE
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		exit (1);
	}
	if ((err = snd_pcm_start (capture_handle)) < 0) {
		fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	high_resolution_clock::time_point t9 = high_resolution_clock::now();
	usleep(ttw);
	
	printf("reading\n"); fflush(stdout);
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (i = 0; i <nbRead; ++i) {
		printf("[i=%i]\t", i); fflush(stdout);
		
		high_resolution_clock::time_point t3 = high_resolution_clock::now();
		if ((err = snd_pcm_readi(capture_handle, buf[i], sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		
		printf("err=%d\n", err);
		high_resolution_clock::time_point t4 = high_resolution_clock::now();
		duration<double> time_span4 = duration_cast<duration<double>>(t4 - t3);
		printf("exec in loop %f seconds.\n", time_span4.count());
		printf("time to wait = %f\n", ttw-time_span4.count()*pow(10,6));
		int ttw_left = ttw-time_span4.count()*pow(10,6);
		if (ttw_left>0) usleep(ttw_left);
		else overruns += ttw_left;
	}
	
	if ((err = snd_pcm_drop(capture_handle)) < 0) {
		fprintf (stderr, "snd_pcm_drop failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	
	
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	printf("It took me %f seconds.\n", time_span.count());
	
	duration<double> time_span9 = duration_cast<duration<double>>(t9 - t8);
	printf("It took for the initialisation %f seconds.\n", time_span9.count());
	
	for(ii=0; ii<=i && af_i<sizefullBuff; ii++){
		for(j=0; j<sizeBuf && af_i<sizefullBuff; j++){				// store the records into the big buffer object
			buffer[0][af_i++] = buf[ii][j];
		}
	}
	
	
	std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), 
				   std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));
	buffer[0].resize(af_i);									// keep only the data that has been recorded
	
	/* SAVE INTO WAV */
	std::cout<<"AudioFile :: "<<std::endl;
	af->setAudioBufferSize(1, sizefullBuff);				// init audioFile buffer
	af->setSampleRate(rate);								// set sample rate
	af->setBitDepth(16);									// set bit depth
	af->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	af->setAudioBuffer(buffer);								// add the new buffer
	af->save(wavname);										// save wav file of the sequence's answer
	printf("saved.\n");
	
	
	af_i=0;
	t9 = high_resolution_clock::now();
	
	/* Prepare PCM for use. */
	high_resolution_clock::time_point t10 = high_resolution_clock::now();
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		exit (1);
	}
	if ((err = snd_pcm_start (capture_handle)) < 0) {
		fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	high_resolution_clock::time_point t11 = high_resolution_clock::now();
	duration<double> time_span11 = duration_cast<duration<double>>(t11 - t10);
	printf("Time to prepare and start : %f seconds.\n", time_span11.count());
	
	usleep(ttw);
	printf("reading\n"); fflush(stdout);
	
	t1 = high_resolution_clock::now();
	for (i = 0; i <nbRead; ++i) {
		high_resolution_clock::time_point t3 = high_resolution_clock::now();
		if ((err = snd_pcm_readi(capture_handle, buf[i], sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		high_resolution_clock::time_point t4 = high_resolution_clock::now();
		duration<double> time_span4 = duration_cast<duration<double>>(t4 - t3);
		int ttw_left = ttw-time_span4.count()*pow(10,6);
		if (ttw_left>0) usleep(ttw_left);
		else overruns += ttw_left;
	}
	
	t2 = high_resolution_clock::now();
	time_span = duration_cast<duration<double>>(t2 - t1);
	printf("It took me %f seconds.\n", time_span.count());
	
	time_span9 = duration_cast<duration<double>>(t9 - t8);
	printf("It took for the initialisation %f seconds.\n", time_span9.count());
	
	for(i=0; i<nbRead && af_i<sizefullBuff; i++){
		for(j=0; j<sizeBuf && af_i<sizefullBuff; j++){				// store the records into the big buffer object
			buffer[0][af_i++] = buf[i][j];
		}
	}
	
	
	std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), 
				   std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));
	buffer[0].resize(af_i);									// keep only the data that has been recorded
	
	
	wavname = "/home/etudiant/Documents/haptiComm/libHaptiComm/results/test_alsa2.wav"; // create the name of the .wav with full path
	
	/* SAVE INTO WAV */
	std::cout<<"AudioFile :: "<<std::endl;
	af->setAudioBufferSize(1, sizefullBuff);				// init audioFile buffer
	af->setSampleRate(rate);								// set sample rate
	af->setBitDepth(16);									// set bit depth
	af->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	af->setAudioBuffer(buffer);								// add the new buffer
	af->save(wavname);										// save wav file of the sequence's answer
	printf("saved.\n");
	
	
	
	
	
	
	

	snd_pcm_close(capture_handle);
	
	
	
	
	
	
	
	exit (0);
}



int init_captureHandle(snd_pcm_t ** capture_handle, unsigned int * exact_rate)
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





void vestige()
{
	/*
	unsigned int saved_rate = rate;

		high_resolution_clock::time_point t8 = high_resolution_clock::now();
		if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			//									  argv[1]
			//                                    "plughw:1,0"
			fprintf (stderr, "cannot open audio device %s (%s)\n", 
				"default",
				 snd_strerror (err));
			exit (1);
		}
		snd_pcm_hw_params_t *hw_params;
		if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
			fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		// Fill params with a full configuration space for a PCM. 	 
		if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
			fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		// Mode de lecture sur le pulse code modulation 
		if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			fprintf (stderr, "cannot set access type (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		//if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_FLOAT64_LE)) < 0) {
			fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
			exit (1);
		}
		// snd_pcm_hw_params_set_rate_near
		if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0)
		{
			fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
			exit (1);
		}
		// Restrict a configuration space to contain only its minimum channels count. 
		if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
			fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
			exit (1);
		}
		// Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it. 
		if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
			fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
			exit (1);
		}

		if (snd_pcm_hw_params_can_pause	(hw_params)	 == 0) {
			printf("pause is not supported!!!\n");
		}
		
				
		snd_pcm_hw_params_free (hw_params);
		*/
		
}






