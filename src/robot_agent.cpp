#include <robot_lib/robot_agent.h>

using namespace std;
using namespace cell_world;
// temp constants
#define MAX_J 55
#define MIN_J 0
#define JOYSTICK 32767
#define TURN_TICK_RATE 1000
#define FORWARD_TICK_RATE 2000
#define MAX_TICKS 50
#define STRAIGHT_TICKS 50
#define CELL_SIZE 0.054

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

    Tick_robot_agent::Tick_robot_agent(const controller::Tick_agent_moves &moves, agent_tracking::Tracking_client &tracking_client, std::string &joystick_path):
            tick_agent_moves(moves),
            world(World::get_from_parameters_name("robot","canonical", "21_05")),
            cells(world.create_cell_group()),
            map(cells),
            tracking_client(tracking_client),
            tick_gamepad(joystick_path)
    {
        move_state = translate;
        //current_coordinates = Coordinates{-8,10};
    }

    void Tick_robot_agent::move_count_reset() {
        cout << "reset move done count" << endl;
        message.speed = -1;
        update();
    }

    int Tick_robot_agent::update() {

        if (message.left == 0 && message.right == 0 && message.speed >= 0) return -1;
        // pause if robot is not ready to receive next command

        //        cout << "update pending..." ;//<< flush;
        while (!is_ready())this_thread::sleep_for(10ms);
//        cout << " done" << endl;
        // move is complete when move_status arg is 0

        if (message.speed > 0) message.move_number = move_counter ++;

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

    float Tick_robot_agent::angle_diff_degrees(float a1, float a2){
        // a1 is goal angle, a2 is measured
        // if return + value means cw rot needed therefore add to left ******
        // TODO: fix this use direction and difference
        auto desired = to_radians(a1);
        auto actual = to_radians(a2);
        return to_degrees(angle_difference(desired, actual) * direction(desired, actual));
    }

    void Tick_robot_agent::received_data(char *buffer, size_t size) {
        // receives move number from robot
        move_done = true;
        completed_move = (int)*((uint32_t *) buffer);
//        cout << "RECEIVED DATA " << completed_move << endl;
        Tick_move_target tmt;
        while (!move_targets.empty() && move_targets.front().move_number <= completed_move){
            tmt = move_targets.front();
            move_targets.pop();
        }

        // Only compute x and y error when robot is rotating ???
        if (tmt.move_number == completed_move && move_state != correct){
            auto tracking_info = tracking_client.get_current_state("predator");

            // theta check
            actual_rotation = tracking_info.rotation; // TODO: get pos values from tracker but for now fix here
            if (actual_rotation < 0) actual_rotation = 360.0 + actual_rotation; // TODO: have agent tracker send values 0 to 360
            orientation_error = angle_diff_degrees(tmt.rotation, actual_rotation);
//            cout << "OE" << " " << orientation_error << " " << timer.to_seconds() << endl;

            // position check - only during rotations
            if (move_state == rotate) {
                move_state = translate;
                location_error = tmt.location - tracking_info.location;      // desired - actual
            }

//             auto reset
            if (tmt.location.dist(tracking_info.location) > CELL_SIZE || orientation_error > 10.0){ // cell size 0.054 world.implementation.cell_transformation.size??
                cout << "error too big auto correct" << endl;
                needs_correction_now = true;
            }
        }
    }

    bool Tick_robot_agent::is_ready() {
        return (completed_move >= move_counter - 2);
    }

    // TODO: for tracker change to 360 orientation
    std::vector<float> rotation_target = {90.0, 120.0, 150.0, 180.0, 210.0, 240.0, 270.0, 300.0, 330.0, 0.0, 30.0, 60.0};

    void Tick_robot_agent::execute_move(cell_world::Move_list moves) {
        Move best_move;
        float min_rotation_diff = -1;
        auto current_rotation = rotation_target[robot_move_orientation];
        auto moves_list = moves;
        for (auto &move:moves_list){
            auto tick_move = tick_agent_moves.find_tick_move(move, robot_move_orientation);
            auto new_orientation = tick_move.update_orientation(robot_move_orientation);
            auto new_rotation = rotation_target[new_orientation];
            auto rotation_diff = abs(angle_diff_degrees(current_rotation, new_rotation));
            if (min_rotation_diff==-1 || min_rotation_diff > rotation_diff){
                min_rotation_diff = rotation_diff;
                best_move = move;
            }
        }
        cout << "BEST MOVE "  << best_move << endl;

        if (needs_correction_now) return;
//        if (use_joystick()) return;

        move_state = translate;
        P_y = 18791.0;
        P_x = 21698.0; // 21698.0
        P_rot = 10.0;//5.0
        P_rot2 = 10.0; // 8.0, 10.0

        // change this be works for now
        auto tick_move = tick_agent_moves.find_tick_move(best_move, robot_move_orientation);
        robot_move_orientation = tick_move.update_orientation(robot_move_orientation);
        cout << "KLJHNFDKJLFHNDKLF" << tick_move << endl;

        // if turning correct based on last angle error
        if (tick_move.orientation != 0)
        {
            move_state = rotate; // only measuring position error at this moment
            orientation_correction = (int32_t)(P_rot * orientation_error);
        } else{
            // straight line orientation correction here - continuous straight line
            tick_move = tick_agent_moves.get_forward_move(robot_move_orientation);
            orientation_correction = (int32_t)(P_rot2 * orientation_error); // if error + more left
        }


        cout << "TICK MOVE 1: " << tick_move << endl;
        message.left = tick_move.left_ticks + orientation_correction;
        message.right = tick_move.right_ticks - orientation_correction;
        message.speed = tick_move.speed;
        auto move_number = update();

        // MOVE FWD AFTER ROTATE WITH ERROR CORRECTION
        if (tick_move.orientation != 0) {
            // update expected coordinate
            move_targets.emplace(move_number, map[current_coordinates].location, rotation_target[robot_move_orientation]);
            auto forward_move = tick_agent_moves.get_forward_move(robot_move_orientation);

            // correction
            if (robot_move_orientation  == 0 || robot_move_orientation == 6) { // moving E or W
                y_correction = 0;
                x_correction = (int32_t)(location_error.x * P_x);
                if (robot_move_orientation == 6){
                    x_correction = -x_correction;
                }
            } else { // Moving N or S
                x_correction = 0;
                y_correction = (int32_t) (location_error.y * P_y);
                if (robot_move_orientation == 1 || robot_move_orientation == 2 || robot_move_orientation == 3 || robot_move_orientation == 4 || robot_move_orientation == 5){ // moving south
                    y_correction = -y_correction;
                }
            }
//            cout << "x_correct " << x_correction << " y correct " << y_correction << endl;
            message.left = forward_move.left_ticks + x_correction + y_correction;
            message.right = forward_move.right_ticks + x_correction + y_correction;
            message.speed = forward_move.speed;
            cout << "TICK MOVE 2: " << forward_move << endl;
            update();
            current_coordinates += best_move ;
        } else {
            // update expected coordinates
            current_coordinates += best_move ;
            move_targets.emplace(move_number, map[current_coordinates].location, rotation_target[robot_move_orientation]);
        }
        if (move_state == completed_move ) move_state = translate;
    }

    Tick_move_target::Tick_move_target(int move_number, cell_world::Location location, float rotation):
    move_number(move_number), location(location), rotation(rotation){

    }

    void Tick_robot_agent::correct_robot(){
        cout << "Correcting..." << endl;
        move_state = correct;
        Coordinates coordinates;
        float rotation1;
        float rotation2;
        auto robot_state = tracking_client.get_current_state("predator");
        auto robot_theta = to_radians(robot_state.rotation);
        auto best_candidate = cells[cells.find(robot_state.location)].coordinates;
        auto candidates = world.connection_pattern.get_candidates(best_candidate);
        auto best_angle_diff = angle_difference(robot_state.location.atan(map[best_candidate].location), robot_theta);
        for (auto &candidate:candidates) {
            if (map.find(candidate)!=Not_found && !map[candidate].occluded){
                auto angle_diff = angle_difference(robot_state.location.atan(map[candidate].location), robot_theta);
                if (angle_diff < best_angle_diff){
                    best_candidate = candidate;
                    best_angle_diff = angle_diff;
                }
            }
        }
        coordinates = best_candidate;
        float new_theta = robot_state.location.atan(map[best_candidate].location);
        rotation1 = to_degrees(new_theta);

        float new_theta2 = to_radians(rotation_target[0]);
        best_angle_diff = angle_difference(new_theta, to_radians(rotation2));
        for(auto rt : rotation_target) {
            auto angle_diff = angle_difference(new_theta,
                                               to_radians(rt));
            if (angle_diff < best_angle_diff){
                new_theta2 = to_radians(rt);
                best_angle_diff = angle_diff;
            }
        }
        rotation2 = to_degrees(new_theta2);
//        cout << "parameters found: " <<rotation1 << coordinates << rotation2 << endl;
        set_rotation(rotation1);
//        cout << "rotation1 finished .. " << endl;
        set_coordinate(coordinates);
//        cout << "coordinates finished .. "<< endl;
        set_rotation(rotation2);
//        cout << "rotation2 finished .. " << endl;
        needs_correction_now  = false;

        cout << "done correcting" << endl;
        cout << "Currrent Coordinate: " << current_coordinates << " Robot move orientation: " << robot_move_orientation << endl;
        move_state = translate;
    }

    // when called set rotation -- then translate to destination -- then set rotation again
    void Tick_robot_agent::set_rotation(float rotation) {
        cout << "ROTATE CORRECT" << endl;
        move_state = correct;
        if (!tracking_client.contains_agent_state("predator")) return;
        auto target = to_radians(rotation);
        auto &tracking_info = tracking_client.get_current_state("predator");

        auto theta = to_radians(tracking_info.rotation);
        auto error = angle_difference(theta, target);
        auto error_direction = direction(theta, target);

        // if rotation takes to long move backwards and repeat
        float t0 = timer.to_seconds();
        float delta_t = 0;
        float t1 = 0;
        bool loop1 = true;

        while (to_degrees(error) > 1 && !joystick_on){  // exit corrector when joystick control
            if (error_direction < 0) {
                message.left = 20;
                message.right = -20;
                message.speed = 2000;
            }else {
                message.left = -20;
                message.right = 20;
                message.speed = 2000;
            }
            t1 = timer.to_seconds();
            delta_t = t1 - t0;
            if (delta_t > 4.0){
                cout << "BACK UP!" << endl;
                message.left = -300;
                message.right = -300;
                message.speed = 2000;
                t0 = t1;
            }


            // move number management
            if (!use_joystick()) {
                auto move_number = update();
                while (completed_move != move_number) this_thread::sleep_for(10ms);

                theta = to_radians(tracking_info.rotation);
                error = angle_difference(theta, target);
                error_direction = direction(theta, target);
            }
        }
        // TODO: coordinate and robot_orientation management
        // robot orientation management
        auto new_orientation = get_corrected_orientation(rotation);
        if (new_orientation != 12) robot_move_orientation = new_orientation;

        // error management (reset)
        orientation_error = 0;
    }

    // send coordinate instead of location
    void Tick_robot_agent::set_coordinate(cell_world::Coordinates correction_coordinate){
        cout << "SET COORDINATE" << endl;
        move_state = correct;
        if (!tracking_client.contains_agent_state("predator")) return;
        auto &tracking_info = tracking_client.get_current_state("predator");

        auto current_location = tracking_info.location;
        auto correction_location = map[correction_coordinate].location;

        auto distance_error = current_location.dist(correction_location);
        float previous_distance_error = distance_error + 1;

        int overshoot_count = 0;

        // error timer - consider time and distance
        auto t0 = timer.to_seconds();
        float delta_t = 0;
        float t1 = 0;

        // will always be moving fwd to destination so just go slow will exit if overshoot
        while(distance_error > 0.003 && overshoot_count < 2 && !joystick_on){
            message.left = 30;
            message.right = 30;
            message.speed = 2000;

            // move number management
            if (!use_joystick()) {
                auto move_number = update();
                while (completed_move != move_number) this_thread::sleep_for(10ms);

                current_location = tracking_info.location;
                previous_distance_error = distance_error;
                distance_error = current_location.dist(correction_location);

                if (previous_distance_error < distance_error) overshoot_count++;
                else overshoot_count = 0; // reset overshoot count want sequential confirmation
            }


        }
        // robot coordinate management
        current_coordinates = correction_coordinate;

        // error management // TODO:test this
        location_error.x = 0;
        location_error.y = 0;

    }

    unsigned int Tick_robot_agent::get_corrected_orientation(float rotation){
        // {90.0, 120.0, 150.0, 180.0, 210.0, 240.0, 270.0, 300.0, 330.0, 0.0, 30.0, 60.0}
        if (rotation == 90.0) return 0;
        else if (rotation == 120.0 ) return 1;
        else if (rotation == 150.0) return 2;
        else if (rotation == 180.0) return 3;
        else if (rotation == 210.0 || rotation == -150.0) return 4;
        else if (rotation == 240.0 || rotation ==-120.0) return 5;
        else if (rotation == 270.0 || rotation ==-90.0) return 6;
        else if (rotation == 300.0 || rotation ==-60.0) return 7;
        else if (rotation == 330.0 || rotation == -30.0) return 8;
        else if (rotation == 0.0) return 9;
        else if (rotation == 30.0) return 10;
        else if (rotation == 60.0) return 11;
        else{
//            cout << "no orientation found" << endl;
            return 12;
        }
    }

    void Tick_robot_agent::joystick_control(){
        cout << "JOYSTICK ON" << endl;
        while (!tick_gamepad.buttons.empty() && tick_gamepad.buttons[5].state == 1){
            float left = (float)-tick_gamepad.axes[1]/JOYSTICK;
            float right = (float)-tick_gamepad.axes[4]/JOYSTICK;
            float move_straight = (float)-tick_gamepad.axes[7]/JOYSTICK * STRAIGHT_TICKS;

            message.left = (int)(left * MAX_TICKS) + move_straight;
            message.right = (int)(right * MAX_TICKS) + move_straight;
            message.speed = 1000;

            // move number management
            auto move_number = update();
            if (move_number != -1) cout << move_number << " " << completed_move<< endl;
            //while(completed_move!=move_number) this_thread::sleep_for(10ms);
        }
        //needs_correction_now = true;
        cout << "JOYSTICK OFF " << endl;
        joystick_on = false;
    }

    void Tick_robot_agent::capture(){
        // test could just add double to embedded
        message.speed = -2;
        update();
    }

    bool Tick_robot_agent::needs_correction() {
        return needs_correction_now;
    }

    bool Tick_robot_agent::use_joystick(){
        if (!tick_gamepad.buttons.empty() && tick_gamepad.buttons[5].state == 1){
            joystick_on = true;
        } else joystick_on = false;
        return joystick_on;
    }

}


//            auto x_err = location_error.x * 2.34 * 100; // cm
//            auto y_err = location_error.y * 2.34 * 100; // cm