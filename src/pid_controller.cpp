#include <pid_controller.h>

using namespace cell_world;

namespace robot{

    Pid_outputs Pid_controller::process(const Pid_inputs &inputs) {
        in = inputs;
        if (inputs.location.dist(inputs.destination) < .01) {
            out.left = 0;
            out.right = 0;
            return out;
        }
        double destination_rotation = inputs.location.atan(inputs.destination);
        error = angle_difference(inputs.rotation, destination_rotation) * direction(inputs.rotation, destination_rotation);
        normalized_error = normalize_error(error);
        error_derivative = last_error - error;
        last_error = error;
        error_integral += error;

        double adjustment = error * parameters.P_value - error_derivative * parameters.D_value + error_integral * parameters.I_value;
        double left =  normalized_error * parameters.speed - adjustment;
        if (left < -127) left = -127;
        if (left > 127) left = 127;
        out.left = (char) left;
        double right = normalized_error * parameters.speed + adjustment;
        if (right < -127) right = -127;
        if (right > 127) right = 127;
        out.right = (char) right;
        return out;
    }

    double Pid_controller::normalize_error(double error){
        double pi_err = M_PI * error;
        return 1 / ( pi_err * pi_err + 1 );
    }

    Pid_controller::Pid_controller(const Pid_parameters &parameters): parameters(parameters) {

    }
}