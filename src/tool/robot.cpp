#include <cell_world_tools/message.h>
#include <cell_world_tools/agent_info.h>
#include <cell_world_tools.h>
#include <easy_tcp.h>
#include <iostream>
#include <robot.h>
#include <pid_controller.h>

using namespace std;
using namespace easy_tcp;
using namespace robot;
using namespace cell_world;


struct Parameters : Parameters_builder{
    World world;
    Coordinates destination;
    string ip;
    int port;
    Parameters_definitions({
                               Add_web_resource(world, ({"world"}));
                               Add_value(destination);
                               Add_value(ip);
                               Add_value(port);
                           })
};

int main(int argc, char *argv[])
{
    Parameters p;
    p.load(argc, argv);
    Cell_group cells = p.world.create_cell_group();
    Map map (cells);
    Location destination = map[p.destination].location;
    Connection tracking = Connection::connect_remote(p.ip, p.port);
    auto pid_paramenters = json_cpp::Json_from_file<Pid_parameters>("pid.json");
    Pid_controller controller (pid_paramenters);
    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    Robot robot (p.ip, p.port);
    Location location;
    double rotation;
    bool goal = false;
    Message request;
    request.command = "get_agent_info";
    auto request_str = request.to_json();
    tracking.send_data(request_str.c_str(), request_str.size());
    Pid_inputs pi;
    bool data_received = false;
    while (!goal){
        if (tracking.receive_data()){
            string data (tracking.buffer);
            Message message;
            data >> message;
            if (message.command == "set_agent_info"){
                Agent_info info;
                message.content >> info;
                location = info.location;
                rotation = info.theta;
                tracking.send_data(request_str.c_str(), request_str.size());
                data_received = true;
                cout << info << endl;
            }
        }
        goal = location.dist(destination) < .01;
        if (data_received && !goal) {
            pi.location = location;
            pi.rotation = rotation;
            pi.destination = destination;
            auto robot_command = controller.process(pi);
            robot.set_left(robot_command.left);
            robot.set_right(robot_command.right);
            robot.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return 0;
}
