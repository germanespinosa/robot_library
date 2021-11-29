#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <robot.h>
#include <params_cpp.h>
#include <agent_tracking/client.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;


int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--rotation"};
    Key interval_key{"-i","--interval"};

    Parser p(argc, argv);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    World world(wc, wi);

    Cell_group cells = world.create_cell_group();
    Map map(cells);
    auto rotation = stof(p.get(rotation_key));
    auto interval = stoi(p.get(interval_key));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key);
    Coordinates spawn_coordinates;
    cout << spawn_coordinates_str << endl;
    try {
        spawn_coordinates = json_cpp::Json_create<Coordinates>(spawn_coordinates_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    Location location = map[spawn_coordinates].location;
    Robot_simulator::start_simulation(world, location, rotation, interval);
    Server<Robot_simulator> server;
    if (!server.start(Robot::port())) {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }
    agent_tracking::Client tracker;
    tracker.connect();
    tracker.register_consumer();
    while (Robot_simulator::is_running())
        if (tracker.contains_agent_state("predator")){
            cout << "track: " << tracker.get_current_state("predator") << endl;
            Timer::wait(.5);
        }
    server.stop();
    return 0;
}
