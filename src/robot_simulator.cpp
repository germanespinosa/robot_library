#include <math.h>
#include <cell_world.h>
#include <robot_lib/robot_simulator.h>
#include <json_cpp.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <robot_lib/tracking_simulator.h>
#include <robot_lib/prey_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {

    double robot_speed = .2;
    double robot_rotation_speed = M_PI ; // 90 degrees at full speed
    Robot_state robot_state;
    Cell_group robot_cells;
    Timer robot_time_stamp;
    World robot_world;
    Polygon habitat_polygon;
    Polygon_list cell_polygons;
    int frame_number = 0;
    mutex rm;
    Tracking_simulator *tracking_simulator = nullptr;

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
        // proportion is based on initial input since it will not change in sim
        // TODO: state machine to manage sending multiple commands
        // TODO: really only have to do this calculation when get new tick command

        time_stamp = json_cpp::Json_date::now();

        // tick target is cumulative so catch incoming commands with tick target
        if ((left_tick_target != prev_tick_target_L) || (right_tick_target != prev_tick_target_R)){
            speed_array[message_count] = speed;
            left_tick_count_array[message_count] = left_tick_target;
            right_tick_count_array[message_count]= right_tick_target;
            cout << "NEW  " << left_tick_count_array[message_count] << endl;
            message_count += 1;
        }


        // update direction of robot when target changes
        if (left_tick_target > prev_tick_target_L){
            direction_L = 1.0;
        } else if (left_tick_target < prev_tick_target_L)  direction_L = -1.0;
        if (right_tick_target > prev_tick_target_R){
            direction_R = 1.0;
        } else if (right_tick_target < prev_tick_target_R)  direction_R = -1.0;


        // Find left and right speed based on goal ticks
        if (abs(left_tick_target) > abs(right_tick_target)) {
            robot_state.left_speed = direction_L * speed;
            robot_state.right_speed = direction_R * (float)abs(right_tick_target) / (float)abs(left_tick_target) * speed;
        } else {
            if (abs(left_tick_target) < abs(right_tick_target)) {
                robot_state.right_speed = direction_R * speed;
                robot_state.left_speed = direction_L * (float)abs(left_tick_target) / (float)abs(right_tick_target) * speed;
            } else {
                robot_state.right_speed = direction_R * speed;
                robot_state.left_speed = direction_L * speed;
            }
        }

        // Find tick count based on speed
        left_tick_counter_float += elapsed * left_speed;    // elapsed: time between updates (0.03), left/right_speed: tick rate
        right_tick_counter_float += elapsed * right_speed;
        left_tick_counter = left_tick_counter_float;        // cumulative number of ticks
        right_tick_counter = right_tick_counter_float;


        float left_tick_error = left_tick_target-left_tick_counter;       // tick goal - current ticks
        float right_tick_error = right_tick_target-right_tick_counter;


        // if goal state will be reached slow down robot and recompute tick count
        if ((direction_L > 0 && left_tick_error < 0) || (direction_L < 0 && left_tick_error > 0)) {
            left_speed = left_tick_error / elapsed;
            left_tick_counter = prev_left_tick_counter + (elapsed * left_speed);
            left_tick_error = 0;
        }
        if ((direction_R > 0 && right_tick_error < 0) || (direction_R < 0 && right_tick_error > 0)) {
            right_speed = right_tick_error / elapsed;
            right_tick_counter = prev_right_tick_counter + (elapsed * right_speed);
            right_tick_error = 0;
        }


        float dl = left_speed / 1800.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        float dr = - right_speed / 1800.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        float d = (left_speed + right_speed) / 3600.0 * robot_speed * elapsed; // convert motor signal to speed
        theta = normalize(theta + dl + dr);
        auto new_location = location.move(theta, d);

        if (habitat_polygon.contains(new_location)) {
            if (!cell_polygons.contains(new_location)) {
                location = location.move(theta, d);
            }
        }

        // if goal reached
        if (!left_tick_error || !right_tick_error){
            speed = 0;
        }
        // store tick target
        prev_left_tick_counter = left_tick_counter;
        prev_right_tick_counter = right_tick_counter;
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

    void Robot_simulator::start_simulation(cell_world::World world, Location location, double rotation, unsigned int interval, Tracking_simulator &new_tracking_simulator) {
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

}