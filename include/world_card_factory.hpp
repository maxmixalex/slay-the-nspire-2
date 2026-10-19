#pragma once
#include "core.hpp"
#include "reference_math.hpp"
#include "world_rng.hpp"
namespace spire {
enum class WorldCardPool:uint8_t { Ironclad,Silent,Defect,Regent,Necrobinder,Colorless,Curse,Status,Event,Token,Quest,Own };
enum class WorldCardOdds:uint8_t { None,RegularEncounter,EliteEncounter,BossEncounter,Shop,Uniform };
enum class WorldCardSource:uint8_t { None,Encounter,Shop,Other };
enum WorldCardFlags:uint16_t {
 WorldNoRarityModification=1,WorldNoUpgradeRoll=2,WorldNoHookUpgrades=4,
 WorldNoModifyHooks=8,WorldNoCardPoolModifications=16,WorldNoCardModelModifications=32,
 WorldForceRarityOddsChange=64,WorldIsCardReward=128,WorldIsFromCombat=256,
 WorldNoUpgrades=6,WorldNoModifications=65535
};
constexpr unsigned WorldPotionUniform=4;
struct WorldCardOptions {
 WorldCardPool pools[11]{WorldCardPool::Own};
 uint8_t pool_count=1;
 uint16_t rarity_mask=0; // Zero means all original pool rarities.
 uint8_t type_mask=0,zero_energy=0;
 WorldCardOdds odds=WorldCardOdds::RegularEncounter;
 WorldCardSource source=WorldCardSource::Other;
 uint16_t flags=WorldNoUpgradeRoll;
 bool (*predicate)(const State&,CardKind,const void*)=nullptr;
 const void* predicate_data=nullptr;
};
constexpr uint16_t world_rarity_mask(unsigned rarity){return uint16_t(1U<<rarity);}
constexpr uint8_t world_type_mask(CardType t){return uint8_t(1U<<unsigned(t));}
CardKind world_card_id(const char* key); // CardKinds is the absent sentinel.
WorldCardPool world_card_pool(CardKind);
unsigned world_card_pool_models(WorldCardPool,CardKind*,unsigned capacity);
unsigned world_card_candidates(const State&,const WorldCardOptions&,CardKind*,unsigned capacity,bool combat=false);
bool world_card_modifier_allowed(CardKind);
bool world_card_combat_allowed(CardKind);
bool world_card_gains_block(CardKind);
bool world_create_reward(State&,reference::Random& selection_rng,unsigned count,const WorldCardOptions&,Card*,EventCardFacts*,unsigned capacity=0,unsigned* result_count=nullptr);
unsigned world_combat_cards(State&,reference::Random&,unsigned count,const WorldCardOptions&,bool distinct,Card*,EventCardFacts*);
bool world_transform_card(State&,reference::Random&,Card original,bool combat,Card& result,EventCardFacts&,CardKind replacement=CardKinds);
unsigned world_random_potions(const State&,reference::Random&,unsigned count,Potion*,unsigned rarity=0,bool combat=false,const Potion* blacklist=nullptr,unsigned blacklisted=0);
void world_stable_cards(CardKind*,unsigned count);
// Exact original NextFloat conversion, represented as software binary64 holding
// a binary32 value. Callers need no floating point runtime on ARMv5.
reference::math::Real world_float(uint32_t ieee32);
reference::math::Real world_float_round(reference::math::Real);
reference::math::Real world_random_float(reference::Random&);
reference::math::Real world_random_float(reference::Random&,int lo,int hi);
int world_float_truncate(reference::math::Real);
uint32_t world_float_bits(reference::math::Real);
// Relic-owned phases; the initial options hook runs separately for each card.
void world_relic_modify_card_creation(State&,WorldCardOptions&);
void world_relic_modify_reward_options(State&,Card*,EventCardFacts*,unsigned& count,unsigned capacity,const WorldCardOptions&);
int world_relic_modify_upgrade_odds(State&,Card,int basis_points);
int world_relic_modify_card_reward_count(State&,int,const WorldCardOptions&);
} // namespace spire
