#include <cell_world.h>
#include <easy_tcp.h>
#include <params_cpp.h>
#include <iostream>
#include <robot.h>
#include <pid_controller.h>
#include <agent_tracking/tracking_client.h>
#include <experiment.h>
#include <controller.h>

using namespace agent_tracking;
using namespace std;
using namespace easy_tcp;
using namespace controller;
using namespace robot;
using namespace cell_world;
using namespace params_cpp;
using namespace tcp_messages;
using namespace experiment;

Location destination;
atomic<bool> puff = false;
atomic<bool> active = true;
atomic<bool> pause = false;

struct Agent_data{
    Agent_data (const string &agent_name){
        step.agent_name = agent_name;
    }
    Step step;
    Timer timer;
    bool is_valid() {
        return !timer.time_out();
    }
};


int main(int argc, char *argv[])
{

    auto &predator_state = Controller_server::predator_state;

    Key occlusions_key{"-o","--occlusions"};
    Key ip_key{"-r","--robot_ip"};
    Key tracker_key{"-t","--tracker_ip"};
    Key view_angle_key{"-v","--view_angle"};
    Key experiment_service_key{"-e","--experiment_service_ip"};

    Controller_server server;
    server.start(Controller_service::get_port());

    Parser p(argc, argv);
    if (!p.contains({occlusions_key})) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    auto &occlusions_name = p.get(occlusions_key);
    auto robot_ip = p.get(ip_key, "127.0.0.1");
    auto view_angle = stof(p.get(view_angle_key, "30"));
    auto tracker_ip = p.get(tracker_key, "127.0.0.1");
    auto experiment_service_ip = p.get(experiment_service_key, "127.0.0.1");
    Coordinates destination_coord;

    auto peeking_parameters = Resources::from("peeking").key("default").get_resource<Peeking_parameters>();
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).key("occlusions").get_resource<Cell_group_builder>();
    auto pb = Resources::from("paths").key("hexagonal").key(occlusions_name).key("astar").get_resource<Path_builder>();
    auto capture_parameters = Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>();

    World world(wc, wi, occlusions);
    Capture capture(capture_parameters, world);
    Peeking peeking(peeking_parameters, world);
    Cell_group cells = world.create_cell_group();
    Graph graph = world.create_graph();
    Paths paths = world.create_paths(pb);

    auto robot_transformation = wi.cell_transformation;
    robot_transformation.size *= 1.25; // 1.4

    Location_visibility navigability(cells,wc.cell_shape,robot_transformation);
    Location_visibility visibility(cells,wc.cell_shape,robot_transformation);


    Map map (cells);

    auto pid_parameters = json_cpp::Json_from_file<Pid_parameters>("../config/pid.json");
    Pid_controller controller (pid_parameters);

    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    cout << " destination: " <<  destination << endl;

    // connect to robot
    Robot robot;
    cout << "starting robot connection ..." << flush;
    if (robot.connect(robot_ip)) {
        cout << " success" << endl;
        // initializes the robot
        robot.set_left(0);
        robot.set_right(0);
        robot.update();

    } else {
        cout << " failed." << endl;
        exit(1);
    }

    Experiment_client experiment_client;

    cout << "starting experiment service connection ..." << flush;
    if (experiment_client.connect(experiment_service_ip)) {
        cout << " success " << endl;
    } else {
        cout << " failed." << endl;
        exit(1);
    }


    // connect to tracker (run agent_tracker)
    // customize the tracker
    struct Tracker : Tracking_client {
        Tracker(Controller_server &server, Location_visibility &visibility, float view_angle, Capture &capture, Experiment_client &experiment_client, Peeking &peeking) :
            server(server),
            visibility(visibility),
            view_angle(view_angle),
            capture(capture),
            experiment_client(experiment_client),
            peeking(peeking){}
        void on_step(const cell_world::Step &step) override {
            if (step.agent_name == "predator") {
                server.send_step(step);
                predator.step = step;
                predator.timer = Timer(.5);
            } else if (step.agent_name == "prey") {
                if (contains_agent_state("predator")) {
                    auto predator = get_current_state("predator");
                    auto is_captured = capture.is_captured( predator.location, to_radians(predator.rotation), step.location);
                    if (is_captured)
                        experiment_client.capture(step.frame);
                    if (visibility.is_visible(predator.location, step.location) &&
                        angle_difference(predator.location.atan(step.location), predator.rotation) < view_angle) {
                        if (peeking.is_seen(predator.location, step.location)) {
                            server.send_step(step);
                        }
                    } else {
                        peeking.not_visible();
                    }
                }
            }
            Tracking_client::on_step(step);
        }
        Agent_data predator{"predator" };
        Location_visibility &visibility;
        Controller_server &server;
        float view_angle = 30;
        Capture &capture;
        Experiment_client &experiment_client;
        Peeking &peeking;
    } tracker(server, visibility, view_angle, capture, experiment_client, peeking);

    cout << "starting tracker connection ..." << flush;
    // tracker ip
    if (tracker.connect(tracker_ip)) { // service ip
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

    Message_server<Controller_service> controller_server;
    cout << "starting controller service ..." << flush;
    if (!controller_server.start(4520)){
        cout << " failed." << endl;
        exit(1);
    }

    destination = tracker.predator.step.location;

    Pid_inputs pi;
    Location next_step;
    predator_state = Playing;
    while (predator_state == Playing){

        // if no new data -> stop robot and wait
        if (!tracker.predator.is_valid()){
            robot.set_left(0);
            robot.set_right(0);
            robot.update();
            continue;
        }

        // determines where to go next

        auto predator_cell = cells[cells.find(tracker.predator.step.location)];

        // check if there is a new destination
        if (Controller_server::get_destination(destination)){
            cout << "new destination received" << endl;
        }

        auto destination_cell = cells[cells.find(destination)];

        // check if the destination is occluded, if so the robot and wait for a new destination
        if (destination_cell.occluded) {
            cout << " destination is occluded " << endl;
            robot.set_left(0);
            robot.set_right(0);
            robot.update();
            continue;
        }

        // assumes it can drive to destination
        next_step = destination;

        while (!navigability.is_visible(tracker.predator.step.location,next_step)) // if the destination is not navigable
        {
            // it aims for one step back
            auto &next_step_cell = map.cells[map.cells.find(next_step)];
            auto move = paths.get_move(next_step_cell, predator_cell);
            if (move == Move{0,0}) break;
            next_step = map[next_step_cell.coordinates + move].location;
        }

        //PID controller
        pi.location = tracker.predator.step.location;
        pi.rotation = tracker.predator.step.rotation;
        pi.destination = next_step;
        auto robot_command = controller.process(pi);
        robot.set_left((char)robot_command.left);
        robot.set_right((char)robot_command.right);

        if (puff) {
            robot.set_puf();
            puff = false;
        }
        if (pause){
            robot.set_left(0);
            robot.set_right(0);
        }

        robot.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    //If the controller is inactive, stop the motors
    robot.set_left(0);
    robot.set_right(0);
    robot.update();
    cout << "finished!" << endl;
    return 0;
}
