#include <cell_world.h>
#include <easy_tcp.h>
#include <params_cpp.h>
#include <iostream>
#include <robot.h>
#include <pid_controller.h>
#include <agent_tracking/tracking_client.h>

using namespace agent_tracking;
using namespace std;
using namespace easy_tcp;
using namespace robot;
using namespace cell_world;
using namespace params_cpp;
using namespace tcp_messages;

Location destination;
atomic<bool> puff = false;
atomic<bool> active = true;

struct Controller_service : Message_service{
    Routes(
            Add_route("set_destination", set_destination, Location);
            Add_route("activate_puff", activate_puff);
            Add_route("stop_controller", stop_controller);
            );
    void set_destination(Location new_destination){
        destination = new_destination;
        send_message(Message("set_destination_result", "ok"));
    }
    void activate_puff(){
        puff = true;
        send_message(Message("activate_puff_result", "ok"));
    }
    void stop_controller(){
        active = false;
        send_message(Message("stop_controller_result", "ok"));
    }
};



int main(int argc, char *argv[])
{
    Key occlusions_key{"-o","--occlusions"};
    Key ip_key{"-r","--robot_ip"};
    Key tracker_key{"-t","--tracker_ip"};

    Parser p(argc, argv);
    if (!p.contains({occlusions_key})) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    auto occlusions_name = p.get(occlusions_key);
    auto robot_ip = p.get(ip_key, "127.0.0.1");
    auto tracker_ip = p.get(ip_key, "127.0.0.1");

    Coordinates destination_coord;

    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    auto dst_space = wi.space;
    auto src_space = Resources::from("world_implementation").key("hexagonal").key("cv").get_resource<World_implementation>().space;
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).key("occlusions").get_resource<Cell_group_builder>();
    auto pb = Resources::from("paths").key("hexagonal").key(occlusions_name).key("astar").get_resource<Path_builder>();
    World world(wc, wi, occlusions);
    Cell_group cells = world.create_cell_group();
    Graph graph = world.create_graph();
    Paths paths = world.create_paths(pb);

    auto robot_transformation = wi.cell_transformation;
    robot_transformation.size *= 1.4;

    Location_visibility navigability(cells,wc.cell_shape,robot_transformation);

    Map map (cells);
    auto pid_parameters = json_cpp::Json_from_file<Pid_parameters>("../config/pid.json");
    Pid_controller controller (pid_parameters);
    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    cout << " destination: " <<  destination << endl;
    agent_tracking::Tracking_client tracker;
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

    Message_server<Controller_service> controller_server;
    cout << "starting controller service ..." << flush;
    if (!controller_server.start(4520)){
        cout << " failed." << endl;
        exit(1);
    }

    auto predator = tracker.get_current_state("predator");
    destination = predator.location;
    Pid_inputs pi;
    Location next_step;
    while (active){
        // receives the predator location in cv implementation and converts to mice
        predator = tracker.get_current_state("predator");
        predator.location = dst_space.transform(predator.location,src_space);
        auto predator_cell = map[predator.coordinates];
        //

        next_step = destination;
        while (!navigability.is_visible(predator.location,next_step)) // if the destination is not navigable
        {
            // it aims for one step back
            auto &next_step_cell = map.cells[map.cells.find(next_step)];
            auto move = paths.get_move(next_step_cell, predator_cell);
            if (move == Move{0,0}) break;
            next_step = map[next_step_cell.coordinates + move].location;
        }

        pi.location = predator.location;
        pi.rotation = predator.rotation;
        pi.destination = next_step;
        auto robot_command = controller.process(pi);
        robot.set_left((char)robot_command.left);
        robot.set_right((char)robot_command.right);
        if (puff) {
            robot.set_puf();
            puff = false;
        }
        robot.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    cout << "finished!" << endl;
    return 0;
}
