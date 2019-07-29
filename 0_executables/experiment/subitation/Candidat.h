/*
 * Candidat.h
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil Duvernoy
 */

#include <algorithm>// std::random_shuffle
#include <array>    // std::array
#include <cstring>
#include <chrono>   // std::clock, std::chrono::high_resolution_clock::now
#include <bitset> 	// to check bit value on number
#include <dirent.h> // readdir
#include <iostream> // std::cin, std::cout
#include <sstream>  // std::istringstream
#include <fstream>  // std::ifstream
#include <map> 		// expEnums order variable
#include <math.h>   // pow
#include <utility>  // pair
#include <stdlib.h> // atoi, rand
#include <stdio.h>  // remove and rename files
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
using namespace std;

// boost library for vocal recognition
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

#ifndef CANDIDAT_H_
#define CANDIDAT_H_

#define SIZE_ARRAY_TIMERS 3

enum expEnum {
  BrailleDevSpace = 10,
  BrailleDevTemp = 11,
  FingersSpace = 20,
  FingersTemp = 21,
  BuzzerSpace = 30,
  BuzzerTemp = 31,
  CalibrationWord = 40,
  CalibrationERM = 50
};

typedef std::chrono::time_point<std::chrono::high_resolution_clock> highresclock_t;
typedef std::chrono::duration<double, milli> msec_t;
typedef array<msec_t, SIZE_ARRAY_TIMERS> msec_array_t;

class Candidat {
public:
  // Create the structure of the candidat
  Candidat(string _pathDict, string _pathDirectory, string _firstname,
      string _lastname);
  virtual ~Candidat();			// Destroy the candidat object
  bool exist();					// check if the candidat already exist
  bool create();					// create the candidat
  bool loadFromDB();			// load the candidat from the path directory variable
  bool is_nextExp();				// return if there is a next experiment
  //bool save_results(vector<std::array<msec_t, 3>>* timers, vector<int> * answers);
  bool save_results(std::vector<msec_array_t> * timers,
      std::vector<int> * answers, std::vector<int> * confidence,
      std::vector<std::vector<int>> * seq, std::vector<int> * identificationWAV,
      int * seq_start, int * seq_end);

  string expstring(expEnum ee);					// get string of an expEnum

  /* getters */
  expEnum get_nextExp();              // return the next experiment to execute
  int get_id();
  string get_firstname();
  string get_lastname();
  vector<pair<bool, expEnum>> get_expeOrder();
  vector<vector<int>> get_sequence();
  string get_pathDirectory();
  string get_pathDict();
  string get_langage();

private:
  /* common variables of the candidat */
  int id; 							// id of the candidat
  string firstname; 						// firstname of the candidat
  string lastname; 						// lastname of the candidat
  int age; 							// age of the candidat
  string gender; 					    // man or woman candidat

  /* general variables for making the use of some functions easier */
  int a, i, j, r, s; 							// classic iteration variables
  int nextit; 						// iteration variable of (??? more precisions needed)
  vector<vector<int>> apc; // sequence of All the Possible Combinaisons for the expEnums

  /* experiment variables */
  int nba; 								// NumBer of Actuators
  int nbr; 								// NumBer of Repetition by subgroup of stimuli
  vector<pair<bool, expEnum>> expeOrder; // the ordered expEnums and the bool statement aknowleding if it has been done already
  vector<vector<int>> seq; 				// SEQuence of the stimuli for the expEnums

  /* file information paths */
  string pathDirectory; 					// path of the main directory of candidat
  string candidatsListFile;
  string infoFile;

  /* dictionary related variables */
  string langage; 						// langage used by the candidat
  string pathDict; 						// path of the main dictionary directory

  /* setters */
  bool set_id(int _id);
  bool set_name(string fn, string ln);
  bool set_langage(string l);
  bool set_age(int _age);
  bool set_gender(string _t);
  bool set_pathDict(string p);
  bool set_pathDirectory(string p);

  /* initialisation */
  bool initexpVariables();
  bool initlangage();
  int read_age();
  string read_gender();
  bool initapc();
  bool initseq();
  bool set_expeOrder();
  bool saveInfo(); 						// save all initalised informations
  /* tools, files and directories modifiers */
  bool seteoe();
  bool fillcsvfile(std::string header, std::vector<std::vector<int>> values,
      int * line_start, int * line_end);
  expEnum str2expEnum(string eestr);
  bool copyDir(boost::filesystem::path const & source,
      boost::filesystem::path const & destination);

};

#endif /* CANDIDAT_H_ */

