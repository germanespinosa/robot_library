#include <robot_lib/gamepad_wrapper.h>
#include <tcp_messages.h>
#include <json_cpp.h>

using namespace gamepad;
using namespace std;
using namespace tcp_messages;

struct Gamepad_data : json_cpp::Json_object {
    Json_object_members(
            Add_member(axes);
            Add_member(buttons);
            )
    json_cpp::Json_vector<int> axes;
    json_cpp::Json_vector<int> buttons;
};

robot::Gamepad_wrapper *active_gamepad = nullptr;

struct Gamepad_service : Message_service {
    Routes(
            Add_route("gamepad_data", new_game_data, Gamepad_data);
            )
    void new_game_data (const Gamepad_data &game_data){
        if (active_gamepad) {
            for (unsigned int i = 0; i < game_data.axes.size(); i++) {
                if (i>=active_gamepad->axes.size()) {
                    active_gamepad->axes.emplace_back();
                };
                active_gamepad->axes[i] = game_data.axes[i];
            }
            for (unsigned int i = 0; i < game_data.buttons.size(); i++) {
                if (i>=active_gamepad->buttons.size()) {
                    auto &b = active_gamepad->buttons.emplace_back();
                    b.state = Gamepad::Button::button_state::none;
                }
                //active_gamepad->buttons[i].state = (Gamepad::Button::button_state) game_data.buttons[i];
                if (game_data.buttons[i] == 0){
                    if (active_gamepad->buttons[i].state != Gamepad::Button::button_state::none) {
                        active_gamepad->buttons[i].state = Gamepad::Button::button_state::released;
                    }
                } else {
                    active_gamepad->buttons[i].state = Gamepad::Button::button_state::pressed;
                }

            }
            active_gamepad->ready = true;
        }
    }
};

namespace robot{
    Message_server<Gamepad_service> gamepad_server;

    Gamepad_wrapper::Gamepad_wrapper(std::string &device_path) : Gamepad(device_path), ready(true) {

    }

    Gamepad_wrapper::Gamepad_wrapper(int port): ready(false) {
        gamepad_server.start(port);
        active_gamepad = this;
        while (!ready);
    }

    Gamepad_wrapper::~Gamepad_wrapper() {
        if (active_gamepad) {
            gamepad_server.stop();
            active_gamepad = nullptr;
        }
    }
}