#include <robot_lib/robot.h>
#include <iostream>
#include <agent_tracking/tracking_client.h>
#include <robot_lib/gamepad_wrapper.h>
#include <params_cpp.h>

#define puff_delay 5

using namespace std;
using namespace robot;
using namespace cell_world;
using namespace gamepad;

int main(int argc, char *argv[]){
    cout << "hi" << endl;
    string robot_ip = "192.168.137.155";
    params_cpp::Key port_key ("-p", "-port");
    params_cpp::Key device_key ("-d", "-device");
    params_cpp::Parser params(argc,argv);

    string port_str = params.get(port_key, "");
    string device = params.get(device_key, "");


    Robot robot;
    if (!robot.connect(robot_ip))
    {
        cout << "Unable to connect to robot." <<  endl;
        exit(0);
    }
    Gamepad *j;

    if (port_str.empty()) {
        if (device.empty()) {
            cerr << "Device path or port must be specified." << endl;
        }
        j = new Gamepad_wrapper(device);
    } else {
        int port = stoi(port_str);
        j = new Gamepad_wrapper(port);
    }

    int pleft = 0;
    int pright = 0;
    bool h = true;
    bool update;

    while (h){
        update = false;
        int left = -j->axes[1] * 2 /  256 / 3;
        int right = -j->axes[4] * 2 / 256 / 3;

        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
            cout << "left: "<< -j->axes[1] << " right: "<<  -j->axes[4]<< endl;
            cout << "left: " << left << " right: " << right << endl;
        }

        pleft = left;
        pright = right;

        if (update) {
            robot.update();
        }
        usleep(30000);
        if (j->buttons[8].state == 1) h = false;
    }
}

