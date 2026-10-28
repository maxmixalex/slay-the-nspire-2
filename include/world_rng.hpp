#pragma once
#include "reference_rng.hpp"
#include "reference_act.hpp"
namespace spire {
struct State;
// Original run streams followed by the three single-player slot-zero streams.
enum class WorldRng:uint8_t {
    UpFront,Shuffle,UnknownMapPoint,CombatCardGeneration,CombatPotionGeneration,
    CombatCardSelection,CombatEnergyCosts,CombatTargets,MonsterAi,Niche,
    CombatOrbs,TreasureRoomRelics,Rewards,Shops,Transformations,Count
};
constexpr unsigned WorldRngCount=unsigned(WorldRng::Count);
bool world_set_seed(State&,const char*);
reference::Random& world_rng(State&,WorldRng);
uint64_t world_run_seed(const State&);
unsigned world_act_index(const State&);
unsigned world_total_floor(const State&);
unsigned world_ascension(const State&);
int world_map_column(const State&);
int world_map_row(const State&);
unsigned world_room_type(const State&);
const reference::ReferenceMap* world_current_map(const State&);
bool world_enter_event_combat(State&,const char* encounter_key,bool resume_parent);
} // namespace spire
