#include <math.h>
#include <cell_world.h>
#include <robot_lib/robot_simulator.h>
#include <json_cpp.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <robot_lib/tracking_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {

    double robot_speed = .2;
    double robot_rotation_speed = M_PI ; // 90 degrees at full speed
    Robot_state robot_state;
    Tick_robot_state prey_robot_state;

    Cell_group robot_cells;
    Timer robot_time_stamp;
    World robot_world;
    Polygon habitat_polygon;
    Polygon_list cell_polygons;
    int frame_number = 0;
    mutex rm;

    unsigned int robot_interval = 50;
    atomic<bool> robot_running = false;
    atomic<bool> robot_finished = false;

    Tracking_simulator Robot_simulator::tracking_server;

    Robot_simulator_server predator_server;
    Prey_robot_simulator_server prey_server;


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
        time_stamp = json_cpp::Json_date::now();
        double dl = ((double)left) / 128.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        double dr = -((double)right) / 128.0 * robot_rotation_speed * elapsed; // convert motor signal to angle
        double d = ((double)left + (double)right) / 255.0 * robot_speed * elapsed; // convert motor signal to speed
        theta = normalize(theta + dl + dr);
        auto new_location = location.move(theta, d);
        if (habitat_polygon.contains(new_location)) {
            if (!cell_polygons.contains(new_location)) {
                location = location.move(theta, d);
            }
        }
        rm.unlock();
    }

    Step Robot_state::to_step() const {
        Step info;
        info.agent_name = "predator";
        info.location = location;
        info.rotation = to_degrees(theta);
        info.time_stamp = robot_time_stamp.to_seconds();
        info.frame = frame_number;
        return info;
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
        } else {
            string message_s (buff);
            try {
                auto message = json_cpp::Json_create<Message>(message_s);
                if (message.header == "stop") {
                    end_simulation();
                    Message response;
                    response.header = "result";
                    response.body = "ok";
                    send_data(response.to_json());
                }
            } catch (...) {

            }
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
            frame_number ++;
            robot_state.update();
            auto step = robot_state.to_step();
            Robot_simulator::tracking_server.send_update(step);
            prey_robot_state.update();
            auto prey_step = prey_robot_state.to_step();
            Robot_simulator::tracking_server.send_update(prey_step);
            std::this_thread::sleep_for(std::chrono::milliseconds(robot_interval));
        }
        log.close();
    }


    thread simulation_thread;

    void Robot_simulator::start_simulation(cell_world::World world, Location location, float rotation, Location prey_location, float prey_rotation, unsigned int interval) {
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
        robot_state.left = 0;
        robot_state.right = 0;
        robot_state.puff = false;
        robot_state.led0 = false;
        robot_state.led1 = false;
        robot_state.led2 = false;
        robot_interval = interval;
        robot_running = true;
        prey_robot_state.queued.move_number = 0;
        prey_robot_state.in_progress.move_number = 0;
        prey_robot_state.location = prey_location;
        prey_robot_state.theta = prey_rotation;
        simulation_thread=thread(&simulation);
        if (!predator_server.start(Robot_agent::port())) {
            std::cout << "Predator server setup failed " << std::endl;
            exit(EXIT_FAILURE);
        }
        if (!prey_server.start(Tick_robot_agent::port())) {
            std::cout << "Prey server setup failed " << std::endl;
            exit(EXIT_FAILURE);
        }
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

    Tick_robot_state Robot_simulator::get_prey_robot_state() {
        return prey_robot_state;
    }

    void Robot_simulator::stop_simulation() {
        prey_server.stop();
        predator_server.stop();
    }

    void Tick_robot_state::update() {
        auto now = std::chrono::system_clock::now();
        double elapsed = 0;
        if (initialized) {
            elapsed = ((double) std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count()) / 1000.0;
        }
        update(elapsed);
        last_update = now;
        initialized = true;
    }

    mutex prm;

#define tick_robot_error_range .001
#define tick_robot_error (tick_robot_error_range * Chance::dice_float(1) - tick_robot_error_range / 2)

    void Tick_robot_state::update(double elapsed) {
        prm.lock();

        // simulation
        left_tick_counter_f += elapsed * left_speed;    // elapsed: time between updates (0.03), left/right_speed: tick rate
        right_tick_counter_f += elapsed * right_speed;

        float dl = 2 * left_speed / 1800.0 * robot_rotation_speed * elapsed ; // convert motor signal to angle
        float dr = 2 * (-right_speed) / 1800.0 * robot_rotation_speed * elapsed ; // convert motor signal to angle
        float d = (left_speed + right_speed) / 3600.0 * robot_speed * elapsed ; // convert motor signal to speed
        theta = normalize(theta + dl + dr);
        auto new_location = location.move(theta, d);

        if (habitat_polygon.contains(new_location)) {
            if (!cell_polygons.contains(new_location)) {
                location = location.move(theta, d);
            }
        }
        time_stamp = json_cpp::Json_date::now();
        left_tick_counter = left_tick_counter_f;
        right_tick_counter = right_tick_counter_f;

        //controller starts
        static int prev_left_tick_counter = left_tick_counter;
        static int prev_right_tick_counter = right_tick_counter;

        // if we went over the target since last iteration
        if ( (prev_left_tick_counter >= left_tick_target && left_tick_counter <= left_tick_target) ||
             (prev_left_tick_counter <= left_tick_target && left_tick_counter >= left_tick_target) ){
            if (in_progress.move_number) {
                for (auto &client: prey_server.clients) {
                    client->send_data((char *) &in_progress.move_number, sizeof(in_progress.move_number));
                }
            }
            if (queued.move_number) {
                in_progress = queued;
                left_tick_target += in_progress.left;
                right_tick_target += in_progress.right;
                speed = in_progress.speed;
                queued.move_number = 0;
            }
        }

        float left_tick_error = left_tick_target - left_tick_counter;       // tick goal - current ticks
        float right_tick_error = right_tick_target - right_tick_counter;

        float direction_L=left_tick_error>0?1:-1;
        float direction_R=right_tick_error>0?1:-1;

        // Find left and right speed based on tick error
        if (abs(left_tick_error) > abs(right_tick_error)) {
            left_speed = direction_L * speed;
            right_speed = direction_R * abs(right_tick_error) / abs(left_tick_error) * speed;
        } else {
            if (abs(left_tick_error) < abs(right_tick_error)) {
                right_speed = direction_R * speed;
                left_speed = direction_L * abs(left_tick_error) / abs(right_tick_error) * speed;
            } else {
                right_speed = direction_R * speed;
                left_speed = direction_L * speed;
            }
        }
        prm.unlock();
    }

    cell_world::Step Tick_robot_state::to_step() const {
        Step info;
        info.agent_name = "prey";
        info.location = location;
        info.rotation = to_degrees(theta);
        info.time_stamp = robot_time_stamp.to_seconds();
        info.frame = frame_number;
        return info;
    }

    void Prey_robot_simulator::on_connect() {
        Service::on_connect();
    }

    void Prey_robot_simulator::on_disconnect() {
        Service::on_disconnect();
    }

    void Prey_robot_simulator::on_incoming_data(const char *buff, int size) {
        Tick_robot_agent::Robot_message message;
        int offset = 0;
        while (size - offset >= sizeof (message)){ // instruction
            message = *((Tick_robot_agent::Robot_message *)buff + offset);
            while (prey_robot_state.queued.move_number);
            prey_robot_state.queued =  message;
            offset += sizeof (message);
        }
    }
}