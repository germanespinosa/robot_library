#include <iostream>
#include <robot_lib/robot_simulator.h>
#include <params_cpp.h>
#include <agent_tracking/tracking_client.h>
#include <cell_world.h>
#include <experiment.h>
#include <controller.h>
#include <map>
#include <robot_lib/robot_agent.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;
using namespace experiment;
using namespace controller;

struct Robot_experiment_client : Experiment_client {
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
    std::map<string, string> experiment_occlusions;
};

int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key prey_key{"-p","--prey"};
    Key interval_key{"-i","--interval"};
    Key noise_key{"-n","--noise"};

    Parser p(argc, argv);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    auto capture_parameters = Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>();
    auto peeking_parameters = Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>();


    World world(wc, wi);
    Capture capture(capture_parameters, world);
    Peeking peeking(peeking_parameters, world);
    Cell_group cells = world.create_cell_group();
    Map map(cells);
    Location_visibility visibility(cells, wc.cell_shape, wi.cell_transformation);

    auto rotation = stof(p.get(rotation_key,"1.5707963267948966"));
    auto interval = stoi(p.get(interval_key,"30"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, "{\"x\":4,\"y\":0}");
    auto verbose = p.contains(Key("-v"));

    Experiment_service::set_logs_folder("experiment_logs/");
    Experiment_server experiment_server;

    auto &controller_experiment_client = experiment_server.create_local_client<Controller_server::Controller_experiment_client>();
    controller_experiment_client.subscribe();

    if (!p.contains(noise_key)){
        Robot_simulator::tracking_server.noise = 0;
        Robot_simulator::tracking_server.frame_drop = 0;
        Robot_simulator::tracking_server.bad_reads = 0;
    }
    auto &experiment_tracking_client = Robot_simulator::tracking_server.create_local_client<Experiment_tracking_client>();
    experiment_tracking_client.subscribe();
    experiment_server.set_tracking_client(experiment_tracking_client);
    experiment_server.start(Experiment_service::get_port());


    auto &tracking_client = Robot_simulator::tracking_server.create_local_client<Controller_server::Controller_tracking_client>(visibility, 90, capture, peeking, "predator", "prey");
    auto &experiment_client= experiment_server.create_local_client<Robot_experiment_client>();
    experiment_client.subscribe();

    Coordinates prey_spawn_coordinates(-20,0);

    Coordinates spawn_coordinates;
    cout << spawn_coordinates_str << endl;
    try {
        spawn_coordinates = json_cpp::Json_create<Coordinates>(spawn_coordinates_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    Location location = map[spawn_coordinates].location;
    Location prey_location = map[prey_spawn_coordinates].location;
    Robot_simulator::start_simulation(world, location, rotation, prey_location, rotation, interval);
    Tick_agent_moves tick_moves;
    tick_moves.load("../config/tick_robot_moves.json");

    auto &tracker = Robot_simulator::tracking_server.create_local_client<agent_tracking::Tracking_client>();
    tracker.connect();
    tracker.subscribe();

    Tick_robot_agent prey_robot(tick_moves, map, tracker);
    prey_robot.connect("127.0.0.1");


    Move_list moves;
    moves.emplace_back(2,0);
    moves.emplace_back(1,1);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
//    moves.emplace_back(2,0);
    int i = 0;
    Coordinates target_coordinates(-20,0);
    Cell target_cell = map[target_coordinates];
    Timer t(3);
    while(!tracker.contains_agent_state("prey"));
    Coordinates last_coordinates;
    float target_distance = 0;
    while (Robot_simulator::is_running()) {
        //cout << "\r prey location: " << tracker.get_current_state("prey").location ;
        if (prey_robot.is_ready() && i < moves.size()){
            auto move = moves[i++];
            target_coordinates += move;
            target_cell = map[target_coordinates];
            prey_robot.execute_move(move);

            auto prey_cell = cells[cells.find(tracker.get_current_state("prey").location)];

        }
        if (i == moves.size()){
            if (prey_robot.completed_move == prey_robot.move_counter - 1) {
                if (t.time_out()) {
                    cout << "done" << endl;
                    break;
                }
            } else {
                t.reset();
            }
        }
        Timer::wait(.2);
//        cout << "prey location: "<< tracker.get_current_state("prey") << endl;
    }
    target_distance = (map[{10,0}].location - map[{-20,0}].location).mod();
//    cout << "error: " << (tracker.get_current_state("prey").location - map[{10,0}].location).mod() << " of " << target_distance << endl;
//    auto prey_cell = cells[cells.find(tracker.get_current_state("prey").location)];
//    cout << "prey cell: "<< prey_cell << endl;
    cout << "prey location: "<< tracker.get_current_state("prey").location << endl;
    cout << "prey orientation: "<< tracker.get_current_state("prey").rotation << endl;
    Robot_simulator::stop_simulation();
    return 0;
}
