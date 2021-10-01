#include <math.h>
#include <robot_simulator.h>
#include <json_cpp.h>
using namespace json_cpp;
using namespace std;
namespace robot {
    double speed = .01;
    double robot_radius;

    struct Robot_location : json_cpp::Json_object  {
        Json_object_members(
                Add_member(x);
                Add_member(y);
                Add_member(theta);
                )
        double x, y, theta;
        void update(char left, char right) {
            double dl = ((double)left) / 128.0 * M_PI / 2.0;
            double dr = ((double)right) / 128.0 * M_PI / 2.0;
            double lw_t = theta-M_PI/2 + dl;
            double rw_t = theta+M_PI/2 + dr;
            double lw_x = x + cos(lw_t) * robot_radius;
            double lw_y = y + sin(lw_t) * robot_radius;
            double rw_x = x + cos(rw_t) * robot_radius;
            double rw_y = y + sin(rw_t) * robot_radius;
            double nx = (lw_x + rw_x)/2;
            double ny = (lw_x + rw_x)/2;
            x=nx;
            y=ny;
            if (left-right)
                theta = atan2(nx-x,ny-y);
            else
                theta += dl;
        }
    } robot_location;
    void Robot_simulator::on_connect() {
        Service::on_connect();
    }

    void Robot_simulator::on_incoming_data(const char *buff, int size) {
        if (size == 3){ // instruction

        } else {
            if (buff[size]) return; //not a string
            try {
                string cmd (buff);
                cmd >> robot_location; ; //if it is a location
            }catch (...){

            }
            send_data(robot_location.to_json()); //always answer the location of the robot
        }
    }

    void Robot_simulator::on_disconnect() {
        Service::on_disconnect();
    }
}