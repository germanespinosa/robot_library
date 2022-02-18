#include <controller/controller_service.h>

using namespace cell_world;
using namespace tcp_messages;
using namespace std;

namespace controller {

    Location destination;
    bool new_destination = false;
    std::atomic<Predator_state> Controller_server::predator_state = Stopped;
    World_info world_info;


    bool Controller_service::set_destination(const cell_world::Location &location) {
        destination = location;
        new_destination = true;
        return true;
    }

    bool Controller_service::prey_acquired() {
        return true;
    }

    bool Controller_service::stop_predator() {
        Controller_server::predator_state = Stopped;
        return true;
    }

    bool Controller_service::pause_predator() {
        Controller_server::predator_state = Paused;
        return true;
    }

    bool Controller_service::resume_predator() {
        Controller_server::predator_state = Playing;
        return true;
    }

    cell_world::World_info Controller_service::get_world_info() {
        return world_info;
    }

    int Controller_service::get_port() {
        string port_str(std::getenv("CONTROLLER_PORT") ? std::getenv("CONTROLLER_PORT") : "4590");
        return atoi(port_str.c_str());
    }

    void Controller_server::send_step(const Step &step) {
        Message update (step.agent_name + "_step", step);
        broadcast_subscribed(update);
    }

    void Controller_server::send_world_info_update(const World_info &new_world_info) {
        world_info = new_world_info;
        Message update("world_update",world_info);
        broadcast_subscribed(update);
    }

    bool Controller_server::get_destination(cell_world::Location &location) {
        if (new_destination) {
            location = destination;
            new_destination = false;
            return true;
        }
        return false;
    }

}

