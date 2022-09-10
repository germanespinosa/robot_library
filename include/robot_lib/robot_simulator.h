#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/tracking_service.h>
#include <robot_lib/tracking_simulator.h>
#include <robot_lib/robot_agent.h>

#define ARRAY_SIZE 200

namespace robot {

    struct Robot_state : json_cpp::Json_object {
        Json_object_members(
                Add_member(time_stamp);
                Add_member(location);
                Add_member(theta);
                Add_member(left);
                Add_member(right);
                Add_member(led0);
                Add_member(led1);
                Add_member(led2);
                Add_member(puff);
        )

        json_cpp::Json_date time_stamp;
        cell_world::Location location;
        double theta;
        char left, right;
        bool led0, led1, led2, puff;

        void update();

        void update(double);

        cell_world::Step to_step() const;

    private:
        std::chrono::time_point<std::chrono::system_clock> last_update;
        bool initialized = false;
    };

    struct Tick_robot_state : json_cpp::Json_object {
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
        )

        json_cpp::Json_date time_stamp;
        cell_world::Location location;
        float theta{};
        float left_speed{}, right_speed{};
        int speed{};
        float left_tick_counter_f{}, right_tick_counter_f{};
        int left_tick_counter{}, right_tick_counter{};
        int left_tick_target{}, right_tick_target{};
        void update();

        void update(double);

        cell_world::Step to_step() const;

        std::chrono::time_point<std::chrono::system_clock> last_update;
        Tick_robot_agent::Robot_message in_progress;
        Tick_robot_agent::Robot_message queued;
        bool initialized = false;
    };

    struct Prey_robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;

    };

    struct Prey_robot_simulator_server : easy_tcp::Server<Prey_robot_simulator> {

    };

    struct Robot_simulator : easy_tcp::Service {
        void on_connect() override;
        void on_incoming_data(const char *, int) override;
        void on_disconnect() override;
        static void set_robot_speed(double);
        static void set_robot_rotation_speed(double);
        static void start_simulation(cell_world::World world, cell_world::Location, float, cell_world::Location, float, unsigned int);
        static void set_occlusions(cell_world::Cell_group_builder occlusions);
        static void end_simulation();
        static bool is_running();
        static void stop_simulation();
        static Robot_state get_robot_state();
        static Tick_robot_state get_prey_robot_state();
        static Tracking_simulator tracking_server;
    };

    struct Robot_simulator_server : easy_tcp::Server<Robot_simulator> {

    };
}