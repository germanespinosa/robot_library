#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <robot.h>
#include <params_cpp.h>
#include <agent_tracking/tracking_client.h>
#include <tracking_simulator.h>
#include <experiment.h>
#include <controller.h>
#include <map>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;
using namespace experiment;
using namespace controller;

int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key interval_key{"-i","--interval"};
    Key frame_drop_key{"-fd","--frame_drop"};
    Key noise_key{"-n","--noise"};
    Key bad_reads_key{"-br","--bad_reads"};

    Parser p(argc, argv);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    World world(wc, wi);
    auto default_coordinates = world.create_cell_group().free_cells().random_cell().coordinates.to_json();
    Cell_group cells = world.create_cell_group();
    Map map(cells);
    auto rotation = stof(p.get(rotation_key,"0"));
    auto interval = stoi(p.get(interval_key,"30"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, default_coordinates);
    auto frame_drop = stof(p.get(frame_drop_key,".1"));
    auto noise = stof(p.get(noise_key,".001"));
    auto bad_reads = stof(p.get(bad_reads_key,".01"));
    auto verbose = p.contains(Key("-v"));

    Experiment_service::set_logs_folder("experiment_logs/");
    Experiment_server experiment_service;
    if (p.contains(Key("-e"))) {
        experiment_service.start(Experiment_service::get_port());
        cout << "experiment service started" << endl;
    }
    std::map<string, string> experiment_occlusions;
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
    Controller_service::set_logs_folder("controller_logs/");
    Controller_server controller_server("../config/pid.json", "127.0.0.1", "127.0.0.1", "127.0.0.1");
    if (!controller_server.start(Controller_service::get_port())) {
        cout << "failed to start controller" << endl;
        exit(1);
    }

    struct Robot_experiment_client : Experiment_client {
        Robot_experiment_client(std::map<string, string> &experiment_occlusions) :
                experiment_occlusions(experiment_occlusions){}
        void on_experiment_started(const Start_experiment_response &experiment) {
            experiment_occlusions[experiment.experiment_name] = experiment.world.occlusions;
        }
        void on_episode_started(const std::string &experiment_name) {
            auto occlusions = Resources::from("cell_group").
                    key("hexagonal").
                    key(experiment_occlusions[experiment_name]).
                    key("occlusions").get_resource<Cell_group_builder>();
            Robot_simulator::set_occlusions(occlusions);
        };
        std::map<string, string> &experiment_occlusions;
    } experiment_client(experiment_occlusions);

    experiment_client.connect("127.0.0.1");
    experiment_client.subscribe();

    agent_tracking::Tracking_client tracker;
    tracker.connect();
    tracker.subscribe();
    while (Robot_simulator::is_running())
        if (tracker.contains_agent_state("predator")){
            if (verbose) cout << "track: " << tracker.get_current_state("predator") << endl;
            Timer::wait(.5);
        }
    server.stop();
    return 0;
}
