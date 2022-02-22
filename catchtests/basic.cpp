#include <catch.h>
#include <experiment/experiment_messages.h>

using namespace experiment;
using namespace std;
TEST_CASE("start_episode") {
    Start_episode_request s;
    "{\"experiment_name\":\"PREFIX_20220221_2256_SUBJECT_10_05_SUFFIX\"}" >> s;
    cout << s << endl;
}