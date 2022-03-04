#pragma once
#include <tcp_messages.h>
#include <cell_world.h>

namespace robot {
    struct Prey_simulator_service : tcp_messages::Message_service {
        Routes(
                Add_route("prey_location", prey_location, cell_world::Location)
                Add_route("prey_rotation", prey_rotation, double)
                )

        static int get_port();
        void prey_location(const cell_world::Location &location);
        void prey_rotation(double rotation);
    };

    struct Prey_simulator_server : tcp_messages::Message_server<Prey_simulator_service> {
        Prey_simulator_server();
        cell_world::Location location;
        double rotation;
        cell_world::Timer last_update;
    };
}