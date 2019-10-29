/*
 * subitation.cpp
 *
 *  Created on: 05 Apr. 2019
 *      Author: Basil Duvernoy
 */

#include <iostream>
#include <getopt.h>
#include <map>
#include <string>

#include "Candidat.h"
#include "Experiment.h"

void usage();
bool setOpt(int *argc, char *argv[], map<string, string> *options);


int main(int argc, char *argv[])
{   
	// Options
	map<string, string> options;
	setOpt(&argc, argv, &options);
	if (options.find("firstname")->second == "" || 
		options.find("lastname")->second == "" || 
		options.find("seqnumb")->second == "")
	{
		usage();
		return 1;
	}
	
	string firstname(options.find("firstname")->second);
	string lastname(options.find("lastname")->second);
	string dirdict(options.find("dirdict")->second);
	string direxp(options.find("direxp")->second);
	string cfgSource(options.find("cfg")->second);
	
	int seqnumb( stoi(options.find("seqnumb")->second) );
	
	/*
	for(int i=0; i<argc; i++)
	{
		cout << "argv[" << i << "] = '" << argv[i] << "'" << endl;
	}
	*/
	
	// create candidat
	Candidat c(dirdict, direxp, firstname, lastname);
	if(!c.exist()) 
	{
		if (c.create())
		{
			cerr << "Problem while creating the participant." << endl;
			cerr << "The program is ending..." << endl; 
			return 1;
		}
		else
		{
			cout << "The candidat has been successfully added." << endl;
			cout << "You can now launch the program again to execute an experiment" << endl;
			return 0;
		}
	}
	else if (c.loadFromDB())
	{
		cerr << "Problem while loading the participant." << endl;
		cerr << "The program is ending..." << endl; 
		return 1;
	}
	// check if the current user has another experiment to do
	if (!c.is_nextExp()) 
	{
		cerr << "No experiment left." << endl;
		cerr << "The program is ending..." << endl; 
		return 1;
	}
		
	// create experiment
	Experiment exp(cfgSource.c_str(), &c, seqnumb);
	
	if (exp.create()) 
	{ 
		cerr << "Issue during the creation of the experiment" << endl; 
		cerr << "The program is ending..." << endl; 
		return 1;
	}
	if (exp.execute()) 
	{
    cerr << "Issue during the execution of the experiment" << endl; 
		cerr << "The program is ending..." << endl; 
		return 1;
	}
	
	
	
  return 0;
}








void usage(){
	fprintf(stderr,
			"\n"
	    "usage: subitation <options>\n"
            "\n"
		"[REQUIRED] \n"
		"  -f <firstname>  first name of the candidat\n"
		"  -l <lastname>  last name of the candidat\n"
		"  -s <seq numb>  number ID of the sequence (between 0-60)\n"
	    "The <options> can be:\n"
	    "  -c <cfg path>  Parse the specified configuration file\n"
		"  -d <dir dict>  dictionnary directory\n"
		"  -e <exp dict>  experiment result directory\n"
	    "\n");
	exit(1);
	
}


/* 
 * getopt_long() refers to '-' hyphen for a single alphanumeric symbol option, '--' for a long option 
 * getopt_long_only() check '-' for long option first, if not found check a single alphanumeric symbol option
 */
bool setOpt(int *argc, char *argv[], map<string, string> * options)
{
	string cfg = "cfg";
	string dirdict = "dirdict";
	string direxp = "direxp";
	string firstname = "firstname";
	string lastname = "lastname";
	string seqnumb = "seqnumb";
	
	static struct option long_options[] =
	{
		{cfg.c_str(), 		required_argument, NULL, 'c'},
		{dirdict.c_str(),  	required_argument, NULL, 'd'},
		{direxp.c_str(),  	required_argument, NULL, 'e'},
		{firstname.c_str(), required_argument, NULL, 'f'},
		{lastname.c_str(),  required_argument, NULL, 'l'},
		{seqnumb.c_str(),  	required_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};

	// character of the option
	int c;
	
	// Detect the end of the options. 
	while ((c = getopt_long(*argc, argv, "c:d:e:f:l:", long_options, NULL)) != -1)
	{
		switch (c)
		{
			case 'c':
				options->insert( std::pair<string,string>(cfg, optarg));
				cout << "option -c with value <" << optarg << ">"<< endl;	
				break;

			case 'd':
				options->insert( std::pair<string,string>(dirdict,optarg));
				cout << "option -d with value <" << optarg << ">"<< endl;				
				break;
				
			case 'e':
				options->insert( std::pair<string,string>(direxp,optarg));
				cout << "option -e with value <" << optarg << ">"<< endl;				
				break;
				
			case 'f':
				options->insert( std::pair<string,string>(firstname,optarg));
				cout << "option -f with value <" << optarg << ">"<< endl;
				break;
			
			case 'l':
				options->insert( std::pair<string,string>(lastname,optarg));
				cout << "option -l with value <" << optarg << ">"<< endl;
				break;
				
			case 's':
				options->insert( std::pair<string,string>(seqnumb,optarg));
				cout << "option -s with value <" << optarg << ">"<< endl;
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







