#pragma once

#include "world_run.hpp"
#include "original_events.hpp"
#include "state_workspace.hpp"

namespace spire {
// The original developer console is included in every build. A failed command preserves the
// complete State, including RNG words.
// Kill-all also cancels a suspended combat choice. A single-target kill is
// accepted only at an idle player-action boundary.
bool debug_kill_enemies(State&,int target=-1);
// Replaces the room at the current map coordinate without inventing a route
// or another floor. None selects the canonical next encounter/event. An
// explicit encounter may be from any act. Dead runs revive at their max HP.
bool debug_enter_room(State&,WorldRoom,EncounterId=EncounterId::None,
                      OriginalEventId=OriginalEventId::None,RelicId forced_ancient_offer=NoWorldRelic);
RelicId debug_ancient_offer(OriginalEventId,const char* partial_key);
// Zero-based act. Regenerates that act and discards its later route history;
// earlier visited acts, deck facts, inventory and named RNG streams survive.
bool debug_enter_act(State&,unsigned act);
bool debug_replace_act(State&,reference::ActKind);
enum class DebugPile:uint8_t {Hand,Draw,Discard,Exhaust,Deck};
enum class DebugValue:uint8_t {Heal,Gold,Energy,Stars,Draw,Block,Damage,Die};
bool debug_card(State&,CardKind,DebugPile,bool remove=false);
bool debug_upgrade(State&,unsigned hand_index);
bool debug_value(State&,DebugValue,int amount,int target=0);
const char* debug_power_key(unsigned);
int debug_power_id(const char*,int target);
bool debug_power(State&,unsigned id,int amount,int target);
bool debug_enchant(State&,EventEnchantment,int amount,unsigned hand_index);
bool debug_afflict(State&,EnemyAffliction,int amount,unsigned hand_index);
bool debug_relic(State&,RelicId,bool remove);
bool debug_potion(State&,Potion);
bool debug_set_godmode(State&,bool);
bool debug_godmode(const State&);
void debug_combat_setup(State&);
void debug_reset_session(State&);
bool debug_travel(const State&);
void debug_set_travel(State&,bool);
bool debug_travel_to(State&,unsigned point);

namespace debug_detail {
// Shared by the two implementation translation units, never by gameplay.
bool begin(State&);
bool finish(State&,bool success);
void abandon_room(State&);
bool transaction_active();
}
} // namespace spire
