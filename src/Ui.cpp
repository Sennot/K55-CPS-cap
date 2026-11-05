#include "Ui.hpp"

#include "Config.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <imgui-cocos.hpp>
#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>

using namespace geode::prelude;

namespace cap::ui {

namespace {

// ------------------------------------------------------------------ look

namespace col {
constexpr ImU32 bg = IM_COL32(14, 14, 14, 255);
constexpr ImU32 panel = IM_COL32(19, 19, 19, 255);
constexpr ImU32 line = IM_COL32(40, 40, 40, 255);
constexpr ImU32 ctrl = IM_COL32(32, 32, 32, 255);
constexpr ImU32 ctrlHover = IM_COL32(44, 44, 44, 255);
constexpr ImU32 text = IM_COL32(214, 214, 214, 255);
constexpr ImU32 textMuted = IM_COL32(96, 96, 96, 255);
constexpr ImU32 accent = IM_COL32(126, 170, 226, 255);
constexpr ImU32 off = IM_COL32(210, 96, 96, 255);
}  // namespace col

ImVec4 v4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

ImFont* g_regular = nullptr;
ImFont* g_bold = nullptr;

float scale() { return std::clamp(ImGui::GetIO().DisplaySize.y / 900.f, 1.f, 2.5f); }
float px(float v) { return std::floor(v * scale()); }
float fontSize() { return std::round(13.f * scale()); }

ImFont* loadFont(char const* file) {
    char const* win = std::getenv("WINDIR");
    if (!win) return nullptr;
    auto path = std::filesystem::path(win) / "Fonts" / file;
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return nullptr;
    ImFontConfig fc;
    fc.OversampleH = 1;
    fc.OversampleV = 1;
    fc.PixelSnapH = true;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), 13.f, &fc);
}

void setupTheme() {
    g_regular = loadFont("verdana.ttf");
    g_bold = loadFont("verdanab.ttf");
    if (!g_regular) g_regular = ImGui::GetIO().Fonts->AddFontDefault();
    if (!g_bold) g_bold = g_regular;

    auto& s = ImGui::GetStyle();
    s.WindowRounding = s.ChildRounding = s.FrameRounding = s.PopupRounding = 0.f;
    s.ScrollbarRounding = s.GrabRounding = s.TabRounding = 0.f;
    s.WindowBorderSize = 1.f;
    s.ChildBorderSize = 1.f;
    s.PopupBorderSize = 1.f;
    s.FrameBorderSize = 0.f;
    s.ScrollbarSize = 8.f;
    s.GrabMinSize = 6.f;
    s.WindowPadding = {10.f, 10.f};
    s.FramePadding = {6.f, 3.f};
    s.ItemSpacing = {6.f, 5.f};
    s.ItemInnerSpacing = {5.f, 4.f};
    s.AntiAliasedLines = false;

    auto* c = s.Colors;
    c[ImGuiCol_Text] = v4(col::text);
    c[ImGuiCol_TextDisabled] = v4(col::textMuted);
    c[ImGuiCol_WindowBg] = v4(col::bg);
    c[ImGuiCol_ChildBg] = v4(col::panel);
    c[ImGuiCol_PopupBg] = v4(col::panel);
    c[ImGuiCol_Border] = v4(col::line);
    c[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
    c[ImGuiCol_FrameBg] = v4(col::ctrl);
    c[ImGuiCol_FrameBgHovered] = v4(col::ctrlHover);
    c[ImGuiCol_FrameBgActive] = v4(col::ctrlHover);
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    c[ImGuiCol_ScrollbarGrab] = v4(IM_COL32(48, 48, 48, 255));
    c[ImGuiCol_ScrollbarGrabHovered] = v4(IM_COL32(64, 64, 64, 255));
    c[ImGuiCol_ScrollbarGrabActive] = v4(IM_COL32(80, 80, 80, 255));
    c[ImGuiCol_CheckMark] = v4(col::accent);
    c[ImGuiCol_SliderGrab] = v4(col::accent);
    c[ImGuiCol_SliderGrabActive] = v4(col::accent);
    c[ImGuiCol_Button] = v4(col::ctrl);
    c[ImGuiCol_ButtonHovered] = v4(col::ctrlHover);
    c[ImGuiCol_ButtonActive] = v4(IM_COL32(54, 54, 54, 255));
    c[ImGuiCol_Header] = v4(col::ctrl);
    c[ImGuiCol_HeaderHovered] = v4(col::ctrlHover);
    c[ImGuiCol_HeaderActive] = v4(col::ctrlHover);
    c[ImGuiCol_Separator] = v4(col::line);
    c[ImGuiCol_ResizeGrip] = {0, 0, 0, 0};
    c[ImGuiCol_ResizeGripHovered] = {0, 0, 0, 0};
    c[ImGuiCol_ResizeGripActive] = {0, 0, 0, 0};
}

// Groupbox with the title set into the top border.
struct Box {
    ImVec2 start;
    float width;
    char const* title;
};

Box boxBegin(char const* title, float width) {
    ImGui::Dummy({0, std::round(ImGui::GetFontSize() * 0.5f)});
    Box b{ImGui::GetCursorScreenPos(), width, title};
    ImGui::BeginGroup();
    ImGui::Dummy({width, px(8)});
    ImGui::Indent(px(10));
    return b;
}

void boxEnd(Box const& b) {
    ImGui::Unindent(px(10));
    ImGui::Dummy({b.width, px(4)});
    ImGui::EndGroup();
    auto dl = ImGui::GetWindowDrawList();
    ImVec2 mn = b.start;
    ImVec2 mx = {b.start.x + b.width, ImGui::GetItemRectMax().y};
    dl->AddRect(mn, mx, col::line);
    auto ts = ImGui::CalcTextSize(b.title);
    ImVec2 tp = {mn.x + px(9), mn.y - std::round(ts.y * 0.5f)};
    dl->AddRectFilled({tp.x - px(3), tp.y}, {tp.x + ts.x + px(3), tp.y + ts.y}, col::bg);
    dl->AddText(tp, col::textMuted, b.title);
}

// ------------------------------------------------------------------ state

struct State {
    bool open = false;
    int selected = 0;
};

State g;

float livePercent(bool& inLevel) {
    auto pl = PlayLayer::get();
    inLevel = pl != nullptr;
    return pl ? pl->getCurrentPercent() : 0.f;
}

// Moves the edited segment so the list stays ordered by percent.
void resortSelected() {
    auto& segs = config().segments;
    int& i = g.selected;
    while (i > 1 && segs[i - 1].from > segs[i].from) {
        std::swap(segs[i - 1], segs[i]);
        i--;
    }
    while (i + 1 < static_cast<int>(segs.size()) && segs[i + 1].from < segs[i].from) {
        std::swap(segs[i + 1], segs[i]);
        i++;
    }
}

bool sliderCps(char const* id, double& cps, float width) {
    double lo = MIN_CPS, hi = MAX_CPS;
    ImGui::SetNextItemWidth(width);
    bool changed = ImGui::SliderScalar(id, ImGuiDataType_Double, &cps, &lo, &hi, "%.2f cps",
                                       ImGuiSliderFlags_AlwaysClamp);
    return changed;
}

bool sliderSplit(char const* id, double& split, float width) {
    double pct = split * 100.0;
    double lo = MIN_SPLIT * 100.0, hi = MAX_SPLIT * 100.0;
    ImGui::SetNextItemWidth(width);
    if (ImGui::SliderScalar(id, ImGuiDataType_Double, &pct, &lo, &hi, "%.0f%% press",
                            ImGuiSliderFlags_AlwaysClamp)) {
        split = std::clamp(pct / 100.0, MIN_SPLIT, MAX_SPLIT);
        return true;
    }
    return false;
}

// Typed value next to a slider, for exact numbers.
bool numberInput(char const* id, double& v, double lo, double hi, char const* format, float width) {
    double tmp = v;
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputScalar(id, ImGuiDataType_Double, &tmp, nullptr, nullptr, format)) {
        v = std::clamp(tmp, lo, hi);
        return true;
    }
    return false;
}

float inputWidth() { return px(64); }

bool buttonEditor(char const* label, ButtonCap& b, float width) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    float sp = ImGui::GetStyle().ItemSpacing.x;
    float k55w = ImGui::CalcTextSize("K55").x + ImGui::GetStyle().FramePadding.x * 2;
    float inW = inputWidth();
    changed |= sliderCps("##cps", b.cps, width - inW - k55w - sp * 2);
    ImGui::SameLine();
    changed |= numberInput("##cpsIn", b.cps, MIN_CPS, MAX_CPS, "%.4f", inW);
    ImGui::SameLine();
    if (ImGui::Button("K55")) {
        b.cps = K55_CPS;
        changed = true;
    }
    changed |= sliderSplit("##split", b.split, width - inW - k55w - sp * 2);
    ImGui::SameLine();
    double pct = b.split * 100.0;
    if (numberInput("##splitIn", pct, MIN_SPLIT * 100.0, MAX_SPLIT * 100.0, "%.1f", inW)) {
        b.split = pct / 100.0;
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

void drawGeneral(float width) {
    auto& c = config();
    bool changed = false;
    auto box = boxBegin("general", width);
    changed |= ImGui::Checkbox("Enabled", &c.enabled);
    changed |= ImGui::Checkbox("Separate buttons", &c.perButton);
    changed |= ImGui::Checkbox("Capped notification", &c.notify);
    changed |= ImGui::Checkbox("Toggle notification", &c.toggleNotify);
    if (ImGui::Button("Keybinds", {width - px(20), 0})) {
        close();
        openSettingsPopup(Mod::get(), false);
    }
    boxEnd(box);
    if (changed) save();
}

void drawSegments(float width, int active, bool inLevel, float percent) {
    auto& segs = config().segments;
    auto box = boxBegin("segments", width);
    float inner = width - px(20);
    for (int i = 0; i < static_cast<int>(segs.size()); i++) {
        auto const& s = segs[i];
        ImGui::PushID(i);
        auto label = s.cap ? fmt::format("{:5.1f}%   {:.2f}", s.from, s.buttons[Jump].cps)
                           : fmt::format("{:5.1f}%   off", s.from);
        bool isActive = inLevel && i == active;
        if (isActive) ImGui::PushStyleColor(ImGuiCol_Text, v4(col::accent));
        if (ImGui::Selectable(label.c_str(), g.selected == i, 0, {inner, 0})) g.selected = i;
        if (isActive) ImGui::PopStyleColor();
        ImGui::PopID();
    }
    float half = std::floor((inner - ImGui::GetStyle().ItemSpacing.x) / 2);
    if (ImGui::Button("Add", {half, 0})) {
        Segment seg = segs[g.selected];
        seg.from = inLevel && percent > 0.f ? std::round(percent * 10.f) / 10.f
                                            : std::min(100.f, segs.back().from + 10.f);
        segs.push_back(seg);
        g.selected = static_cast<int>(segs.size()) - 1;
        resortSelected();
        save();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(segs.size() <= 1);
    if (ImGui::Button("Remove", {half, 0})) {
        segs.erase(segs.begin() + g.selected);
        normalize();
        g.selected = std::min(g.selected, static_cast<int>(segs.size()) - 1);
        save();
    }
    ImGui::EndDisabled();
    boxEnd(box);
}

void drawSegment(float width) {
    auto& c = config();
    auto& segs = c.segments;
    auto& s = segs[g.selected];
    bool changed = false;
    float inner = width - px(20);

    auto box = boxBegin("segment", width);
    ImGui::BeginDisabled(g.selected == 0);
    float inW = inputWidth();
    ImGui::SetNextItemWidth(inner - inW - ImGui::GetStyle().ItemSpacing.x);
    bool fromChanged = ImGui::SliderFloat("##from", &s.from, 0.f, 100.f, "from %.2f%%", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    double from = s.from;
    if (numberInput("##fromIn", from, 0.0, 100.0, "%.2f", inW)) {
        s.from = static_cast<float>(from);
        fromChanged = true;
    }
    if (fromChanged) {
        resortSelected();
        changed = true;
    }
    ImGui::EndDisabled();
    changed |= ImGui::Checkbox("Cap", &segs[g.selected].cap);
    ImGui::SameLine(0, px(16));
    changed |= ImGui::Checkbox("Debounce", &segs[g.selected].debounce);
    boxEnd(box);

    auto& seg = segs[g.selected];
    ImGui::BeginDisabled(!seg.cap);
    auto btnBox = boxBegin("buttons", width);
    if (c.perButton) {
        changed |= buttonEditor("Jump", seg.buttons[Jump], inner);
        ImGui::Dummy({0, px(2)});
        changed |= buttonEditor("Left", seg.buttons[Left], inner);
        ImGui::Dummy({0, px(2)});
        changed |= buttonEditor("Right", seg.buttons[Right], inner);
    } else {
        if (buttonEditor("All", seg.buttons[Jump], inner)) {
            seg.buttons[Left] = seg.buttons[Right] = seg.buttons[Jump];
            changed = true;
        }
    }
    boxEnd(btnBox);
    ImGui::EndDisabled();

    if (changed) save();
}

void drawWindow() {
    auto& c = config();
    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    bool open = true;
    if (!ImGui::Begin("##cpscap", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                                             ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    auto dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    dl->AddRectFilled(wp, {wp.x + ws.x, wp.y + px(2)}, col::accent);

    g.selected = std::clamp(g.selected, 0, static_cast<int>(c.segments.size()) - 1);
    bool inLevel = false;
    float percent = livePercent(inLevel);
    int active = segmentIndexAt(percent);

    float leftW = px(210), rightW = px(320);
    float spacing = ImGui::GetStyle().ItemSpacing.x * 2;

    ImGui::PushFont(g_bold, std::round(fontSize() * 1.15f));
    ImGui::TextUnformatted("cps cap");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextColored(v4(c.enabled ? col::accent : col::off), c.enabled ? "on" : "off");
    if (inLevel) {
        ImGui::SameLine();
        ImGui::TextDisabled("%.1f%%  /  segment %d", percent, active + 1);
    }
    float closeW = ImGui::CalcTextSize(" x ").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + leftW + spacing + rightW - closeW);
    if (ImGui::SmallButton(" x ")) open = false;

    ImGui::BeginGroup();
    drawGeneral(leftW);
    drawSegments(leftW, active, inLevel, percent);
    ImGui::EndGroup();
    ImGui::SameLine(0, spacing);
    ImGui::BeginGroup();
    drawSegment(rightW);
    ImGui::EndGroup();

    ImGui::End();
    if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) close();
}

void draw() {
    if (!g.open) return;
    auto& io = ImGui::GetIO();
    auto pl = PlayLayer::get();
    io.MouseDrawCursor = pl && !pl->m_isPaused;
    ImGui::PushFont(g_regular, fontSize());
    drawWindow();
    ImGui::PopFont();
}

}  // namespace

// ------------------------------------------------------------------ public

void init() {
    ImGuiCocos::get().setup([] { setupTheme(); }).draw([] { draw(); });
    ImGuiCocos::get().setVisible(false);
}

void tick() {
    auto& ic = ImGuiCocos::get();
    if (ic.isVisible() != g.open) ic.setVisible(g.open);
    static bool blocking = false;
    if (blocking != g.open) {
        blocking = g.open;
        ic.setInputMode(g.open ? ImGuiCocos::InputMode::Blocking : ImGuiCocos::InputMode::Default);
        if (!g.open) ImGui::GetIO().MouseDrawCursor = false;
    }
}

void open() { g.open = true; }

void close() { g.open = false; }

void toggle() {
    // Debounce on the wall clock: ImGui's clock stops while nothing is drawn.
    using clock = std::chrono::steady_clock;
    static clock::time_point last{};
    auto t = clock::now();
    if (t - last < std::chrono::milliseconds(150)) return;
    last = t;
    g.open = !g.open;
}

}  // namespace cap::ui
