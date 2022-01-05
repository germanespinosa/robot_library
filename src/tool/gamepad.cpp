#include <robot.h>
#include <iostream>
#include <agent_tracking/client.h>
#include <gamepad_lib/include/gamepad.h>

#define puff_delay 5

using namespace std;
using namespace robot;
using namespace cell_world;
using namespace gamepad;


int main(int argc, char *argv[]){
    Timer prey_ts;
    Timer predator_ts;
    mutex mtx_update;
    agent_tracking::Client tracking;
    cell_world::Location prey_location {-10000,-10000};
    cell_world::Location predator_location {10000,10000};
    double predator_rotation;
    Timer puff_timer;
    Timer robot_update;
    puff_timer.reset();
    tracking.connect();
    tracking.register_consumer();
    if (argc != 2) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./joystick [device_path]" << endl;
        exit(1);
    }

    string device_path (argv[1]);
    Robot robot;
    robot.connect();
    Gamepad j(device_path);

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
        int left = -j.axes[1] * 3 /  256 / 3;
        int right = -j.axes[4] * 3 / 256 / 3;

        if (pleft != left || pright != right) {
            robot.set_left(left);
            robot.set_right(right);
            update = true;
        }

        pleft = left;
        pright = right;

//       for (int i = 0;i<j.axes.size();i++) {
//            cout << j.axes[i] << "\t";
//       }
//	     for (int i = 0;i<j.buttons.size();i++) {
//            cout << j.buttons[i].state << "\t";
//       }
//       cout << endl;

        if (tracking.contains("predator_step")){
            Step predator_step = tracking.get_last_message("predator_step").get_body<Step>();
            predator_location = predator_step.location;
            predator_rotation = predator_step.rotation;
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

        if (j.buttons[1].state == 1) {
            robot.set_leds(false);
            update = true;
        }
        if (j.buttons[0].state == 1) {
            robot.set_leds(true);
            update = true;
        }

        if ( puff_timer.to_seconds() > puff_delay && (j.buttons[5].state == 1 || ( distance < 100))){
            if (enabled) {
                robot.set_puf();
                tracking.update_puff();
                puff_timer.reset();
                prey_location = {-10000,-10000};
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


