#include <iostream>
#include <cell_world.h>
#include <robot_simulator.h>
#include <easy_tcp.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace robot;

int main(int argc, char *argv[])
{
    Connection connection = Connection::connect_remote("127.0.0.1", Robot_simulator::port());
    Message message ("get_agent_info");
    string msg_string;
    msg_string << message;
    connection.send_data(msg_string.c_str(), msg_string.size() + 1);
    while (!connection.receive_data());
    string result_str(connection.buffer);
    Message result;
    result_str >> result;
    cout << result.get_body<Robot_state>() << endl;
    return 0;
}
