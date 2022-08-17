#include <robot_lib/robot_agent.h>

using namespace std;
// temp constants
#define JOYSTICK 32767
#define MAX_PWM 1000 // max pwm value

namespace robot{
    Robot_agent::Robot_agent():
            Robot_agent("/dev/input/js0"){  // joystick device
    }

    void Robot_agent::set_left(int left_value) {
        // add joystick modifications here ??
        // if joystick activated replace left_value
        if (gamepad.buttons[5].state == 1){
            float joystick_left = ((float) -gamepad.axes[1]/ (float) JOYSTICK) * MAX_PWM;
            left_value = (int) joystick_left;
        }
        cout << "LEFT: "<< left_value << endl;
        message.left = left_value;
    }

    void Robot_agent::set_right(int right_value) {
        if (gamepad.buttons[5].state == 1){
            float joystick_right = ((float) -gamepad.axes[4]/ (float) JOYSTICK) * MAX_PWM;
            right_value = (int) joystick_right;
        }
        cout << "RIGHT: "<< right_value << endl;
        message.right = right_value;
    }

    void Robot_agent::set_speed(int speed_value) {
        if (gamepad.buttons[5].state == 1) speed_value = -1; // send neg speed when gamepad is pressed
        message.speed = speed_value;
    }

    void Robot_agent::capture() {
        //need_update = true;
    }


    int Robot_agent::update() {
        // TODO: ask why have move counter
        if (message.speed > 0) message.move_number = move_counter ++;
        bool res = ((easy_tcp::Connection *)this)->send_data((const char*) &message,sizeof(message));
        if (!res) return -1;
        return (int)message.move_number;
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
        return false; // false
    }

    void Robot_agent::received_data(char *buffer, size_t size) {
        // receives move number from robot
        move_done = true;
        int move_id = (int)*((uint32_t *) buffer);
        move_finished(move_id);
    }

    Robot_agent::Robot_agent(std::string device_path):
//            message{0,0,0},
            gamepad(device_path){
    }


}