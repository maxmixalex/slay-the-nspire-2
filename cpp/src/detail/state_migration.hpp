#pragma once
#include "core.hpp"

namespace spire::state_migration {
// Cold compatibility operations used only while reading historical saves.
void register_post_play(State&, unsigned actor);
void set_temporary_cost(State&, int instance);
bool valid_silent(const State&);
void restore_card_result_order(State&);
} // namespace spire::state_migration
