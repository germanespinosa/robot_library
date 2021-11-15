#pragma once
#include <easy_tcp.h>

namespace robot {
    struct Robot {
        Robot(const std::string &, int);
        void set_left(char);
        void set_right(char);
        void set_puf();
        void set_led(int, bool);
        void set_leds(bool);
        void increase_brightness();
        void decrease_brightness();
        bool update();
        char message[3];
        ~Robot();
        static int port();
    private:
        easy_tcp::Connection connection;
        bool need_update = false;
    };
}