#pragma once
#include <stdint.h>

namespace spire {

// Public original animator metadata. The directory images remain in the private
// NSP2/14 pack. Model ids are EnemyId values; state ids are local to that model.
// A missing trigger/clip returns -1 and means hold the current animation.
using WorldEnemyFactReader = bool (*)(void* context,const char* name,int& value);
struct WorldEnemyVisualModel {
    const char* name;
    uint16_t state_first,state_count,transition_first,transition_count;
    uint16_t move_first,move_count,skin_first,skin_count,clip_first,clip_count;
    int16_t initial_state;
    uint8_t shared_group,shared_side; // 0=ordinary; 1=KaiserCrab, side 1/2.
};
struct WorldEnemyVisualState {
    const char* clip;
    const char* bounds;
    int16_t next;
    uint8_t loop;
};
struct WorldEnemyVisualTransition {
    const char* trigger;
    int8_t from,to; // from -1 is the original any-state transition; generator bounds both ids.
    uint16_t condition;
};
struct WorldEnemyVisualAction {
    const char* name;
    uint8_t direct; // 0=trigger; 1=literal original clip, including shared tracks.
};
struct WorldEnemyVisualMove {
    const char* key;
    const char* next_clip; // Non-null for direct shared-arm actions.
    int16_t node;
    uint16_t action_first;
    uint8_t action_count;
};
struct WorldEnemyVisualSkin {
    const char* key;
    uint16_t weight;
    uint8_t slot_mask; // Named-slot skins; zero means unrestricted.
};
struct WorldEnemyVisualClip {
    const char* native_clip;
    const char* key; // Full Content directory key, without any localization.
    uint16_t directory,condition;
    int16_t move; // -1=ordinary context; exact matching move takes precedence.
    uint16_t duration_ms,original_ms; // Exact source milliseconds; generator rejects overflow.
    uint8_t skin,loop;
};

const WorldEnemyVisualModel* world_enemy_visual(unsigned model);
const WorldEnemyVisualState* world_enemy_state(unsigned model,int state);
const WorldEnemyVisualMove* world_enemy_move(unsigned model,int node);
const WorldEnemyVisualAction* world_enemy_action(const WorldEnemyVisualMove&,unsigned index);
const WorldEnemyVisualSkin* world_enemy_skin(unsigned model,unsigned skin);
const WorldEnemyVisualClip* world_enemy_clip_row(unsigned model,unsigned index);
const WorldEnemyVisualClip* world_enemy_clip_directory(unsigned directory);
const WorldEnemyVisualTransition* world_enemy_transition_row(unsigned model,unsigned index);
bool world_enemy_condition(unsigned condition,WorldEnemyFactReader,void* context);
int world_enemy_initial(unsigned model,WorldEnemyFactReader=nullptr,void* context=nullptr);
int world_enemy_resume(unsigned model,WorldEnemyFactReader=nullptr,void* context=nullptr);
int world_enemy_transition(unsigned model,int state,const char* trigger,
                           WorldEnemyFactReader=nullptr,void* context=nullptr);
int world_enemy_resolve_clip(unsigned model,const char* native_clip,unsigned skin=0,int move=-1,
                            WorldEnemyFactReader=nullptr,void* context=nullptr);
// Names used by original predicates and the finite baked phase contexts. A UI
// can snapshot this list before processing queued actions. Unknown facts do not
// pass a predicate; they never silently read as zero.
unsigned world_enemy_fact_count();
const char* world_enemy_fact_name(unsigned index);

} // namespace spire
