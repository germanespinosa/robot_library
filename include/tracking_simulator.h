#pragma once
#include <easy_tcp.h>
#include <cell_world.h>
#include <agent_tracking/service.h>

namespace robot {

    struct Tracking_simulator_service : agent_tracking::Service {

        // routes
        //experiment
        void new_experiment(const std::string &) override {};
        void new_episode(agent_tracking::New_episode_message) override {};
        void end_episode() override {};

        //camera
        void update_background() override {};
        void reset_cameras() override {};
        void update_puff() override {};

        //visualization
        void show_occlusions(const std::string &) override {};
        void hide_occlusions() override {};
        //unrouted
    };

    struct Tracking_simulator{
        static bool start();
        static bool send_update(const cell_world::Step &);
        static bool stop();
        static void set_noise(double );
        static void set_bad_reads(double );
        static void set_frame_drop(double );
    };
}