
#ifndef HAPTIBRAILLEDRIVER_H_
#define HAPTIBRAILLEDRIVER_H_

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
    
    bool executeSymbol(std::vector<int> act_pins, unsigned long long  duration_ns);
    bool executeSymbol(std::multimap<uint8_t,std::vector<uint16_t>> wfLetter, unsigned long long  duration_ns);
    
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
