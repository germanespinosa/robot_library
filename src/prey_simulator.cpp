#include <robot_lib/prey_simulator.h>
using namespace std;


int robot::Prey_simulator_service::get_port() {
    string port_str (std::getenv("PREY_PORT")?std::getenv("PREY_PORT"):"4630");   // 4500
    return atoi(port_str.c_str());
}

void robot::Prey_simulator_service::prey_location(const cell_world::Location &location) {
    auto server = ((Prey_simulator_server *)_server);
    server->last_update = cell_world::Timer(10);
    server->location = location;
}

void robot::Prey_simulator_service::prey_rotation(double rotation) {
    auto server = ((Prey_simulator_server *)_server);
    server->last_update = cell_world::Timer(10);
    server->rotation = rotation;
}

robot::Prey_simulator_server::Prey_simulator_server(): last_update(-1) {

}
