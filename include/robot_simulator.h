#include <easy_tcp.h>
#include <cell_world.h>

namespace robot {

    struct Robot_state : json_cpp::Json_object  {
        Json_object_members(
                Add_member(location);
                Add_member(rotation);
                Add_optional_member(left);
                Add_optional_member(right);
                Add_optional_member(led0);
                Add_optional_member(led1);
                Add_optional_member(led2);
                Add_optional_member(puff);
                )
        cell_world::Location location;
        double rotation;
        int left, right;
        bool led0, led1, led2, puff;
        void update();
    };

    struct Robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;
        static void set_robot_radius(double);
        static void set_robot_speed(double);
        static void set_robot_rotation_speed(double);
        static void start_simulation(cell_world::Location, double, unsigned int);
        static void end_simulation();
        static Robot_state get_robot_state();
    };
}