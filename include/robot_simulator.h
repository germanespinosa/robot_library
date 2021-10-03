#include <easy_tcp.h>
#include <cell_world.h>

namespace robot {

    struct Robot_state : json_cpp::Json_object  {
        Json_object_members(
                Add_member(time_stamp);
                Add_member(location);
                Add_member(rotation);
                Add_member(left);
                Add_member(right);
                Add_member(led0);
                Add_member(led1);
                Add_member(led2);
                Add_member(puff);
                )
        json_cpp::Json_date time_stamp;
        cell_world::Location location;
        double rotation;
        int left, right;
        bool led0, led1, led2, puff;
        void update();
        void update(double);

    private:
        std::chrono::time_point<std::chrono::system_clock> last_update;
        bool initialized = false;
    };

    struct Robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;
        static void set_robot_speed(double);
        static void set_robot_rotation_speed(double);
        static void set_log_file_name(std::string);
        static void start_simulation(cell_world::Location, double, unsigned int);
        static void end_simulation();
        static bool is_running();
        static int port();
        static Robot_state get_robot_state();
    };
}