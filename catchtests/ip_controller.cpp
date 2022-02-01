#include <catch.h>
#include <gamepad_wrapper.h>
#include <iostream>


using namespace robot;
using namespace std;

TEST_CASE("ip_controller") {
    Gamepad_wrapper j(6500);
    cout << endl;
    while (true){
        for (auto a : j.buttons) {
            cout << a.state << " " ;
        }
        cout << "\r";
    }
}