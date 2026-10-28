#include "core.hpp"
#include "world_rng.hpp"
namespace spire {
static const char* const world_stream_names[WorldRngCount]={
    "up_front","shuffle","unknown_map_point","combat_card_generation",
    "combat_potion_generation","combat_card_selection","combat_energy_costs",
    "combat_targets","monster_ai","niche","combat_orbs","treasure_room_relics",
    "rewards","shops","transformations"
};
static void world_init_streams(State&s,uint64_t seed){
    s.world.run_seed=seed;
    for(unsigned i=0;i<WorldRngCount;++i)s.world.rng[i].named(seed,world_stream_names[i]);
    s.world.rng_ready=1;
}
bool world_set_seed(State&s,const char*seed){
    char canonical[65]{};
    if(!reference::canonicalize_ascii(seed,canonical,sizeof canonical))return false;
    unsigned length=0;while(canonical[length])++length;
    for(unsigned i=0;i<sizeof canonical;++i)s.world.seed_text[i]=canonical[i];
    world_init_streams(s,reference::hash64(canonical,length));return true;
}
reference::Random& world_rng(State&s,WorldRng stream){
    // Standalone card/encounter fixtures may run without a world map. Their
    // named streams are deterministic without mutating either legacy stream.
    if(!s.world.rng_ready)world_init_streams(s,s.seed);
    unsigned i=unsigned(stream);
    if(i>=WorldRngCount){s.phase=Phase::Error;i=unsigned(WorldRng::Niche);}
    return s.world.rng[i];
}
uint64_t world_run_seed(const State&s){return s.world.rng_ready?s.world.run_seed:s.seed;}
unsigned world_act_index(const State&s){return s.world.act_index;}
unsigned world_total_floor(const State&s){return s.world.active?s.world.total_floor:s.floor;}
unsigned world_ascension(const State&s){return s.world.ascension;}
int world_map_column(const State&s){return s.world.map_column;}
int world_map_row(const State&s){return s.world.map_row;}
unsigned world_room_type(const State&s){
    if(s.world.active)return s.world.room_type;
    if(combat_phase(s.phase))return s.encounter?2:1;
    return s.phase==Phase::Shop?5:s.phase==Phase::Camp?7:s.phase==Phase::Treasure?4:s.phase==Phase::Event?6:0;
}
const reference::ReferenceMap* world_current_map(const State&s){
    return s.world.active&&s.world.act_index<3&&s.world.map_ready[s.world.act_index]?&s.world.maps[s.world.act_index]:nullptr;
}
} // namespace spire
