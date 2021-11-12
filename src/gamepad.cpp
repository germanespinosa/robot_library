#include <gamepad.h>
#include <iostream>

using namespace std;

namespace robot {
    Gamepad::Gamepad(string &device_path) {
        _js = open(device_path.c_str(), O_RDONLY);

        __u8 count;
        if (ioctl(_js, JSIOCGAXES, &count) != -1) {
            for (int i = 0; i < count; i++) {
                axes.push_back(0);
            }
        }
        cout << "axes : " << (int) count << endl;
        if (ioctl(_js, JSIOCGBUTTONS, &count) != -1) {
            for (int i = 0; i < count; i++) {
                buttons.push_back({Button::button_state::none, {}});
            }
        }
        cout << "buttons : " << (int) count << endl;
        _update_thread = std::thread([this]() { _update_(*this); });
    }

    void Gamepad::_update_(Gamepad &j) {
        while (j._active) {
            js_event event{0, 0, 0,0};
            ssize_t bytes = read(j._js, &event, sizeof(event));
            if (bytes == sizeof(event)) {
                switch (event.type) {
                    case JS_EVENT_BUTTON: {
                        j.buttons[event.number].record_event(event.value ? Button::pressed : Button::released);
                        break;
                    }
                    case JS_EVENT_AXIS: {
                        j.axes[event.number] = event.value;
                    }
                    default: {
                        /* Ignore init events. */
                        break;
                    }
                }
            }
        }
    }

    void Gamepad::Button::record_event(Gamepad::Button::button_state s) {
        state = s;
        if (s != none) {
            _events.push(s);
        }
    }

    Gamepad::Button::button_state Gamepad::Button::read_event() {
        if (_events.empty()) {
            return none;
        } else {
            Gamepad::Button::button_state r = _events.front();
            _events.pop();
            return r;
        }
    }
}