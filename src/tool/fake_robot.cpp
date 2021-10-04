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
    int port = Robot_simulator::port();
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
    cout << "fake robot stopped" << endl;
    server.stop();
    return 0;
}
