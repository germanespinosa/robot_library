#include <math.h>
#include <cell_world.h>
#include <robot_lib/robot_simulator.h>
#include <json_cpp.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <robot_lib/tracking_simulator.h>
#include <robot_lib/prey_simulator.h>

// TODO: add system argument to modify code depending on whether controller or tuner is being used

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {

    double robot_speed = .2;
    double robot_rotation_speed = M_PI ; // 90 degrees at full speed
    Robot_agent local_robot;
    Robot_state robot_state;
    Cell_group robot_cells;
    Timer robot_time_stamp;
    World robot_world;
    Polygon habitat_polygon;
    Polygon_list cell_polygons;
    int frame_number = 0;
    mutex rm;
    Tracking_simulator *tracking_simulator = nullptr;
    Fake_robot_server fake_robot_server(local_robot);
    Robot_simulator_server *robot_simulator_server;

    Prey_simulator_server prey_simulator;

    unsigned int robot_interval = 50;
    atomic<bool> robot_running = false;
    atomic<bool> robot_finished = false;

    void Robot_state::update() {
        auto now = std::chrono::system_clock::now();
        double elapsed = 0;
        if (initialized) {
            elapsed = ((double) std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count()) / 1000.0;
        }
        update(elapsed);
        last_update = now;
        initialized = true;
    }


    void Robot_state::update(double elapsed) {
        rm.lock();
        // TODO: deal with case where messages sent exceed array size
        time_stamp = json_cpp::Json_date::now();

        // catch and store new messages
        if ((left_tick_target != prev_tick_target_L) || (right_tick_target != prev_tick_target_R)){
            speed_array[message_count] = speed;
            left_tick_target_array[message_count] = left_tick_target;
            right_tick_target_array[message_count]= right_tick_target;

            // store desired direction of robot for move request
            if (left_tick_target > prev_tick_target_L){
                left_direction_array[message_count] = 1.0;
            } else if (left_tick_target < prev_tick_target_L)  left_direction_array[message_count] = -1.0;
            if (right_tick_target > prev_tick_target_R){
                right_direction_array[message_count] = 1.0;
            } else if (right_tick_target < prev_tick_target_R)  right_direction_array[message_count] = -1.0;
            message_count += 1;
        }
        direction_L = left_direction_array[move_number] ;
        direction_R = right_direction_array[move_number];

        float left_tick_error = left_tick_target_array[move_number]-left_tick_counter;       // tick goal - current ticks
        float right_tick_error = right_tick_target_array[move_number]-right_tick_counter;

        // if move completed stop or execute next move - when error is 0
        if (!left_tick_error || !right_tick_error){

            // for multiple messages
            if ((message_count > 0) && (message_count > move_number)){
                for (auto &client:robot_simulator_server->clients){
                    client->send_data((char *)&move_number,sizeof(move_number));
                }
                cout << "SHOULD BE TRUE  " << local_robot.is_move_done() << endl;
                cout << "MOVES_EXECUTED" << move_number << endl;
                cout << "left_ticks: " << left_tick_counter << "    right_ticks: " << right_tick_counter << endl;
                move_number += 1;
            }
        }

        // Find left and right speed based on tick error
        if (abs(left_tick_error) > abs(right_tick_error)) {
            robot_state.left_speed = direction_L * speed_array[move_number];
            robot_state.right_speed = direction_R * abs(right_tick_error) / abs(left_tick_error) * speed;
        } else {
            if (abs(left_tick_error) < abs(right_tick_error)) {
                robot_state.right_speed = direction_R * speed_array[move_number];
                robot_state.left_speed = direction_L * abs(left_tick_error) / abs(right_tick_error) * speed;
            } else {
                robot_state.right_speed = direction_R * speed_array[move_number];
                robot_state.left_speed = direction_L * speed_array[move_number];
            }
        }

        // check if error is zero or if speed needs to be reduced to prevent overshoot on last move
        if ((abs(left_tick_error) < abs(left_speed * elapsed)) || (abs(right_tick_error) < abs(right_speed * elapsed))){
            left_speed = left_tick_error/ elapsed;
            right_speed = right_tick_error/ elapsed;
        }

        float dl = 2 *left_speed / 1800.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        float dr = 2 *- right_speed / 1800.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        float d = (left_speed + right_speed) / 3600.0 * robot_speed * elapsed; // convert motor signal to speed
        theta = normalize(theta + dl + dr);
        auto new_location = location.move(theta, d);

        if (habitat_polygon.contains(new_location)) {
            if (!cell_polygons.contains(new_location)) {
                location = location.move(theta, d);
            }
        }
        // Measure position of robot
        left_tick_counter += elapsed * left_speed;    // elapsed: time between updates (0.03), left/right_speed: tick rate
        right_tick_counter += elapsed * right_speed;

        // store tick target
        prev_tick_target_L = left_tick_target;
        prev_tick_target_R = right_tick_target;
        rm.unlock();
    }

    Step Robot_state::to_step() const {
        Step info;
        info.agent_name = "predator";
        info.location = location;
        info.rotation = to_degrees(theta);
        info.time_stamp = robot_time_stamp.to_seconds();
        info.frame = ++frame_number;
        return info;
    }

    void Robot_simulator::on_connect() {
        Service::on_connect();
    }


    void Robot_simulator::on_incoming_data(const char *buff, int size) {
        // robot agent -> robot sim
        struct Robot_message {
            int32_t left, right, speed;
        } message;
        if (size == sizeof(message)){ // instruction
            message = *((Robot_message *)buff);
            rm.lock();
            robot_state.speed = message.speed;
            robot_state.left_tick_target += message.left;
            robot_state.right_tick_target += message.right;
            rm.unlock();
        }
    }

    void Robot_simulator::on_disconnect() {
        Service::on_disconnect();
    }

    void Robot_simulator::set_robot_speed(double speed) {
        robot_speed = speed;
    }

    void simulation (){
        ofstream log;
        while (robot_running){
            std::this_thread::sleep_for(std::chrono::milliseconds(robot_interval));
            robot_state.update();
            auto step = robot_state.to_step();
            tracking_simulator->send_update(step);
        }
        log.close();
    }


    thread simulation_thread;


    void Robot_simulator::start_simulation(cell_world::World world, Location location, double rotation, unsigned int interval, Tracking_simulator &new_tracking_simulator, Robot_simulator_server &new_robot_simulator_server) {
        robot_simulator_server = &new_robot_simulator_server;
        tracking_simulator = &new_tracking_simulator;
        robot_world = world;
        habitat_polygon = Polygon(robot_world.space.center, robot_world.space.shape, robot_world.space.transformation);
        robot_cells = robot_world.create_cell_group();
        cell_polygons.clear();
        auto occluded_cells = robot_world.create_cell_group().occluded_cells();
        for (auto &cell:occluded_cells) {
            cell_polygons.push_back(Polygon(cell.get().location,robot_world.cell_shape, robot_world.cell_transformation));
        }
        robot_state.location = location;
        robot_state.theta = rotation;
        robot_state.left_tick_counter = 0;
        robot_state.right_tick_counter = 0;
        robot_state.left_speed = 0;
        robot_state.right_speed = 0;
        robot_state.puff = false;
        robot_state.led0 = false;
        robot_state.led1 = false;
        robot_state.led2 = false;
        robot_interval = interval;
        robot_running = true;
        simulation_thread=thread(&simulation);

        if (!local_robot.connect("127.0.0.1")){
            std::cout << "Can't connect to robot " << std::endl;
            exit(1);
        };
        fake_robot_server.start(6300);
    }

    Robot_state Robot_simulator::get_robot_state() {
        return robot_state;
    }

    void Robot_simulator::end_simulation() {
        robot_running = false;
        simulation_thread.join();
        robot_finished = true;
    }

    void Robot_simulator::set_robot_rotation_speed(double rotation_speed) {
        robot_rotation_speed = rotation_speed;
    }

    bool Robot_simulator::is_running() {
        return !robot_finished;
    }

    void Robot_simulator::set_occlusions(cell_world::Cell_group_builder occlusions) {
        robot_world.set_occlusions(occlusions);
        cell_polygons.clear();
        auto occluded_cells = robot_world.create_cell_group().occluded_cells();
        for (auto &cell:occluded_cells) {
            cell_polygons.push_back(Polygon(cell.get().location,robot_world.cell_shape, robot_world.cell_transformation));
        }
    }

    bool Robot_simulator::start_prey() {
        return prey_simulator.start(Prey_simulator_service::get_port());
    }

    bool Fake_robot_service::set_left(int v) {
        return ((Fake_robot_server *)this->_server)->set_left(v);
    }

    bool Fake_robot_service::set_right(int v) {
        return ((Fake_robot_server *)this->_server)->set_right(v);
    }

    bool Fake_robot_service::set_speed(int v) {
        return ((Fake_robot_server *)this->_server)->set_speed(v);
    }

    bool Fake_robot_service::update() {
        return ((Fake_robot_server *)this->_server)->update();
    }

    bool Fake_robot_service::is_move_done() {
        return ((Fake_robot_server *)this->_server)->robot.is_move_done();
    }
}