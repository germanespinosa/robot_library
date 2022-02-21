#pragma once
#include <tcp_messages.h>
#include <controller/agent.h>
#include <cell_world.h>
#include <robot.h>
#include <controller/pid_controller.h>
#include <agent_tracking/tracking_client.h>
#include <experiment/experiment_client.h>

namespace controller {

    enum Controller_state{
        Stopped,
        Playing,
        Paused,
    };

    struct Controller_service : tcp_messages::Message_service {
        Routes (
            Add_route_with_response("set_destination", set_destination, cell_world::Location);
            Add_route_with_response("stop", stop_controller);
            Add_route_with_response("pause", pause_controller);
            Add_route_with_response("resume", resume_controller);
            Add_route_with_response("set_behavior", set_behavior, int);
            Allow_subscription();
        );

        bool set_destination(const cell_world::Location &);
        bool stop_controller();
        bool pause_controller();
        bool resume_controller();
        bool set_behavior(int);
        static int get_port();

    };


    struct Agent_data{
        Agent_data (const std::string &agent_name) :
        agent_name(agent_name) {
            step.agent_name = agent_name;
        }
        std::string agent_name;
        cell_world::Step step;
        cell_world::Timer timer;
        bool is_valid() {
            return !timer.time_out();
        }
    };

    struct Controller_server : tcp_messages::Message_server<Controller_service> {
        Controller_server(const std::string &pid_config_file_path, const std::string &agent_ip, const std::string &tracker_ip, const std::string &experiment_service_ip);
        void send_step(const cell_world::Step &);
        bool set_destination(const cell_world::Location &);
        bool pause();
        bool resume();
        void set_world(const cell_world::World_info&);
        bool set_behavior(int behavior);
        void join();
        cell_world::Location destination;
        cell_world::Timer destination_timer;
        bool new_destination_data;
        std::atomic<Controller_state> state;
        Agent agent;

        struct Controller_experiment_client : experiment::Experiment_client {
            explicit Controller_experiment_client(Controller_server &);
            void on_experiment_started(const experiment::Start_experiment_response &experiment) override;
            void on_episode_started(const std::string &experiment_name) override;
            Controller_server &controller_server;
        } experiment_client;

        Behavior behavior = Explore;
        cell_world::Peeking peeking;
        cell_world::Location_visibility visibility;
        cell_world::Location_visibility navigability;
        Pid_controller pid_controller;
        cell_world::World_configuration world_configuration;
        cell_world::World_implementation world_implementation;
        cell_world::Cell_group_builder occlusions;
        cell_world::World world;
        cell_world::Cell_group cells;
        cell_world::Graph graph;
        cell_world::Paths paths;
        cell_world::Map map;
        cell_world::Capture capture;
        std::thread process;
        cell_world::Location get_next_stop();

        struct Controller_tracker : agent_tracking::Tracking_client {
            Controller_tracker(Controller_server &server, cell_world::Location_visibility &visibility, float view_angle, cell_world::Capture &capture, experiment::Experiment_client &experiment_client, cell_world::Peeking &peeking, const std::string &agent_name, const std::string &adversary_name) :
            agent(agent_name),
            adversary(adversary_name),
            server(server),
                    visibility(visibility),
                    view_angle(view_angle),
                    capture(capture),
                    experiment_client(experiment_client),
                    peeking(peeking){
            }
            void on_step(const cell_world::Step &step) override;
            Agent_data agent;
            Agent_data adversary;
            cell_world::Location_visibility &visibility;
            Controller_server &server;
            float view_angle;
            cell_world::Capture &capture;
            experiment::Experiment_client &experiment_client;
            cell_world::Peeking &peeking;
        } tracker;

        void controller_process();
    };
}