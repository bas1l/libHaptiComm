
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
    HAPTIBRAILLEDRIVER(int nb_actuator);
    ~HAPTIBRAILLEDRIVER();

    /**
     * @brief Initializes channels properties with default parameters
     */
    bool configure(int _duration);
    
    bool 
    
private:
    
    int gpio_open(int pin, int inout);
    bool gpio_close(int gpio_fd);
    
    int gpio_write(int gpio_fd, int value);
    int gpio_direction(int pin, int dir);
    int gpio_export(int pin);
    
    int * actuators;
};
#endif // HAPTIBRAILLEDRIVER_H_
