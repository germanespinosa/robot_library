#pragma once
#include <cell_world.h>
#include <controller/agent.h>
#include <robot_lib/gamepad_wrapper.h>
#include <agent_tracking/tracking_client.h>
#include <easy_tcp.h>

namespace robot {
    struct Robot_agent : controller::Agent {
        explicit Robot_agent(const controller::Agent_operational_limits &limits);
        explicit Robot_agent(const controller::Agent_operational_limits &limits, int game_pad_port);
        explicit Robot_agent(const controller::Agent_operational_limits &limits, std::string device_path);
        bool connect();
        bool connect(const std::string &);
        bool connect(const std::string &, int);
        void set_left(double) override;
        void set_right(double) override;
        void capture() override;
        bool update() override;
        bool stop() override;
        void set_led(int, bool);
        void set_leds(bool);
        void increase_brightness();
        void decrease_brightness();
        char message[3];
        ~Robot_agent();
        static int port();
        controller::Agent_operational_limits limits;
        Gamepad_wrapper gamepad;
    private:
        easy_tcp::Connection connection{-1};
        bool need_update = false;
    };
/////////////////////////////////////////////////////////////////////////////////////////////////////
    struct Tick_move_target{
        Tick_move_target() = default;
        Tick_move_target(int, cell_world::Location, float);
        int move_number{};
        cell_world::Location location;
        float rotation{};
    };

    struct Tick_robot_agent : controller::Tick_agent , easy_tcp::Client {
        Tick_robot_agent(const controller::Tick_agent_moves &moves, agent_tracking::Tracking_client &, std::string &);

//        Tick_robot_agent(const controller::Tick_agent_moves &moves, agent_tracking::Tracking_client &, std::string &);
        bool connect();
        bool connect(const std::string &);
        void execute_move(cell_world::Move_list) override;
        void move_count_reset() override;
        int update();
        void received_data(char *, size_t) override;
        void set_rotation(float) override;
        void set_coordinate(cell_world::Coordinates) override;
        void correct_robot() override;
        unsigned int get_corrected_orientation(float);
        void joystick_control() override;
        void capture() override;
        bool needs_correction () override;
        bool use_joystick () override;
        bool is_ready() override;
        bool is_move_done();
        struct Robot_message {
            int32_t left, right, speed;
            uint32_t move_number{};
        } message;
        static int port();
        unsigned int robot_move_orientation = 0;
        int move_counter = 1;
        std::atomic<int> completed_move = 0;
        std::atomic<bool> move_done;
        controller::Tick_agent_moves tick_agent_moves;
        std::queue<Tick_move_target> move_targets;
        cell_world::Location location_error{};
        float orientation_error{};
        cell_world::World world;
        cell_world::Cell_group cells;
        cell_world::Map map;
        agent_tracking::Tracking_client &tracking_client;
        float angle_diff_degrees(float, float);
        int orientation_correction{};
        int x_correction{};
        int y_correction{};
        float P_rot{};// = 2.5;
        float P_rot2{};
        float P_x{};// = 9189.0;
        float P_y{};// = 9189.0;
        float actual_rotation = 0;
        bool needs_correction_now = false;
        bool joystick_on = false;
        enum Move_state {translate, rotate, correct} move_state;
        enum Correction_state{rotation1, rotation2}correction_state;

        Gamepad_wrapper tick_gamepad;
    };
}