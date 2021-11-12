#include <math.h>
#include <cell_world.h>
#include <robot_simulator.h>
#include <json_cpp.h>
#include <mutex>
#include <atomic>
#include <chrono>

using namespace json_cpp;
using namespace std;
using namespace cell_world;

namespace robot {

    double robot_speed = .1;
    double robot_rotation_speed = M_PI / 2; //90 degrees at full speed
    Robot_state robot_state;
    cell_world::Cell_group robot_cells;
    unsigned int robot_interval = 50;
    atomic<bool> robot_running = false;
    atomic<bool> robot_finished = false;
    mutex rm;

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
        rotation = normalize(rotation + dl + dr);
        auto new_location = location.move(rotation,d);
        if (new_location.x >= 0 && new_location.x <= 1 && new_location.y >=0 && new_location.y <= 1)
            location = location.move(rotation,d);
        rm.unlock();
    }

    Step Robot_state::to_agent_info() const {
        rm.lock();
        Step info;
        info.agent_name = "predator";
        info.location = location;
        info.rotation = rotation;
        auto cell_id = robot_cells.find(info.location);
        if (cell_id == Not_found){
            info.coordinates = Cell::ghost_cell().coordinates;
        }else{
            info.coordinates = robot_cells[cell_id].coordinates;
        }
        rm.unlock();
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
                if (message.title == "stop") {
                    end_simulation();
                    Message response;
                    response.title = "result";
                    response.body = "ok";
                    send_data(response.to_json());
                } else if (message.title == "get_agent_info") {
                    Message response;
                    response.title = "set_agent_info";
                    response.body = robot_state.to_agent_info().to_json();
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
            cout << robot_state.to_agent_info() << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(robot_interval));
        }
        log.close();
    }

    thread simulation_thread;

    void Robot_simulator::start_simulation(const cell_world::Cell_group &cell_group,Location location, double rotation, unsigned int interval) {
        robot_cells = cell_group;
        robot_state.location = location;
        robot_state.rotation = rotation;
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

    int Robot_simulator::port() {
        string port_str (std::getenv("FAKE_ROBOT_PORT")?std::getenv("FAKE_ROBOT_PORT"):"5000");
        return atoi(port_str.c_str());
    }

}