
#ifndef ADXL345_H_
#define ADXL345_H_

#include <map>
#include <stdint.h>
#include <unistd.h>				//Needed for SPI port
#include <vector>
#include <linux/types.h>
#include <linux/spi/spidev.h>	//Needed for SPI port

#include "adxl345_defines.h"




class ADXL345
{
public:
    ADXL345();
    ~ADXL345();

    /**
     * @brief Initializes channels properties with default parameters
     */
    bool configure();

    /** Currently does not work
	 * @brief enable_int2_wire
     * @param pin
	 * @return true if the gpio has been opened with errors.
	 */	
    bool enable_int2_wire(int pin);
    /**
     * @brief spi_open
     * @param port_name
     * @param transfer_mode
     * @param bit_justification
     * @param bits_per_word
     * @param max_speed
     * @return
     */
    bool spi_open(int cs_pin = 25,									const char* port_name = "/dev/spidev0.0", 
    			  uint8_t transfer_mode = SPI_MODE_3,				uint8_t bit_justification = 0, 
				  uint8_t bits_per_word = ADXL345_SPI_SELECT_BPW, 	uint32_t max_speed = ADXL345_SPI_SELECT_CLOCK_HZ);
    
    /**
     * @brief spi_close
     * @return
     */
    bool spi_close();
    
    /**
     * @brief get_axis
     * @return the 16bits corresponding to the axis given in parameter (X, Y, Z)
     */
    uint16_t get_axis(uint8_t axis);
        
    /**
	 * @brief is_tap
	 * @return true if the acc have recorded a tap.
	 */	
    bool int_tap_detected_reg();

    /** Currently does not work
	 * @brief is_tap
	 * @return true if the acc have recorded a tap.
	 */	
    bool int_tap_detected_wire();
    

    
    void write_register(uint8_t reg_name, uint8_t value);
    
    uint8_t 	read_register(uint8_t reg_name);
    uint16_t 	read_2registers(uint8_t reg_name);

    void print_outinbuffer();
    
private:
    uint8_t* 	format_msg(uint8_t reg_read, uint8_t reg_mb, uint8_t reg_addr, uint8_t reg_data);
    uint8_t 	spi_xfer(uint8_t reg_read, uint8_t reg_mb, uint8_t reg_addr, uint8_t reg_data);
    uint8_t 	spi_xfer();
    uint8_t 	spi_xfer8(uint8_t data, bool start_transfer, bool stop_transfer);
    
    int gpio_open(int pin, int inout);
    bool gpio_close(int gpio_fd);
    int gpio_write(int gpio_fd, int value);
    int gpio_read(int gpio_fd);
    int gpio_direction(int pin, int dir);
    int gpio_export(int pin);
    
    uint8_t  _in_buffer[2];
    uint8_t _out_buffer[2];

    bool _connected_spi;
    bool _connected_int2;
    
    int _spi_fd;
    int _timer_fd;
    int _gpio_cs_fd;
    int _gpio_int2_fd;
    
};
#endif // ADXL345_H_
