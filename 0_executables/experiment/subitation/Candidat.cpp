/*
 * Candidat.cpp
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil
 */

#include "Candidat.h"

Candidat::Candidat(string _pathDict, string _pathDirectory, 
				   string _firstname, string _lastname)
	: nextit(0), nba(6), nbr(10), 
	  infoFile("info.txt"), 
	  candidatsListFile("candidatsList.txt")
{
	cout<<"[candidat] new"<<endl;
	set_pathDict(_pathDict);
	set_pathDirectory(_pathDirectory);
	set_name(_firstname, _lastname);
}


Candidat::~Candidat() {
	// TODO Auto-generated destructor stub
}


bool Candidat::exist()
{
	/* go to the participant list IDs */
	string file(this->pathDirectory + this->candidatsListFile);
	std::ifstream ifs(file.c_str());
	// if the file is not properly opened
	if (!ifs.is_open()) cerr<<"[candidat] The file"<<file<<"can't be loaded."<<endl;
	string line;
	int success = -1;
	// get the line corresponding to the candidats IDs
	while (success != 0 && getline(ifs, line))
	{
		success = line.compare("candidats:");
	}
	// if there is no line concerning candidats
	success == 0 ? cout<<"[candidat] Checking if the candidat exists..." :
		cerr<<"[candidat] The file"<<file<<"do not have \"candidat:\" lines."<<endl;


	/* check if the participant already exists */
	bool exist = false;
	string _firstname;
	string _lastname;
	string _id = "1";
	// checking if the current participant has been already created
	while (!exist && getline(ifs, line))
	{
		istringstream lineStream(line);
		getline(lineStream, _firstname, ' ');
		getline(lineStream, _lastname, ',');
		getline(lineStream, _id);

		if (_firstname.compare(this->firstname) == 0 
				&& _lastname.compare(this->lastname) == 0)
			exist = true;
	}
	ifs.close();

	// if the participant already exists
	if (exist)
	{
		this->set_id(stoi(_id));
		cout<<"[candidat] Exists in the database: id="<<to_string(this->id)<<endl;
	}
	else
	{
		cout<<"[candidat] Does not exist."<<endl;
	}


	return exist;
}


bool Candidat::create()
{
	bool err=false;
	/* get the next id for the candidat */
	struct dirent * dirp;
	int idMax = 0;
	DIR * dp;
	if((dp  = opendir(this->pathDirectory.c_str())) == NULL) {
		cout<<"[candidat] Error("<<errno<<") opening "<<this->pathDirectory<<endl;
		err=true;
	}
	// looping over all candidate files in the main directory
	while ((dirp = readdir(dp)) != NULL) 
	{
		// atoi() will automatically put '0' value if the current file is not an integer
		if (idMax<atoi(dirp->d_name))
			idMax = atoi(dirp->d_name);
		//cout<<"file: "<<string(dirp->d_name)<<" and its N value: "<<atoi(dirp->d_name)<<endl;
	}
	//cout<<"idmax= "<<idMax<<endl; 
	// set the id for the new candidate
	this->set_id(idMax+1);
	closedir(dp);

	/* initialize the sequence vector  */
	this->initexpVariables();

	/* add a line in the candidatList.txt for the new candidate */
	string clFile(this->pathDirectory + this->candidatsListFile);
	std::ofstream clfstream;
	// std::ios::app is the open mode "append" meaning
	// new data will be written to the end of the file.
	clfstream.open(clFile.c_str(), std::ios::app);
	string lineStr(this->firstname+' '+this->lastname+", "+to_string(this->id));
	// add the line 
	clfstream<<lineStr<<endl;
	clfstream.close();

	/* create the candidat directory and corresponding files */
	string candidatFolder(this->pathDirectory + to_string(this->id));
	// copy the model folder into the candidate directory
	string model(this->pathDirectory + "modelDir");
	this->copyDir(boost::filesystem::path(model), boost::filesystem::path(candidatFolder));
	//this->copyDir(boost::filesystem::path("/home/basil/haptiComm/results/subitation/modelDir"),
	//		boost::filesystem::path("/home/basil/haptiComm/results/subitation/15"));

	/* save all the informations into the infoFile */
	this->saveInfo();

	cout<<"[candidat] [SUCCESS] Candidate('"<<this->firstname<<' '<<this->lastname 
			<<"') has been created with its current folder named '" 
			<<to_string(this->id)<<"'."<<endl;

	return err;
}


bool Candidat::loadFromDB(){
	bool err = false;
	cout<<"[candidat][loadFromDB] Start..."<<endl;

	/* condition variables */
	string flname = "firstname,lastname";
	string lang   = "langage";
	string expord = "expEnum order";
	string expseq = "experimental sequences";
	bool nextis_name = false;
	bool nextis_lang = false;
	bool nextis_eord = false;
	bool nextis_eseq = false;

	/* reading variables */
	string line;
	string fileinstr(this->pathDirectory+to_string(this->id)+"/"+this->infoFile); // info.txt
	std::ifstream filein(fileinstr); //File to read from
	if (!filein)
	{
		cerr<<"[candidat][loadFromDB] "<<fileinstr<<": Error opening file"<<endl;
		err = true;
	}

	/* tmp output variables */
	string bstr;
	string eestr;
	bool b;
	expEnum ee;
	vector<int> v(6);

	while(getline(filein, line)) { // read the file
		if ( line.find_first_not_of(' ') == std::string::npos )// if empty line, going to another parameter
		{
			nextis_name = false;
			nextis_lang = false;
			nextis_eord = false;
			nextis_eseq = false;
			cout<<"empty line";
		}
		else if(nextis_eseq)
		{
			v[0] = stoi(line.substr(0,1));
			v[1] = stoi(line.substr(2,3));
			v[2] = stoi(line.substr(4,5));
			v[3] = stoi(line.substr(6,7));
			v[4] = stoi(line.substr(8,9));
			v[5] = stoi(line.substr(10,11));
			this->seq.push_back(v);
		}
		else if(nextis_name)
		{
			this->firstname = line.substr(0, line.find(","));
			this->lastname  = line.substr(line.find(",")+1);
			cout<<"firstname="<<firstname<<" and lastname="<<lastname<<endl;

		}
		else if(nextis_lang)
		{
			this->langage = line;
			cout<<"langage="<<langage<<endl;
		}
		else if(nextis_eord)
		{
			bstr  = line.substr(0, line.find(","));
			eestr = line.substr(line.find(",")+1);
			b=(bstr.compare("false")==0)?false:true;
			ee = this->str2expEnum(eestr);
			this->expeOrder.push_back( std::make_pair(b, ee) );
			cout<<"bool="<<bstr<<" and expEnum="<<eestr<<endl;
		}
		else if (line.find(flname) != string::npos) { nextis_name = true; cout<<"TRUE1" <<endl; }
		else if (line.find(lang)   != string::npos) { nextis_lang = true; cout<<"TRUE2" <<endl; }
		else if (line.find(expord) != string::npos) { nextis_eord = true; cout<<"TRUE3" <<endl; }
		else if (line.find(expseq) != string::npos) { nextis_eseq = true; cout<<"TRUE4" <<endl; }

		//cout<<line<<endl;
	}
	filein.close();

	cout<<"[candidat][loadFromDB] \t...End"<<endl;

	return err;
}


// Return true if there is another exp to do
bool Candidat::is_nextExp()
{
	bool found = false;

	for (vector<pair<bool, expEnum>>::iterator it = this->expeOrder.begin() ; it != this->expeOrder.end(); ++it)
	{
		if (it->first == false)
			found = true;
	}
	if (!found) { cerr<<"[candidat] No experiment left."<<endl; }
	return found; 
}


// return the next experiment to execute
expEnum Candidat::get_nextExp(){
	expEnum ee;
	bool found = false;

	for (vector<pair<bool, expEnum>>::iterator it = this->expeOrder.begin() ; it != this->expeOrder.end(); ++it)
	{
		if (it->first == false)
		{
			ee = it->second;
			found = true;
		}
	}

	if (found) { cout<<"[candidat] Next experiment is "<<expstring(ee)<<endl; }
	else { cerr<<"[candidat] No experiment left."<<endl; }

	return ee;
}


bool Candidat::save_results(std::vector<msec_array_t> * timers, 
						   std::vector<int> * answers, 
						   std::vector<int> * confidence, 
						   std::vector<std::vector<int>> * seqq, 
						   std::vector<int> * identificationWAV, 
						   int * seq_start, int * seq_end) {

  std::cout<<"[save_results] "<<std::flush;
	std::vector<std::vector<int>> results;
	std::string header;
	int nb_lines, nb_columns, nb_timers, curr_item;
	int l, t;
	int x=0;
	
	// header of the CSV file
	header = "id_seq";
	header += ",actuator1,actuator2,actuator3,actuator4,actuator5,actuator6";
	header += ",before_stimuli(ms),after_stimuli(ms),time_answer(ms)";
	if (answers->size() != 0) {
		header += ",value_answer(0:6)";
		nb_columns++;
	}
	if (confidence->size() != 0) {
		header += ",confidence(0:4)";
		nb_columns++;
	}
	if (identificationWAV->size() != 0) {
		header += ",identificationWAV";
		nb_columns++;
	}
  std::cout<<"initialisation, "<<std::flush;
	
	nb_timers = (timers->at(0)).size();
	nb_columns += (seqq->at(0)).size() + nb_timers + 1; // +1 for ID sequence slots
	nb_lines = timers->size();
	results.resize(nb_lines, std::vector<int>(nb_columns));
  std::cout<<"nb_lines="<<nb_lines<<", "<<std::flush;
	// for each lines
	for (l=0; l<nb_lines; l++, curr_item=0) {
		// id of the sequence
		results[l][curr_item++] = l;
	  std::cout<<"sequence="<<l<<", "<<std::flush;
		// value of the sequence
		for (t=0; t<(seqq->at(l)).size(); ++t) {
			results[l][curr_item++] = (seqq->at(l))[t];
		}
    std::cout<<"seqq, "<<std::flush;
		// for each timers
		for (t=0; t<nb_timers; ++t) {
			results[l][curr_item++] = (timers->at(l))[t].count();
		}
    std::cout<<"timers, "<<std::flush;
		// answer
		results[l][curr_item++] = answers->at(l);
    std::cout<<"answers="<<answers->at(l)<<", "<<std::flush;
		// confidence
		results[l][curr_item++] = confidence->at(l);
    std::cout<<"confidence="<<confidence->at(l)<<", "<<std::flush;
		// wav identification for 50, 100 or 150/200ms duration ERM
		results[l][curr_item] = identificationWAV->at(l);
    std::cout<<"identificationWAV="<<identificationWAV->at(l)<<", "<<std::flush;
	}
  std::cout<<"initialisation2, "<<std::flush;
	
	
	/* save the results in a csv file */
	fillcsvfile(header, results, seq_start, seq_end);

  std::cout<<"fillcsvfile, "<<std::flush;
	/* if seq_end = max, the experiment has been done */
	if (seqq->size() == *seq_end) {
		return seteoe();
	}
  std::cout<<"seteoe, "<<std::flush;

  std::cout<<std::endl;

	return false;
}




/*************
 * GETTER
 *************/
int 						  Candidat::get_id(){ return this->id; }
string 						Candidat::get_firstname(){ return this->firstname; }
string 						Candidat::get_lastname(){ return this->lastname; }
vector<pair<bool, expEnum>> Candidat::get_expeOrder(){ return this->expeOrder; }
vector<vector<int>> Candidat::get_sequence(){ return this->seq; }
string 						Candidat::get_pathDirectory(){ return this->pathDirectory; }
string 						Candidat::get_pathDict(){ return this->pathDict; }
string 						Candidat::get_langage(){ return this->langage; }


/*************
 * SETTER
 *************/
bool Candidat::set_id(int _id) {
	this->id = _id;
	return true;
}

bool Candidat::set_name(string fn, string ln) {
	this->firstname = fn;
	this->lastname = ln;
	return true;
}

bool Candidat::set_langage(string l) {
	bool ret = true;
	if (l.compare("fr-fr") == 0)
	{
		this->langage = l;
	}
	else if (l.compare("en-us") == 0)
	{
		this->langage = l;
	}
	else
	{
		ret = false;
		cerr<<"The langage <"<<l<<"> is not supported."<<endl;
		cerr<<"Try with 'fr-fr' or 'en-us'."<<endl;
	}
	
	return ret;
}

bool Candidat::set_age(int _age) {
	this->age = _age;
	return true;
}

bool Candidat::set_gender(string _t) {
	this->gender = _t;
	return true;	
}

bool Candidat::set_pathDict(string p) {
	this->pathDict = p;
	return true;
}

bool Candidat::set_pathDirectory(string p) {
	this->pathDirectory = p;
	return true;
}




/*************
 * INITIALISATION
 *************/
bool Candidat::initexpVariables() {
	cout<<"More informations are needed to create the candidat:"<<endl;
	this->initlangage();
    this->set_age(this->read_age());
    this->set_gender(this->read_gender());
	this->initapc();
	this->initseq();
	this->set_expeOrder();
	return true;
}

bool Candidat::initlangage() {
	string tmp = "fr-fr";
	//cout<<"Which is the langage to use? ('en-us' or 'fr-fr')"<<endl;
	//cin >> tmp;
	
	return set_langage(tmp);
}

int Candidat::read_age() {
	string tmp;
	
	cout<<"How old are you?"<<endl;
	cin >> tmp;
	
	return atoi(tmp.c_str());
}

string Candidat::read_gender() {
	string tmp;
	
	cout<<"Are you a man, a woman? ('h','m' or 'f', 'w')"<<endl;
	cin >> tmp;
	
	return tmp;
}

/* Initialise all the possible combinaisons of actuators */
bool Candidat::initapc() {
	int nbs = pow(2, this->nba) - 1; // (number of sequences) - (the '000000' sequence)
	cout<<"[candidat] initapc> Number of (sequence, motor) = ("<<nbs<<", "<<this->nba<<")"<<endl;
	for(int i=1; i<nbs+1; i++)
	{
		std::vector<int> tmp(this->nba); // copy elison
		for(int a=0; a<this->nba; a++)
		{
			tmp[a] = ((i & (1<<a)) >> a);
		}
		tmp.shrink_to_fit(); // shrink
		this->apc.emplace_back(std::move(tmp));
	}
	
	return true;
}

/* Initialise the sequences that will be used for the expEnums */
bool Candidat::initseq() {
	std::srand ( unsigned ( std::time(0) ) ); // initialise the rand and srand
	vector<int> idx; // index vector of the combinaisons used for a specific number of actuator
	int sum_of_elems; // sum of the actuator used during a sequence
	// for all the subgroup of stimuli ( 1 actuator used, 2 act used, ..., 6 act used)
	for(a=1; a<=nba; ++a)
	{
		idx.clear();
		// (2) idx = get_idxCombinaison(apc, a)
		// for all the sequences
		for(s=0; s<this->apc.size(); ++s)
		{
			sum_of_elems = 0;
			// for all the actuators inside the current sequence
			for (j=0; j<this->apc[0].size();++j)
			{
				//cout<<this->apc[s][j] ;
				sum_of_elems += this->apc[s][j];
			}
			// if the current sequence equals to the specific number of actuator
			if (sum_of_elems==a)
			{
				idx.push_back(s); // add the current index to the index table
			}
		}
		// (3) for nbr repeated of a subgroup sequence, get random sequence
		for(r=1; r<=nbr; ++r)
		{
			// ranIdx = randomise(combinaison, idx)
			// get a random sequence for a current subgroup of sequences (guided by the number of actuators)
			std::vector<int> tmp = this->apc[idx[rand() % idx.size()]];
			tmp.shrink_to_fit(); // shrink
			this->seq.emplace_back(std::move(tmp)); // push back(combinaison(idx), tmp vec)
		}
	}
	// randomize positions tmp vec into this->seq
	std::random_shuffle(this->seq.begin(), this->seq.end());
	
	return true;
}

bool Candidat::set_expeOrder(){
	expeOrder.push_back( std::make_pair(true, BrailleDevTemp) );
	expeOrder.push_back( std::make_pair(true, FingersTemp) );
	expeOrder.push_back( std::make_pair(true, BuzzerTemp) );
	
	expeOrder.push_back( std::make_pair(true, FingersSpace) );
	expeOrder.push_back( std::make_pair(false, BrailleDevSpace) );
	expeOrder.push_back( std::make_pair(false, BuzzerSpace) );
	
	std::random_shuffle(expeOrder.begin(), expeOrder.end());
	
	expeOrder.push_back( std::make_pair(true, CalibrationERM) );
	expeOrder.push_back( std::make_pair(false, CalibrationWord) );	// when writing and reading, it will be executed first
	
	return true;
}

bool Candidat::saveInfo(){
	//for (std::map<char,int>::iterator it=mymap.begin(); it!=mymap.end(); ++it)
	//    std::cout<<it->first<<" => "<<it->second<<'\n';
	string file(this->pathDirectory+to_string(this->id)+"/"+this->infoFile);
	cout<<"[candidat][saveInfo] writing "<<file<<"...";

	std::ofstream mf;
	mf.open(file);

	mf<<"firstname,lastname:"<<endl;
	mf<<this->firstname<<","<<this->lastname<<endl;
	mf<<endl;

	mf<<"age:"<<endl;
	mf<<this->age<<endl;
	mf<<endl;

	mf<<"type:"<<endl;
	mf<<this->gender<<endl;
	mf<<endl;

	mf<<"langage:"<<endl;
	mf<<this->langage<<endl;
	mf<<endl;

	mf<<"expEnum order:"<<endl;
	for (vector<pair<bool, expEnum>>::iterator it=this->expeOrder.begin(); it!=this->expeOrder.end(); ++it)
	{
		if (it->first == false)
			mf<<"false,";
		else
			mf<<"true,";

		mf<<expstring(it->second)<<endl;
	}
	mf<<endl;

	mf<<"experimental sequences:"<<endl;
	for(i=0; i<this->seq.size(); i++)
	{
		for(j=0; j<this->seq[0].size()-1; j++)
		{
			mf<<this->seq[i][j]<<",";
		}
		mf<<this->seq[i][j]<<endl;
	}
	mf<<endl;

	mf.close();

	cout<<"done."<<endl;
}


/*************
 * TOOLS
 *************/
bool Candidat::seteoe()
{
	string expname( this->expstring(this->get_nextExp()) ); // current experiment string
	string line;
	string expnew = "true," + expname;
	string filein(this->pathDirectory+to_string(this->id)+"/"+this->infoFile);
	string fileout(this->pathDirectory+to_string(this->id)+"/"+this->infoFile+".tmp");
	std::ifstream fin(filein); //File to read from
	std::ofstream fout(fileout); //Temporary file
	if (!fin)  { cerr<<"[candidat][save_results] "<<filein <<": Error opening file"<<endl; return true;}
	if (!fout) { cerr<<"[candidat][save_results] "<<fileout<<": Error opening file"<<endl; return true;}

	while(getline(fin, line)) { // fill the tmp file
		if (line.find(expname) != string::npos) // if the line of the current exp has been found
			fout<<expnew;
		else
			fout<<line;
		fout<<endl;
	}
	fin.close();
	fout.close();

	if (remove(filein.c_str()) != 0) { cerr<<"[candidat][save_results] "<<filein<<": Error deleting file"<<endl; }
	if (rename(fileout.c_str(), filein.c_str()) != 0) { cerr<<"[candidat][save_results] "<<fileout<<": Error renaming file"<<endl; }

	return false;
}


bool Candidat::fillcsvfile(std::string header, 
						   std::vector<std::vector<int>> values,
						   int * line_start, int * line_end){	
	// variables
	std::ofstream f_out;//Tmp file to write to
	std::ifstream f_in; //File to read from
	struct stat buffer;
	string expname, fname, tmpname, line;
	int ans_idx, cptline, i, j;
	bool is_new;
	
	// initialisation
	expname = this->expstring(this->get_nextExp()); // current experiment string
	fname = this->pathDirectory+to_string(this->id)+"/"+expname+".csv";
	tmpname = fname+".tmp";
	cptline = -1; // remove the header's count, start to -1
	is_new = (fopen(fname.c_str(), "r"))?false:true; // if new file
	try {
		f_out.open(tmpname);
	}
	catch (std::ifstream::failure e) {
		std::cerr << "[candidat][save_results] Exception opening "<<tmpname<<"file"<<std::endl;
	}
	
	// process
	if (is_new) { 	// header
		f_out<<header<<std::endl;
		cptline++;
	}
	else
	{ // copy from existing file
		try {
			f_in.open(fname);
		}
		catch (std::ifstream::failure e) {
			std::cerr << "[candidat][save_results] Exception opening "<<fname<<"file"<<std::endl;
		}
		while(getline(f_in, line)) { // fill the tmp file up to line_start lines
			f_out<<line<<std::endl;
			cptline++;
			if (cptline == *line_start){
				break;
			}
		}
	}
	while (cptline<*line_start) { // possible: gap between prev_nb_seq and seq_start
		f_out<<to_string(cptline++)<<",-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1"<<std::endl;
	}
	
	// for each lines, 
	for (i=0; i!=values.size(); ++i) {
		// sequence id
		f_out<<to_string(values[i][0]);
		std::cout<<"["<<i<<"]"<<values[i][0]<<std::flush;
		// for each values,
		for (j=1; j<values[i].size(); ++j) {
			f_out<<","<<to_string(values[i][j]);
			std::cout<<","<<values[i][j]<<std::flush;
		}
		f_out<<std::endl;
		std::cout<<std::endl;
	}
	
	// if there are lines after the ones freshly added
	while(getline(f_in, line)) {
		if (cptline<*line_end){ // skip the line
			cptline++;
		}
		else
		{
			f_out<<line<<std::endl;
		}
	}
	
	// close the file streams and
	// copy to main file and remove the tmp file
	f_out.close();
	if (!is_new) {
		f_in.close();
		if (remove(fname.c_str()) != 0) { 
			std::cerr<<"[candidat][save_results] "<<fname<<": Error deleting file"<<std::endl; 
		}
	}
	if (rename(tmpname.c_str(), fname.c_str()) != 0) {
		std::cerr<<"[candidat][save_results] "<<tmpname<<": Error renaming file"<<std::endl; 
	}
	std::cout<<"[candidat][save_results] Results in: "<<fname<<"...[SUCCES]"<<std::endl;
	
	return false;
}


string Candidat::expstring(expEnum ee)
{
	string ret;

	if(ee == BrailleDevSpace)
		ret = "BrailleDevSpace";
	else if(ee == BrailleDevTemp)
		ret = "BrailleDevTemp";
	else if(ee == FingersSpace)
		ret = "FingersSpace";
	else if(ee == FingersTemp)
		ret = "FingersTemp";
	else if(ee == BuzzerSpace)
		ret = "BuzzerSpace";
	else if(ee == BuzzerTemp)
		ret = "BuzzerTemp";
	else if(ee == CalibrationWord)
		ret = "CalibrationWord";
	else if(ee == CalibrationERM)
		ret = "CalibrationERM";
	else
		ret = "";
	
	return ret;
}


expEnum Candidat::str2expEnum(string eestr)
{
	expEnum ee;

	if (eestr.compare("BrailleDevSpace") == 0)
	{
		ee = BrailleDevSpace;
	}
	else if (eestr.compare("BrailleDevTemp") == 0)
	{
		ee = BrailleDevTemp;
	}
	else if (eestr.compare("FingersSpace") == 0)
	{
		ee = FingersSpace;
	}
	else if (eestr.compare("FingersTemp") == 0)
	{
		ee = FingersTemp;
	}
	else if (eestr.compare("BuzzerSpace") == 0)
	{
		ee = BuzzerSpace;
	}
	else if (eestr.compare("BuzzerTemp") == 0)
	{
		ee = BuzzerTemp;
	}
	else if (eestr.compare("CalibrationWord") == 0)
	{
		ee = CalibrationWord;
	}
	else if (eestr.compare("CalibrationERM") == 0)
	{
		ee = CalibrationERM;
	}
	else
	{
		cerr<<"[candidat][str2expEnum] String is not corresponding to a expEnum value."<<endl;
	}

	return ee;
}

bool Candidat::copyDir(boost::filesystem::path const & source,
			 boost::filesystem::path const & destination){
    namespace fs = boost::filesystem;
    
    try
    {
        // Check whether the function call is valid
        if(!fs::exists(source) || !fs::is_directory(source))
        {
            std::cerr<<"Source directory "<<source.string()
                     <<" does not exist or is not a directory."<<endl;
            return false;
        }
        if(fs::exists(destination))
        {
            std::cerr<<"Destination directory "<<destination.string()
                	 <<" already exists."<<endl;
            return false;
        }
        // Create the destination directory
        if(!fs::create_directory(destination))
        {
            std::cerr<<"Unable to create destination directory" 
            		 <<destination.string()<<endl;
            return false;
        }
    }
    catch(fs::filesystem_error const & e)
    {
        std::cerr<<e.what()<<endl;
        return false;
    }
    
    // Iterate through the source directory
    for(
        fs::directory_iterator file(source);
        file != fs::directory_iterator(); ++file
    )
    {
        try
        {
            fs::path current(file->path());
            if(fs::is_directory(current))
            {
                // Found directory: Recursion
                if(
                    !copyDir(
                        current,
                        destination / current.filename()
                    )
                )
                {
                    return false;
                }
            }
            else
            {
                // Found file: Copy
                fs::copy_file(
                    current,
                    destination / current.filename()
                );
            }
        }
        catch(fs::filesystem_error const & e)
        {
            std::cerr<<e.what()<<std::endl;
        }
    }
    
    return true;
}






