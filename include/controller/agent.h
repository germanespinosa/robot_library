#pragma once
#include <easy_tcp.h>
#include <json_cpp.h>

namespace controller {
    struct Agent_operational_limits :json_cpp::Json_object {
        Json_object_members(
        Add_member(max_forward);
        Add_member(min_forward);
        Add_member(max_backward);
        Add_member(min_backward);
        Add_member(stop);
        )
        double max_forward;
        double min_forward;
        double max_backward;
        double min_backward;
        double stop;
        double convert(double);
    };

    struct Agent {
        virtual void set_left(double) = 0;
        virtual void set_right(double)  = 0;
        virtual void capture()  = 0;
        virtual bool update()  = 0;
    };
}