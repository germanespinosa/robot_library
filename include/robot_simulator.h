#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/service.h>

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
        char left, right;
        bool led0, led1, led2, puff;
        void update();
        void update(double);
        cell_world::Step to_step() const;

    private:
        std::chrono::time_point<std::chrono::system_clock> last_update;
        bool initialized = false;
    };

    struct Tracking_simulator : agent_tracking::Service {

        // routes
        //experiment
        void new_experiment(const std::string &) override {};
        void new_episode(agent_tracking::New_episode_message) override {};
        void end_episode() override {};
        //camera
        void update_background() override {};
        void reset_cameras() override {};
        void update_puff() override {};
        //visualization
        void show_occlusions(const std::string &) override {};
        void hide_occlusions() override {};

        //unrouted
        void unrouted_message(const cell_world::Message &) override;
    };

    struct Robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;
        static void set_robot_speed(double);
        static void set_robot_rotation_speed(double);
        static void start_simulation(const cell_world::Cell_group &, cell_world::Location, double, unsigned int);
        static void end_simulation();
        static bool is_running();
        static int port();
        static Robot_state get_robot_state();
    };
}