#include <gamepad.h>
#include <iostream>

using namespace std;
using namespace robot;
int main(int argc, char *argv[]){
    if (argc != 2) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./joystick [device_path]" << endl;
        exit(1);
    }
    string device_path (argv[1]);
    Gamepad j(device_path);
    while (true){
        //cout << j.axes[0] << "\t" << j.axes[1] << "\t" << j.axes[2] << "\t" << j.axes[3] << "\t" << j.axes[4] << "\t" << j.axes[5] << "\t";
	    for (int i = 0;i<j.buttons.size();i++) {
            cout << j.buttons[i].state << "\t";
        }
        cout << endl;
    }
}
