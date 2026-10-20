#pragma once
#include "core.hpp"

namespace spire {
// RoomType values match the pinned run model; the legacy Room enum is a
// separate, frozen save format and must never be cast to these values.
enum class WorldRoom : uint8_t {
    None, Monster, Elite, Boss, Treasure, Shop, Event, Rest, Map
};

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
bool world_valid_run(const State&);
bool world_room_is_combat(const State&);
unsigned world_available_points(const State&, uint8_t* out, unsigned capacity);
const char* world_region_key(const State&);
bool world_refresh_map_modifiers(State&);
int world_shop_price(const State&,int item);
void world_resume_room(State&);
} // namespace spire
