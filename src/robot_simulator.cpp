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
        info.frame = ++frame_number;
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
            robot_state.update();
            auto step = robot_state.to_step();
            tracking_simulator->send_update(step);
            if (!prey_simulator.last_update.time_out()) {
                auto prey_step = step;
                prey_step.location = prey_simulator.location;
                prey_step.rotation = prey_simulator.rotation;
                prey_step.agent_name = "prey";
                tracking_simulator->send_update(prey_step);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(robot_interval));
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
        robot_state.left = 0;
        robot_state.right = 0;
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