#include <pid_controller.h>
// constants for motor threshold speeds
#define MAX_FWD 105      // 90
#define MIN_FWD 70      // 70
#define MAX_BCK -105     // -90
#define MIN_BCK -65     // -65
#define MAX 127

using namespace cell_world;

namespace robot{

    Pid_outputs Pid_controller::process(const Pid_inputs &inputs) {
        in = inputs;
        auto dist = inputs.location.dist(inputs.destination);
        // 0.01
        if ( dist < .05) {
            out.left = 0;
            out.right = 0;
            return out;
        }
        double destination_theta = inputs.location.atan(inputs.destination);
        auto theta = to_radians(inputs.rotation);
        error = angle_difference(theta, destination_theta) * direction(theta, destination_theta);
        normalized_error = normalize_error(error);
        error_derivative = last_error - error;
        last_error = error;
        error_integral += error;

        double adjustment = error * parameters.P_value - error_derivative * parameters.D_value + error_integral * parameters.I_value;
        double left =  normalized_error * parameters.speed * ( dist + 1 ) - adjustment;
        int fwd_range = MAX_FWD - MIN_FWD;
        int bck_range = MIN_BCK - MAX_BCK;
        // catches outliers
//        if (left < -127) left = -127;
//        if (left > 127) left = 127;
        if (left < 0) {
            left = ((left/MAX) * bck_range) + MIN_BCK;
//            if (left < -90) left = MAX_BCK;
//            else if (left > -65) left = MIN_BCK;
        }
        if (left > 0){
            left = ((left/MAX) * fwd_range) + MIN_FWD;
//            if (left > 90) left = MAX_FWD;
//            else if (left < 70) left = MIN_FWD;
        }




        out.left = (char) left;

        double right = normalized_error * parameters.speed * ( dist + 1 ) + adjustment;
//        if (right < -127) right = -127;
//        if (right > 127) right = 127;
        if (right < 0) {
            right = ((right/MAX) * bck_range) + MIN_BCK;
//            if (right < -90) right = MAX_BCK;
//            else if (right > -65) right = MIN_BCK;
        }
        if (right > 1){
            right = ((right/MAX) * fwd_range) + MIN_FWD;
//            if (right > 90) right = MAX_FWD;
//            else if (right < 70) right = MIN_FWD;
        }
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