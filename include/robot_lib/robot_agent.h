#pragma once
#include <controller/agent.h>

namespace robot {
    struct Robot_agent : controller::Agent {
        explicit Robot_agent(const controller::Agent_operational_limits &limits);
        bool connect();
        bool connect(const std::string &);
        bool connect(const std::string &, int);
        virtual void set_left(double) override;
        virtual void set_right(double) override;
        virtual void capture() override;
        virtual bool update() override;
        void set_led(int, bool);
        void set_leds(bool);
        void increase_brightness();
        void decrease_brightness();
        char message[3];
        ~Robot_agent();
        static int port();
        controller::Agent_operational_limits limits;
    private:
        easy_tcp::Connection connection{-1};
        bool need_update = false;
    };
}