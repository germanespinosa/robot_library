#include <iostream>
#include <easy_tcp.h>
#include <robot_simulator.h>
#include <chrono>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace robot;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./fake_robot [log_file]" << endl;
        exit(1);
    }
    int port = Robot_simulator::port();
    string log_file (argv[1]);
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
    cout << "fake robot running" << endl;
    while(Robot_simulator::is_running()){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    };
    Robot_simulator::end_simulation();
    return 0;
}
