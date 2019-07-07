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

#include "adxl345.h"
using namespace std;


ADXL345::ADXL345() : _spi_fd(0), _gpio_cs_fd(0), _gpio_int2_fd(0), 
					_connected_spi(false), _connected_int2(false) {}

ADXL345::~ADXL345() {
	spi_close();
	gpio_close(_gpio_cs_fd);
}

bool ADXL345::enable_int2_wire(int pin){
	if( !(_gpio_int2_fd = gpio_open(pin, ADXL345_GPIO_IN)) ) return false;
	write_register(ADXL345_REG_INT_MAP, ADXL345_SELECT_INT_SINGLE_TAP);
	_connected_int2 = true;
	
	return true;
}

bool ADXL345::configure() {
	
	if(!_connected_spi)
	{
		errno = ENOTCONN;
        perror("configure/_connected_spi");
		return false;
	}

	if (read_register(ADXL345_REG_DEVID) != ADXL345_READBACK_DEVID)
	{
		errno = ENXIO;
        perror("configure/ADXL345_REG_DEVID");
		return false;
	}
	
	write_register(ADXL345_REG_POWER_CTL, 	ADXL345_SELECT_POWER_CTL_MEASUREMENT_MODE);
	write_register(ADXL345_REG_DATA_FORMAT, ADXL345_SELECT_DATA_FORMAT_16_G);
	
	write_register(ADXL345_REG_TAP_AXES, 	ADXL345_SELECT_TAP_AXES_ALL);
	write_register(ADXL345_REG_DUR, 		ADXL345_SELECT_DUR);
	write_register(ADXL345_REG_THRESH_TAP, 	ADXL345_SELECT_THRESH_TAP);
	write_register(ADXL345_REG_INT_ENABLE, 	ADXL345_SELECT_INT_SINGLE_TAP);
	
	
	return true;
}

bool ADXL345::spi_open(int cs_pin, 	  		  const char *port_name, 
					   uint8_t transfer_mode, uint8_t bit_justification, 
					   uint8_t bits_per_word, uint32_t max_speed) {
	// homemade chip select
	_gpio_cs_fd = gpio_open(cs_pin, ADXL345_GPIO_OUT);
	gpio_write(_gpio_cs_fd, ADXL345_GPIO_HIGH);
	/*
	 * open spi
	 */
    if (!(_spi_fd = open(port_name, O_RDWR)))
    {
        perror("spi_open/open");
        _spi_fd = 0;
        return false;
    }

    /*
     * spi mode
     */
    if (ioctl(_spi_fd, SPI_IOC_WR_MODE, &transfer_mode) != 0)
    {
        perror("spi_open/SPI_IOC_WR_MODE");
        _spi_fd = 0;
        return false;
    }
    
    if (ioctl(_spi_fd, SPI_IOC_RD_MODE, &transfer_mode) != 0)
	{
		perror("spi_open/SPI_IOC_RD_MODE");
		_spi_fd = 0;
		return false;
	}	

    /*
     * bit justification
     */
    if (ioctl(_spi_fd, SPI_IOC_WR_LSB_FIRST, &bit_justification) != 0)
    {
        perror("spi_open/SPI_IOC_WR_LSB_FIRST");
        _spi_fd = 0;
        return false;
    }
    
    if (ioctl(_spi_fd, SPI_IOC_RD_LSB_FIRST, &bit_justification) != 0)
    {
        perror("spi_open/SPI_IOC_RD_LSB_FIRST");
        _spi_fd = 0;
        return false;
    }

    /*
     * bits per word
     */
    if (ioctl(_spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) != 0)
    {
        perror("spi_open/SPI_IOC_RD_BITS_PER_WORD");
        _spi_fd = 0;
        return false;
    }

    if (ioctl(_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) != 0)
    {
        perror("spi_open/SPI_IOC_WR_BITS_PER_WORD");
        _spi_fd = 0;
        return false;
    }

    /*
     * max speed hz
     */
    if (ioctl(_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) != 0)
    {
        perror("spi_open/SPI_IOC_WR_MAX_SPEED_HZ");
        _spi_fd = 0;
        return false;
    }

    if (ioctl(_spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) != 0)
    {
        perror("spi_open/SPI_IOC_RD_MAX_SPEED_HZ");
        _spi_fd = 0;
        return false;
    }

    _connected_spi = true;

    return true;
}


bool ADXL345::spi_close() {
	if (!_connected_spi)
		return true;
	
    if(!_spi_fd || close(_spi_fd) != 0)
    {
        perror("spi_close/close");
        return false;
    }
    
    _connected_spi = false;
    return true;
}

uint16_t ADXL345::get_axis(uint8_t axis)
{
	uint8_t reg0;
	
	switch(axis)
	{
		case ADXL345_SELECT_X:
			reg0 = ADXL345_REG_DATAX0;
			break;
	
		case ADXL345_SELECT_Y:
			reg0 = ADXL345_REG_DATAY0;
			break;
	
		case ADXL345_SELECT_Z:
			reg0 = ADXL345_REG_DATAZ0;
			break;
	
		default:
			printf("get_axis/axis: This axis is not recognised.\n");
			return 0;
	}
	
	return (read_2registers(reg0) * ADXL345_MG2G_MULTIPLIER * ADXL345_GRAVITY_EARTH);
}

bool ADXL345::int_tap_detected_reg()
{
	return (read_register(ADXL345_REG_INT_SOURCE) & ADXL345_SELECT_INT_SINGLE_TAP);
}

bool ADXL345::int_tap_detected_wire()
{
	if(!_connected_int2)
	{
		errno = ENOTCONN;
        perror("int_tap_detected_wire/_connected_int2");
		return false;
	}
	
	int res = gpio_read(_gpio_int2_fd);
	std::cout<<"result is: "<<res<<std::endl;
	
	return false;
}

void ADXL345::write_register(uint8_t reg_name, uint8_t value)
{
	spi_xfer(ADXL345_SELECT_WRITE, ADXL345_SELECT_SB, reg_name, value);
}

uint8_t ADXL345::read_register(uint8_t reg_name)
{
	return spi_xfer(ADXL345_SELECT_READ, ADXL345_SELECT_SB, reg_name, 0xFF);
}

uint16_t ADXL345::read_2registers(uint8_t reg_name)
{
	spi_xfer8(ADXL345_SELECT_READ | ADXL345_SELECT_MB | reg_name, true, false);
    return spi_xfer8(0xFF, false, false) | (spi_xfer8(0xFF, false, true)<<8);
}

uint8_t* ADXL345::format_msg(uint8_t reg_read, uint8_t reg_mb, uint8_t reg_addr, uint8_t reg_data) {
    _out_buffer[0] = reg_read | reg_mb 	| reg_addr;
    _out_buffer[1] = reg_data;

    _in_buffer[0] = 0; 
    _in_buffer[1] = 0;

    return _out_buffer;
}


uint8_t ADXL345::spi_xfer(uint8_t reg_read, uint8_t reg_mb, uint8_t reg_addr, uint8_t reg_data) {
    format_msg(reg_read, reg_mb, reg_addr, reg_data);
    return spi_xfer();
}

uint8_t ADXL345::spi_xfer() {
    if(!_spi_fd)
        return 0;
    
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)_out_buffer,
		.rx_buf = (unsigned long)_in_buffer,
		
		.len = ARRAY_SIZE(_out_buffer),
		.speed_hz = ADXL345_SPI_SELECT_CLOCK_HZ,
				
		.delay_usecs = 0,
		.bits_per_word = ADXL345_SPI_SELECT_BPW,
		.cs_change = false
	};
    
    // home made cs
    if (gpio_write(_gpio_cs_fd, ADXL345_GPIO_LOW) == -1)
    {
    	perror("spi_xfer/gpio_write");
		return 0;
    }
    
    if(ioctl(_spi_fd, SPI_IOC_MESSAGE(1), &tr) == -1)
    {
        perror("spi_xfer/ioctl");
        return 0;
    }

    // home made cs
    if (gpio_write(_gpio_cs_fd, ADXL345_GPIO_HIGH) == -1)
	{
		perror("spi_xfer/gpio_write");
		return 0;
	}
		
    return _in_buffer[1];
}

uint8_t ADXL345::spi_xfer8(uint8_t data, bool start_transfer, bool stop_transfer) {
	uint8_t in_buffer = 0;
	
    if(!_spi_fd)
        return 0;
    
    struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)&data,
		.rx_buf = (unsigned long)&in_buffer,
		
		.len = 1,
		.speed_hz = ADXL345_SPI_SELECT_CLOCK_HZ,
				
		.delay_usecs = 0,
		.bits_per_word = ADXL345_SPI_SELECT_BPW,
		.cs_change = start_transfer
	};
    

    // home made cs
    if (start_transfer && gpio_write(_gpio_cs_fd, ADXL345_GPIO_LOW) == -1)
    {
    	perror("spi_xfer/gpio_write");
		return 0;
    }
    
    if(ioctl(_spi_fd, SPI_IOC_MESSAGE(1), &tr) == -1)
    {
        perror("spi_xfer/ioctl");
        return 0;
    }

    // home made cs
	if (stop_transfer && gpio_write(_gpio_cs_fd, ADXL345_GPIO_HIGH) == -1)
	{
		perror("spi_xfer/gpio_write");
		return 0;
	}
	
	
    return in_buffer;
}

void ADXL345::print_outinbuffer() {
	std::cout << "out : 0b " << std::bitset<8>(_out_buffer[0]) << " " << std::bitset<8>(_out_buffer[1]) << std::endl;
	std::cout << "in  : 0b " << std::bitset<8>( _in_buffer[0]) << " " << std::bitset<8>( _in_buffer[1]) << std::endl;
}


int ADXL345::gpio_open(int pin, int inout) {
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

bool ADXL345::gpio_close(int gpio_fd) {
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
int ADXL345::gpio_export(int pin) {
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

int ADXL345::gpio_direction(int pin, int dir) {
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

	if (-1 == write(fd, &s_directions_str[ADXL345_GPIO_IN == dir ? 0 : 3], ADXL345_GPIO_IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

int ADXL345::gpio_read(int gpio_fd) {
	char value_str[3];
	char value;
	if (-1 == gpio_fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(gpio_fd, &value, 1)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	return(value - '0');
//	return(atoi(value_str));
}

int ADXL345::gpio_write(int gpio_fd, int value) {
	static const char s_values_str[] = "01";

	if (1 != write(gpio_fd, &s_values_str[ADXL345_GPIO_LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	return(0);
}















