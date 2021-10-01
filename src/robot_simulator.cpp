#include <math.h>
#include <robot_simulator.h>
#include <json_cpp.h>
#include <mutex>

using namespace json_cpp;
using namespace std;
using namespace cell_world;

namespace robot {

    double robot_speed = 1;
    double robot_rotation_speed = M_PI / 2; //90 degrees at full speed
    double robot_radius= .02;
    Robot_state robot_state;

    mutex rm;

    void Robot_state::update() {
        rm.lock();
        double dl = ((double)left) / 128.0 * robot_rotation_speed; // convert motor signal to angle
        double dr = ((double)right) / 128.0 * robot_rotation_speed; // convert motor signal to angle
        rm.unlock();
        Location lw = location.move(rotation-M_PI/2 + dl, robot_radius); // rotate left wheel angle with respect of robot center
        Location rw = location.move(rotation+M_PI/2 + dr, robot_radius); // rotate right wheel angle with respect of robot center

        Location heading = (lw + rw) / 2;

        if (dl-dr)
            rotation = location.atan(heading);
        else
            rotation += dl;
        location = location.move(rotation,robot_speed * location.dist(heading));
    }

    void Robot_simulator::on_connect() {
        Service::on_connect();
    }

    void Robot_simulator::on_incoming_data(const char *buff, int size) {
        if (size == 3){ // instruction
            rm.lock();
            robot_state.left = buff[0];
            robot_state.right = buff[1];
            rm.unlock();
        }
    }

    void Robot_simulator::on_disconnect() {
        Service::on_disconnect();
    }

    void Robot_simulator::set_robot_radius(double radius) {
        robot_radius = radius;
    }

    void Robot_simulator::set_robot_speed(double speed) {
        robot_speed = speed;
    }

    unsigned int robot_interval = 200;
    bool robot_running = false;
    void simulation (){
        while (robot_running){
            robot_state.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    thread simulation_thread;

    void Robot_simulator::start_simulation(Location location, double rotation, unsigned int interval) {
        robot_state.location = location;
        robot_state.rotation = rotation;
        robot_state.left = 0;
        robot_state.right = 0;
        robot_state.puff = false;
        robot_state.led0 = false;
        robot_state.led1 = false;
        robot_state.led2 = false;
        robot_interval = interval;
        robot_running = true;
        simulation_thread=thread(&simulation);
    }

    Robot_state Robot_simulator::get_robot_state() {
        return robot_state;
    }

    void Robot_simulator::end_simulation() {
        robot_running = false;
        simulation_thread.join();
    }

}