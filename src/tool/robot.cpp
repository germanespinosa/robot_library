#include <iostream>
#include <robot.h>

using namespace std;
using namespace easy_tcp;
using namespace robot;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cout << "Wrong parameters." << endl;
        cout << "Usage: ./robot [ip] [port]" << endl;
        exit(1);
    }
    string ip = argv[1];
    int port = atoi(argv[2]);

    cout << "Robot testing app" << endl;
    cout << "-----------------" << endl;
    string fullName;
    cout << "type command (L R): ";
    string left, right;
    cin >> left;
    Robot robot (ip, port);
    while (left !=""){
        int l = atoi (left.c_str());
        cin >> right;
        int r = atoi (right.c_str());
        robot.set_left((char)l);
        robot.set_right((char)r);
        robot.update();
        cout << "type command (L R): ";
        cin >> left;
    }
    return 0;
}
