#include <controller/pid_controller.h>

using namespace cell_world;

namespace controller{

    Pid_outputs Pid_controller::process(const Pid_inputs &inputs, Behavior behavior) {
        double speed;
        if (behavior==Explore){
            speed = parameters.explore_speed;
        } else {
            speed = parameters.pursue_speed;
        }
        in = inputs;
        auto dist = inputs.location.dist(inputs.destination);
        double destination_theta = inputs.location.atan(inputs.destination);
        auto theta = to_radians(inputs.rotation);
        error = angle_difference(theta, destination_theta) * direction(theta, destination_theta);
        normalized_error = normalize_error(error);
        error_derivative = last_error - error;
        last_error = error;
        error_integral += error;
        double adjustment = error * parameters.P_value - error_derivative * parameters.D_value + error_integral * parameters.I_value;
        out.left =  normalized_error * speed * ( dist + 1 ) - adjustment;
        // catches outliers
        if (out.left < -1) {
            out.left = -1;
        }
        if (out.left > 1){
            out.left = 1;
        }
        out.right = normalized_error * speed * ( dist + 1 ) + adjustment;
        if (out.right < -1) {
            out.right = -1;
        }
        if (out.right > 1){
            out.right = 1;
        }
        return out;
    }

    double Pid_controller::normalize_error(double error){
        double pi_err = M_PI * error;
        return 1 / ( pi_err * pi_err + 1 );
    }

    Pid_controller::Pid_controller(const Pid_parameters &parameters): parameters(parameters) {

    }
}