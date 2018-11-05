#include "toolstap.h"


void 
print_fau(int * f, int * a, int * u)
{
    
    printw("\n");
    printw("f=%i, ", *f);
    printw("a=%i, ", *a);
    printw("u=%i", *u);
    refresh();
}


void 
print_instructions()
{
    //printw("---------------------------------------\n");
    //printw(" Apparent motion: variables controller\n");
    //printw("---------------------------------------\n");
    //printw("(0) When you are done, press '*' to Exit\n");
    //printw("(1) Press [ENTER] to start one apparent motion\n");
    //printw("(2) Press [V] to save the current variables\n");
    //printw("(3) Press [ARROW] to increase or decrease the value of the selected variable:\n");
    printw("\t [RIGHT]=+1");
    printw("\t [UP]=+100");
    printw("\t [LEFT]=-1");
    printw("\t [DOWN]=-100");
    printw("(3) Press [Q],[W],[E],[A],[S],[D],[Z] or [X] to choose a variable:\n");
    //printw("(4) Press [J] to reset the variable\n");
    //printw("(5) Press [L] to reset all the variables\n");

    printw("---------------------------------------\n");
    printw("\n");
    
}


void
usage()
{
    fprintf(stderr,
        "\n"
        "usage: demo <options>\n"
        "\n"
        "The <options> can be:\n"
        "  -h             Print this usage statement\n"
        "  -cfg <source>  Parse the specified configuration file\n"
        "  -scope <name>  Application scope in the configuration source\n"
        "\n"
        "A configuration <source> can be one of the following:\n"
        "  file.cfg       A configuration file\n"
        "  file#file.cfg  A configuration file\n"
        "  exec#<command> Output from executing the specified command\n\n");
    exit(1);
}


uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset)
{
	uint16_t * s;
	s = (uint16_t*) malloc(nsample * sizeof(uint16_t));
	
        // Suivant le nombre d'échantillons voulus :
	float incr = 2*M_PI/((float)nsample);
	for (int i=0; i<nsample; i++){
                //s[i] = sin(i*incr*freq +phase);
		s[i] = (uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset);
                //printw("%i,", s[i]);
	}
	return s;
}


std::vector<uint16_t> createsine_vector(int freq, int ampl, int offset, double phase, int nsample)
{
    std::vector<uint16_t> sinus;
    float incr = 2*M_PI/((float)nsample);
    
    for(int i=0; i < nsample; i++){
        sinus.push_back((uint16_t) floor(ampl * sin(i*incr*freq +phase) + offset));
    }
    
    return sinus;
}


std::vector<std::vector<uint16_t>> creatematrix(int nbsample, int value)
{
    std::vector<std::vector<uint16_t>> result(AD5383::num_channels);
    std::vector<uint16_t> onechan(nbsample, value);
    
    for(int c=0; c<AD5383::num_channels; c++)
    {
        result[c].assign(onechan.begin(), onechan.end());
    }
    
    return result;
}

