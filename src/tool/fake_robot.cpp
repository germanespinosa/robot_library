#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <robot.h>
#include <params_cpp.h>
#include <agent_tracking/client.h>
#include <tracking_simulator.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;


int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key interval_key{"-i","--interval"};
    Key frame_drop_key{"-fd","--frame_drop"};
    Key noise_key{"-n","--noise"};
    Key bad_reads_key{"-br","--bad_reads"};
    Key occlusions_key{"-o","--occlusions"};

    Parser p(argc, argv);
    auto occlusions_name = p.get(occlusions_key);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).key("occlusions").get_resource<Cell_group_builder>();
    World world(wc, wi, occlusions);
    auto default_coordinates = world.create_cell_group().free_cells().random_cell().coordinates.to_json();
    Cell_group cells = world.create_cell_group();
    Map map(cells);
    auto rotation = stof(p.get(rotation_key,"0"));
    auto interval = stoi(p.get(interval_key,"100"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, default_coordinates);
    auto frame_drop = stof(p.get(frame_drop_key,".1"));
    auto noise = stof(p.get(noise_key,".001"));
    auto bad_reads = stof(p.get(bad_reads_key,".01"));

    Tracking_simulator::set_frame_drop(frame_drop);
    Tracking_simulator::set_noise(noise);
    Tracking_simulator::set_bad_reads(bad_reads);

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
