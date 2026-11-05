#include "Config.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace cap {

namespace {

Config g_config;

ButtonCap buttonFromJson(matjson::Value const& v) {
    ButtonCap b;
    b.cps = std::clamp(v["cps"].asDouble().unwrapOr(K55_CPS), MIN_CPS, MAX_CPS);
    b.split = std::clamp(v["split"].asDouble().unwrapOr(DEFAULT_SPLIT), MIN_SPLIT, MAX_SPLIT);
    return b;
}

matjson::Value buttonToJson(ButtonCap const& b) {
    return matjson::makeObject({{"cps", b.cps}, {"split", b.split}});
}

char const* kSlotKeys[SlotCount] = {"jump", "left", "right"};

}  // namespace

Config& config() { return g_config; }

void load() {
    auto mod = Mod::get();
    Config c;
    if (mod->hasSavedValue("config")) {
        auto v = mod->getSavedValue<matjson::Value>("config");
        c.enabled = v["enabled"].asBool().unwrapOr(true);
        c.perButton = v["perButton"].asBool().unwrapOr(false);
        c.notify = v["notify"].asBool().unwrapOr(true);
        c.toggleNotify = v["toggleNotify"].asBool().unwrapOr(true);
        c.segments.clear();
        if (v["segments"].isArray()) {
            for (auto const& s : v["segments"]) {
                Segment seg;
                seg.from = static_cast<float>(std::clamp(s["from"].asDouble().unwrapOr(0.0), 0.0, 100.0));
                seg.cap = s["cap"].asBool().unwrapOr(true);
                seg.debounce = s["debounce"].asBool().unwrapOr(false);
                for (int i = 0; i < SlotCount; i++) seg.buttons[i] = buttonFromJson(s[kSlotKeys[i]]);
                c.segments.push_back(seg);
            }
        }
    } else {
        // Values saved by 3.0.x.
        Segment seg;
        seg.debounce = mod->getSavedValue<bool>("debounce", false);
        ButtonCap b;
        b.cps = std::clamp(mod->getSavedValue<double>("max-cps", K55_CPS), MIN_CPS, MAX_CPS);
        b.split = std::clamp(mod->getSavedValue<double>("hold-fraction", DEFAULT_SPLIT), MIN_SPLIT, MAX_SPLIT);
        for (auto& btn : seg.buttons) btn = b;
        c.enabled = mod->getSavedValue<bool>("enabled", true);
        c.notify = mod->getSavedValue<bool>("show-notification", true);
        c.segments = {seg};
    }
    g_config = std::move(c);
    normalize();
}

void save() {
    std::vector<matjson::Value> segs;
    for (auto const& s : g_config.segments) {
        auto obj = matjson::makeObject({
            {"from", static_cast<double>(s.from)},
            {"cap", s.cap},
            {"debounce", s.debounce},
        });
        for (int i = 0; i < SlotCount; i++) obj.set(kSlotKeys[i], buttonToJson(s.buttons[i]));
        segs.push_back(std::move(obj));
    }
    Mod::get()->setSavedValue("config", matjson::makeObject({
        {"enabled", g_config.enabled},
        {"perButton", g_config.perButton},
        {"notify", g_config.notify},
        {"toggleNotify", g_config.toggleNotify},
        {"segments", matjson::Value(segs)},
    }));
}

void normalize() {
    auto& segs = g_config.segments;
    if (segs.empty()) segs.push_back(Segment{});
    std::stable_sort(segs.begin(), segs.end(), [](Segment const& a, Segment const& b) { return a.from < b.from; });
    segs.front().from = 0.f;
}

int segmentIndexAt(float percent) {
    auto const& segs = g_config.segments;
    int index = 0;
    for (int i = 1; i < static_cast<int>(segs.size()); i++) {
        if (percent >= segs[i].from) index = i;
    }
    return index;
}

Segment const& segmentAt(float percent) { return g_config.segments[segmentIndexAt(percent)]; }

}  // namespace cap
