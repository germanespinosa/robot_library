#include <robot_lib/robot.h>
#include <iostream>
#include <robot_lib/gamepad_wrapper.h>


#define puff_delay 5

using namespace std;
using namespace robot;
using namespace gamepad;

int main(){
    cout << "hi" << endl;
    string robot_ip = "192.168.137.155";
    string device = "/dev/input/js0";

//
//    Robot robot;
//    if (!robot.connect(robot_ip))
//    {
//        cout << "Unable to connect to robot." <<  endl;
//        exit(0);
//    }
    Gamepad *j;
    j = new Gamepad_wrapper(device);
    //j = new Gamepad_wrapper(port);

    int pleft = 0;
    int pright = 0;
    bool h = true;
    bool update;

    while (h){
        update = false;
        int left = -j->axes[1] * 2 /  256 / 3;
        int right = -j->axes[4] * 2 / 256 / 3;

//        if (pleft != left || pright != right) {
//            robot.set_left(left);
//            robot.set_right(right);
//            update = true;
//            cout << "left: "<< -j->axes[1] << " right: "<<  -j->axes[4]<< endl;
//            cout << "left: " << left << " right: " << right << endl;
//        }
//
//        pleft = left;
//        pright = right;
//
//        if (update) {
//            robot.update();
//        }

        // output joystick inputs
        for (int i = 0;i<j->axes.size();i++) {
            if (j->axes[i])
                cout << "axis " << i << ": " << j->axes[i] << endl;
        }
         for (int i = 0;i<j->buttons.size();i++) {
             if (j->buttons[i].state == 1)
                 cout << "button " << i << ": " << j->buttons[i].state << "\t";
        }

        usleep(30000);
        if (j->buttons[8].state == 1) h = false; // exit loop
    }
}

