#include <gamepad.h>
#include <robot.h>
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
    Robot robot("192.168.137.155",80);
    Gamepad j(device_path);

    int pleft = 0;
    int pright = 0;
    bool h = true;
    bool update;
    bool enabled = true;
    bool enabled_l = true;

    while (h){
        update = false;
        int left = -j.axes[1] * 3 /  256 / 4;
        int right = -j.axes[4] * 3 / 256 / 4;

        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
        }

        pleft = left;
        pright = right;

        for (int i = 0;i<j.axes.size();i++) {
            cout << j.axes[i] << "\t";
        }

	    for (int i = 0;i<j.buttons.size();i++) {
            cout << j.buttons[i].state << "\t";
        }

        cout << endl;

        if (j.buttons[5].state == 1){
            if (enabled) {
                robot.set_puf();
                //server.send_data(message.c_str(), message.size()+1);
                update = true;
            }
            enabled = false;
        } else {
            enabled = true;
        }

        if (j.axes[7]) {
            if (enabled_l) {
                if (j.axes[7] < 0) robot.increase_brightness();
                if (j.axes[7] > 0) robot.decrease_brightness();
                update = true;
            }
            enabled_l = false;
        } else {
            enabled_l = true;
        }

        if (update) {
            robot.update();
        }

        usleep(30000);

        if (j.buttons[8].state == 1) h = false;
    }
}


/*
    auto server = Connection::connect_remote("192.168.137.1", 4000);
    string message = "{\"command\":\"update_puff\",\"content\":\"\"}";
    //Robot robot("192.168.137.155",80);
    string device("/dev/input/js0");
    Gamepad j(device);
    bool h = true;
    bool enabled = true;
    int pleft = 0;
    int pright = 0;
    bool update = false;
    while (h){
        update = false;
        int left = -j.axes[1] * 3 /  256 / 4;
        int right = -j.axes[4] * 3 / 256 / 4;

//        cout << j.axes[0] << "\t" << j.axes[1] << "\t" << j.axes[2] << "\t" << j.axes[3] << "\t" << j.axes[4] << "\t" << j.axes[5] << "\t";
//	    for (int i = 0;i<j.buttons.size();i++) {
//            cout << j.buttons[i].state << "\t";
//        }
//	    cout << j.buttons[5].state << "\t" << (int) left << "\t" << (int) right << endl;


        if (j.buttons[8].state == 1) h = false;

//        if (pleft != left || pright != right) {
//            robot.set_left(left);
//            robot.set_right(right);
//            update = true;
//        }
        pleft = left;
        pright = right;
        if (j.buttons[5].state == 1){
            if (enabled) {
//                robot.set_puf();
                server.send_data(message.c_str(), message.size()+1);
                update = true;
            }
            enabled = false;
        } else {
            enabled = true;
        }
        if (update) {
//            robot.update();
        }
        usleep(30000);
    }
    return 0;
    }
 */