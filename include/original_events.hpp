#pragma once
#include "core.hpp"

namespace spire {
const char* original_event_key(OriginalEventId);
OriginalEventId original_event_id(const char* key);
// A registered name is not an implemented event. Unknown bodies fail before
// mutating the run; the complete model list is retained for coverage audits.
bool original_event_implemented(OriginalEventId);
bool original_event_allowed(const State&,OriginalEventId,unsigned act_index,unsigned total_floor);
bool original_event_begin(State&,OriginalEventId,uint64_t run_seed,unsigned act_index);
unsigned original_event_option_count(const State&);
bool original_event_option_enabled(const State&,int option);
bool original_event_choose(State&,int option);
void original_event_resume(State&);
bool original_event_select(State&,const uint16_t* indices,unsigned count);
unsigned original_event_eligible(const State&,uint16_t* out,unsigned capacity);
bool original_event_potion(State&,int slot); // -1 declines; occupied slot replaces.
bool original_event_reward(State&,int index); // -1 leaves the entire reward set.
bool original_event_leave(State&);
void original_event_after_combat(State&,bool won);
bool original_event_crystal_tool(State&,bool large);
bool original_event_crystal_click(State&,unsigned x,unsigned y);
bool original_event_text_key(const State&,int option,bool description,char* out,unsigned capacity);
bool original_event_description_value(const State&,const char* key,int& result);
void original_event_new_run(State&);
void original_event_finish_run(State&);
bool original_event_blocks_potions(const State&);
bool original_event_foul_potion(State&);
bool valid_original_event_state(const State&);
} // namespace spire
