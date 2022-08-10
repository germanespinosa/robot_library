#include <robot_lib/robot.h>
#include <cell_world.h>
#include <iostream>
#include <robot_lib/robot_agent.h>


using namespace std;
using namespace robot;
using namespace gamepad;
using namespace controller;

struct MyData {
    int32_t left, right;
    int32_t speed;
} mydata;

int main(){
    bool update;
    cout << "hi" << endl;
    string robot_ip = "127.0.0.1";

    // connect to robot
    Robot_agent robot;
    if (!robot.connect(robot_ip))
    {
        cout << "Unable to connect to robot." <<  endl;
        exit(0);
    }
    robot.set_left(300);
    robot.set_right(-300);
    robot.set_speed(1800);

    cell_world::Timer().wait(2);
    robot.update();
    cell_world::Timer().wait(2);


    cell_world::Timer().wait(10);
    int i;
    while (1){
        i += 1;
    }

        //    mydata.left = 1000;
        //    mydata.right = 1000;
        //    mydata.speed = 1000;

}

