#pragma once
#include "core.hpp"

namespace spire {
enum class OriginalTextReferenceKind:uint8_t {None,Localized,Card,Relic,RelicDescription,Potion,Enchantment,EnchantmentDescription};
struct OriginalTextReference {
 OriginalTextReferenceKind kind=OriginalTextReferenceKind::None;
 uint16_t model=0; // Card uses card_token; other models use their public id.
 EventCardFacts facts{};
 const char* table=nullptr;
 char key[128]{};
};
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
bool original_event_has_card_reward(const State&);
// Consume the current CardReward for an alternative. Caller applies that
// alternative, then calls original_event_resume after its own hooks finish.
bool original_event_consume_card_reward(State&);
bool original_event_can_reroll_reward(const State&);
bool original_event_reroll_reward(State&);
bool original_event_leave(State&);
void original_event_after_combat(State&,bool won);
bool original_event_crystal_tool(State&,bool large);
bool original_event_crystal_click(State&,unsigned x,unsigned y);
bool original_event_text_key(const State&,int option,bool description,char* out,unsigned capacity);
bool original_event_description_value(const State&,const char* key,int& result);
// option=-1 addresses the page; description distinguishes dynamic option
// titles (notably potion rarity) from option descriptions (card rarity).
bool original_event_description_reference(const State&,const char* key,int option,bool description,OriginalTextReference&);
void original_event_new_run(State&);
void original_event_finish_run(State&);
bool original_event_blocks_potions(const State&);
bool original_event_can_use_foul(const State&);
// CombatRoom.ExtraRewards descriptors; reading never consumes random draws.
// kind 2 relic (NoWorldRelic = random), 3 potion (0 = random), 5 special card.
unsigned original_event_combat_reward_count(const State&);
bool original_event_combat_reward(const State&,unsigned index,uint8_t& kind,uint16_t& value);
bool original_event_foul_potion(State&);
bool valid_original_event_state(const State&);
} // namespace spire
