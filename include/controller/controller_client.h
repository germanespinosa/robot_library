#include <tcp_messages.h>
#include <cell_world.h>

namespace controller{
    struct Controller_client : tcp_messages::Message_client {
        Routes(
                Add_route("(.*)(step)", on_step, cell_world::Step);
                Add_route("world_update", on_world_update, cell_world::World_info);
                )
        virtual void on_step(const cell_world::Step &) {}
        virtual void on_world_update(const cell_world::World_info &) {}
        bool pause();
        bool resume();
        bool stop();
        bool set_destination(const cell_world::Location &);
        bool prey_acquired();
        cell_world::World_info get_world_info();
    };
}