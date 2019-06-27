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


HAPTIBRAILLEDRIVER::HAPTIBRAILLEDRIVER(int nb_actuator) : {
	actuators = 
}

HAPTIBRAILLEDRIVER::~HAPTIBRAILLEDRIVER() {
}


bool HAPTIBRAILLEDRIVER::configure() {
	
	if(!_connected_spi)
	{
		errno = ENOTCONN;
        perror("configure/_connected_spi");
		return false;
	}

	if (read_register(HAPTIBRAILLEDRIVER_REG_DEVID) != HAPTIBRAILLEDRIVER_READBACK_DEVID)
	{
		errno = ENXIO;
        perror("configure/HAPTIBRAILLEDRIVER_REG_DEVID");
		return false;
	}
	
	write_register(HAPTIBRAILLEDRIVER_REG_POWER_CTL, 	HAPTIBRAILLEDRIVER_SELECT_POWER_CTL_MEASUREMENT_MODE);
	write_register(HAPTIBRAILLEDRIVER_REG_DATA_FORMAT, HAPTIBRAILLEDRIVER_SELECT_DATA_FORMAT_16_G);
	
	write_register(HAPTIBRAILLEDRIVER_REG_TAP_AXES, 	HAPTIBRAILLEDRIVER_SELECT_TAP_AXES_ALL);
	write_register(HAPTIBRAILLEDRIVER_REG_DUR, 		HAPTIBRAILLEDRIVER_SELECT_DUR);
	write_register(HAPTIBRAILLEDRIVER_REG_THRESH_TAP, 	HAPTIBRAILLEDRIVER_SELECT_THRESH_TAP);
	write_register(HAPTIBRAILLEDRIVER_REG_INT_ENABLE, 	HAPTIBRAILLEDRIVER_SELECT_INT_SINGLE_TAP);
	
	
	return true;
}


uint8_t HAPTIBRAILLEDRIVER::spi_xfer() {
    if(!_spi_fd)
        return 0;
    
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)_out_buffer,
		.rx_buf = (unsigned long)_in_buffer,
		
		.len = ARRAY_SIZE(_out_buffer),
		.speed_hz = HAPTIBRAILLEDRIVER_SPI_SELECT_CLOCK_HZ,
				
		.delay_usecs = 0,
		.bits_per_word = HAPTIBRAILLEDRIVER_SPI_SELECT_BPW,
		.cs_change = false
	};
    
    
    if (gpio_write(_gpio_cs_fd, HAPTIBRAILLEDRIVER_GPIO_LOW) == -1)
    {
    	perror("spi_xfer/gpio_write");
		return 0;
    }
    
    if (gpio_write(_gpio_cs_fd, HAPTIBRAILLEDRIVER_GPIO_HIGH) == -1)
	{
		perror("spi_xfer/gpio_write");
		return 0;
	}
		
    return _in_buffer[1];
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
	if (-1 == _gpio_cs_fd) {
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

	if (-1 == write(fd, &s_directions_str[HAPTIBRAILLEDRIVER_GPIO_IN == dir ? 0 : 3], HAPTIBRAILLEDRIVER_GPIO_IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}


int HAPTIBRAILLEDRIVER::gpio_write(int gpio_fd, int value) {
	static const char s_values_str[] = "01";

	if (1 != write(gpio_fd, &s_values_str[HAPTIBRAILLEDRIVER_GPIO_LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	return(0);
}















