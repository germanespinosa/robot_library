#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>

namespace robot {

    struct Gamepad {
        struct Button {
            enum button_state {
                none,
                pressed,
                released
            };

            button_state read_event();

            void record_event(button_state);

            button_state state;
            std::queue<button_state> _events;
        };

        explicit Gamepad(std::string &);

        std::vector<int32_t> axes;
        std::vector<Button> buttons;

        std::thread _update_thread;

        static void _update_(Gamepad &);

        bool _active = true;
        int _js;
    };

}