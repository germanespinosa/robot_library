#include <tracking_simulator.h>

using namespace json_cpp;
using namespace std;
using namespace cell_world;
using namespace agent_tracking;
using namespace tcp_messages;

namespace robot {
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
        send_step(transformed_step);
        return true;
    }
}