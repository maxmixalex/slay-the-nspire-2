#pragma once
#ifndef SPIRE_DEBUG_COMMANDS
#define SPIRE_DEBUG_COMMANDS 0
#endif
#ifndef SPIRE_DEBUG_TEXT_INPUT
#define SPIRE_DEBUG_TEXT_INPUT 1
#endif

#if SPIRE_DEBUG_COMMANDS
#include "world_run.hpp"
#include "original_events.hpp"

namespace spire {
// These entry points and their transaction storage do not exist in a normal
// build. A failed command preserves the complete State, including RNG words.
// Kill-all also cancels a suspended combat choice. A single-target kill is
// accepted only at an idle player-action boundary.
bool debug_kill_enemies(State&,int target=-1);
// Replaces the room at the current map coordinate without inventing a route
// or another floor. None selects the canonical next encounter/event. An
// explicit encounter may be from any act. Dead runs revive at their max HP.
bool debug_enter_room(State&,WorldRoom,EncounterId=EncounterId::None,
                      OriginalEventId=OriginalEventId::None);
// Zero-based act. Regenerates that act and discards its later route history;
// earlier visited acts, deck facts, inventory and named RNG streams survive.
bool debug_enter_act(State&,unsigned act);

namespace debug_detail {
// Shared by the two implementation translation units, never by gameplay.
bool begin(State&);
bool finish(State&,bool success);
void abandon_room(State&);
}
} // namespace spire
#endif
