#pragma once

namespace cap::ui {

void init();
// Once per frame from CCScheduler::update.
void tick();
void open();
void close();
void toggle();

}  // namespace cap::ui
