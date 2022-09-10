#include <robot_lib/robot_agent.h>

using namespace std;
using namespace cell_world;
// temp constants
#define MAX_J 55
#define MIN_J 0
#define JOYSTICK 32767
#define TURN_TICK_RATE 1000
#define FORWARD_TICK_RATE 2000

namespace robot{
    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits):
        //Robot_agent(limits,4690){  // port for joystick 4690
        Robot_agent(limits,"/dev/input/js0"){  // joystick device
        set_leds(true);
    }

    void Robot_agent::set_left(double left_value) {
        char left = limits.convert(left_value);
        // for joystick control press R2
        if (!gamepad.buttons.empty() && gamepad.buttons[5].state == 1){
            float joystick_left = (float)-gamepad.axes[1]/JOYSTICK; // normalize this to config file
            if (joystick_left > 0){
                joystick_left = abs(joystick_left) * (MAX_J - MIN_J) + MIN_J;
            } else if (joystick_left < 0){
                joystick_left = -(abs(joystick_left) * (MAX_J - MIN_J) + MIN_J);
            }
            // drive straight
            if (gamepad.axes[7] == -32767){
                joystick_left = joystick_left/2 + 30; // max value for char 127

            }
            else if (gamepad.axes[7] == 32767){
                joystick_left = joystick_left/2 - 30;
            }
            left = (char) joystick_left;
        }


        // autonomous
        if (message[0] != left)
            need_update = true;
        message[0] = left;
    }

    void Robot_agent::set_right(double right_value) {
        char right = limits.convert(right_value);
        if (!gamepad.buttons.empty() && gamepad.buttons[5].state == 1){
            float joystick_right = (float)-gamepad.axes[4]/JOYSTICK;
            if (joystick_right > 0){
                joystick_right = abs(joystick_right) * (MAX_J - MIN_J) + MIN_J;
            } else if (joystick_right < 0){
                joystick_right = -(abs(joystick_right) * (MAX_J - MIN_J) + MIN_J);
            }
            // drive straight
            if (gamepad.axes[7] == -32767){
                joystick_right = joystick_right/2 + 30; // 20
            }
            if (gamepad.axes[7] == 32767){
                joystick_right = joystick_right/2 - 30;
            }
            right = (char) joystick_right;
        }
        if (message[1] != right)
            need_update = true;
        message[1] = right;
    }

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

//        cout << "robot_agent " << int(message[0]) << " : " << int(message[1]) << endl;
        bool res = connection.send_data(message,3);
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

    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits, int game_pad_port):
            message{0,0,0},
            limits(limits),
            gamepad(game_pad_port){
        set_leds(true);
    }

    Robot_agent::Robot_agent(const controller::Agent_operational_limits &limits, std::string device_path):
            message{0,0,0},
            limits(limits),
            gamepad(device_path){
        set_leds(true);
    }

    bool Robot_agent::stop() {
        message[2] |= 1UL << 6;
        need_update = true;
        return true;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////

    Tick_robot_agent::Tick_robot_agent()
    {
        robot_moves.push_back(Move(2,0));
        robot_moves.push_back(Move(1,-1));
        robot_moves.push_back(Move(-1,-1));
        robot_moves.push_back(Move(-2,0));
        robot_moves.push_back(Move(-1,1));
        robot_moves.push_back(Move(1,1));
    }


    int Tick_robot_agent::update() {

        if (message.left == 0 && message.right == 0) return -1;
        // pause if robot is not ready to receive next command
        while (!is_ready());
        // move is complete when move_status arg is 0
        message.move_number = move_counter ++;
        bool res = ((easy_tcp::Connection *)this)->send_data((const char*) &message,sizeof(message));
        if (!res) return -1;
        return (int) message.move_number;
    }

    int Tick_robot_agent::port() {
        string port_str (std::getenv("ROBOT_PORT")?std::getenv("ROBOT_PORT"):"4501");   // 4500
        return atoi(port_str.c_str());
    }

    bool Tick_robot_agent::connect(const string &ip) {
        return ((easy_tcp::Client *)this)->connect(ip, port());
    }

    bool Tick_robot_agent::connect() {
        return connect("127.0.0.1");
    }

    bool Tick_robot_agent::is_move_done() {
        // switch turns move done "on/off"
        if (move_done){
            move_done = false;
            return true;
        }
        return false; // false
    }

    void Tick_robot_agent::received_data(char *buffer, size_t size) {
        // receives move number from robot
        move_done = true;
        completed_move = (int)*((uint32_t *) buffer);
    }

    bool Tick_robot_agent::is_ready() {
        return (completed_move >= move_counter - 2);
    }

#define forward {432,432}
    void Tick_robot_agent::execute_move(cell_world::Move move) {
        auto move_number = (robot_moves.index_of(move) - robot_move_orientation) % 6;
        robot_move_orientation = (robot_move_orientation + move_number) % 6;
        current_coordinates += move;

        vector<pair<int,int>> move_ticks = {{0,0}, {150,-150}, {300,-300}, {450,-450}, {-300,300}, {-150,150}}; // m0,m1,m2,m3,m4,m5    212,815
        message.left = move_ticks[move_number].first;
        message.right = move_ticks[move_number].second;
        message.speed = TURN_TICK_RATE;

        update();
        // if not move 0 move requires 2 updates
        message.left = 432;
        message.right = 432;
        message.speed = FORWARD_TICK_RATE;
        update();
    }
}

