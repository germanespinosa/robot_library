#include <robot_lib/robot_agent_new.h>

using namespace std;
// temp constants
#define MAX_J 55
#define MIN_J 0
#define JOYSTICK 32767

namespace robot{
    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits):
    //Robot_agent(limits,4690){  // port for joystick 4690
    Robot_agent(limits,"/dev/input/js0"){  // joystick device
        set_leds(true);
        message = (char*)&raw_message;
        message_size = sizeof (Robot_message);

    }

    // TODO: need to set left ticks, rigth ticks, and speed
    void Robot_agent::set_left(double left_value) {
        //TODO: change agent.cpp convert function to fit new model
        char left = left_value;

        // autonomous
        if (message[0] != left)
            need_update = true;
        message[0] = left;
    }

    void Robot_agent::set_right(double right_value) {
        char right = right_value;

        if (message[1] != right)
            need_update = true;
        message[1] = right;
    }

//    void Robot_agent::set_speed(double speed_value) {
//        char speed = speed_value;
//        if (message[])
//
//    }







    void Robot_agent::capture() {
        message[2] |= 1UL << 3; //puff
        message[2] |= 1UL << 6; // stop
        need_update = true;
    }

    void Robot_agent::set_led(int led_number, bool val) {
        if (val)
            message[2] |= 1UL << led_number;
        else
            message[2] &=~(1UL << led_number);
    }

    bool Robot_agent::update() {
        if (!need_update) return true;

        bool res = connection.send_data(message,message_size);
        message[2] &=~(1UL << 3);
        message[2] &=~(1UL << 4);
        message[2] &=~(1UL << 5);
        message[2] &=~(1UL << 6);
        return res;
    }

    Robot_agent::~Robot_agent() {
        message[0] = 0;
        message[1] = 0;
        message[2] = 0;
        message[3] = 0; // TODO: send speed
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

//    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits, int game_pad_port):
//            message{},  // TODO: was message {0,0,0} need to modify this to correlate with new size of message
//            limits(limits),
//            gamepad(game_pad_port){
//        set_leds(true);
//    }
//
//    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits, std::string device_path):
//            message{}, // TODO: was message {0,0,0} need to modify this to correlate with new size of message
//            limits(limits),
//            gamepad(device_path){
//        set_leds(true);
//    }

    bool Robot_agent::stop() {
        message[2] |= 1UL << 6;
        need_update = true;
        return true;
    }

}//
// Created by gabbie on 6/15/22.
//
