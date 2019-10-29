/*
 * subitation.cpp
 *
 *  Created on: 12 Apr. 2019
 *      Author: Basil Duvernoy
 */

#include <iostream>
#include <string>
#include <vector>

#include "AudioFile.h"


int main(int argc, char *argv[])
{   
	AudioFile<double> audioFile;
	bool loadedOK = audioFile.load("~/Documents/haptiComm/test.wav");
	
	
	return 0;
}







