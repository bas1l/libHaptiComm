#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
	string line;
	//ifstream myfile ("m.mp3");
        //ifstream myfile ("../Libtacom/readMp3/m.mp3", ios::in | ios::binary);
        ifstream myfile ("../Libtacom/sample.au", ios::in | ios::binary);
        
        int i = 0;
	if (myfile.is_open()) {
            while (! myfile.eof() ) {
                
                getline(myfile,line);
                cout << line << endl;
                i++;
            }
            
            myfile.close();
            cout << "Yay! i= " << i << endl;
	}
	else {
            cout << "Erorr!\n";
        }
        
	return 0;
}