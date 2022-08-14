#pragma once
#include <controller/agent.h>
#include <robot_lib/gamepad_wrapper.h>

namespace robot {
    struct Robot_agent : controller::Agent , easy_tcp::Client {
        bool connect();
        bool connect(const std::string &);
        void set_left(int) override;
        void set_right(int) override;
        void set_speed(int) override;
        void capture() override;
        int update() override;
        virtual void received_data(char *, size_t) override;
        bool is_move_done();
        struct Robot_message {
            int32_t left, right, speed;
            uint32_t move_number{};
        } message;
        ~Robot_agent();
        static int port();
    private:
        unsigned int move_counter{};
        std::atomic<bool> move_done;
    };
}