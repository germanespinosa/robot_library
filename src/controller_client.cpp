#include <controller/controller_client.h>

using namespace tcp_messages;
using namespace cell_world;

namespace controller {

    bool Controller_client::pause() {
        return send_request(Message("pause_predator")).get_body<bool>();
    }

    bool Controller_client::resume() {
        return send_request(Message("resume_predator")).get_body<bool>();
    }

    bool Controller_client::stop() {
        return send_request(Message("stop_predator")).get_body<bool>();
    }

    bool Controller_client::set_destination(const cell_world::Location &location) {
        return send_request(Message("set_destination", location)).get_body<bool>();
    }

    bool Controller_client::prey_acquired() {
        return send_request(Message("prey_acquired")).get_body<bool>();
    }

    World_info Controller_client::get_world_info() {
        return send_request(Message("get_world_info")).get_body<World_info>();
    }

    bool Controller_client::set_behavior(Behavior behavior) {
        return send_request(Message("set_behavior", behavior)).get_body<bool>();
    }
}