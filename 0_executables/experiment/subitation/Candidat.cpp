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
	cout << "[candidat] new" << endl;
	setPathDict(_pathDict);
	setPathDirectory(_pathDirectory);
	setName(_firstname, _lastname);
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
	if (!ifs.is_open()) cerr << "[candidat] The file" << file << "can't be loaded." << endl;
	string line;
	int success = -1;
	// get the line corresponding to the candidats IDs
	while (success != 0 && getline(ifs, line))
	{
		success = line.compare("candidats:");
	}
	// if there is no line concerning candidats
	success == 0 ? cout << "[candidat] Checking if the candidat exists..." :
		cerr << "[candidat] The file" << file << "do not have \"candidat:\" lines." << endl;
	
	
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
		this->setId(stoi(_id));
		cout << "[candidat] Exists in the database: id=" << to_string(this->id) << endl;
	}
	else
	{
		cout << "[candidat] Does not exist." << endl;
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
		cout << "[candidat] Error(" << errno << ") opening " << this->pathDirectory << endl;
		err=true;
	}
	// looping over all candidate files in the main directory
	while ((dirp = readdir(dp)) != NULL) 
	{
		// atoi() will automatically put '0' value if the current file is not an integer
		if (idMax<atoi(dirp->d_name))
			idMax = atoi(dirp->d_name);
		//cout << "file: " << string(dirp->d_name) << " and its N value: " << atoi(dirp->d_name) << endl;
	}
	//cout << "idmax= " << idMax << endl; 
	// set the id for the new candidate
	this->setId(idMax+1);
	closedir(dp);
	
	
	/* add a line in the candidatList.txt for the new candidate */
	string clFile(this->pathDirectory + this->candidatsListFile);
	std::ofstream clfstream;
	// std::ios::app is the open mode "append" meaning
	// new data will be written to the end of the file.
	clfstream.open(clFile.c_str(), std::ios::app);
	string lineStr(this->firstname+' '+this->lastname+", "+to_string(this->id));
	// add the line 
	clfstream << lineStr << endl;
	clfstream.close();
	
	
	/* create the candidat directory and corresponding files */
	string candidatFolder(this->pathDirectory + to_string(this->id));
	// copy the model folder into the candidate directory
	string model(this->pathDirectory + "modelDir");
	this->copyDir(boost::filesystem::path(model), boost::filesystem::path(candidatFolder));
	//this->copyDir(boost::filesystem::path("/home/basil/haptiComm/results/subitation/modelDir"),
	//		boost::filesystem::path("/home/basil/haptiComm/results/subitation/15"));

	/* initialize the sequence vector  */
	this->initexpVariables();
	
	/* save all the informations into the infoFile */
	this->saveInfo();
	
	cout << "[candidat] [SUCCESS] Candidate('" << this->firstname << ' ' << this->lastname 
	 << "') has been created with its current folder named '" 
	 << to_string(this->id) << "'." << endl;
	 
	return err;
}



bool Candidat::loadFromDB(){
	bool err = false;
	cout << "[candidat][loadFromDB] Start..." << endl;
	
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
		cerr << "[candidat][loadFromDB] " << fileinstr  << ": Error opening file" << endl;
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
			cout << "empty line";
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
			cout << "firstname=" << firstname << " and lastname=" << lastname << endl;
					
		}
		else if(nextis_lang)
		{
			this->langage = line;
			cout << "langage=" << langage << endl;
		}
		else if(nextis_eord)
		{
			bstr  = line.substr(0, line.find(","));
			eestr = line.substr(line.find(",")+1);
			b=(bstr.compare("false")==0)?false:true;
			ee = this->str2expEnum(eestr);
			this->expeOrder.push_back( std::make_pair(b, ee) );
			cout << "bool=" << bstr << " and expEnum=" << eestr << endl;
		}
		else if (line.find(flname) != string::npos) { nextis_name = true; cout << "TRUE1" <<endl; }
		else if (line.find(lang)   != string::npos) { nextis_lang = true; cout << "TRUE2" <<endl; }
		else if (line.find(expord) != string::npos) { nextis_eord = true; cout << "TRUE3" <<endl; }
		else if (line.find(expseq) != string::npos) { nextis_eseq = true; cout << "TRUE4" <<endl; }
		
		//cout << line << endl;
	}
	filein.close();
	
	cout << "[candidat][loadFromDB] \t...End" << endl;
	
	return err;
}


// Return true if there is another exp to do
bool Candidat::isNextExp()
{
	bool found = false;
		
	for (vector<pair<bool, expEnum>>::iterator it = this->expeOrder.begin() ; it != this->expeOrder.end(); ++it)
	{
		if (it->first == false)
			found = true;
	}
	
	if (!found) { cerr << "[candidat] No experiment left." << endl; }
	
	return found; 
}

// return the next experiment to execute
expEnum Candidat::nextExp(){
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
	
	if (found) { cout << "[candidat] Next experiment is " << expstring(ee) << endl; }
	else { cerr << "[candidat] No experiment left." << endl;}
	
	return ee; 
}



bool Candidat::saveResults(vector<std::array<td_msec, 3>>* timers, vector<int> * answers){
	/* save the results into the corresponding file (current experiment) */
	string expstr = this->expstring(this->nextExp()); // current experiment string
	string file(this->pathDirectory+to_string(this->id)+"/"+expstr+".csv");
	cout << "[candidat][saveResults] writing results into: " << file << endl;
	std::ofstream mf(file);
	if(!mf) { cerr << "[candidat][saveResults] " << file << ": Error opening file" << endl; }
		
	mf << "actuator1,actuator2,actuator3,actuator4,actuator5,actuator6,";
	mf << "time1,time2,time3,";
	mf << "answer";
	mf << endl;
	for (int i=0; i<this->seq.size(); ++i)
	{
		for (int j=0; j<this->seq[i].size(); ++j)
		{
			mf << to_string(this->seq[i][j]) << ",";
		}
		mf << to_string((*timers)[i][0].count()) << "," << to_string((*timers)[i][1].count()) << "," << to_string((*timers)[i][2].count()) << ",";
		mf << to_string((*answers)[i]);
		mf << endl;
	}
	mf.close();
	
	
	/* The current experiment becomes true (info.txt), meaning that the experiment has been done */
	string line;
	string expnew = "true," + expstr;
	string fileinstr(this->pathDirectory+to_string(this->id)+"/"+this->infoFile);
	string fileoutstr(this->pathDirectory+to_string(this->id)+"/"+this->infoFile+".tmp");
	std::ifstream filein(fileinstr); //File to read from
	std::ofstream fileout(fileoutstr); //Temporary file
	if (!filein)  { cerr << "[candidat][saveResults] " << fileinstr  << ": Error opening file" << endl; return true;}
	if (!fileout) { cerr << "[candidat][saveResults] " << fileoutstr << ": Error opening file" << endl; return true;}
	
	while(getline(filein, line)) { // fill the tmp file
		if (line.find(expstr) != string::npos) // if the line of the current exp has been found
			fileout << expnew;
		else
			fileout << line;
		fileout << endl;
	}
	filein.close();
	fileout.close();
	
	if (remove(fileinstr.c_str()) != 0) { cerr << "[candidat][saveResults] " << fileinstr << ": Error deleting file" << endl; }
	if (rename(fileoutstr.c_str(), fileinstr.c_str()) != 0) { cerr << "[candidat][saveResults] " << fileinstr << ": Error renaming file" << endl; }
	
	
	return false;
}




/*************
 * GETTER
 *************/
int Candidat::getId(){
	return this->id;
}

string Candidat::getFirstname(){
	return this->firstname;
}

string Candidat::getLastname(){
	return this->lastname;
}

vector<pair<bool, expEnum>> Candidat::getExpeOrder(){
	return this->expeOrder;
}

vector<vector<int>> Candidat::getSequence(){
	return this->seq;
}

string Candidat::getPathDirectory(){
	return this->pathDirectory;
}
string Candidat::getPathDict(){
	return this->pathDict;
}
string Candidat::getLangage(){
	return this->langage;
}
	



/*************
 * SETTER
 *************/
bool Candidat::setId(int _id){
	this->id = _id;
	return true;
}

bool Candidat::setName(string fn, string ln){
	this->firstname = fn;
	this->lastname = ln;
	return true;
}

bool Candidat::setlangage(string l){
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
		cerr << "The langage <" << l << "> is not supported." << endl;
		cerr << "Try with 'fr-fr' or 'en-us'." << endl;
	}
	
	return ret;
}

bool Candidat::setExpeOrder(){

	return true;
}
bool Candidat::setSequence(){

	return true;
}
bool Candidat::setPathDict(string p){
	this->pathDict = p;
	return true;
}
bool Candidat::setPathDirectory(string p){
	this->pathDirectory = p;
	return true;
}




/*************
 * INITIALISATION
 *************/
bool Candidat::initexpVariables(){
	this->initlangage();
	this->initapc();
	this->initseq();
	this->initexpeOrder();
	
	
	/* Display *
	for (int i=0; i<this->seq.size();++i)
	{
		for (int j=0; j<this->seq[0].size();++j)
		{
			cout << this->seq[i][j] ;
		}
		cout << "\t";
	}
	cout << endl;
	*/
	return true;
}



/* Initialise all the possible combinaisons of actuators */
bool Candidat::initlangage(){
	string tmpL;
	
	cout << "Candidat is going to be created. Which is the langage to use? ('en-us' or 'fr-fr')" << endl;
	cin >> tmpL;
	
	return setlangage(tmpL);
}

/* Initialise all the possible combinaisons of actuators */
bool Candidat::initapc(){
	int nbs = pow(2, this->nba) - 1; // (number of sequences) - (the '000000' sequence)
	cout << "[candidat] initapc> Number of (sequence, motor) = (" << nbs << ", " << this->nba << ")" << endl;
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
bool Candidat::initseq(){
	std::srand ( unsigned ( std::time(0) ) ); // initialise the rand and srand
	vector<int> idx; // index vector of the combinaisons used for a specific number of actuator
	int sum_of_elems; // sum of the actuator used during a sequence
	// for all the subgroup of stimuli ( 1 actuator used, 2 act used, ..., 6 act used)
	for(a=1; a<=nba; ++a)
	{
		idx.clear();
		// (2) idx = getIdxCombinaison(apc, a)
		// for all the sequences
		for(s=0; s<this->apc.size(); ++s)
		{
			sum_of_elems = 0;
			// for all the actuators inside the current sequence
			for (j=0; j<this->apc[0].size();++j)
			{
				//cout << this->apc[s][j] ;
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






bool Candidat::initexpeOrder(){
	expeOrder.push_back( std::make_pair(false,BrailleDev) );
	expeOrder.push_back( std::make_pair(true, Fingers) );
	expeOrder.push_back( std::make_pair(false,Buzzer) );
	std::random_shuffle(expeOrder.begin(), expeOrder.end());
	
	return true;
}



bool Candidat::saveInfo(){
	//for (std::map<char,int>::iterator it=mymap.begin(); it!=mymap.end(); ++it)
	//    std::cout << it->first << " => " << it->second << '\n';
	string file(this->pathDirectory+to_string(this->id)+"/"+this->infoFile);
	cout << "[candidat][saveInfo] writing " << file << "...";

	std::ofstream mf;
	mf.open(file);
	
	mf << "firstname,lastname:" << endl;
	mf << this->firstname << "," << this->lastname << endl;
	mf << endl;
	
	mf << "langage:" << endl;
	mf << this->langage << endl;
	mf << endl;
	
	mf << "expEnum order:" << endl;
	for (vector<pair<bool, expEnum>>::iterator it=this->expeOrder.begin(); it!=this->expeOrder.end(); ++it)
	{
		if (it->first == false)
			mf << "false,";
		else
			mf << "true,";

		mf << expstring(it->second) << endl;
	}
	mf << endl;

	mf << "experimental sequences:" << endl;
	for(i=0; i<this->seq.size(); i++)
	{
		for(j=0; j<this->seq[0].size()-1; j++)
		{
			mf << this->seq[i][j] << ",";
		}
		mf << this->seq[i][j] << endl;
	}
	mf << endl;
	
	mf.close();
	
	cout << "done." << endl;
}


/*************
 * TOOLS
 *************/

string Candidat::expstring(expEnum ee)
{
	string ret;

	if(ee == BrailleDev)
		ret = "BrailleDev";
	else if(ee == Fingers)
		ret = "Finger";
	else if(ee == Buzzer)
		ret = "Buzzer";
	else
		ret = "";
	
	return ret;
}


expEnum Candidat::str2expEnum(string eestr)
{
	expEnum ee;

	if (eestr.compare("BrailleDev") == 0)
	{
		ee = BrailleDev;
	}
	else if (eestr.compare("Finger") == 0)
	{
		ee = Fingers;
	}
	else if (eestr.compare("Buzzer") == 0)
	{
		ee = Buzzer;
	}
	else
	{
		cerr << "[candidat][str2expEnum] String is not corresponding to a expEnum value." << endl;
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
            std::cerr << "Source directory " << source.string()
                      << " does not exist or is not a directory." << endl;
            return false;
        }
        if(fs::exists(destination))
        {
            std::cerr << "Destination directory " << destination.string()
                	  << " already exists." << endl;
            return false;
        }
        // Create the destination directory
        if(!fs::create_directory(destination))
        {
            std::cerr << "Unable to create destination directory" 
            		  << destination.string() << endl;
            return false;
        }
    }
    catch(fs::filesystem_error const & e)
    {
        std::cerr << e.what() << endl;
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
            std:: cerr << e.what() << '\n';
        }
    }
    
    return true;
}






