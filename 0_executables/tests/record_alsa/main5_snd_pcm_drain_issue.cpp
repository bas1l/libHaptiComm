
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


typedef std::chrono::duration<double, std::milli> td_msec;


snd_pcm_t * init_alsa(snd_pcm_t * capture_handle, unsigned int * exact_rate);

main (int argc, char *argv[])
{
	/* MEMORY ALLOCATION */
	int i, ii, j, err;
	unsigned int rate;
	int sizeBuf = 2048, sizefullBuff;
	snd_pcm_t *capture_handle;
	snd_pcm_status_t * pcm_state;
	snd_pcm_status_alloca(&pcm_state);
	
	int af_i, durationRecord_sec, nbItPerSec, nbRead, ttw;
	AudioFile<double> * AF;
	AudioFile<double>::AudioBuffer buffer;
	
	/* INITIALISATION VARIABLES */
	std::string wavname("/home/etudiant/Documents/haptiComm/libHaptiComm/results/test_alsa.wav"); // create the name of the .wav with full path
	durationRecord_sec = 2;
	rate 			= 44100;
	//capture_handle 	= init_alsa(capture_handle, &rate);
	snd_pcm_hw_params_t *hw_params;
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
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	/* Fill params with a full configuration space for a PCM. */		 
	if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
	/* Mode de lecture sur le pulse code modulation */
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
	/* snd_pcm_hw_params_set_rate_near */
	if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0)
	{
		fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
		exit (1);
	}
	/* Restrict a configuration space to contain only its minimum channels count. */
	if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
		exit (1);
	}
	/* Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it. */
	if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
		exit (1);
	}

	if (snd_pcm_hw_params_can_pause	(hw_params)	 == 0) {
		printf("pause is not supported!!!\n");
	}
	
			
	snd_pcm_hw_params_free (hw_params);
	/* Prepare PCM for use. */
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		exit (1);
	}
	// wav variables
	af_i 			= 0;
	sizefullBuff 	= rate*durationRecord_sec;
	AF = new AudioFile<double>();
	buffer.resize(1);
	buffer[0].resize (sizefullBuff);
	// loop variables
	nbItPerSec		= (rate/sizeBuf)+1;
	nbRead 			= durationRecord_sec*nbItPerSec;
	ttw 			= pow(10,6)*sizeBuf/rate; // in nanosecond. 
	short buf[nbRead][(int)(sizeBuf*1.5)];
	
	printf("[nbItPerSec = %i]\t\t[ttw = %i]\n", nbItPerSec, ttw);
	printf("[rate = %i]\t\t\t[saved_rate = %i]\n", rate, saved_rate);

	high_resolution_clock::time_point t5 = high_resolution_clock::now();
	high_resolution_clock::time_point t6 = high_resolution_clock::now();
	duration<double> time_span6 = duration_cast<duration<double>>(t6 - t5);
	high_resolution_clock::time_point t7 = high_resolution_clock::now();
	duration<double> time_span7 = duration_cast<duration<double>>(t7 - t6);
	printf("without duration_cast %f seconds.\n", time_span6.count());
	printf("with duration_cast %f seconds.\n", time_span7.count());

	/* RECORD FROM MICROPHONE*/
	if ((err = snd_pcm_start (capture_handle)) < 0) {
		fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	high_resolution_clock::time_point t9 = high_resolution_clock::now();
	//snd_pcm_prepare(pcm_handle);
	//snd_pcm_drop(capture_handle);
	usleep(ttw);
	
	printf("reading\n"); fflush(stdout);
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (i = 0; i <nbRead; ++i) {
		//printf("[i=%i]\t", i); fflush(stdout);
		
		high_resolution_clock::time_point t3 = high_resolution_clock::now();
		if ((err = snd_pcm_readi(capture_handle, buf[i], sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		
		//printf("err=%d\n", err);
		high_resolution_clock::time_point t4 = high_resolution_clock::now();
		duration<double> time_span4 = duration_cast<duration<double>>(t4 - t3);
		//printf("exec in loop %f seconds.\n", time_span4.count());
		
		usleep(ttw-time_span4.count()*pow(10,6));
	}
	snd_pcm_avail(capture_handle);
	if ((err = snd_pcm_drain(capture_handle)) < 0) {
		fprintf (stderr, "snd_pcm_drain failed (%s)\n", snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_status(capture_handle, pcm_state)) < 0) {
		fprintf (stderr, "snd_pcm_status_t failed (%s)\n", snd_strerror (err));
		exit (1);
	}
	printf("%lu samples left.\n", snd_pcm_status_get_avail(pcm_state));

	if( snd_pcm_status_get_state(pcm_state) == SND_PCM_STATE_SETUP )
	{
		printf("YO MAN\n");
	}
	else
	{
		printf("bye MAN\n");
	}
	
	while( snd_pcm_status_get_state(pcm_state) != SND_PCM_STATE_SETUP )
	{
		printf("%lu samples left.\n", snd_pcm_status_get_avail(pcm_state));
		
		if ((err = snd_pcm_readi(capture_handle, buf[i++], sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
	}
	
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	printf("It took me %f seconds.\n", time_span.count());
	
	duration<double> time_span9 = duration_cast<duration<double>>(t9 - t8);
	printf("It took for the initialisation %f seconds.\n", time_span9.count());
	
	for(ii=0; ii<i && af_i<sizefullBuff; ii++){
		for(j=0; j<sizeBuf && af_i<sizefullBuff; j++){				// store the records into the big buffer object
			buffer[0][af_i++] = buf[ii][j];
		}
	}
	
	
	std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), 
				   std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));
	buffer[0].resize(af_i);									// keep only the data that has been recorded

	/* SAVE INTO WAV */
	std::cout<<"AudioFile :: "<<std::endl;
	AF->setAudioBufferSize(1, sizefullBuff);				// init audioFile buffer
	AF->setSampleRate(rate);								// set sample rate
	AF->setBitDepth(16);									// set bit depth
	AF->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	AF->setAudioBuffer(buffer);								// add the new buffer
	AF->save(wavname);										// save wav file of the sequence's answer
	printf("saved.\n");
	
	
	
	
	
	
	
	
	
	/*
	
	t9 = high_resolution_clock::now();
	//snd_pcm_prepare(pcm_handle);
	//snd_pcm_drop(capture_handle);
	usleep(ttw);
	
	printf("reading\n"); fflush(stdout);
	t1 = high_resolution_clock::now();
	for (i = 0; i <nbRead; ++i) {
		high_resolution_clock::time_point t3 = high_resolution_clock::now();
		if ((err = snd_pcm_readi(capture_handle, buf[i], sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		usleep(ttw-pow(10,6)*duration_cast<duration<double>>(high_resolution_clock::now() - t3).count());
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
	
	/* SAVE INTO WAV *
	std::cout<<"AudioFile :: "<<std::endl;
	AF->setAudioBufferSize(1, sizefullBuff);				// init audioFile buffer
	AF->setSampleRate(rate);								// set sample rate
	AF->setBitDepth(16);									// set bit depth
	AF->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	AF->setAudioBuffer(buffer);								// add the new buffer
	AF->save(wavname);										// save wav file of the sequence's answer
	printf("saved.\n");
	
	
	
	*/
	
	
	

	snd_pcm_close(capture_handle);
	
	
	
	
	
	
	
	exit (0);
}



snd_pcm_t * init_alsa(snd_pcm_t * capture_handle, unsigned int * exact_rate)
{
	snd_pcm_hw_params_t *hw_params;
	int err;
		
		if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			//									  argv[1]
			//                                    "plughw:1,0"
			fprintf (stderr, "cannot open audio device %s (%s)\n", 
				"default",
				 snd_strerror (err));
			exit (1);
		}
		
		if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
			fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		
		/* Fill params with a full configuration space for a PCM. */		 
		if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
			fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		
		/* Mode de lecture sur le pulse code modulation */
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
		
		/* snd_pcm_hw_params_set_rate_near */
		unsigned int rate = *exact_rate;
		if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, exact_rate, 0)) < 0)
		{
			fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
			exit (1);
		}
		if (rate != *exact_rate)
		{
			fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n==> Using %d Hz instead.\n", rate, exact_rate);
		}

		/* Restrict a configuration space to contain only its minimum channels count. */
		if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
			fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
			exit (1);
		}

		/* Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it. */
		if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
			fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
			exit (1);
		}

		snd_pcm_hw_params_free (hw_params);

		/* Prepare PCM for use. */
		if ((err = snd_pcm_prepare (capture_handle)) < 0) {
			fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
			exit (1);
		}
	
	return capture_handle;
}












