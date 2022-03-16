#include <robot_lib/robot.h>
#include <iostream>
#include <robot_lib/gamepad_wrapper.h>

#define MAX 100
#define MIN 60
#define JOYSTICK_MAX 32767

#define puff_delay 5

using namespace std;
using namespace robot;
using namespace gamepad;



int main(){
    cout << "hi" << endl;
    string robot_ip = "192.168.137.155";
    string device = "/dev/input/js0";

    // connect to robot
    Robot robot;
    if (!robot.connect(robot_ip))
    {
        cout << "Unable to connect to robot." <<  endl;
        exit(0);
    }
    Gamepad *j;
    j = new Gamepad_wrapper(device);
    //j = new Gamepad_wrapper(port);

    int pleft = 0;
    int pright = 0;
    bool h = true;
    bool update;

    while (h){
        update = false;

        float left = -j->axes[1];
        float right = -j->axes[4];


        // modify speeds to robot range
        if (left > 0){
            left = abs((float)(left/JOYSTICK_MAX)) * (MAX - MIN) + MIN;;
        } else if (left < 0){
            left = -(abs((float)(left/JOYSTICK_MAX)) * (MAX - MIN) + MIN);
        }
        if (right > 0){
            right = abs((float)(right/JOYSTICK_MAX)) * (MAX - MIN) + MIN;
        } else if (right < 0){
            right = -(abs((float)(right/JOYSTICK_MAX)) * (MAX - MIN) + MIN);
        }


        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
            //cout << "left: "<< -j->axes[1] << " right: "<<  -j->axes[4]<< endl;
            cout << "left: " << left << " right: " << right << endl;
        }

        pleft = left;
        pright = right;

        if (update) {
            robot.update();
        }

        // test all joystick inputs
//        for (int i = 0;i<j->axes.size();i++) {
//            if (j->axes[i])
//                cout << "axis " << i << ": " << j->axes[i] << endl;
//        }
//         for (int i = 0;i<j->buttons.size();i++) {
//             if (j->buttons[i].state == 1)
//                 cout << "button " << i << ": " << j->buttons[i].state << "\t";
//        }

        usleep(30000);
        if (j->buttons[8].state == 1) h = false; // exit loop
    }
}

