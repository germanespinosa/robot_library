#include <gamepad_lib/include/gamepad.h>
namespace robot {
    struct Gamepad_wrapper : gamepad::Gamepad {
        explicit Gamepad_wrapper(std::string &);
        explicit Gamepad_wrapper(int);
        ~Gamepad_wrapper();
    };
}
