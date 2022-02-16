#pragma once
#include <tcp_messages.h>
#include <cell_world.h>

namespace controller {

    enum Predator_state{
        Stopped,
        Playing,
        Paused,
    };

    struct Controller_service : tcp_messages::Message_service {
        Routes (
            Add_route_with_response("set_destination", set_destination, cell_world::Location);
            Add_route_with_response("prey_acquired", prey_acquired);
            Add_route_with_response("stop_predator", stop_predator);
            Add_route_with_response("pause_predator", pause_predator);
            Add_route_with_response("resume_predator", resume_predator);
            Add_route_with_response("get_world_info", get_world_info);
            Allow_subscription();
        );

        static bool set_destination(const cell_world::Location &);

        static bool prey_acquired();

        static bool stop_predator();

        static bool pause_predator();

        static bool resume_predator();

        static cell_world::World_info get_world_info();

        static int get_port();
    };

    struct Controller_server : tcp_messages::Message_server<Controller_service> {
        void send_step(const cell_world::Step &);
        void send_world_info_update(const  cell_world::World_info &);
        static bool get_destination(cell_world::Location &);
        static std::atomic<Predator_state> predator_state;
    };

}