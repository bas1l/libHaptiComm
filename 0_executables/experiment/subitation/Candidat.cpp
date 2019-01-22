/*
 * Candidat.cpp
 *
 *  Created on: 14 janv. 2019
 *      Author: Basil
 */

#include "Candidat.h"

Candidat::Candidat(string _pathDirectory, string _firstname, string _lastname)
	: nextit(0), nba(6), nbr(10), 
	  infoFile("info.txt"), 
	  candidatsListFile("candidatsList.txt")
{
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
	if (!ifs.is_open()) cerr << "The file" << file << "can't be loaded." << endl;
	string line;
	int success = -1;
	// get the line corresponding to the candidats IDs
	while (success != 0 && getline(ifs, line))
	{
		success = line.compare("candidats:");
	}
	// if there is no line concerning candidats
	success == 0 ? cout << "Checking if the candidat exists..." :
		cerr << "The file" << file << "do not have \"candidat:\" lines." << endl;
	
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
		cout << "Exists in the database." << endl;
		this->setId(stoi(_id));
	}
	else
	{
		cout << "Does not exist." << endl;
	}
		
	
	return exist;
}




bool Candidat::create()
{
	/* get the next id for the candidat */
	struct dirent * dirp;
	int idMax = 0;
	DIR * dp;
	if((dp  = opendir(this->pathDirectory.c_str())) == NULL) {
		cout << "Error(" << errno << ") opening " << this->pathDirectory << endl;
		return errno;
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
	cout << "[SUCCESS] Candidate('" << this->firstname << ' ' << this->lastname 
		 << "') has been created with its current folder named '" 
		 << to_string(this->id) << "'." << endl;
	
	
	/* initialize the sequence vector  */
	this->initexpVariables();
	
	/* save all the informations into the infoFile */
	this->saveInfo();
	
	return true;
}

bool Candidat::loadFromDB(){

	return true;
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
bool Candidat::setExpeOrder(){

	return true;
}
bool Candidat::setSequence(){

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
bool Candidat::initapc(){
	int nbs = pow(2, this->nba) - 1; // (number of sequences) - (the '000000' sequence)
	cout << "initapc> Number of (sequence, motor) = (" << ", " << this->nba << ")" << endl;
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
	expeOrder.push_back( std::make_pair(false,Fingers) );
	expeOrder.push_back( std::make_pair(false,Buzzer) );
	std::random_shuffle(expeOrder.begin(), expeOrder.end());
	
	return true;
}



bool Candidat::saveInfo(){
	//for (std::map<char,int>::iterator it=mymap.begin(); it!=mymap.end(); ++it)
	//    std::cout << it->first << " => " << it->second << '\n';
	
	
	string file(this->pathDirectory+to_string(this->id)+"/"+this->infoFile);
	cout << "writing the resume: " << file << endl;

	std::ofstream mf;
	
	mf.open(file);
	mf << "firstname, lastname:" << endl;
	mf << this->firstname << ", " << this->lastname << endl;
	mf << endl;
	mf << "expEnum order:" << endl;
	for (vector<pair<bool, expEnum>>::iterator it=this->expeOrder.begin(); it!=this->expeOrder.end(); ++it)
	{
		if (it->first == false)
			mf << "false, ";
		else
			mf << "true, ";

		if(it->second == BrailleDev)
			mf << "BrailleDev" << endl;
		else if(it->second == Fingers)
			mf << "Fingers" << endl;
		else
			mf << "Buzzer" << endl;
	}
	mf << endl;

	mf << "expEnumal sequences:" << endl;
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
}


/*************
 * TOOLS
 *************/
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






