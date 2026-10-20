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
bool world_reachable(const State&, unsigned point);
bool world_choose_point(State&, unsigned point);
bool world_choose_column(State&, int column);
bool world_finish_room(State&);
bool world_claim_reward_card(State&, int index);
bool world_claim_reward_relic(State&);
// Bit 1 is Pael's Wing's SACRIFICE. The predicate only exposes true
// CardReward screens; generated-card offers and event CardSelect are excluded.
unsigned world_card_reward_alternatives(const State&);
bool world_take_card_reward_alternative(State&,unsigned alternative_bit);
bool world_valid_run(const State&);
bool world_room_is_combat(const State&);
unsigned world_available_points(const State&, uint8_t* out, unsigned capacity);
const char* world_region_key(const State&);
bool world_refresh_map_modifiers(State&);
int world_shop_price(const State&,int item);
void world_resume_room(State&);
// Reward additions retain their own RNG draw and claim, including fixed gold.
// The combat result reports whether any additional card reward was generated.
bool world_relic_combat_rewards(State&);
void world_relic_rest_rewards(State&);
unsigned world_rest_option_count(const State&);
const char* world_rest_option_key(const State&,unsigned index);
bool world_rest_option_enabled(const State&,unsigned index);
bool world_rest_choose(State&,unsigned index);
bool world_rest_cancel(State&);
// Cook accepts either a cancelled selection or exactly two cards.
bool world_rest_selection_allowed(const State&,unsigned count);
bool world_relic_rest_select(State&,bool cook);
void world_relic_gain_strength(State&,int amount);
void world_relic_gain_dexterity(State&,int amount);
} // namespace spire
