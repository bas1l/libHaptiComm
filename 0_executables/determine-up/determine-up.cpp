#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <vector> 
 
using namespace std;


uint16_t * create_sin(int freq, int ampl, int phase, int nsample, int offset);
void write_file(std::vector<uint16_t> values, int freq, int ampl, int upto);
std::vector<uint16_t> push_sine_wave_ret(int freq, int ampl, int offset);

int main(int argc, char *argv[])
{
    
    int freq[] = {1, 2, 5, 10, 20, 50, 100, 200, 300, 500, 700};
    int freq_max = sizeof(freq)/sizeof(int);
    
    int ampl[] = {50, 100, 150, 200, 250, 300, 400, 600, 800, 1000};
    int ampl_max = sizeof(ampl)/sizeof(int);
    
    int upto[] = {1000, 1500, 2048};
    int upto_max = sizeof(upto)/sizeof(int);
    std::cout << upto_max << ", " << freq_max << ", " << ampl_max << std::endl;
    
    int f;
    int u;
    int a;
    std::vector<uint16_t> v;
    
    for (int freq_id=0;freq_id<freq_max; freq_id++)
    {
        f = freq[freq_id];
        for (int ampl_id=0;ampl_id<ampl_max; ampl_id++)
        {
            a = ampl[ampl_id];
            for (int upto_id=0;upto_id<upto_max; upto_id++)
            {
                u = upto[upto_id];
                std::cout << "[begin]" << std::endl;
                std::cout << u << ", " << f << ", " << a << std::endl;
                v = push_sine_wave_ret(f, a, u);
                write_file(v, f, a, u);
                std::cout << "[end]" << std::endl;
            }
        }
    }
    
    return 0;
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

void write_file(std::vector<uint16_t> values, int freq, int ampl, int upto)
{
    std::string path = "determine-up/results/";
    std::string name =  path +
                        "up" + std::to_string(upto) + 
                        "f" + std::to_string(freq) +
                        "amp" + std::to_string(ampl) +
                        "_theoric.csv";
    ofstream fichier(name, ios::out | ios::trunc);  //déclaration du flux et ouverture du fichier

    if(fichier)  // si l'ouverture a réussi
    {
        // instructions
        for (int w=0; w<values.size(); ++w)
        {
            fichier << values[w];  // on l'affiche
            fichier << ", ";  // on l'affiche
        }
        fichier.close();  // on referme le fichier
    }
    else  // sinon
    {
        cerr << "Erreur a l'ouverture !" << endl;
    }
    
    
}


std::vector<uint16_t> push_sine_wave_ret(int freq, int ampl, int offset)
{
    int phase = 0;
    int nsample = 2000;
    std::vector<uint16_t> channels;//nothing, just a break.
    
    
    uint16_t * s = create_sin(freq, ampl, phase, nsample, offset);
    
    
    //printw("\nact=9,freq=%i,ampl=%i\n", freq, ampl);
    for(int i=0; i < nsample; i++){
        channels.push_back(s[i]);
        //printw("%i, ", s[i]);
    }
    channels.push_back(2048);
    
    return channels;
}

