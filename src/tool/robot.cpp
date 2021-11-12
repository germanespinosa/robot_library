#include <cell_world.h>
#include <easy_tcp.h>
#include <params_cpp.h>
#include <iostream>
#include <robot.h>
#include <pid_controller.h>

using namespace std;
using namespace easy_tcp;
using namespace robot;
using namespace cell_world;
using namespace params_cpp;

int main(int argc, char *argv[])
{
    Key occlusions_key{"-o","--occlusions"};
    Key destination_key{"-d","--destination"};
    Key ip_key{"-i","--ip"};
    Key port_key{"-p","--port"};

    Parser p(argc, argv);
    if (!p.contains({occlusions_key,destination_key})) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    auto &occlusions_name = p.get(occlusions_key);
    auto &destination_str = p.get(destination_key);
    auto &ip = p.get(ip_key);
    auto port = stoi(p.get(port_key));
    Coordinates destination_coord;
    try {
        destination_coord = json_cpp::Json_create<Coordinates>(destination_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).get_resource<Cell_group_builder>();
    auto pb = Resources::from("paths").key("hexagonal").key(occlusions_name).get_resource<Path_builder>();
    World world(wc, wi, occlusions);
    Cell_group cells = world.create_cell_group();
    Graph graph = world.create_graph();
    Paths paths = world.create_paths(pb);

    auto robot_transformation = wi.cell_transformation;
    robot_transformation.size *= 1.5;

    Location_visibility navigability(cells,wc.cell_shape,wi.cell_transformation);

    cout << "world:" << occlusions << endl;
    Map map (cells);
    Location destination = map[destination_coord].location;

    Connection tracking = Connection::connect_remote(ip, port);

    auto pid_parameters = json_cpp::Json_from_file<Pid_parameters>("pid.json");
    Pid_controller controller (pid_parameters);
    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    Robot robot (ip, port);
    Location location;
    double rotation;
    bool goal = false;
    Message request;
    request.title = "get_agent_info";
    auto request_str = request.to_json();
    tracking.send_data(request_str.c_str(), request_str.size());
    Pid_inputs pi;
    bool data_received = false;
    Location next_step;
    while (!goal){
        if (tracking.receive_data()){
            string data (tracking.buffer);
            Message message;
            data >> message;
            if (message.title == "set_agent_info"){
                auto info = message.get_body<Step>();
                location = info.location;
                rotation = info.rotation;
                tracking.send_data(request_str.c_str(), request_str.size());
                Cell current_cell = map[info.coordinates];
                Coordinates next_coordinates = info.coordinates + paths.get_move(current_cell,map[destination_coord]);
                Cell next_cell = map[next_coordinates];
                Cell confirmed = Cell::ghost_cell();
                while(navigability.is_visible(info.location,next_cell.location) && confirmed.coordinates != destination_coord){
                    confirmed = next_cell;
                    cout << "confirmed: " << confirmed << endl;
                    next_coordinates = confirmed.coordinates + paths.get_move(confirmed,map[destination_coord]);
                    next_cell = map[next_coordinates];
                    cout << "trying: " << next_cell << endl;
                }
                next_step = next_cell.location;
                data_received = true;
            }
        }
        goal = location.dist(destination) < .01;
        if (data_received && !goal) {
            pi.location = location;
            pi.rotation = rotation;
            pi.destination = next_step;
            auto robot_command = controller.process(pi);
            robot.set_left(robot_command.left);
            robot.set_right(robot_command.right);
            robot.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return 0;
}
