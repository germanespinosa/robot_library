#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/tracking_service.h>

namespace robot {
    struct Tracking_simulator : agent_tracking::Tracking_server {
        bool send_update(const cell_world::Step &);
        double frame_drop = .05; //send 95% of the updates (simulates missing frames)
        double noise = .001; //reads are up to .1% off
        double bad_reads = .01; // 1% of reads are bad
    };
}