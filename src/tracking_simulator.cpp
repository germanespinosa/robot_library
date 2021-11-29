#include <tracking_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;

namespace robot {
    easy_tcp::Server<Tracking_simulator_service> robot_tracker;

    bool Tracking_simulator::start() {
        int port = agent_tracking::Service::get_port();
        return robot_tracker.start(port);
    }

    bool Tracking_simulator::send_update(const cell_world::Step &step){
        auto msg = Message(step.agent_name+"_step", step);
        Tracking_simulator_service::send_update(msg);
        return true;
    }

    bool Tracking_simulator::stop() {
        robot_tracker.stop();
        return true;
    }

    void Tracking_simulator_service::unrouted_message(const Message &m) {
        cout << "Unrouted message: " << m << endl;
    }

}