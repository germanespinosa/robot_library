#include <cell_world.h>
#include <easy_tcp.h>
#include <params_cpp.h>
#include <iostream>
#include <robot.h>
#include <pid_controller.h>
#include <agent_tracking/client.h>

using namespace agent_tracking;
using namespace std;
using namespace easy_tcp;
using namespace robot;
using namespace cell_world;
using namespace params_cpp;

int main(int argc, char *argv[])
{
    Key occlusions_key{"-o","--occlusions"};
    Key destination_key{"-d","--destination"};
    Key ip_key{"-r","--robot_ip"};
    Key tracker_key{"-t","--tracker_ip"};

    Parser p(argc, argv);
    if (!p.contains({occlusions_key,destination_key})) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    auto occlusions_name = p.get(occlusions_key);
    auto destination_str = p.get(destination_key);
    auto robot_ip = p.get(ip_key, "127.0.0.1");
    auto tracker_ip = p.get(ip_key, "127.0.0.1");

    Coordinates destination_coord;
    try {
        destination_coord = json_cpp::Json_create<Coordinates>(destination_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }

    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).key("occlusions").get_resource<Cell_group_builder>();
    auto pb = Resources::from("paths").key("hexagonal").key(occlusions_name).key("astar").get_resource<Path_builder>();
    World world(wc, wi, occlusions);
    Cell_group cells = world.create_cell_group();
    Graph graph = world.create_graph();
    Paths paths = world.create_paths(pb);

    auto robot_transformation = wi.cell_transformation;
    robot_transformation.size *= 1.5;

    Location_visibility navigability(cells,wc.cell_shape,wi.cell_transformation);

    Map map (cells);
    Location destination = map[destination_coord].location;

    auto pid_parameters = json_cpp::Json_from_file<Pid_parameters>("../config/pid.json");
    Pid_controller controller (pid_parameters);
    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    cout << " destination: " <<  destination << endl;
    agent_tracking::Client tracker;
    cout << "starting tracker connection ..." << flush;
    if (tracker.connect(tracker_ip)) {
        if (!tracker.register_consumer()) {
            cout << " failed." << endl;
            exit(1);
        }
        while(!tracker.contains_agent_state("predator"));
        cout << " success" << endl;
    }else {
        cout << " failed." << endl;
        exit(1);
    }

    Robot robot;
    cout << "starting robot connection ..." << flush;
    if (robot.connect(robot_ip)) {
        cout << " success" << endl;
    } else {
        cout << " failed." << endl;
        exit(1);
    }
    bool goal = false;
    Pid_inputs pi;
    Location next_step;
    while (!goal){
        auto predator = tracker.get_current_state("predator");
        cout << "state :" <<  predator << endl;
        Cell current_cell = map[predator.coordinates];
        Coordinates next_coordinates = predator.coordinates + paths.get_move(current_cell,map[destination_coord]);
        Cell next_cell = map[next_coordinates];
        Cell confirmed = Cell::ghost_cell();
        if (navigability.is_visible(predator.location,destination)) confirmed = destination_coord;
        while(navigability.is_visible(predator.location,next_cell.location) && confirmed.coordinates != destination_coord){
            cout << " from " << confirmed.coordinates << " to " << destination_coord << endl;
            confirmed = next_cell;
            cout << "confirmed: " << confirmed << endl;
            auto move = paths.get_move(confirmed,map[destination_coord]);
            if (move==Move{0,0}) {
                cout << "unable to navigate to destination" << endl;
                exit(1);
            }
            next_coordinates = confirmed.coordinates + move;
            next_cell = map[next_coordinates];
            cout << "trying: " << next_cell << endl;
        }
        cout << "goal: " << destination << " - current: " << predator.location << " - distance :" << predator.location.dist(destination) << endl;
        next_step = next_cell.location;
        goal = predator.location.dist(destination) < .03;
        if (!goal) {
            pi.location = predator.location;
            pi.rotation = predator.rotation;
            pi.destination = next_step;
            auto robot_command = controller.process(pi);
            robot.set_left(robot_command.left);
            robot.set_right(robot_command.right);
            robot.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    cout << "finished!" << endl;
    return 0;
}
