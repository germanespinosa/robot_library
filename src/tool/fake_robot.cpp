#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <robot.h>
#include <params_cpp.h>
#include <agent_tracking/tracking_client.h>
#include <tracking_simulator.h>
#include <cell_world.h>
#include <experiment.h>
#include <controller.h>
#include <map>
#include <robot_agent.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;
using namespace experiment;
using namespace controller;

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
};

int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key interval_key{"-i","--interval"};

    Parser p(argc, argv);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    auto capture_parameters = Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>();
    auto peeking_parameters = Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>();


    World world(wc, wi);

    Capture capture(capture_parameters, world);
    Peeking peeking(peeking_parameters, world);
    Cell_group cells = world.create_cell_group();
    Location_visibility visibility(cells, wc.cell_shape, wi.cell_transformation);

    auto default_coordinates = world.create_cell_group().free_cells().random_cell().coordinates.to_json();
    Map map(cells);
    auto rotation = stof(p.get(rotation_key,"0"));
    auto interval = stoi(p.get(interval_key,"30"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, default_coordinates);
    auto verbose = p.contains(Key("-v"));

    Experiment_service::set_logs_folder("experiment_logs/");
    Experiment_server experiment_server;
    experiment_server.start(Experiment_service::get_port());

    auto &controller_experiment_client = experiment_server.create_local_client<Controller_server::Controller_experiment_client>();
    controller_experiment_client.subscribe();
    std::map<string, string> experiment_occlusions;
    Tracking_simulator tracking_server;

    auto &tracking_client = tracking_server.create_local_client<Controller_server::Controller_tracker>(visibility, 90, capture, std::ref(controller_experiment_client), peeking, "predator", "prey");

    auto &experiment_client= experiment_server.create_local_client<Robot_experiment_client>(experiment_occlusions);
    experiment_client.subscribe();


    Coordinates spawn_coordinates;
    cout << spawn_coordinates_str << endl;
    try {
        spawn_coordinates = json_cpp::Json_create<Coordinates>(spawn_coordinates_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    Location location = map[spawn_coordinates].location;
    Robot_simulator::start_simulation(world, location, rotation, interval, tracking_server);
    Server<Robot_simulator> server;
    if (!server.start(Robot::port())) {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }
    Agent_operational_limits limits;
    limits.load("../config/robot_simulator_operational_limits.json");
    Robot_agent robot(limits);
    robot.connect("127.0.0.1");


    Controller_service::set_logs_folder("controller_logs/");
    Controller_server controller_server("../config/pid.json", robot, tracking_client, controller_experiment_client);
    if (!controller_server.start(Controller_service::get_port())) {
        cout << "failed to start controller" << endl;
        exit(1);
    }

    auto &tracker = tracking_server.create_local_client<agent_tracking::Tracking_client>();
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
