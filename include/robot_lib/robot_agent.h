#pragma once
#include <controller/agent.h>
#include <robot_lib/gamepad_wrapper.h>

namespace robot {
    struct Robot_agent : controller::Agent {
        bool connect();
        bool connect(const std::string &);
        bool connect(const std::string &, int);
        void set_left(int) override;
        void set_right(int) override;
        void set_speed(int) override;
        void capture() override;
        bool update() override;
        struct Robot_message {
            int32_t left, right, speed;
        } message;
        ~Robot_agent();
        static int port();
    private:
        easy_tcp::Connection connection{-1};
        bool need_update = false;
    };
}