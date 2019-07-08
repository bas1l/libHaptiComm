
#ifndef HAPTIBRAILLEDRIVER_H_
#define HAPTIBRAILLEDRIVER_H_

#include <math.h>
#include <map>
#include <stdint.h>
#include <unistd.h>				//Needed for SPI port
#include <vector>
#include <linux/types.h>

#include "haptiBrailleDriver_defines.h"




class HAPTIBRAILLEDRIVER
{
public:
    HAPTIBRAILLEDRIVER();
    ~HAPTIBRAILLEDRIVER();

    /**
     * @brief Initializes channels properties with default parameters
     */
    bool configure(std::vector<int> act_pins);
    
    bool executeSymbol(std::vector<uint8_t> act_pins, int duration_ms);
    bool executeSymbol(std::multimap<uint8_t,std::vector<uint16_t>> wfLetter, int duration_ms);
    
    bool executeSymbol6by6(std::vector<uint8_t> actChans, int duration_ms, int duration_between_symbol_ms);
    bool executeSymbol6by6(std::multimap<uint8_t,std::vector<uint16_t>> wfLetter, int duration_ms, int duration_between_symbol_ms);
    
    
private:
    
    int gpio_open(int pin, int inout);
    bool gpio_close(int gpio_fd);
    
    int gpio_write(int gpio_fd, int value);
    int gpio_direction(int pin, int dir);
    int gpio_export(int pin);
    
    std::vector<int> actuators_pins;
    std::vector<int> actuators_fd;
    int nb_actuators;
};
#endif // HAPTIBRAILLEDRIVER_H_
