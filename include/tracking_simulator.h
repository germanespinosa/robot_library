#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/tracking_service.h>

namespace robot {
    struct Tracking_simulator{
        static bool start();
        static bool send_update(const cell_world::Step &);
        static bool stop();
        static void set_noise(double );
        static void set_bad_reads(double );
        static void set_frame_drop(double );
    };
}