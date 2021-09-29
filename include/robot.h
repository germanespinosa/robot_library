#include <easy_tcp.h>

namespace robot {
    struct Robot {
        Robot(const std::string &, int);
        void set_left(char);
        void set_right(char);
        void set_puf();
        void set_led(int, bool);
        bool update();
        char message[3];
        easy_tcp::Connection connection;
        bool need_update = false;
        ~Robot();
    };
}