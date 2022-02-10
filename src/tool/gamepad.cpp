#include <robot.h>
#include <iostream>
#include <agent_tracking/tracking_client.h>
#include <gamepad_wrapper.h>
#include <params_cpp.h>

#define puff_delay 5

using namespace std;
using namespace robot;
using namespace cell_world;
using namespace gamepad;

int main(int argc, char *argv[]){
    params_cpp::Key port_key ("-p", "-port");
    params_cpp::Key device_key ("-d", "-device");
    params_cpp::Key robot_key ("-r", "-robot");
    params_cpp::Parser params(argc,argv);

    string port_str = params.get(port_key, "");
    string device = params.get(device_key, "");
    string robot_ip = params.get(robot_key);

    Timer prey_ts;
    Timer predator_ts;
    mutex mtx_update;

    agent_tracking::Tracking_client tracking;
    cell_world::Location prey_location {-10000,-10000};
    cell_world::Location predator_location {10000,10000};
    Timer puff_timer;
    Timer robot_update;
    puff_timer.reset();
    if (!tracking.connect())
    {
        cout << "Unable to connect to Tracking service." <<  endl;
        exit(0);
    }
    tracking.register_consumer();
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
    bool enabled = true;
    bool enabled_l = true;
    predator_ts.reset();
    prey_ts.reset();
    while (h){
        update = false;
        int left = -j->axes[1] * 2 /  256 / 3;
        int right = -j->axes[3] * 2 / 256 / 3;

        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
            cout << "left: " << left << " right: " << right << endl;
        }

        pleft = left;
        pright = right;

//        for (int i = 0;i<j->axes.size();i++) {
//            if (j->axes[i])
//                cout << "axis " << i << ": " << j->axes[i] << endl;
//        }
//         for (int i = 0;i<j->buttons.size();i++) {
//             if (j->buttons[i].state == 1)
//                 cout << "button " << i << ": " << j->buttons[i].state << "\t";
//        }
        if (tracking.contains("predator_step")){
            Step predator_step = tracking.get_last_message("predator_step").get_body<Step>();
            predator_location = predator_step.location;
            predator_ts.reset();
        }
        if (predator_ts.to_seconds() > 2){
            predator_location = {10000,10000};
        }
        if (tracking.contains("prey_step")){
            Step predator_step = tracking.get_last_message("prey_step").get_body<Step>();
            prey_location = predator_step.location;
            prey_ts.reset();
        }
        if (prey_ts.to_seconds() > 2){
            prey_location = {-10000,-10000};
        }

        auto distance = predator_location.dist(prey_location);
        auto theta = predator_location.atan(prey_location);

        //cout << prey_location << predator_location << " distance: " << distance << endl;

        if (j->buttons[1].state == 1) {
            robot.set_leds(false);
            update = true;
        }
        if (j->buttons[0].state == 1) {
            robot.set_leds(true);
            update = true;
        }

        if ( puff_timer.to_seconds() > puff_delay && (j->buttons[5].state == 1 || ( distance < 100))){
            if (enabled) {
                robot.set_puf();
                //tracking.update_puff();
                puff_timer.reset();
                prey_location = {-10000,-10000};
                update = true;
            }
            enabled = false;
        } else {
            enabled = true;
        }

        if (j->axes[7]) {
            if (enabled_l) {
                if (j->axes[7] < 0) robot.increase_brightness();
                if (j->axes[7] > 0) robot.decrease_brightness();
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
        if (j->buttons[8].state == 1) h = false;
    }
}


