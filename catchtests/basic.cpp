#include<catch.h>
#include<robot.h>
#include<robot_simulator.h>
#include<tracking_simulator.h>
#include<unistd.h>
#include <agent_tracking/client.h>


using namespace agent_tracking;
using namespace cell_world;
using namespace robot;
using namespace std;

/*
TEST_CASE("robot") {
    Robot r;
    r.connect("192.168.137.155",80);
    //Robot r("127.0.0.1",6000);
    sleep(1);
    r.set_left(100);
    r.set_right(100);
    r.update();
    sleep(1);
    r.set_left(-100);
    r.set_right(-100);
    r.update();
    sleep(1);
    r.set_left(0);
    r.set_right(0);
    r.set_puf();
    r.update();
    r.update();
    sleep(1);
    for (int i = 0; i<5 ; i++) {
        for (int l = 0; l<3 ; l++){
            r.set_led(l, true);
            r.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            r.set_led(l, false);
            r.update();
        }
    }
}
*/

//
//TEST_CASE("robot_simulation"){
//    Robot_state state;
//    state.location = {0,0};
//    state.theta = 0;
//    state.left = 0;
//    state.right = 0;
//    state.update(.1);
//    cout << state << endl;
//    state.left = 50;
//    state.right = 50;
//    state.update(.1);
//    cout << state << endl;
//    state.left = -50;
//    state.right = -50;
//    state.update(.1);
//    cout << state << endl;
//    state.update(.1);
//    cout << state << endl;
//    state.update(.1);
//    cout << state << endl;
//    state.left = 0;
//    state.update(.1);
//    cout << state << endl;
//    state.update(.1);
//    cout << state << endl;
//}

TEST_CASE("catch!"){
    CHECK(1==1);
}