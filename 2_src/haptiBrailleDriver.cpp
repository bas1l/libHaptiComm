#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fcntl.h>			//Needed for SPI port
#include <unistd.h>
#include <string.h>
#include <bitset>
#include <stdexcept>
#include <sys/ioctl.h>		//Needed for SPI port
#include <sys/timerfd.h>

#include "haptiBrailleDriver.h"
using namespace std;


//int actuators_pins = {12, 13, 14, 16, 18, 22};
HAPTIBRAILLEDRIVER::HAPTIBRAILLEDRIVER(){}

HAPTIBRAILLEDRIVER::~HAPTIBRAILLEDRIVER() {}


bool HAPTIBRAILLEDRIVER::configure(std::vector<int> act_pins) {
	this->nb_actuators = act_pins.size();
	this->actuators_pins = act_pins;
	this->actuators_fd.resize(this->nb_actuators);
	
	for (int i=0; i<this->nb_actuators; i++)
	{
		if( !(this->actuators_fd[i] = gpio_open(this->actuators_pins[i], GPIO_OUT)) ) 
			return false;
	}  
	
	return true;
}


bool HAPTIBRAILLEDRIVER::executeSymbol(std::vector<int> act_pins, unsigned long long  duration_ns)
{
	int i, idx;
	std::vector<int>::iterator it, it2;
	struct timespec tim_model, tim_rem, handled_rem;
	
	i = 0;
	idx = 0;
	tim_model.tv_sec  = duration_ns / 1000000000;
	tim_model.tv_nsec = duration_ns % 1000000; // msec*10^6
	
	for (it=act_pins.begin(); it!=act_pins.end(); ++it)
        {
		it2 = std::find(actuators_pins.begin(), actuators_pins.end(), *it);
		idx = std::distance(actuators_pins.begin(), it2);
		if (gpio_write(actuators_fd[idx], GPIO_HIGH) == -1)
		{
			perror("spi_xfer/gpio_write");
			return 0;
		}
        }
	
	//std::cout<<"sleep..."<<std::endl;
	nanosleep(&tim_model, &handled_rem);
	//std::cout<<"awake..."<<std::endl;
	
	for (it=act_pins.begin(); it!=act_pins.end(); ++it)
        {
		it2 = std::find(actuators_pins.begin(), actuators_pins.end(), *it);
		idx = std::distance(actuators_pins.begin(), it2);
		if (gpio_write(actuators_fd[idx], GPIO_LOW) == -1)
		{
			perror("spi_xfer/gpio_write");
			return 0;
		}
        }
	//std::cout<<"end."<<std::endl;
	
	return false;
	
}
bool HAPTIBRAILLEDRIVER::executeSymbol(std::multimap<uint8_t,std::vector<uint16_t>> wfLetter, unsigned long long  duration_ns)
{
	int i, idx;
	std::multimap<uint8_t, std::vector<uint16_t>>::iterator it = wfLetter.begin();
	std::vector<int>::iterator it2;
	struct timespec tim_model, tim_rem, handled_rem;
	
	i = 0;
	idx = 0;
	tim_model.tv_sec = duration_ns % 1000000;
	tim_model.tv_nsec = duration_ns; // msec*10^6
	
	for (it=wfLetter.begin(); it!=wfLetter.end(); ++it)
        {
		it2 = std::find(actuators_pins.begin(), actuators_pins.end(), it->first);
		idx = std::distance(actuators_pins.begin(), it2);
		if (gpio_write(actuators_fd[idx], GPIO_HIGH) == -1)
		{
			perror("spi_xfer/gpio_write");
			return 0;
		}
        }
	
	//std::cout<<"sleep..."<<std::endl;
	nanosleep(&tim_model, &handled_rem);
	//std::cout<<"awake..."<<std::endl;
	
	for (it=wfLetter.begin(); it!=wfLetter.end(); ++it)
        {
		it2 = std::find(actuators_pins.begin(), actuators_pins.end(), it->first);
		idx = std::distance(actuators_pins.begin(), it2);
		if (gpio_write(actuators_fd[idx], GPIO_LOW) == -1)
		{
			perror("spi_xfer/gpio_write");
			return 0;
		}
        }
	//std::cout<<"end."<<std::endl;
	
	return false;
}

int HAPTIBRAILLEDRIVER::gpio_open(int pin, int inout) {
#define VALUE_MAX 30
	int gpio_fd = 0;
	
	if (-1 == gpio_export(pin))
		return false;
	
	if (-1 == gpio_direction(pin, inout))
		return false;
	
	char path[VALUE_MAX];

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	gpio_fd = open(path, O_WRONLY);
	if (-1 == gpio_fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return false;
	}
	
	return gpio_fd;
}

bool HAPTIBRAILLEDRIVER::gpio_close(int gpio_fd) {
	if (gpio_fd) close(gpio_fd);
	else return false;
	
	return true;
}

	




/*
 * Raspberry Pi GPIO example using sysfs interface.
 * Guillermo A. Amaral B. <g@maral.me>
 *
 * This file blinks GPIO 4 (P1-07) while reading GPIO 24 (P1_18).
 */
int HAPTIBRAILLEDRIVER::gpio_export(int pin) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

int HAPTIBRAILLEDRIVER::gpio_direction(int pin, int dir) {
#define DIRECTION_MAX 35
	static const char s_directions_str[]  = "in\0out";
	char path[DIRECTION_MAX];
	int fd;
	
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[GPIO_IN == dir ? 0 : 3], GPIO_IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}


int HAPTIBRAILLEDRIVER::gpio_write(int gpio_fd, int value) {
	static const char s_values_str[] = "01";

	if (1 != write(gpio_fd, &s_values_str[GPIO_LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	return(0);
}





