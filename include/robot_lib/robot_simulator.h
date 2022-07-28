#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/tracking_service.h>
#include <robot_lib/tracking_simulator.h>

namespace robot {

    struct Robot_state : json_cpp::Json_object  {
        Json_object_members(
                Add_member(time_stamp);
                Add_member(location);
                Add_member(theta);
                Add_member(left_speed);
                Add_member(right_speed);
                Add_member(left_tick_target);
                Add_member(right_tick_target);
                Add_member(left_tick_counter);
                Add_member(left_tick_counter);
                Add_member(led0);
                Add_member(led1);
                Add_member(led2);
                Add_member(puff);
                )
        json_cpp::Json_date time_stamp;
        cell_world::Location location;
        double theta{};
        float left_speed{}, right_speed{};
        int speed{};
        float left_tick_counter_float{}, right_tick_counter_float{};
        int left_tick_counter{}, right_tick_counter{};
        int left_tick_target{}, right_tick_target{};
        bool led0{}, led1{}, led2{}, puff{};
        void update();
        void update(double);
        cell_world::Step to_step() const;

    private:
        std::chrono::time_point<std::chrono::system_clock> last_update;
        bool initialized = false;
        int prev_tick_target_L;
        int prev_tick_target_R;
        int direction_L = 0;
        int direction_R = 0;
    };

    struct Robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;
        static void set_robot_speed(double);
        static void set_robot_rotation_speed(double);
        static void start_simulation(cell_world::World world, cell_world::Location, double, unsigned int, Tracking_simulator &);
        static void set_occlusions(cell_world::Cell_group_builder occlusions);
        static void end_simulation();
        static bool is_running();
        static Robot_state get_robot_state();
        static bool start_prey();

    };
}