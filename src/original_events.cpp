#include "original_events.hpp"
#include "world_card_factory.hpp"

namespace spire {
namespace {
bool same(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
template<unsigned N> bool copy_text(char(&out)[N],const char*text){
 unsigned n=0;if(!text)return false;while(text[n]){if(n+1>=N)return false;++n;}
 for(unsigned i=0;i<N;i++)out[i]=i<n?text[i]:0;return true;
}
bool terminated(const char*text,unsigned capacity){for(unsigned i=0;i<capacity;i++)if(!text[i])return true;return false;}
int compare_text(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return unsigned(*a)-unsigned(*b);}
int smaller(int a,int b){return a<b?a:b;}
int larger(int a,int b){return a>b?a:b;}
struct EventName { const char*model;const char*key; };
const EventName names[]={
#include "original_event_names.inc"
};
static_assert(sizeof(names)/sizeof(names[0])==unsigned(OriginalEventId::Count),"Complete event name registry");
void fail(State&s){s.phase=Phase::Error;record(s,LogKind::Capacity,MaxOriginalEventSteps);}
reference::Random event_random(const State&s){reference::Random rng;
 for(unsigned i=0;i<4;i++)rng.words[i]=s.original_events.rng_words[i];
 rng.calls=s.original_events.rng_calls;rng.good=(rng.words[0]|rng.words[1]|rng.words[2]|rng.words[3])!=0;return rng;
}
void keep_random(State&s,const reference::Random&rng){
 for(unsigned i=0;i<4;i++)s.original_events.rng_words[i]=rng.words[i];s.original_events.rng_calls=rng.calls;
}
bool basic_strike(CardKind k){return k==Strike||k==StrikeSilent||k==DefectStrike||k==RegentStrike||k==NecroStrikeNecrobinder;}
bool basic_defend(CardKind k){return k==Defend||k==DefendSilent||k==DefectDefend||k==RegentDefend||k==NecroDefendNecrobinder;}
bool deck_eligible(const State&s,unsigned index){
 if(index>=s.deck_count)return false;const auto&e=s.original_events;Card card=s.deck[index];
 switch(e.selection){
 case OriginalEventSelection::Remove:if(!card_removable(s,card,int(index)))return false;break;
 case OriginalEventSelection::Upgrade:if(!card_upgradeable(card))return false;break;
 case OriginalEventSelection::Transform:if(!card_transformable(s,card,int(index)))return false;break;
 case OriginalEventSelection::Enchant:if(!event_card_can_enchant(s,int(index),EventEnchantment(e.select_enchantment)))return false;break;
 case OriginalEventSelection::Duplicate:if(s.deck_count>=MaxDeck)return false;break;
 default:break;
 }
 switch(e.filter){
 case OriginalEventFilter::Any:return true;
 case OriginalEventFilter::Removable:return card_removable(s,card,int(index));
 case OriginalEventFilter::Upgradeable:return card_upgradeable(card);
 case OriginalEventFilter::Transformable:return card_transformable(s,card,int(index));
 case OriginalEventFilter::BasicStrike:return basic_strike(card.kind);
 case OriginalEventFilter::BasicDefend:return basic_defend(card.kind);
 case OriginalEventFilter::Basic:return card_rarity(card.kind)==1;
 case OriginalEventFilter::Attack:return card_type(s,card,-1,int(index))==CardType::Attack;
 case OriginalEventFilter::Skill:return card_type(s,card,-1,int(index))==CardType::Skill;
 case OriginalEventFilter::Power:return card_type(s,card,-1,int(index))==CardType::Power;
 case OriginalEventFilter::Curse:return card_type(s,card,-1,int(index))==CardType::Curse;
 case OriginalEventFilter::Enchantment:return s.event_cards.deck[index].enchantment!=EventEnchantment::None;
 case OriginalEventFilter::NotBasic:return !basic_strike(card.kind)&&!basic_defend(card.kind);
 default:return false;
 }
}
void clear_selection(State&s){auto&e=s.original_events;
 e.selection=OriginalEventSelection::None;e.filter=OriginalEventFilter::Any;
 e.select_min=e.select_max=e.select_cancelable=0;e.select_enchantment=0;e.select_amount=e.select_extra=0;
 e.selected_count=e.selected_cursor=0;e.selection_committed=0;
 for(auto&i:e.selected)i=0;for(auto&card:e.selected_cards)card={};
 e.offer_count=0;for(auto&i:e.offers)i=0;for(auto&f:e.offer_facts)f={};
 if(s.phase==Phase::OriginalEventChoice)s.phase=Phase::Event;
}
bool selection_kind(OriginalEventSelection choice){
 switch(choice){case OriginalEventSelection::Remove:case OriginalEventSelection::Upgrade:
 case OriginalEventSelection::Transform:case OriginalEventSelection::Enchant:
 case OriginalEventSelection::Duplicate:case OriginalEventSelection::OfferCard:
 case OriginalEventSelection::Potion:case OriginalEventSelection::Relic:case OriginalEventSelection::Rewards:return true;
 default:return false;}
}
bool pause_selection(State&s,OriginalEventSelection kind,OriginalEventFilter filter,unsigned low,unsigned high,bool cancelable){
 auto&e=s.original_events;if(!selection_kind(kind)||low>high||high>MaxDeck)return false;
 e.selection=kind;e.filter=filter;e.select_min=uint8_t(low);e.select_max=uint8_t(high);e.select_cancelable=uint8_t(cancelable);
 e.selected_count=e.selected_cursor=0;e.selection_committed=0;s.phase=Phase::OriginalEventChoice;return true;
}
bool event_special(State&,int,int,int,int,int);
#include "original_event_rewards.inc"
void apply_selection(State&s){auto&e=s.original_events;
 while(e.selection_committed&&e.selected_cursor<e.selected_count&&!world_relic_pending(s)&&s.phase!=Phase::Dead&&s.phase!=Phase::Error){
  const unsigned cursor=e.selected_cursor++,selected=e.selected[cursor];
  if(e.selection==OriginalEventSelection::OfferCard){
   const auto&snap=e.selected_cards[cursor];if(event_add_deck_card(s,snap.card,&snap.facts)<0){fail(s);return;}
  }else if(e.selection==OriginalEventSelection::Relic){if(!gain_world_relic(s,e.offers[selected])){fail(s);return;}}
  else if(e.selection==OriginalEventSelection::Duplicate){
   if(event_restore_deck_card(s,e.selected_cards[cursor])<0){fail(s);return;}
   world_relic_hook(s,WorldRelicHook::AfterDeckAdded,s.deck_count-1);
  }else{
   unsigned index=selected;
   if(e.selection==OriginalEventSelection::Remove)for(unsigned i=0;i<cursor;i++)if(e.selected[i]<selected)--index;
   if(index>=s.deck_count){fail(s);return;}
   switch(e.selection){
   case OriginalEventSelection::Remove:if(!event_remove_deck_card(s,int(index))){fail(s);return;}break;
   case OriginalEventSelection::Upgrade:s.deck[index].upgraded=1;record(s,LogKind::Upgrade,s.deck[index].kind);break;
   case OriginalEventSelection::Enchant:if(!event_card_enchant(s,int(index),EventEnchantment(e.select_enchantment),e.select_amount)){fail(s);return;}break;
   case OriginalEventSelection::Transform:{
    auto rng=event_random(s);Card replacement{};EventCardFacts facts{};
    CardKind fixed=e.select_extra>0?CardKind(e.select_extra-1):CardKind(CardKinds);
    if(!world_transform_card(s,rng,s.deck[index],false,replacement,facts,fixed)||!event_replace_deck_card(s,int(index),replacement,&facts)){fail(s);return;}
    keep_random(s,rng);break;
   }
   default:fail(s);return;
   }
  }
 }
 if(e.selection_committed&&e.selected_cursor==e.selected_count&&!world_relic_pending(s)&&s.phase!=Phase::Dead&&s.phase!=Phase::Error){
  if(e.reward_active&&e.reward_current<e.reward_count){if(e.selected_count)e.rewards[e.reward_current].claimed=1;e.reward_current=255;}clear_selection(s);
 }
}
bool perform_step(State&s,const OriginalEventStep&step){auto&e=s.original_events;
 switch(step.op){
 case OriginalEventOp::None:return true;
 case OriginalEventOp::Damage:if(step.a<0)return false;player_hp_loss(s,step.a,false,false,-1);return true;
 case OriginalEventOp::Heal:heal(s,step.a<0?s.max_hp:step.a);return true;
 case OriginalEventOp::MaxHp:
  if(step.a>=0)increase_max_hp(s,step.a);else{s.max_hp=int16_t(larger(1,s.max_hp+step.a));s.hp=int16_t(smaller(s.hp,s.max_hp));}return true;
 case OriginalEventOp::Gold:if(step.a<0&&s.gold<-step.a)return false;gain_gold(s,step.a);return true;
 case OriginalEventOp::AddCard:case OriginalEventOp::AddCurse:
  if(step.a<0||step.a>=CardKinds||step.b!=1||step.c<0||step.c>1||s.deck_count>=MaxDeck)return false;
  if(step.op==OriginalEventOp::AddCurse&&definitions[step.a].type!=CardType::Curse)return false;
  return event_add_deck_card(s,{CardKind(step.a),uint8_t(step.c)})>=0;
 case OriginalEventOp::GainRelic:return step.a>=0&&step.a<WorldRelicKinds&&gain_world_relic(s,RelicId(step.a));
 case OriginalEventOp::LoseRelic:return step.a>=0&&step.a<WorldRelicKinds&&lose_world_relic(s,RelicId(step.a));
 case OriginalEventOp::Potion:
  if(step.a<=0||step.a> PotionKinds)return false;
  // Every PotionReward is optional, including when an empty slot is available.
  e.offer_count=1;e.offers[0]=uint16_t(step.a);return pause_selection(s,OriginalEventSelection::Potion,OriginalEventFilter::Any,0,1,true);
 case OriginalEventOp::Select:{
  if(step.a<=0||step.a>=int(OriginalEventSelection::Count)||step.b<0||step.b>=int(OriginalEventFilter::Count)||step.c<0||step.d<0)return false;
  if(!pause_selection(s,OriginalEventSelection(step.a),OriginalEventFilter(step.b),unsigned(step.c),unsigned(step.d),step.e!=0))return false;
  unsigned available=original_event_eligible(s,nullptr,0);
  if(!available){clear_selection(s);return true;}
  e.select_max=uint8_t(smaller(e.select_max,int(available)));e.select_min=uint8_t(smaller(e.select_min,e.select_max));return true;
 }
 case OriginalEventOp::RandomUpgrade:{
  if(step.a<0)return false;uint16_t ids[MaxDeck];unsigned n=0;
  for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))ids[n++]=uint16_t(i);
  // StableShuffle first orders CardModels by model key, then upgrade level.
  if(step.b)for(unsigned i=1;i<n;i++){auto value=ids[i];unsigned j=i;while(j&&compare_text(definitions[s.deck[ids[j-1]].kind].key,definitions[s.deck[value].kind].key)>0){ids[j]=ids[j-1];--j;}ids[j]=value;}
  auto rng=event_random(s);reference::shuffle(ids,n,rng);keep_random(s,rng);
  for(unsigned i=0;i<n&&i<unsigned(step.a);i++){s.deck[ids[i]].upgraded=1;record(s,LogKind::Upgrade,s.deck[ids[i]].kind);}return true;
 }
 case OriginalEventOp::CardReward:{
  if(step.a<=0||step.a>int(MaxOriginalEventOffers)||step.b<0||step.b>int(WorldCardPool::Own)||step.c< -1||step.c>10||step.d<0||step.e<step.d||step.e>step.a)return false;
  WorldCardOptions options;options.pools[0]=WorldCardPool(step.b);options.odds=WorldCardOdds::Uniform;
  options.flags=WorldNoUpgradeRoll|WorldIsCardReward;if(step.c>0)options.rarity_mask=world_rarity_mask(unsigned(step.c));if(step.c<0)options.odds=WorldCardOdds::RegularEncounter;
  Card cards[MaxOriginalEventOffers];EventCardFacts facts[MaxOriginalEventOffers];unsigned count=0;auto&rng=world_rng(s,WorldRng::Rewards);
  if(!world_create_reward(s,rng,unsigned(step.a),options,cards,facts,MaxOriginalEventOffers,&count)||count<unsigned(step.d))return false;
  e.offer_count=uint16_t(count);for(unsigned i=0;i<count;i++){e.offers[i]=uint16_t(card_token(cards[i]));e.offer_facts[i]=facts[i];}
  return pause_selection(s,OriginalEventSelection::OfferCard,OriginalEventFilter::Any,unsigned(step.d),unsigned(smaller(step.e,int(count))),step.d==0);
 }
 case OriginalEventOp::RandomRelic:{
  if(step.a<0||step.a>int(WorldRelicPool::Ancient))return false;RelicId relic=random_world_relic(s,WorldRelicPool(step.a));
  return relic!=NoWorldRelic&&gain_world_relic(s,relic);
 }
 case OriginalEventOp::RandomPotion:{
  Potion potion{};auto event_rng=event_random(s);auto&rng=step.b?world_rng(s,WorldRng::Rewards):event_rng;
  if(world_random_potions(s,rng,1,&potion,unsigned(step.a))!=1)return false;if(!step.b)keep_random(s,rng);
  OriginalEventStep next{OriginalEventOp::Potion,int16_t(potion)};return perform_step(s,next);
 }
 case OriginalEventOp::Combat:{
  if(step.a<=0||step.a>=int(EncounterId::Count))return false;
  const auto*definition=encounter_definition(EncounterId(step.a));if(!definition)return false;
  e.combat_pending=1;e.resume_after_combat=uint8_t(step.b!=0);e.encounter=uint16_t(step.a);
  if(!world_enter_event_combat(s,definition->key,e.resume_after_combat!=0)){e.combat_pending=e.resume_after_combat=0;e.encounter=0;return false;}return true;
 }
 case OriginalEventOp::Rest:heal(s,rest_heal_amount(s));world_relic_hook(s,WorldRelicHook::AfterRest);return true;
 case OriginalEventOp::Finish:e.finished=1;e.option_count=1;e.options[0]=0;copy_text(e.option_keys[0],"LEAVE");return true;
 case OriginalEventOp::Page:if(step.a<0||step.a>255)return false;e.page=uint8_t(step.a);return true;
 case OriginalEventOp::EnchantAll:
  if(step.a<=0||step.a>=int(EventEnchantment::Count)||step.b<0)return false;
  for(unsigned i=0;i<s.deck_count;i++)if(event_card_can_enchant(s,int(i),EventEnchantment(step.a)))event_card_enchant(s,int(i),EventEnchantment(step.a),step.b);return true;
 case OriginalEventOp::RemoveKind:
  if(step.a<0||step.a>=CardKinds||step.b<0)return false;
  for(int i=int(s.deck_count)-1,left=step.b;i>=0&&left;i--)if(s.deck[i].kind==step.a&&card_removable(s,s.deck[i],i)){if(!event_remove_deck_card(s,i))return false;--left;}return true;
 case OriginalEventOp::Special:return event_special(s,step.a,step.b,step.c,step.d,step.e);
 default:return false;
 }
}
#include "original_event_rules.inc"
}

const char* original_event_key(OriginalEventId id){return unsigned(id)<unsigned(OriginalEventId::Count)?names[unsigned(id)].key:"";}
OriginalEventId original_event_id(const char*key){
 for(unsigned i=1;i<unsigned(OriginalEventId::Count);i++)if(same(key,names[i].key)||same(key,names[i].model))return OriginalEventId(i);return OriginalEventId::None;
}
bool original_event_implemented(OriginalEventId id){return rules_implemented(id);}
bool original_event_allowed(const State&s,OriginalEventId id,unsigned act,unsigned floor){
 if(act>=3||id==OriginalEventId::None||unsigned(id)>=unsigned(OriginalEventId::Count)||id==OriginalEventId::WarHistorianRepy)return false;
 return original_event_implemented(id)&&rules_allowed(s,id,act,floor);
}
bool original_event_begin(State&s,OriginalEventId id,uint64_t run_seed,unsigned act){
 if(!rules_implemented(id)||act>=3||s.original_events.active||s.hp<=0||world_relic_pending(s)||combat_phase(s.phase)||choice_phase(s.phase))return false;
 s.original_events={};auto&e=s.original_events;e.id=id;e.active=1;e.act_index=uint8_t(act);
 reference::Random rng;rng.named(run_seed,original_event_key(id));keep_random(s,rng);
 s.phase=Phase::Event;if(!rules_begin(s)){fail(s);return false;}
 original_event_resume(s);++s.revision;return s.phase!=Phase::Error;
}
unsigned original_event_option_count(const State&s){const auto&e=s.original_events;return e.active&&s.phase==Phase::Event&&!e.combat_pending&&!world_relic_pending(s)&&e.selection==OriginalEventSelection::None?e.option_count:0;}
bool original_event_option_enabled(const State&s,int option){
 const auto&e=s.original_events;if(option<0||unsigned(option)>=original_event_option_count(s)||e.selection_committed)return false;
 if(e.finished)return option==0;return rules_enabled(s,unsigned(option));
}
bool original_event_choose(State&s,int option){
 if(!original_event_option_enabled(s,option))return false;if(s.original_events.finished)return original_event_leave(s);
 if(!rules_choose(s,unsigned(option)))return false;record(s,LogKind::Event,int(s.original_events.id),option);
 original_event_resume(s);++s.revision;return s.phase!=Phase::Error;
}
void original_event_resume(State&s){auto&e=s.original_events;
 if(!e.active||e.combat_pending||world_relic_pending(s)||s.phase==Phase::Error||s.phase==Phase::Dead)return;
 if(e.selection_committed)apply_selection(s);
 if(e.selection!=OriginalEventSelection::None||world_relic_pending(s)||s.phase==Phase::Error||s.phase==Phase::Dead)return;
 s.phase=Phase::Event;
 if(e.reward_active){reward_menu(s);return;}
 while(e.step_cursor<e.step_count&&!e.combat_pending&&!world_relic_pending(s)&&e.selection==OriginalEventSelection::None&&s.phase!=Phase::Error&&s.phase!=Phase::Dead){
  auto step=e.steps[e.step_cursor++];if(!perform_step(s,step)){fail(s);return;}
 }
 if(e.step_cursor==e.step_count&&!e.combat_pending&&e.selection==OriginalEventSelection::None&&!world_relic_pending(s)){
  for(auto&step:e.steps)step={};e.step_count=e.step_cursor=0;
 }
}
unsigned original_event_eligible(const State&s,uint16_t*out,unsigned capacity){const auto&e=s.original_events;unsigned n=0;
 if(!e.active||e.selection==OriginalEventSelection::None||e.selection_committed)return 0;
 if(e.selection==OriginalEventSelection::Rewards){for(unsigned i=0;i<e.reward_count;i++)if(!e.rewards[i].claimed){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;}
 if(e.selection==OriginalEventSelection::OfferCard||e.selection==OriginalEventSelection::Relic){for(unsigned i=0;i<e.offer_count;i++){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;}
 if(e.selection==OriginalEventSelection::Potion){for(unsigned i=0;i<potion_slot_count(s);i++){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;}
 if(!selection_kind(e.selection))return 0;
 for(unsigned i=0;i<s.deck_count;i++)if(deck_eligible(s,i)){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;
}
bool original_event_select(State&s,const uint16_t*ids,unsigned count){auto&e=s.original_events;
 if(!e.active||s.phase!=Phase::OriginalEventChoice||e.selection_committed||!selection_kind(e.selection)||count<e.select_min||count>e.select_max||count>MaxDeck||(!ids&&count))return false;
 if(e.selection==OriginalEventSelection::Potion)return original_event_potion(s,count?int(ids[0]):-1);
 if(e.selection==OriginalEventSelection::Rewards)return count<=1&&original_event_reward(s,count?int(ids[0]):-1);
 if(!count&&!e.select_cancelable&&e.select_min)return false;
 uint16_t eligible[MaxDeck];const unsigned n=original_event_eligible(s,eligible,MaxDeck);
 for(unsigned i=0;i<count;i++){
  bool found=false;for(unsigned j=0;j<n&&j<MaxDeck;j++)if(ids[i]==eligible[j])found=true;
  if(!found)return false;for(unsigned j=0;j<i;j++)if(ids[j]==ids[i])return false;
 }
 if((e.selection==OriginalEventSelection::OfferCard||e.selection==OriginalEventSelection::Duplicate)&&s.deck_count+count>MaxDeck)return false;
 e.selected_count=uint16_t(count);e.selected_cursor=0;e.selection_committed=1;
 for(unsigned i=0;i<count;i++){
  e.selected[i]=ids[i];
  if(e.selection==OriginalEventSelection::OfferCard)e.selected_cards[i]={card_from_token(e.offers[ids[i]]),e.offer_facts[ids[i]],0,0};
  else if(e.selection==OriginalEventSelection::Duplicate)e.selected_cards[i]={s.deck[ids[i]],s.event_cards.deck[ids[i]],s.defect.genetic_deck[ids[i]],s.necro.scythe_deck[ids[i]]};
 }
 s.phase=Phase::Event;original_event_resume(s);++s.revision;return s.phase!=Phase::Error;
}
bool original_event_potion(State&s,int slot){auto&e=s.original_events;
 if(!e.active||s.phase!=Phase::OriginalEventChoice||e.selection!=OriginalEventSelection::Potion||e.offer_count!=1||slot< -1||slot>=int(potion_slot_count(s))||(!e.select_cancelable&&slot<0))return false;
 Potion potion=Potion(e.offers[0]);if(!potion_definition(potion))return false;
 if(slot>=0){
  // Acquisition callbacks run even when a full inventory required a decision.
  // Sozu prevents acquisition before any existing bottle can be discarded.
  if(has_world_relic(s,WorldRelicSozu))return false;
  const Phase phase=s.phase;s.phase=Phase::Event;
  if(potion_at(s,unsigned(slot))!=Potion::Empty&&!discard_potion(s,slot)){s.phase=phase;return false;}
  if(!potion_set(s,unsigned(slot),potion))return false;world_relic_hook(s,WorldRelicHook::AfterPotionProcured,int(potion));
 }
 if(e.reward_active&&e.reward_current<e.reward_count){if(slot>=0)e.rewards[e.reward_current].claimed=1;e.reward_current=255;}
 clear_selection(s);original_event_resume(s);++s.revision;return s.phase!=Phase::Error;
}
bool original_event_reward(State&s,int index){auto&e=s.original_events;
 if(!e.active||!e.reward_active||s.phase!=Phase::OriginalEventChoice||e.selection!=OriginalEventSelection::Rewards||world_relic_pending(s)||index< -1||index>=int(e.reward_count))return false;
 if(index<0){e.reward_active=0;e.reward_current=255;clear_selection(s);original_event_resume(s);++s.revision;return s.phase!=Phase::Error;}
 auto&r=e.rewards[index];if(r.claimed)return false;
 clear_selection(s);s.phase=Phase::Event;e.reward_current=uint8_t(index);
 if(r.kind==1){r.claimed=1;gain_gold(s,r.amount);e.reward_current=255;}
 else if(r.kind==2){e.offer_count=1;e.offers[0]=r.model;pause_selection(s,OriginalEventSelection::Potion,OriginalEventFilter::Any,0,1,true);}
 else if(r.kind==3){r.claimed=1;if(!gain_world_relic(s,r.model)){fail(s);return false;}e.reward_current=255;}
 else if(r.kind==4){e.offer_count=r.count;for(unsigned i=0;i<r.count;i++){e.offers[i]=e.reward_cards[r.first+i];e.offer_facts[i]=e.reward_facts[r.first+i];}pause_selection(s,OriginalEventSelection::OfferCard,OriginalEventFilter::Any,0,1,true);}
 else{fail(s);return false;}
 original_event_resume(s);++s.revision;return s.phase!=Phase::Error;
}
bool original_event_leave(State&s){auto&e=s.original_events;
 if(!e.active||!e.finished||e.combat_pending||e.selection!=OriginalEventSelection::None||e.step_cursor<e.step_count||world_relic_pending(s)||s.phase!=Phase::Event)return false;
 e.active=0;finish_room(s);++s.revision;return true;
}
void original_event_after_combat(State&s,bool won){auto&e=s.original_events;
 if(!e.active||!e.combat_pending)return;e.combat_pending=0;bool resume=e.resume_after_combat!=0;e.resume_after_combat=0;e.encounter=0;
 if(!won||s.hp<=0){s.phase=Phase::Dead;return;}
 s.phase=Phase::Event;
 if(!resume){e.finished=1;e.option_count=1;e.options[0]=0;copy_text(e.option_keys[0],"LEAVE");}
 original_event_resume(s);++s.revision;
}
bool original_event_crystal_tool(State&,bool){return false;}
bool original_event_crystal_click(State&,unsigned,unsigned){return false;}
bool original_event_text_key(const State&s,int option,bool description,char*out,unsigned capacity){
 if(!out||!capacity)return false;const auto&e=s.original_events;
 if(!e.active||unsigned(e.id)>=unsigned(OriginalEventId::Count)||option< -1||option>=int(e.option_count))return false;
 const char*parts[5]={original_event_key(e.id),".pages.",option<0?e.page_key:e.option_keys[option],description?".description":".title",""};
 unsigned size=0;for(const char*p:parts)for(unsigned i=0;p[i];i++)++size;if(size+1>capacity)return false;
 unsigned at=0;for(const char*p:parts)for(unsigned i=0;p[i];i++)out[at++]=p[i];out[at]=0;return true;
}
bool original_event_description_value(const State&s,const char*key,int&value){
 if(s.original_events.id==OriginalEventId::AbyssalBaths){
  if(same(key,"MaxHp"))value=2;else if(same(key,"Damage"))value=s.original_events.values[0];else if(same(key,"Heal"))value=10;else return false;return true;
 }return false;
}
void original_event_new_run(State&s){s.original_events={};s.event_run.wongo_earned=0;s.event_run.freed_repy=s.event_run.completion_recorded=0;}
void original_event_finish_run(State&s){s.event_run.completion_recorded=1;}
bool original_event_blocks_potions(const State&){return false;}
bool original_event_foul_potion(State&){return false;}

#include "original_event_validation.inc"
} // namespace spire
