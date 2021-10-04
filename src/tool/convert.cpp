#include <fstream>
#include <iostream>


using namespace std;

int main(int argc, char *argv[]){
    if (argc != 3) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./convert [input_file] [output_file]" << endl;
        exit(1);
    }
    string ifile_name = argv[1];
    string ofile_name = argv[2];
    ifstream ifile (ifile_name);
    ofstream ofile (ofile_name);
    string line;
    ofile << "[";
    std::getline(ifile, line);
    ofile << line;
    while (!ifile.eof()) {
        std::getline(ifile, line);
        ofile << "," << endl << line;
    }
    ofile << "]";

}