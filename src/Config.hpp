#pragma once

#include <vector>

namespace cap {

constexpr double K55_CPS = 16.6667;
constexpr double DEFAULT_SPLIT = 0.5;
constexpr double MIN_CPS = 1.0;
constexpr double MAX_CPS = 240.0;
constexpr double MIN_SPLIT = 0.1;
constexpr double MAX_SPLIT = 0.9;

enum Slot { Jump = 0, Left = 1, Right = 2, SlotCount = 3 };

struct ButtonCap {
    double cps = K55_CPS;
    double split = DEFAULT_SPLIT;  // share of the period the press stays locked

    double pressLock() const { return split / cps; }
    double releaseLock() const { return (1.0 - split) / cps; }
};

// Settings that apply from `from` percent until the next segment.
struct Segment {
    float from = 0.f;
    bool cap = true;
    bool debounce = false;
    ButtonCap buttons[SlotCount];
};

struct Config {
    bool enabled = true;
    bool perButton = false;
    bool notify = true;
    bool toggleNotify = true;
    std::vector<Segment> segments{Segment{}};
};

Config& config();
void load();
void save();
// Keeps segments ordered by `from`, the first one at 0 %.
void normalize();
Segment const& segmentAt(float percent);
int segmentIndexAt(float percent);

}  // namespace cap
