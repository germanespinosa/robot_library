#include <iostream>
#include <cell_world_vr.h>
#include <easy_tcp.h>
#include <robot_simulator.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace cell_world::vr;
using namespace easy_tcp;
using namespace robot;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./fake_robot [port] [log_file]" << endl;
        exit(1);
    }
    int port = atoi(argv[1]);
    string log_file (argv[2]);
    Robot_simulator::set_log_file_name(log_file);

    // start server on port 65123
    Robot_simulator::start_simulation({0,0}, 0, 100);
    Server<Robot_simulator> server;
    if (server.start(port)) {
        std::cout << "Server setup succeeded on port " << port << std::endl;
    } else {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }
    while(1){
        string firstName;
        cout << "Stop fake_robot (N/y)? ";
        cin >> firstName; // get user input from the keyboard
        if (firstName == "y") {
            break;
        }
    };
    Robot_simulator::end_simulation();
    return 0;
}
