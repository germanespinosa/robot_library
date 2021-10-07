#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <chrono>
#include <cell_world_tools.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace robot;

struct Parameters : Parameters_builder{
    World world;
    Location location;
    double theta;
    int interval;
    int port;
    Parameters_definitions({
        Add_web_resource(world, ({"world"}));
        Add_value(location);
        Add_value(theta);
        Add_value(interval);
        Add_value(port);
    })
};

int main(int argc, char *argv[])
{
    Parameters p;
    p.load(argc, argv);
    Cell_group cells = p.world.create_cell_group();
    // start server on port 65123
    Robot_simulator::start_simulation(cells, p.location, p.theta, p.interval);
    Server<Robot_simulator> server;
    if (!server.start(p.port)) {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }
    while(Robot_simulator::is_running()){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    server.stop();
    return 0;
}
