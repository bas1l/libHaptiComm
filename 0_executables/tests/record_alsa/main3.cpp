
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


snd_pcm_t * init_alsa(snd_pcm_t * capture_handle, unsigned int * exact_rate);

main (int argc, char *argv[])
{
	int i, j, err;
	unsigned int rate;
	int sizeBuf = 2048, sizefullBuff;
	short buf[sizeBuf];
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	
	int af_i, durationRecord_sec, nbItPerSec, ttw;
	AudioFile<double> * AF;
	AudioFile<double>::AudioBuffer buffer;
	
	AF = new AudioFile<double>();
	buffer.resize(1);
	buffer[0].resize (sizefullBuff);
	
	std::string wavname("/home/etudiant/Documents/haptiComm/libHaptiComm/results/test_alsa.wav"); // create the name of the .wav with full path
	durationRecord_sec = 2;
	rate 			= 16000;
	capture_handle 	= init_alsa(capture_handle, &rate);
	af_i 			= 0;
	sizefullBuff 	= rate*durationRecord_sec;
	nbItPerSec		= (rate/sizeBuf)+1;
	ttw 			= pow(10,6)*sizeBuf/rate; // in nanosecond. 

	printf("[nbItPerSec = %i]\t\t[ttw = %i]\n", nbItPerSec, ttw);
	printf("[rate = %i]\t\t[sizefullBuff = %i]\n", rate, sizefullBuff);
	printf("reading\n"); fflush(stdout);
	for (i = 0; i <durationRecord_sec*nbItPerSec; ++i) {
		printf("[i=%i] ", i); fflush(stdout);
		
		if ((err = snd_pcm_readi (capture_handle, buf, sizeBuf)) < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
			exit (1);
		}
		for(j=0; j<err && af_i<sizefullBuff; j++){				// store the records into the big buffer object
			buffer[0][af_i++] = buf[j];
		}
		printf("err=%d\n", err);
		usleep(ttw);
	}
	
	std::transform(buffer[0].begin(), buffer[0].end(), buffer[0].begin(), 
				   std::bind(std::divides<double>(), std::placeholders::_1, SHRT_MAX));
	printf("stop, af_i=%d\n", af_i);
	snd_pcm_close(capture_handle);
	
	
	buffer[0].resize(af_i);									// keep only the data that has been recorded

	// init wav writer (AudioFile library)
	std::cout<<"AudioFile :: "<<std::endl;
	AF->setAudioBufferSize(1, sizefullBuff);				// init audioFile buffer
	AF->setSampleRate(rate);								// set sample rate
	AF->setBitDepth(16);									// set bit depth
	AF->setNumSamplesPerChannel(buffer[0].size());			// resize the audioFile object
	AF->setAudioBuffer(buffer);								// add the new buffer
	AF->save(wavname);										// save wav file of the sequence's answer
	printf("saved.\n");
	
	
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












