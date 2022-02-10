#include <tracking_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {
    Tracking_server robot_tracker;
    double frame_drop = .05; //send 95% of the updates (simulates missing frames)
    double noise = .001; //reads are up to .1% off
    double bad_reads = .01; // 1% of reads are bad

    bool Tracking_simulator::start() {
        int port = agent_tracking::Tracking_service::get_port();
        return robot_tracker.start(port);
    }

    bool Tracking_simulator::send_update(const cell_world::Step &step){
        if (Chance::coin_toss(frame_drop)) return true; //send 95% of the updates (simulates missing frames)
        auto transformed_step = step;
        //simulates noise
        transformed_step.location.x += Chance::dice_double(-noise, noise);
        transformed_step.location.y -= Chance::dice_double(-noise, noise);
        // simulates bad readings
        if (Chance::coin_toss(bad_reads)) { //1% of the updates are garbage
            transformed_step.location = Location(Chance::dice_double(0, 1),Chance::dice_double(0, 1));
        }
        robot_tracker.send_step(transformed_step);
        return true;
    }

    bool Tracking_simulator::stop() {
        robot_tracker.stop();
        return true;
    }

    void Tracking_simulator::set_noise(double new_noise) {
        noise = new_noise;
    }

    void Tracking_simulator::set_bad_reads(double new_bad_reads) {
        bad_reads = new_bad_reads;
    }

    void Tracking_simulator::set_frame_drop(double new_frame_drop) {
        frame_drop = new_frame_drop;
    }

}