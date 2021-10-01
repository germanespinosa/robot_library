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
        cout << "Usage: ./fake_robot [destination_folder] [spawn_location]" << endl;
        exit(1);
    }
    int port = 80;
    string destination_folder (argv[1]);
//    Vr_service::set_destination_folder(destination_folder);
//    Vr_service::new_experiment();
    string location_s(argv[3]);
    auto spawn_location = Json_create<Location>(location_s);


    // start server on port 65123
    Server<Robot_simulator> server ;
    if (server.start(port)) {
        std::cout << "Server setup succeeded on port " << port << std::endl;
    } else {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }
    while(1);
    return 0;
}
