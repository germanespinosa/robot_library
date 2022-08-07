#include <robot_lib/robot_agent.h>

using namespace std;
// temp constants
#define MAX_J 55
#define MIN_J 0
#define JOYSTICK 32767

namespace robot{
    void Robot_agent::set_left(int left_value) {
        message.left = left_value;
    }

    void Robot_agent::set_right(int right_value) {
        message.right = right_value;
    }

    void Robot_agent::set_speed(int speed_value) {
        message.speed = speed_value;
    }

    void Robot_agent::capture() {
        //need_update = true;
    }

    bool Robot_agent::update() {
        bool res = ((easy_tcp::Connection *)this)->send_data((const char*) &message,sizeof(message));
        //wait for finish on the robot.
        return res;
    }

    Robot_agent::~Robot_agent() {
        stop();
    }

    int Robot_agent::port() {
        string port_str (std::getenv("ROBOT_PORT")?std::getenv("ROBOT_PORT"):"4500");   // 4500
        return atoi(port_str.c_str());
    }

    bool Robot_agent::connect(const string &ip) {
        return ((easy_tcp::Client *)this)->connect(ip, port());
    }

    bool Robot_agent::connect() {
        //return connect("192.168.137.155");
        return connect("127.0.0.1");
    }

    bool Robot_agent::is_move_done() {
        // switch turns move done "on/off"
        if (move_done){
            move_done = false;
            return true;
        }
        return false; // flase
    }

    void Robot_agent::received_data(char *buffer, size_t size) {
        move_done = true;
    }
}