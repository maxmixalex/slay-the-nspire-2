#include "world_card_factory.hpp"
#include "run_options.hpp"
#include "inplace_reset.hpp"
namespace spire {
namespace {
int compare_keys(const char*a,const char*b){
 while(*a&&static_cast<unsigned char>(*a)==static_cast<unsigned char>(*b)){++a;++b;}
 return int(static_cast<unsigned char>(*a))-int(static_cast<unsigned char>(*b));
}
#include "world_card_metadata.inc"
#include "world_potion_metadata.inc"
using reference::math::Real;
bool at_most(Real a,Real b){return !(b<a);}
bool accepts(const State&s,const WorldCardOptions&o,CardKind k,bool combat){
 if(k>=CardKinds)return false;
 if(o.rarity_mask&&!(o.rarity_mask&world_rarity_mask(card_rarity(k))))return false;
 if(o.type_mask&&!(o.type_mask&world_type_mask(definitions[k].type)))return false;
 if(o.zero_energy&&cost({k,0})!=0)return false;
 if(combat&&(!world_card_combat_allowed(k)||card_rarity(k)==1||card_rarity(k)==5||card_rarity(k)==6))return false;
 return !o.predicate||o.predicate(s,k,o.predicate_data);
}
unsigned next_rarity(unsigned rarity){return rarity==4?2:rarity>=2&&rarity<4?rarity+1:0;}
unsigned roll_rarity(State&s,const WorldCardOptions&o){
 bool scarce=world_ascension(s)>=7;
 Real rare=world_float(scarce?0x3c741f21U:0x3cf5c28fU),uncommon=world_float(0x3ebd70a4U);
 if(o.odds==WorldCardOdds::EliteEncounter){rare=world_float(scarce?0x3d4ccccdU:0x3dcccccdU);uncommon=world_float(0x3ecccccdU);}
 else if(o.odds==WorldCardOdds::BossEncounter){rare=Real::integer(1);uncommon=Real::integer(0);}
 else if(o.odds==WorldCardOdds::Shop)rare=world_float(scarce?0x3d3851ecU:0x3db851ecU);
 bool grow=(o.flags&WorldForceRarityOddsChange)||(o.source==WorldCardSource::Encounter&&o.odds>=WorldCardOdds::RegularEncounter&&o.odds<=WorldCardOdds::BossEncounter);
 if(!s.world_factory.initialized){s.world_factory.initialized=1;s.world_factory.rarity_offset_bits=0xbd4ccccdU;}
 if(grow&&o.odds!=WorldCardOdds::BossEncounter)rare=world_float_round(rare+world_float(s.world_factory.rarity_offset_bits));
 Real value=world_random_float(world_rng(s,WorldRng::Rewards));
 unsigned rarity=value<rare?4:value<world_float_round(rare+uncommon)?3:2;
 if(grow){
  Real offset=rarity==4?world_float(0xbd4ccccdU):world_float_round(world_float(s.world_factory.rarity_offset_bits)+world_float(scarce?0x3ba3d70aU:0x3c23d70aU));
  if(world_float(0x3ecccccdU)<offset)offset=world_float(0x3ecccccdU);
  s.world_factory.rarity_offset_bits=world_float_bits(offset);
 }
 return rarity;
}
void default_facts(Card c,EventCardFacts&f){clear_zero_object(f);if(c.kind==EventCardSpoilsMap)f.progress=1;}
}
Real world_float(uint32_t bits){
 uint64_t sign=uint64_t(bits>>31)<<63;unsigned e=(bits>>23)&255;uint32_t m=bits&0x7fffff;
 if(!e){if(!m)return Real::raw(sign);int exponent=-126;while(!(m&0x800000)){m<<=1;--exponent;}return Real::raw(sign|(uint64_t(exponent+1023)<<52)|(uint64_t(m&0x7fffff)<<29));}
 if(e==255)return Real::raw(sign|UINT64_C(0x7ff0000000000000)|(uint64_t(m)<<29));
 return Real::raw(sign|(uint64_t(e+896)<<52)|(uint64_t(m)<<29));
}
Real world_float_round(Real x){
 // All gameplay float values lie in the normal binary32 range; preserve zero.
 if(!(x.bits&UINT64_C(0x7fffffffffffffff)))return x;
 uint64_t lower=x.bits&UINT64_C(0x1fffffff),rounded=x.bits&~UINT64_C(0x1fffffff);
 if(lower>UINT64_C(0x10000000)||(lower==UINT64_C(0x10000000)&&(rounded&UINT64_C(0x20000000))))rounded+=UINT64_C(0x20000000);
 return Real::raw(rounded);
}
uint32_t world_float_bits(Real value){
 value=world_float_round(value);uint32_t sign=uint32_t(value.bits>>32)&0x80000000U;
 if(!(value.bits&UINT64_C(0x7fffffffffffffff)))return sign;
 int e=int((value.bits>>52)&2047)-896;
 return sign|(uint32_t(e)<<23)|uint32_t((value.bits&UINT64_C(0xfffffffffffff))>>29);
}
Real world_random_float(reference::Random&rng){return world_float_round(Real::fraction53(rng.next()>>11));}
Real world_random_float(reference::Random&rng,int lo,int hi){return world_float_round(Real::fraction53(rng.next()>>11)*Real::integer(hi-lo)+Real::integer(lo));}
int world_float_truncate(Real x){
 bool negative=(x.bits>>63)!=0;int e=int((x.bits>>52)&2047)-1023;
 if(e<0)return 0;if(e>30)return negative?-2147483647:2147483647;
 uint64_t m=(x.bits&UINT64_C(0xfffffffffffff))|UINT64_C(0x10000000000000);
 int n=int(m>>(52-e));return negative?-n:n;
}
CardKind world_card_id(const char*key){if(key)for(unsigned k=0;k<CardKinds;++k)if(!compare_keys(key,definitions[k].key))return CardKind(k);return CardKinds;}
WorldCardPool world_card_pool(CardKind k){return k<CardKinds?WorldCardPool(world_model_pool[k]):WorldCardPool::Event;}
unsigned world_card_pool_models(WorldCardPool p,CardKind*out,unsigned capacity){
 if(unsigned(p)>=11)return 0;const auto&v=world_pools[unsigned(p)];unsigned n=v.count<capacity?v.count:capacity;for(unsigned i=0;i<n;++i)out[i]=v.ids[i];return n;
}
bool world_card_modifier_allowed(CardKind k){return k<CardKinds&&(world_model_policy[k]&2);}
bool world_card_combat_allowed(CardKind k){return k<CardKinds&&(world_model_policy[k]&1);}
bool world_card_gains_block(CardKind k){return k<CardKinds&&(world_model_policy[k]&4);}
void world_stable_cards(CardKind*cards,unsigned count){for(unsigned i=1;i<count;++i){CardKind c=cards[i];unsigned j=i;while(j&&compare_keys(definitions[cards[j-1]].key,definitions[c].key)>0){cards[j]=cards[j-1];--j;}cards[j]=c;}}
unsigned world_card_candidates(const State&s,const WorldCardOptions&o,CardKind*out,unsigned capacity,bool combat){
 unsigned n=0;bool seen[CardKinds]{};
 for(unsigned i=0;i<o.pool_count&&i<11;++i){unsigned p=o.pools[i]==WorldCardPool::Own?unsigned(s.character):unsigned(o.pools[i]);if(p>=11)continue;
  const auto&pool=world_pools[p];for(unsigned j=0;j<pool.count;++j){CardKind k=pool.ids[j];if(seen[k]||!accepts(s,o,k,combat))continue;seen[k]=true;if(n<capacity)out[n++]=k;}}
 return n;
}
void world_modifier_card_pools(const State&s,WorldCardOptions&o,bool merchant){
 const uint8_t mask=run_modifier_character_mask(s);if(!mask)return;
 for(unsigned i=0;i<run_modifier_count(s.run_options);++i){
  RunModifier m=run_modifier_at(s.run_options,i);Character c;
  switch(m){case RunModifier::IroncladCards:c=Character::Ironclad;break;case RunModifier::SilentCards:c=Character::Silent;break;
   case RunModifier::RegentCards:c=Character::Regent;break;case RunModifier::NecrobinderCards:c=Character::Necrobinder;break;case RunModifier::DefectCards:c=Character::Defect;break;default:continue;}
  // Merchant hooks Concat even the player's own pool, then stop extending
  // after a foreign pool has been introduced. Reward hooks use Union.
  bool present=false;for(unsigned j=0;j<o.pool_count;++j){unsigned p=o.pools[j]==WorldCardPool::Own?unsigned(s.character):unsigned(o.pools[j]);if(merchant&&p!=unsigned(s.character))return;if(p==unsigned(c))present=true;}
  if((merchant||!present)&&o.pool_count<11)o.pools[o.pool_count++]=WorldCardPool(unsigned(c));
 }
}
unsigned world_merchant_card_candidates(const State&s,const WorldCardOptions&base,const CardKind*blacklist,unsigned blacklisted,CardKind*out,unsigned capacity){
 WorldCardOptions options=base;world_modifier_card_pools(s,options,true);
 bool seen[CardKinds]{};unsigned count=0;
 for(unsigned i=0;i<options.pool_count&&i<11;++i){
  const unsigned p=options.pools[i]==WorldCardPool::Own?unsigned(s.character):unsigned(options.pools[i]);if(p>=11)continue;
  const auto&pool=world_pools[p];
  for(unsigned j=0;j<pool.count;++j){const CardKind card=pool.ids[j];
   if(i<base.pool_count){
    // MerchantCardEntry.Except runs BEFORE ModifyMerchantCardPool. A later
    // Concat may restore an already-stocked card and preserve its weighting.
    bool excluded=seen[card];seen[card]=true;
    for(unsigned k=0;k<blacklisted;++k)if(blacklist[k]==card)excluded=true;
    if(excluded)continue;
   }
   if(!accepts(s,options,card,false))continue;
   if(count>=capacity||!out)return 0;out[count++]=card;
  }
 }
 return count;
}
static void modifier_card_creation(const State&s,WorldCardOptions&o){
 if(!(o.flags&WorldIsCardReward)||(o.flags&WorldNoCardPoolModifications))return;
 world_modifier_card_pools(s,o);
 if(has_run_modifier(s,RunModifier::BigGameHunter)&&o.source==WorldCardSource::Encounter&&o.odds==WorldCardOdds::EliteEncounter&&!(o.flags&WorldNoRarityModification)){
  o.odds=WorldCardOdds::Uniform;o.rarity_mask=world_rarity_mask(4);
  CardKind candidates[CardKinds];if(!world_card_candidates(s,o,candidates,CardKinds)){o.pool_count=1;o.pools[0]=WorldCardPool::Own;}
 }
}
bool world_create_reward(State&s,reference::Random&rng,unsigned count,const WorldCardOptions&original,Card*out,EventCardFacts*facts,unsigned capacity,unsigned*result_count){
 if(!capacity)capacity=count;if(!out||!facts||count>CardKinds||count>capacity)return false;
 for(unsigned i=0;i<count;++i){
  WorldCardOptions o=original;world_relic_modify_card_creation(s,o);modifier_card_creation(s,o);
  CardKind pool[CardKinds];unsigned n=world_card_candidates(s,o,pool,CardKinds);
  unsigned used=0;for(unsigned j=0;j<n;++j){bool duplicate=false;for(unsigned q=0;q<i;++q)if(out[q].kind==pool[j])duplicate=true;if(!duplicate)pool[used++]=pool[j];}n=used;
  if(o.odds==WorldCardOdds::Uniform){used=0;for(unsigned j=0;j<n;++j)if(card_rarity(pool[j])!=1&&card_rarity(pool[j])!=5)pool[used++]=pool[j];n=used;}
  else {unsigned rarity=roll_rarity(s,o),first=rarity;bool found=false;
   do{for(unsigned j=0;j<n;++j)if(unsigned(card_rarity(pool[j]))==rarity)found=true;if(!found)rarity=next_rarity(rarity);}while(!found&&rarity&&rarity!=first);
   if(!found)return false;used=0;for(unsigned j=0;j<n;++j)if(unsigned(card_rarity(pool[j]))==rarity)pool[used++]=pool[j];n=used;
  }
  if(!n)return false;out[i]={pool[rng.below(n)],0};default_facts(out[i],facts[i]);
  if(!(original.flags&WorldNoUpgradeRoll)){
   Real roll=world_random_float(rng); // Even an unupgradable card consumes this draw.
   if(card_upgradeable(out[i])){int odds=card_rarity(out[i].kind)==4?0:int(world_act_index(s))*(world_ascension(s)>=7?1250:2500);odds=world_relic_modify_upgrade_odds(s,out[i],odds);
    if(at_most(roll,Real::integer(odds)/Real::integer(10000)))out[i].upgraded=1;}
  }
 }
 if(!(original.flags&WorldNoModifyHooks))world_relic_modify_reward_options(s,out,facts,count,capacity,original);
 if(result_count)*result_count=count;return s.phase!=Phase::Error&&count<=capacity;
}
unsigned world_combat_cards(State&s,reference::Random&rng,unsigned count,const WorldCardOptions&o,bool distinct,Card*out,EventCardFacts*facts){
 CardKind pool[CardKinds];unsigned n=world_card_candidates(s,o,pool,CardKinds,true);if(!n)return 0;
 if(distinct){reference::shuffle(pool,n,rng);if(count>n)count=n;}
 for(unsigned i=0;i<count;++i){out[i]={distinct?pool[i]:pool[rng.below(n)],0};default_facts(out[i],facts[i]);}return count;
}
bool world_transform_card(State&s,reference::Random&rng,Card original,bool combat,Card&out,EventCardFacts&facts,CardKind replacement){
 if(original.kind>=CardKinds)return false;
 if(replacement<CardKinds){out={replacement,0};default_facts(out,facts);return true;}
 WorldCardOptions o;unsigned rarity=card_rarity(original.kind);
 o.pools[0]=definitions[original.kind].type==CardType::Quest||rarity==5||rarity==6||rarity==7?WorldCardPool::Colorless:world_card_pool(original.kind);
 if(rarity!=8&&rarity!=9)o.rarity_mask=world_rarity_mask(2)|world_rarity_mask(3)|world_rarity_mask(4);
 CardKind pool[CardKinds];unsigned n=world_card_candidates(s,o,pool,CardKinds),used=0;
 for(unsigned i=0;i<n;++i)if(pool[i]!=original.kind&&(!combat||world_card_combat_allowed(pool[i])))pool[used++]=pool[i];
 if(!used)return false;out={pool[rng.below(used)],0};default_facts(out,facts);return true;
}
unsigned world_random_potions(const State&s,reference::Random&rng,unsigned count,Potion*out,unsigned rarity,bool combat,const Potion*blacklist,unsigned blacklisted){
 Potion pool[PotionKinds];unsigned n=0;
 for(unsigned part=0;part<2;++part){const auto&v=world_potion_pools[part?5:unsigned(s.character)];for(unsigned i=0;i<v.count;++i){Potion p=Potion(v.ids[i]);bool skip=false;
   for(unsigned j=0;j<blacklisted;++j)if(blacklist[j]==p)skip=true;
   if(combat&&(p==Potion::FairyInABottle||p==Potion::Fruit||p==Potion::RegenPotion))skip=true;
   if(!skip)pool[n++]=p;}}
 unsigned made=0;while(made<count&&n){unsigned selected=rarity;
  if(!selected){Real roll=world_random_float(rng);selected=at_most(roll,world_float(0x3dcccccdU))?3:at_most(roll,world_float(0x3eb33333U))?2:1;}
  unsigned choices[PotionKinds],m=0;for(unsigned i=0;i<n;++i)if(selected==WorldPotionUniform||potion_definition(pool[i])->rarity==selected)choices[m++]=i;
  if(!m)break;unsigned index=choices[rng.below(m)];out[made++]=pool[index];for(unsigned i=index+1;i<n;++i)pool[i-1]=pool[i];--n;
 }return made;
}
} // namespace spire
