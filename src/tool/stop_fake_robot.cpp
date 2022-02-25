#include <iostream>
#include <cell_world.h>
#include <robot_lib/robot_simulator.h>
#include <robot_lib/robot.h>
#include <easy_tcp.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace robot;
using namespace tcp_messages;

int main(int argc, char *argv[])
{
    cout << "Stopping fake robot... " << flush;
    Connection connection = Connection::connect_remote("127.0.0.1", Robot::port());
    Message message("stop");
    string msg_string;
    msg_string << message;
    connection.send_data(msg_string.c_str(), msg_string.size() + 1);
    while (!connection.receive_data());
    string result_str(connection.buffer);
    Message result;
    result_str >> result;
    cout << result.body << endl;
    return 0;
}
