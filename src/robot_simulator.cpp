#include <math.h>
#include <robot_simulator.h>
#include <json_cpp.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include <atomic>
#include <chrono>

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
        auto now = std::chrono::system_clock::now();
        auto elapsed = 0;
        if (initialized)
            elapsed = ((double)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count()) / 1000;
        update(elapsed);
        last_update = now;
        initialized = true;
    }

    void Robot_state::update(double elapsed) {
        rm.lock();
        time_stamp = json_cpp::Json_date::now();
        double dl = ((double)left) / 128.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        double dr = -((double)right) / 128.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        double d = ((double)left + (double)right) / 255.0 * robot_speed * elapsed; // convert motor signal to speed
        rm.unlock();
        rotation = rotation + dl + dr;
        location = location.move(rotation,d);
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
    atomic<bool> robot_running = false;
    string log_file;
    void simulation (){
        ofstream log;
        log.open(log_file);
        while (robot_running){
            robot_state.update();
            log << robot_state << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(robot_interval));
        }
        log.close();
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

    void Robot_simulator::set_robot_rotation_speed(double rotation_speed) {
        robot_rotation_speed = rotation_speed;
    }

    void Robot_simulator::set_log_file_name(std::string file_name) {
        log_file = file_name;
    }

}