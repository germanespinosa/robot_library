//
// Created by gabbie on 6/15/22.
//

#ifndef ROBOT_LIB_ROBOT_AGENT_NEW_H
#define ROBOT_LIB_ROBOT_AGENT_NEW_H

#pragma once
#include <controller/agent.h>
#include <robot_lib/gamepad_wrapper.h>

namespace robot {
    struct Robot_agent : controller::Agent {
        // needs to match struct for microcontroller
        struct Robot_message {
            int32_t left_tick, right_tick;
            int16_t left_pwm, right_pwm;
            uint8_t data;
        } raw_message;

        explicit Robot_agent(const controller::Agent_operational_limits &limits);
        explicit Robot_agent(const controller::Agent_operational_limits &limits, int game_pad_port);
        explicit Robot_agent(const controller::Agent_operational_limits &limits, std::string device_path);
        bool connect();
        bool connect(const std::string &);
        bool connect(const std::string &, int);
        void set_left(double) override;
        void set_right(double) override;

        void set_speed(double);  // TODO: need to create set speed function ... virtual?

        virtual void capture() override;
        virtual bool update() override;
        virtual bool stop() override;
        void set_led(int, bool);
        void set_leds(bool);
        void increase_brightness();
        void decrease_brightness();
        char *message;
        size_t message_size;
        ~Robot_agent();
        static int port();
        controller::Agent_operational_limits limits;
        Gamepad_wrapper gamepad;
    private:
        easy_tcp::Connection connection{-1};
        bool need_update = false;
    };
}

#endif //ROBOT_LIB_ROBOT_AGENT_NEW_H
