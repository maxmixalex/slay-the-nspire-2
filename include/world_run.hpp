#pragma once
#include "core.hpp"

namespace spire {
// RoomType values match the pinned run model; the legacy Room enum is a
// separate, frozen save format and must never be cast to these values.
enum class WorldRoom : uint8_t {
    None, Monster, Elite, Boss, Treasure, Shop, Event, Rest, Map
};
enum class WorldRestOption : uint8_t { Heal,Smith,Hatch,Dig,Lift,Kindle,Cook,Clone,Count };

// The caller creates the character/deck first. A seed set with world_set_seed
// is retained; otherwise the run's numeric seed initializes named streams.
// first_region accepts only Overgrowth or Underdocks. Timeline/unlock state is
// outside the supported single-player scope, so all pinned solo pools are open.
bool world_begin_run(State&, reference::ActKind first_region=reference::ActKind::Overgrowth,
                     unsigned ascension=0);
// Route availability is independent of opening the map. An eligible current
// room remains active until an actual destination is submitted.
bool world_can_travel(const State&);
bool world_reachable(const State&, unsigned point);
bool world_choose_point(State&, unsigned point);
bool world_choose_column(State&, int column);
bool world_finish_room(State&);
// An ordinary terminal RewardsSet stays live behind its map until travel.
// Nested event rewards and act-ending boss sets have a real exit instead.
bool world_retained_reward_group(const State&);
bool world_claim_reward_card(State&, int index);
bool world_claim_reward_relic(State&);
bool world_reroll_reward(State&);
// Bit 1 is Pael's Wing's SACRIFICE; bit 2 is Driftwood's REROLL.
// Each reroll belongs to one reward and can only be used once. Exposes true
// CardReward screens; generated-card offers and event CardSelect are excluded.
unsigned world_card_reward_alternatives(const State&);
bool world_take_card_reward_alternative(State&,unsigned alternative_bit);
bool world_valid_run(const State&);
bool world_room_is_combat(const State&);
// Presentation may retain existing power nodes at this original lifecycle
// boundary; the effect still clears the canonical combat models normally.
bool world_effect_clears_combat_powers(Effect);
unsigned world_available_points(const State&, uint8_t* out, unsigned capacity);
const char* world_region_key(const State&);
bool world_refresh_map_modifiers(State&);
int world_shop_price(const State&,int item);
void world_resume_room(State&);
// Pre-existing extra rewards retain their own population draw and claim.
// Fixed-gold rewards appended by a modifier are already populated.
// The combat result reports whether any additional card reward was generated.
bool world_relic_combat_rewards(State&);
void world_relic_rest_rewards(State&);
unsigned world_rest_option_count(const State&);
const char* world_rest_option_key(const State&,unsigned index);
// Slots keep their original identity after a selection. Hidden options were
// consumed or disabled by the completed choice; disabled visible options remain
// in the original room's remaining-options list.
bool world_rest_option_visible(const State&,unsigned index);
bool world_rest_option_enabled(const State&,unsigned index);
bool world_rest_choose(State&,unsigned index);
bool world_rest_cancel(State&);
bool world_rest_can_proceed(const State&);
bool world_rest_proceed(State&);
// Cook accepts either a cancelled selection or exactly two cards.
bool world_rest_selection_allowed(const State&,unsigned count);
bool world_relic_rest_select(State&,bool cook);
void world_relic_gain_strength(State&,int amount);
void world_relic_gain_dexterity(State&,int amount);
} // namespace spire
