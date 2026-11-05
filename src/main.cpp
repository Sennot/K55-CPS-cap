#include "Config.hpp"
#include "Ui.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Notification.hpp>

#ifdef GEODE_IS_MOBILE
#include <Geode/modify/PauseLayer.hpp>
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>

using namespace geode::prelude;
using namespace cap;

namespace {

struct Gate {
    bool held = false;
    bool queued = false;
    PlayerButton pendingButton = PlayerButton::Jump;
    double unlockFrame = 0.0;
};

struct PlayerGates {
    Gate gates[SlotCount];
};

std::unordered_map<PlayerObject*, PlayerGates> gates;
// Separate gates per button are only switched on/off between attempts.
bool perButtonActive = false;
uint64_t frameCount = 0;
double medianFrameTime = 1.0 / 240.0;
std::array<float, 9> frameTimes{};
size_t frameTimeIndex = 0;
bool replaying = false;
bool cappedThisAttempt = false;

void resetGates() {
    gates.clear();
    perButtonActive = config().perButton;
    cappedThisAttempt = false;
}

int buttonSlot(PlayerButton button) {
    switch (button) {
        case PlayerButton::Left: return Left;
        case PlayerButton::Right: return Right;
        default: return Jump;
    }
}

void trackFrameTime(float dt) {
    frameTimes[frameTimeIndex % frameTimes.size()] = dt;
    frameTimeIndex++;

    auto count = std::min(frameTimeIndex, frameTimes.size());
    auto sorted = frameTimes;
    std::sort(sorted.begin(), sorted.begin() + count);
    medianFrameTime = sorted[count / 2];
}

double toFrames(double seconds) {
    return seconds / medianFrameTime;
}

bool unlocked(double unlockFrame) {
    return static_cast<double>(frameCount) >= unlockFrame - 0.1;
}

double carryUnlock(double unlockFrame) {
    return static_cast<double>(frameCount) <= unlockFrame + 1.0
        ? unlockFrame
        : static_cast<double>(frameCount);
}

float currentPercent() {
    auto pl = PlayLayer::get();
    return pl ? pl->getCurrentPercent() : 0.f;
}

Segment const& activeSegment() {
    return segmentAt(currentPercent());
}

ButtonCap const& buttonCap(Segment const& seg, int slot) {
    return seg.buttons[perButtonActive ? slot : Jump];
}

bool filterEdge(PlayerObject* player, PlayerButton button, bool press) {
    if (replaying) {
        return true;
    }

    int slot = buttonSlot(button);
    auto& gate = gates[player].gates[perButtonActive ? slot : Jump];
    auto const& seg = activeSegment();

    if (!config().enabled || !seg.cap) {
        // Keep the gate in sync with the real input so capping can resume mid-hold.
        gate.held = press;
        gate.queued = false;
        return true;
    }

    auto const& btn = buttonCap(seg, slot);

    if (seg.debounce) {
        if (!press) {
            gate.held = false;
            return true;
        }
        if (unlocked(gate.unlockFrame)) {
            gate.held = true;
            gate.unlockFrame = carryUnlock(gate.unlockFrame)
                + toFrames(btn.pressLock() + btn.releaseLock());
            return true;
        }
        cappedThisAttempt = true;
        return false;
    }

    if (!gate.queued && unlocked(gate.unlockFrame)) {
        gate.held = press;
        gate.pendingButton = button;
        gate.unlockFrame = carryUnlock(gate.unlockFrame)
            + toFrames(press ? btn.pressLock() : btn.releaseLock());
        return true;
    }

    bool effectiveHeld = gate.queued ? !gate.held : gate.held;
    cappedThisAttempt = true;
    if (press == effectiveHeld) {
        return false;
    }

    if (!gate.queued) {
        gate.queued = true;
        gate.pendingButton = button;
    } else {
        gate.queued = false;
    }
    return false;
}

void flushGate(Gate& gate, PlayerObject* player, Segment const& seg) {
    while (gate.queued && unlocked(gate.unlockFrame)) {
        gate.held = !gate.held;
        gate.queued = false;
        auto const& btn = buttonCap(seg, buttonSlot(gate.pendingButton));
        gate.unlockFrame += toFrames(gate.held ? btn.pressLock() : btn.releaseLock());

        replaying = true;
        if (gate.held) {
            player->pushButton(gate.pendingButton);
        } else {
            player->releaseButton(gate.pendingButton);
        }
        replaying = false;
    }
}

void flushPlayer(PlayerObject* player, Segment const& seg) {
    auto it = gates.find(player);
    if (it == gates.end()) {
        return;
    }
    for (auto& gate : it->second.gates) {
        flushGate(gate, player, seg);
    }
}

void toggleCap() {
    auto& c = config();
    c.enabled = !c.enabled;
    save();
    if (c.toggleNotify) {
        Notification::create(
            c.enabled ? "CPS Cap: ON" : "CPS Cap: OFF",
            c.enabled ? NotificationIcon::Success : NotificationIcon::Error,
            0.6f
        )->show();
    }
}

}

$on_mod(Loaded) {
    cap::load();
    perButtonActive = config().perButton;
    cap::ui::init();
}

$on_game(Loaded) {
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) cap::ui::toggle();
    });
    listenForKeybindSettingPresses("toggle-cap", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) toggleCap();
    });
}

class $modify(CappedBaseLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (dt > 0.f) {
            frameCount++;
            trackFrameTime(dt);
        }

        // Queued edges are replayed even with the cap off so no press/release gets lost.
        if (!gates.empty()) {
            auto const& seg = activeSegment();
            if (m_player1) flushPlayer(m_player1, seg);
            if (m_player2) flushPlayer(m_player2, seg);
        }

        GJBaseGameLayer::update(dt);
    }
};

class $modify(CappedPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        resetGates();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void levelComplete() {
        PlayLayer::levelComplete();

        if (cappedThisAttempt && config().notify) {
            Notification::create("CPS capped", NotificationIcon::Info)->show();
        }
        cappedThisAttempt = false;
    }

    void resetLevel() {
        resetGates();
        PlayLayer::resetLevel();
    }

    void resetLevelFromStart() {
        resetGates();
        PlayLayer::resetLevelFromStart();
    }

    void onQuit() {
        resetGates();
        cap::ui::close();
        PlayLayer::onQuit();
    }
};

class $modify(CappedScheduler, CCScheduler) {
    void update(float dt) {
        cap::ui::tick();
        CCScheduler::update(dt);
    }
};

#ifdef GEODE_IS_MOBILE
// No keyboard on mobile, so the menu stays reachable from the pause menu there.
class $modify(CappedPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto sprite = CCSprite::create("MenuCap.png"_spr);
        if (!sprite) return;
        sprite->setScale(30.f / sprite->getContentSize().height);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(CappedPauseLayer::onCPSCap)
        );
        button->setID("cps-cap-button"_spr);

        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        }
    }

    void onCPSCap(CCObject*) {
        cap::ui::open();
    }
};
#endif

class $modify(CappedPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        if (!filterEdge(this, button, true)) {
            return false;
        }
        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        if (!filterEdge(this, button, false)) {
            return false;
        }
        return PlayerObject::releaseButton(button);
    }
};
