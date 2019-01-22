/*
 * subitation.cpp
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil Duvernoy
 */

// C++ program to create a directory in Linux
#include <iostream>
#include <getopt.h>
#include <map>
#include <string>

#include "Candidat.h"
#include "Experiment.h"
using namespace std;

bool setOpt(int *argc, char *argv[], map<string, string> *options);
/*
 * Argument 1 = Name of the participant
 *
 *
 */
int main(int argc, char *argv[])
{   
	map<string, string> options;
	setOpt(&argc, argv, &options);
	
	string firstname(options.find("firstname")->second);
	string lastname(options.find("lastname")->second);
	string expDirectory(options.find("expdir")->second);
	Candidat c(expDirectory, firstname, lastname);
	
	if(!c.exist()) 
		c.create();
	
	//c.launchNextExp();
	
    return 1;
}


/* getopt_long() refers to '-' hyphen for a single alphanumeric symbol option, '--' for a long option 
 * getopt_long_only() check '-' for long option first, if not found check a single alphanumeric symbol option
 */
bool setOpt(int *argc, char *argv[], map<string, string> * options)
{
	static struct option long_options[] =
	{
		{"cfg", 		required_argument, NULL, 'c'},
		{"expdir",  	required_argument, NULL, 'e'},
		{"firstname",  	required_argument, NULL, 'f'},
		{"lastname",    required_argument, NULL, 'l'},
		{NULL, 0, NULL, 0}
	};

	// character of the option
	int c;
	
	// Detect the end of the options. 
	while ((c = getopt_long(*argc, argv, "c:e:f:l:", long_options, NULL)) != -1)
	{
		switch (c)
		{
			case 'c':
				options->insert( std::pair<string,string>("cfg", optarg));
				cout << "option -c with value " << optarg << endl;	
				break;

			case 'e':
				options->insert( std::pair<string,string>("expdir",optarg));
				cout << "option -e with value " << optarg << endl;				
				break;
				
			case 'f':
				options->insert( std::pair<string,string>("firstname",optarg));
				cout << "option -f with value " << optarg << endl;
				break;
			
			case 'l':
				options->insert( std::pair<string,string>("lastname",optarg));
				cout << "option -l with value " << optarg << endl;
				break;
				
			case '?':
				/* getopt_long already printed an error message. */
				break;
			
			default:
				abort();
		}
	}
	
	
	/* Print any remaining command line arguments (not options). */
	if (optind < *argc)
	{
		printf ("non-option ARGV-elements: ");
		while (optind < *argc)
			printf ("%s ", argv[optind++]);
		putchar ('\n');
	}
	
	return true;
}










