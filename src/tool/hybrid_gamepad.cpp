#include <iostream>
#include <robot_lib/gamepad_wrapper.h>
#include <robot_lib/robot_agent.h>

#define MAX 40 // why did I think this was 26
#define MIN 0
#define JOYSTICK_MAX 32767

#define puff_delay 5

using namespace std;
using namespace robot;
using namespace gamepad;
using namespace controller;



int main(){
    cout << "hi" << endl;
    string robot_ip = "192.168.137.155";
    string device = "/dev/input/js0";
    Agent_operational_limits limits;
    limits.load("../config/robot_operational_limits.json");
    // connect to robot
    Robot_agent robot(limits);
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
        float joystick_left = abs((float)(left/JOYSTICK_MAX));
        float joystick_right = abs((float)(right/JOYSTICK_MAX));
//
//
        // modify speeds to robot range
        if (left > 0){
//            left = 47;
            left = joystick_left * (MAX - MIN) + MIN;
        } else if (left < 0){
//            left = -47;
            left = -joystick_left * (MAX - MIN) + MIN;
        }
        if (right > 0){
//            right = 47;
            right = joystick_right * (MAX - MIN) + MIN;
        } else if (right < 0){
//            right = -47;
            right = -joystick_right * (MAX - MIN) + MIN;
        }

        if (j->axes[7] == -32767){
            left = left/2 + 30; // max value for char 127
            right = right/2 + 30;

        }
        else if (j->axes[7] == 32767){
            left = left/2 - 30; // max value for char 127
            right = right/2 - 30;

        }



//
        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
            //cout << "left: "<< -j->axes[1] << " right: "<<  -j->axes[4]<< endl;
            cout << "left: " << left << " right: " << right << endl;
        }

        if (j->buttons[0].state == 1){
            while (j->buttons[0].state == 1);
            robot.stop();
            robot.update();
            cout << "hi" << endl;
        }
        if (j->buttons[1].state == 1){
            while (j->buttons[1].state == 1);
            robot.set_left(0);
            robot.set_right(0);
            robot.capture();
            robot.update();
            robot.update();
            cout << "stop" << endl;

        }

        pleft = left;
        pright = right;




        // test all joystick inputs
//        for (int i = 0;i<j->axes.size();i++) {
//            if (j->axes[i])
//                cout << "axis " << i << ": " << j->axes[i] << endl;
//        }


         if (update) {
            robot.update();
        }

        usleep(30000);
        if (j->buttons[8].state == 1) h = false; // exit loop
    }
}

