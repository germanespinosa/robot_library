#include <controller/controller_service.h>

using namespace cell_world;
using namespace tcp_messages;
using namespace std;
using namespace json_cpp;

namespace controller {

    bool Controller_service::set_destination(const cell_world::Location &location) {
        return ((Controller_server *) _server)->set_destination(location);
    }

    bool Controller_service::stop_controller() {
        return true;
    }

    bool Controller_service::pause_controller() {
        return ((Controller_server *) _server)->pause();
    }

    bool Controller_service::resume_controller() {
        return ((Controller_server *) _server)->resume();
    }

    int Controller_service::get_port() {
        string port_str(std::getenv("CONTROLLER_PORT") ? std::getenv("CONTROLLER_PORT") : "4590");
        return atoi(port_str.c_str());
    }

    bool Controller_service::set_behavior(int behavior) {
        return ((Controller_server *) _server)->set_behavior(behavior);
}

    void Controller_server::send_step(const Step &step) {
        Message update (step.agent_name + "_step", step);
        broadcast_subscribed(update);
    }

    Controller_server::Controller_server(const string &pid_config_file_path,
                                         const string &agent_ip,
                                         const string &tracker_ip,
                                         const string &experiment_service_ip):
            pid_controller (Json_from_file<Pid_parameters>(pid_config_file_path)),
            world_configuration(Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>()),
            world_implementation(Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>()),
            world(world_configuration,world_implementation),
            cells(world.create_cell_group()),
            graph(world.create_graph()),
            map(cells),
            paths(world.create_paths(Resources::from("paths").key("hexagonal").key("00_00").key("astar").get_resource<Path_builder>())),
            visibility(cells, world.cell_shape, world.cell_transformation),
            navigability(cells, world.cell_shape,Transformation(world.cell_transformation.size, world.cell_transformation.rotation)),
            capture(Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>(), world),
            peeking(Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>(),world),
            tracker(*this, visibility, 90,  capture, experiment_client, peeking, "predator", "prey"),
            destination_timer(5),
            experiment_client(*this)
    {
        if (!experiment_client.connect(experiment_service_ip)) {
            cout << "failed to connect to experiment server" << endl;
            exit(0);
        }
        experiment_client.subscribe();
        cout << "connected to experiment server"<< endl;
        if (!agent.connect(agent_ip)) {
            cout << "failed to connect to agent" << endl;
            exit(0);
        }
        cout << "connected to agent"<< endl;
        if (!tracker.connect(tracker_ip)) {
            cout << "failed to connect to tracking service" << endl;
            exit(0);
        }
        tracker.subscribe();
        cout << "connected to tracking service" << endl;
        state = Controller_state::Stopped;
        process = thread(&Controller_server::controller_process, this);
        while (state != Controller_state::Stopped);
        cout << "agent controller started" << endl;
    }

    void Controller_server::controller_process() {
        state = Controller_state::Playing;
        Pid_inputs pi;
        while(state != Controller_state::Stopped){
            // if there is no information from the tracker
            if (!tracker.agent.is_valid() ||
                state == Controller_state::Paused ||
                destination_timer.time_out()){
                agent.set_left(0);
                agent.set_right(0);
                agent.update();
                continue;
            }

            //PID controller
            pi.location = tracker.agent.step.location;
            pi.rotation = tracker.agent.step.rotation;
            pi.destination = get_next_stop();
            auto robot_command = pid_controller.process(pi, behavior);
            agent.set_left((char)robot_command.left);
            agent.set_right((char)robot_command.right);
            agent.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    bool Controller_server::set_destination(const cell_world::Location &new_destination) {
        auto destination_cell = cells[cells.find(new_destination)];
        // check if the destination is occluded
        if (destination_cell.occluded) {
            cout << " destination " << new_destination << " is occluded " << endl;
            return false;
        }
        destination = new_destination;
        destination_timer.reset();
        new_destination_data = true;
        return true;
    }

    cell_world::Location Controller_server::get_next_stop() {
        auto agent_location = tracker.agent.step.location;
        if (navigability.is_visible(agent_location, destination)) {
            return destination;
        }
        auto destination_cell_index = cells.find(destination);
        auto next_stop_test = cells.find(agent_location);
        auto next_stop = next_stop_test;
        while (navigability.is_visible(agent_location, cells[next_stop_test].location)){
            next_stop = next_stop_test;
            auto move = paths.get_move(cells[next_stop], cells[destination_cell_index]);
            if (move == Move{0,0}) break;
            next_stop_test = cells.find(map[cells[next_stop].coordinates + move]);
        }
        return cells[next_stop].location;
    }

    bool Controller_server::pause() {
        if (state == Controller_state::Playing) {
            state = Controller_state::Paused;
            return true;
        }
        return false;
    }

    bool Controller_server::resume() {
        if (state == Controller_state::Paused) {
            state = Controller_state::Playing;
            return true;
        }
        return false;
    }

    void Controller_server::set_world(const World_info &world_info) {
        world_configuration = Resources::from("world_configuration").key(world_info.world_configuration).get_resource<World_configuration>();
        world_implementation = Resources::from("world_implementation").key(world_info.world_implementation).get_resource<World_implementation>();
        occlusions = Resources::from("cell_group").key(world_info.world_configuration).key(world_info.occlusions).get_resource<Cell_group_builder>();
        world = World(world_configuration,world_implementation, occlusions);
        cells = world.create_cell_group();
        graph = world.create_graph();
        map = Map(cells);
        paths = Paths(world.create_paths(Resources::from("hexagonal").key("canonical").key("00_00").key("astar").get_resource<Path_builder>()));
        visibility = Location_visibility(cells, world.cell_shape, world.cell_transformation);
        navigability = Location_visibility(cells, world.cell_shape,Transformation(world.cell_transformation.size, world.cell_transformation.rotation));
        capture = Capture(Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>(), world);
        peeking = Peeking(Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>(),world);
    }

    void Controller_server::join() {
        process.join();
        Server::join();
    }

    bool Controller_server::set_behavior(int behavior) {
        this->behavior = static_cast<Behavior> (behavior);
        return true;
    }

    void Controller_server::Controller_tracker::on_step(const Step &step) {
        if (step.agent_name == agent.agent_name) {
            server.send_step(step);
            agent.step = step;
            agent.timer = Timer(.5);
        } else if (step.agent_name == adversary.agent_name) {
            if (contains_agent_state(agent.agent_name)) {
                auto predator = get_current_state(agent.agent_name);
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
    string logs_path = "logs/";

    string get_experiment_file(const string &experiment_name){
        return logs_path + experiment_name + ".json";
    }

    void Controller_server::Controller_experiment_client::on_experiment_started(
            const experiment::Start_experiment_response &experiment) {
        experiment.save(get_experiment_file(experiment.experiment_name));
        Experiment_client::on_experiment_started(experiment);
    }

    void Controller_server::Controller_experiment_client::on_episode_started(const string &experiment_name) {
        experiment::Start_experiment_response experiment;
        experiment.load(get_experiment_file(experiment_name));
        controller_server.set_world(experiment.world);
        Experiment_client::on_episode_started(experiment_name);
    }

    Controller_server::Controller_experiment_client::Controller_experiment_client(Controller_server &controller_server):
            controller_server(controller_server){

    }
}

