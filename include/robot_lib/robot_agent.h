#pragma once
#include <controller/agent.h>
#include <robot_lib/gamepad_wrapper.h>

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

    struct Tick_robot_agent : controller::Tick_agent , easy_tcp::Client {

        Tick_robot_agent();  // TODO: not sure if this is correct, need to add operational limits
        bool connect();
        bool connect(const std::string &);
        void set_left(int) override;
        void set_right(int) override;
        void set_speed(int) override;
        void capture() override;
        int update() override;
        void received_data(char *, size_t) override;
        bool is_move_done();
        struct Robot_message {
            int32_t left, right, speed;
            uint32_t move_number{};
        } message;
        ~Tick_robot_agent();
        static int port();
    private:
        unsigned int move_counter{};
        int completed_move{};
        std::atomic<bool> move_done;
    };
}