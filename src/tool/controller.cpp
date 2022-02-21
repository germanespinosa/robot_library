#include <params_cpp.h>
#include <iostream>
#include <controller.h>

using namespace std;
using namespace controller;
using namespace robot;
using namespace cell_world;
using namespace params_cpp;

int main(int argc, char *argv[])
{

    Key ip_key{"-r","--robot_ip"};
    Key tracker_key{"-t","--tracker_ip"};
    Key experiment_service_key{"-e","--experiment_service_ip"};


    Parser p(argc, argv);

    auto robot_ip = p.get(ip_key, "127.0.0.1");
    auto tracker_ip = p.get(tracker_key, "127.0.0.1");
    auto experiment_service_ip = p.get(experiment_service_key, "127.0.0.1");

    Controller_server server("../config/pid.json",robot_ip, tracker_ip, experiment_service_ip);
    if (!server.start(Controller_service::get_port())) {
        cout << "failed to start controller" << endl;
        exit(1);
    }
    server.join();
    return 0;
}
