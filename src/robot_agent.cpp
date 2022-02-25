#include <robot_agent.h>

using namespace std;

namespace robot{
    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits):
            message{0,0,0},
            limits(limits){
        set_leds(true);
    }

    void Robot_agent::set_left(double left_value) {
        char left = limits.convert(left_value);
        message[0] = left;
        need_update = true;
    }

    void Robot_agent::set_right(double right_value) {
        char right = limits.convert(right_value);
        message[1] = right;
        need_update = true;
    }

    void Robot_agent::capture() {
        message[2] |= 1UL << 3;
        need_update = true;
        update();
    }

    void Robot_agent::set_led(int led_number, bool val) {
        if (val)
            message[2] |= 1UL << led_number;
        else
            message[2] &=~(1UL << led_number);
    }

    bool Robot_agent::update() {
        if (!need_update) return true;
        bool res = connection.send_data(message,3);
        message[2] &=~(1UL << 3);
        message[2] &=~(1UL << 4);
        message[2] &=~(1UL << 5);
        return res;
    }

    Robot_agent::~Robot_agent() {
        message[0] = 0;
        message[1] = 0;
        message[2] = 0;
        need_update = true;
        update();
    }

    int Robot_agent::port() {
        string port_str (std::getenv("ROBOT_PORT")?std::getenv("ROBOT_PORT"):"4500");   // 4500
        return atoi(port_str.c_str());
    }

    void Robot_agent::set_leds(bool val) {
        for (int i=0; i<3;i++) set_led(i,val);
    }

    void Robot_agent::increase_brightness() {
        set_led(4,true);
    }

    void Robot_agent::decrease_brightness() {
        set_led(5,true);
    }

    bool Robot_agent::connect(const string &ip, int port) {
        try {
            connection = connection.connect_remote(ip, port);
            return true;
        } catch(...) {
            return false;
        }
    }

    bool Robot_agent::connect(const string &ip) {
        return connect(ip, port());
    }

    bool Robot_agent::connect() {
        //return connect("192.168.137.155");
        return connect("127.0.0.1");
    }

}