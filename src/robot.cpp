#include <robot.h>
#include <thread>

using namespace easy_tcp;
using namespace std;

namespace robot {
    Robot::Robot(){
        message[0] = 0;
        message[1] = 0;
        message[2] = 0;
        set_leds(true);
    }

    void Robot::set_left(char left) {
        message[0] = left;
        need_update = true;
    }

    void Robot::set_right(char right) {
        message[1] = right;
        need_update = true;
    }

    void Robot::set_puf() {
        message[2] |= 1UL << 3;
        need_update = true;
        update();
    }

    void Robot::set_led(int led_number, bool val) {
        if (val)
            message[2] |= 1UL << led_number;
        else
            message[2] &=~(1UL << led_number);
    }

    bool Robot::update() {
        if (!need_update) return true;
        bool res = connection.send_data(message,3);
        message[2] &=~(1UL << 3);
        message[2] &=~(1UL << 4);
        message[2] &=~(1UL << 5);
        return res;
    }

    Robot::~Robot() {
        message[0] = 0;
        message[1] = 0;
        message[2] = 0;
        need_update = true;
        update();
    }

    int Robot::port() {
        string port_str (std::getenv("ROBOT_PORT")?std::getenv("ROBOT_PORT"):"4500");
        return atoi(port_str.c_str());
    }

    void Robot::set_leds(bool val) {
        for (int i=0; i<3;i++) set_led(i,val);
    }

    void Robot::increase_brightness() {
        set_led(4,true);
    }

    void Robot::decrease_brightness() {
        set_led(5,true);
    }

    bool Robot::connect(const string &ip, int port) {
        try {
            connection = connection.connect_remote(ip, port);
            return true;
        } catch(...) {
            return false;
        }
    }

    bool Robot::connect(const string &ip) {
        return connect(ip, port());
    }

    bool Robot::connect() {
        return connect("127.0.0.1");
    }
}