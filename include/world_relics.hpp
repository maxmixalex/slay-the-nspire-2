#pragma once
#include <stdint.h>
namespace spire {
struct State;
struct Card;
struct EventCardFacts;
struct OriginalTextReference;
using RelicId=uint16_t;
constexpr RelicId NoWorldRelic=65535;
constexpr unsigned WorldRelicCapacity=320;
enum WorldRelicModel:RelicId {
#include "world_relic_ids.inc"
 WorldRelicKinds
};
static_assert(WorldRelicKinds<=WorldRelicCapacity,"Relic model registry exceeds bounded storage");
enum class WorldRelicPool:uint8_t { Reward,Starter,Common,Uncommon,Rare,Shop,Event,Ancient };
enum class WorldRelicChoice:uint8_t { None,Remove,Upgrade,Transform,Duplicate,Enchant,OfferCard,OfferBundle,OfferReward,Potion,Hand,CombatOffer,Count };
enum class WorldRelicOp:uint8_t { None,Obtain,Damage,Heal,MaxHp,Gold,AddCard,Select,CardReward,PotionReward,RelicReward,RandomUpgrade,TransformBasic,RemoveKind,EnchantAll,Finish,RewardGroup,Special,RestoreCard,ApplyChoice,ProcurePotion,DiscardPotion,TransformAppend,Count };
enum class WorldRelicHook:uint8_t {
 BeforeCombatStart,BeforeCombatStartLate,BeforeSideTurnStart,AfterSideTurnStart,
 BeforeHandDraw,AfterHandDraw,AfterHandDrawLate,BeforeSideTurnEndVeryEarly,
 BeforeSideTurnEndEarly,BeforeSideTurnEnd,AfterSideTurnEnd,AfterCombatVictoryEarly,
 AfterCombatVictory,AfterCombatEnd,BeforeCardPlayed,AfterCardPlayed,
 AfterCardExhausted,AfterCardDiscarded,AfterCardGenerated,AfterCardDrawn,
 AfterCardChangedPiles,AfterHandEmptied,AfterShuffle,AfterAttack,AfterBlock,
 AfterBlockCleared,AfterBlockBroken,AfterHpChanged,AfterHpLost,AfterEnemyDeath,
 AfterEnemyAdded,AfterOrbChanneled,AfterStarsSpent,AfterPotionUsed,
 AfterPotionProcured,AfterPotionDiscarded,AfterRoomEntered,AfterRest,
 AfterGoldGained,AfterItemPurchased,AfterDeckAdded,AfterDeckRemoved,
 AfterEnergyReset,AfterEnergyResetLate,AfterFlush,AfterDiedToDoom,
 AfterAutoPrePlay,AfterAutoPrePlayLate,AfterCardEnteredCombat,Internal,Acquisition,Count
};
enum WorldRelicFlags:uint8_t { WorldRelicWax=1,WorldRelicMelted=2,WorldRelicDisabled=4 };
struct WorldRelicDefinition {const char* key;const char* model;uint8_t rarity,character_mask,flags;};
extern const WorldRelicDefinition world_relic_definitions[WorldRelicKinds];
RelicId world_relic_id(const char* key);
const char* world_relic_key(RelicId);
bool has_world_relic(const State&,RelicId);
bool gain_world_relic(State&,RelicId);
bool lose_world_relic(State&,RelicId);
bool world_relic_available(const State&,RelicId,bool in_shop=false);
bool world_relic_tradable(const State&,RelicId);
unsigned world_relic_list(const State&,RelicId* out,unsigned capacity,bool tradable_only=false);
unsigned world_relic_candidates(const State&,RelicId* out,unsigned capacity,WorldRelicPool,bool in_shop=false);
RelicId random_world_relic(State&,WorldRelicPool pool=WorldRelicPool::Reward);
RelicId world_relic_pull(State&,WorldRelicPool,bool from_back=false,bool in_shop=false);
RelicId world_relic_pull_shared(State&,WorldRelicPool);
bool world_relic_pending(const State&);
bool world_relic_select(State&,const uint16_t*,unsigned count);
bool world_relic_potion(State&,int slot); // -1 returns to the containing reward set.
bool world_relic_can_reroll_reward(const State&);
bool world_relic_reroll_reward(State&);
unsigned world_relic_eligible(const State&,uint16_t* out,unsigned capacity);
void init_world_relics(State&);
void world_relic_hook(State&,WorldRelicHook,int a=0,int b=0,int c=0);
void world_relic_effect(State&,int frame_index);
void world_relic_resume(State&);
// A saved combat continuation yields to every callback emitted by one pickup
// step before the next pickup step can execute.
void world_relic_schedule_acquisition(State&);
void world_relic_acquisition_resume(State&);
bool world_relic_replace(State&,unsigned serial,RelicId);
int world_relic_counter(const State&,RelicId);
// Per owned instance, including duplicates. Returns the stable original counter
// visibility/value; activation-flash animation is separate presentation state.
bool world_relic_display(const State&,unsigned position,int& amount);
// Original disabled presentation is not a blanket listener-disable flag.
bool world_relic_disabled(const State&,unsigned position);
bool world_relic_description_value(const State&,RelicId,const char* key,int& amount,int position=-1);
bool world_relic_description_reference(const State&,RelicId,const char* key,OriginalTextReference&,int position=-1,unsigned item=0);
unsigned world_relic_description_reference_count(const State&,RelicId,const char* key,int position=-1);
bool valid_world_relic_state(const State&);
bool world_relic_golden_path(const State&,unsigned act);
void world_relic_map_generated(State&);
bool world_relic_fur_room(const State&);
// Prepared ancient offerings keep their random choice before acquisition.
bool world_relic_prepare(State&,RelicId,int value=-1,int second=-1);
bool world_relic_setup_offer(State&,RelicId,int value=-1);
void world_relic_clear_offers(State&);
bool world_relic_allowed_at_neow(const State&,RelicId);
bool world_has_event_pet(const State&);
bool world_relic_modify_deck_card(State&,Card&,EventCardFacts&);
void world_relic_modify_merchant_cards(State&,Card*,EventCardFacts*,unsigned);
void world_relic_before_reward_offered(State&,bool has_card_rewards);
int world_relic_prepared_value(const State&,RelicId,unsigned which=0);
// Ordered acquisition internals shared by the core continuation implementation.
int world_relic_instance_index(const State&,unsigned serial);
void world_relic_flash(State&,RelicId);
void world_relic_combat_acquired(State&,unsigned serial);
void world_relic_combat_select(State&,unsigned serial,const uint16_t*,unsigned);
void world_relic_run_hook(State&,unsigned serial,WorldRelicHook,int,int,int);
} // namespace spire
