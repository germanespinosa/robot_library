#pragma once
#include <json_cpp.h>
#include <cell_world.h>

namespace controller {

    struct Pid_outputs : json_cpp::Json_object{
        Json_object_members(
                Add_member(left);
                Add_member(right);
        )
        int left;
        int right;
    };

    struct Pid_inputs : json_cpp::Json_object{
        Json_object_members(
                Add_member(location);
                Add_member(rotation);
                Add_member(destination);
        );
        cell_world::Location location;
        double rotation;
        cell_world::Location destination;
    };

    // values located in robot_library/config/pid.json
    struct Pid_parameters : json_cpp::Json_object{
        Json_object_members(
                Add_member(P_value);
                Add_member(I_value);
                Add_member(D_value);
                Add_member(speed);
                );
        double P_value;
        double I_value;
        double D_value;
        double speed;
    };

    struct Pid_controller : json_cpp::Json_object {
        Pid_controller (const Pid_parameters &);
        Json_object_members(
                Add_member(error);
                Add_member(normalized_error);
                Add_member(error_integral);
                Add_member(error_derivative);
                Add_member(last_error);
                Add_member(out);
                Add_member(in);
                );
        Pid_outputs process(const Pid_inputs &);
        static double normalize_error(double);
        Pid_parameters parameters;
        double error{};
        double error_integral{};
        double error_derivative{};
        double normalized_error{};
        double last_error{};
        Pid_outputs out;
        Pid_inputs in;
    };
}