#include <controller/agent.h>
#include <thread>

using namespace easy_tcp;
using namespace std;

namespace controller {
    double Agent_operational_limits::convert(double v) {
        if (v==0) return stop;
        if (v > 0) { // forward values
            return abs(max_forward - min_forward) * v + min_forward;
        }
        return abs(max_backward - min_backward) * v + min_backward;
    }
}