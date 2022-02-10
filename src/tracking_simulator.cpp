#include <tracking_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {
    Tracking_server robot_tracker;
    Space src_space = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>().space;
    Space dst_space = Resources::from("world_implementation").key("hexagonal").key("cv").get_resource<World_implementation>().space;
    double frame_drop = .1; //send 90% of the updates (simulates missing frames)
    double noise = .001; //reads are up to .1% off
    double bad_reads = .01; // 1% of reads are bad

    bool Tracking_simulator::start() {
        int port = Tracking_service::get_port();
        return robot_tracker.start(port);
    }

    bool Tracking_simulator::send_update(const cell_world::Step &step){
        if (Chance::coin_toss(frame_drop)) return true; //send 90% of the updates (simulates missing frames)
        auto transformed_step = step;
        auto new_location = dst_space.transform(step.location, src_space);
        transformed_step.location = new_location;
        //simulates noise
        double noise = noise * dst_space.transformation.size;
        transformed_step.location.x += Chance::dice_double(-noise, noise);
        transformed_step.location.y -= Chance::dice_double(-noise, noise);
        // simulates bad readings

        if (Chance::coin_toss(bad_reads)) { //1% of the updates are garbage
            transformed_step.location = dst_space.center;
            transformed_step.location.x += Chance::dice_double(-dst_space.transformation.size / 4, dst_space.transformation.size / 4);
            transformed_step.location.y -= Chance::dice_double(-dst_space.transformation.size / 4, dst_space.transformation.size / 4);
        }
        auto msg = Message(step.agent_name + "_step", transformed_step);
        robot_tracker.broadcast_subscribed(msg);
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