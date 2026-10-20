// Included in world_relics.cpp. Values and hook timing follow the pinned
// v0.111.0 run/rest/reward models; every continuation stays in saved State.
namespace {
void wr_strength(State& s,int amount){
 world_relic_gain_strength(s,amount);
}
void wr_red_skull(State& s,WorldRelicInstance& r){
 if(s.hp<=0||world_room_type(s)<1||world_room_type(s)>3)return;
 bool low=int(s.hp)*2<=s.max_hp;
 if(low&&!r.values[0]){r.values[0]=1;world_relic_flash(s,r.id);wr_strength(s,3);}
 else if(!low&&r.values[0]){r.values[0]=0;world_relic_flash(s,r.id);wr_strength(s,-3);}
}
void wr_ghost(State& s,unsigned id){
 if(id<s.card_count&&(w_basic_strike(s.cards[id].kind)||w_basic_defend(s.cards[id].kind)))s.necro.ethereal[id]=1;
}
void wr_stone_cracker(State& s){
 uint16_t ids[MaxCards];unsigned count=0;
 for(unsigned i=0;i<s.draw.count;++i){unsigned id=s.draw.ids[i];if(id<s.card_count&&card_upgradeable(s.cards[id]))ids[count++]=uint16_t(id);}
 // CardModel.CompareTo orders model id, then upgrade level. All candidates
 // here are unupgraded; equal copies keep their draw-pile order.
 for(unsigned i=1;i<count;++i){uint16_t id=ids[i];unsigned j=i;
  while(j&&w_cmp(definitions[s.cards[ids[j-1]].kind].key,definitions[s.cards[id].kind].key)>0){ids[j]=ids[j-1];--j;}ids[j]=id;
 }
 reference::shuffle(ids,count,world_rng(s,WorldRng::CombatCardSelection));
 for(unsigned i=0;i<count&&i<2;++i){s.cards[ids[i]].upgraded=1;record(s,LogKind::Upgrade,s.cards[ids[i]].kind);}
}
WorldCardOptions wr_room_card_options(bool boss=false){
 WorldCardOptions options;options.source=WorldCardSource::Encounter;
 options.odds=boss?WorldCardOdds::BossEncounter:WorldCardOdds::RegularEncounter;
 options.flags=WorldIsCardReward;return options;
}
}

void world_relic_run_hook(State& s,unsigned serial,WorldRelicHook hook,int a,int b,int c){
 int at=world_relic_instance_index(s,serial);if(at<0)return;
 auto& r=s.world_relics.entries[at];if(!w_present(r))return;
 const bool combat=world_room_type(s)>=1&&world_room_type(s)<=3;
 if(hook==WorldRelicHook::AfterRoomEntered){
  switch(r.id){
  case WorldRelicMawBank:if(!s.world.event_combat_parent&&!r.values[0]){world_relic_flash(s,r.id);gain_gold(s,12);}break;
  case WorldRelicEternalFeather:if(world_room_type(s)==unsigned(WorldRoom::Rest)){world_relic_flash(s,r.id);heal(s,3*(s.deck_count/5));}break;
  case WorldRelicMealTicket:if(world_room_type(s)==unsigned(WorldRoom::Shop)){world_relic_flash(s,r.id);heal(s,15);}break;
  case WorldRelicPlanisphere:{const auto* map=world_current_map(s);
   if(s.hp>0&&!s.world.event_combat_parent&&map&&s.world.current_point<map->count&&map->points[s.world.current_point].type==reference::PointType::Unknown){world_relic_flash(s,r.id);heal(s,5);}break;}
  case WorldRelicLavaLamp:r.values[0]=0;break;
  case WorldRelicSilverCrucible:if(world_room_type(s)==unsigned(WorldRoom::Treasure))++r.values[1];break;
  case WorldRelicVenerableTeaSet:case WorldRelicFakeVenerableTeaSet:if(world_room_type(s)==unsigned(WorldRoom::Rest))r.values[0]=1;break;
  case WorldRelicBurningSticks:case WorldRelicMetronome:case WorldRelicPermafrost:case WorldRelicThrowingAxe:case WorldRelicVelvetChoker:if(combat)r.values[0]=0;break;
  case WorldRelicDivineRight:if(combat)regent_gain_stars(s,3);break;
  case WorldRelicEmberTea:if(combat&&r.values[0]>0){world_relic_flash(s,r.id);wr_strength(s,2);--r.values[0];}break;
  case WorldRelicGirya:if(combat&&r.values[0]>0){world_relic_flash(s,r.id);wr_strength(s,r.values[0]);}break;
  case WorldRelicVajra:if(combat){world_relic_flash(s,r.id);wr_strength(s,1);}break;
  case WorldRelicSwordOfJade:if(combat){world_relic_flash(s,r.id);wr_strength(s,3);}break;
  case WorldRelicOddlySmoothStone:if(combat){world_relic_flash(s,r.id);world_relic_gain_dexterity(s,1);}break;
  case WorldRelicBronzeScales:if(combat){world_relic_flash(s,r.id);s.thorns=int16_t(w_min(9999,s.thorns+3));}break;
  case WorldRelicSlingOfCourage:if(world_room_type(s)==unsigned(WorldRoom::Elite)){world_relic_flash(s,r.id);wr_strength(s,2);}break;
  case WorldRelicDataDisk:if(combat){world_relic_flash(s,r.id);gain_power(s,XPower::Focus,1);}break;
  case WorldRelicGorget:if(combat){world_relic_flash(s,r.id);gain_power(s,XPower::Plating,4);}break;
  case WorldRelicPhilosophersStone:if(combat){world_relic_flash(s,r.id);for(unsigned i=0;i<s.enemy_count;++i)if(s.enemies[i].hp>0)s.enemies[i].strength=int16_t(w_min(9999,s.enemies[i].strength+1));}break;
  case WorldRelicRedSkull:if(combat)wr_red_skull(s,r);break;
  case WorldRelicStoneCracker:if(combat){world_relic_flash(s,r.id);wr_stone_cracker(s);}break;
  case WorldRelicGhostSeed:if(combat)for(unsigned i=0;i<s.card_count;++i)wr_ghost(s,i);break;
  case WorldRelicLordsParasol:if(world_room_type(s)==unsigned(WorldRoom::Shop))s.world.shop_auto_active=1;break;
  default:break;
  }
 }else if(hook==WorldRelicHook::AfterRest){
  if(r.id==WorldRelicStoneHumidifier){world_relic_flash(s,r.id);increase_max_hp(s,5);}
  else if(r.id==WorldRelicRegalPillow)world_relic_flash(s,r.id);
 }else if(hook==WorldRelicHook::AfterGoldGained){
  if(r.id==WorldRelicDragonFruit&&a>0){world_relic_flash(s,r.id);increase_max_hp(s,1);}
 }else if(hook==WorldRelicHook::AfterItemPurchased){
  if(r.id==WorldRelicMawBank&&b>0&&!r.values[0]){world_relic_flash(s,r.id);r.values[0]=1;}
 }else if(hook==WorldRelicHook::AfterDeckAdded){
  if(a<0||a>=s.deck_count)return;
  if(r.id==WorldRelicDarkstonePeriapt&&card_type(s,s.deck[a],-1)==CardType::Curse){world_relic_flash(s,r.id);increase_max_hp(s,6);}
  else if(r.id==WorldRelicLuckyFysh){world_relic_flash(s,r.id);gain_gold(s,15);}
  else if(r.id==WorldRelicBookOfFiveRings&&s.hp>0){r.values[0]=(r.values[0]+1)%5;if(!r.values[0]){world_relic_flash(s,r.id);heal(s,20);}}
  else if(r.id==WorldRelicBingBong&&!c){
   EventDeckSnapshot snap{s.deck[a],s.event_cards.deck[a],s.defect.genetic_deck[a],s.necro.scythe_deck[a]};
   int added=event_restore_deck_card(s,snap);if(added<0){w_error(s);return;}world_relic_flash(s,r.id);
   // Original clonedBy is non-null for every duplicate created by BingBong,
   // including other owned copies of the relic. It must not clone recursively.
   world_relic_hook(s,WorldRelicHook::AfterDeckAdded,added,0,int(serial));
  }
 }else if(hook==WorldRelicHook::AfterHpChanged){
  if(r.id==WorldRelicRedSkull&&s.phase==Phase::Combat&&c<0)wr_red_skull(s,r);
 }else if(hook==WorldRelicHook::AfterCardEnteredCombat){
  if(r.id==WorldRelicGhostSeed&&a>=0)wr_ghost(s,unsigned(a));
 }else if(hook==WorldRelicHook::AfterCombatEnd){
  if(r.id==WorldRelicWongosMysteryTicket)++r.values[0];
  else if(r.id==WorldRelicPumpkinCandle)r.values[0]=w_max(0,r.values[0]-1);
  else if(r.id==WorldRelicRedSkull)r.values[0]=0;
 }
}

unsigned world_card_reward_alternatives(const State& s){
 if(!s.world_relics.active||w_first(s,WorldRelicPaelsWing,true)<0)return 0;
 if(s.phase==Phase::Reward&&s.world.active&&s.world.reward_count&&!(s.reward_claimed&2))return 1;
 const auto&w=s.world_relics;
 if(s.phase==Phase::WorldRelicChoice&&w.choice==WorldRelicChoice::OfferCard&&w.choice_reward&&w.choice_reward<=w.reward_count){
  const auto&r=w.rewards[w.choice_reward-1];if(r.group&&r.kind==1&&!(r.flags&1))return 1;
 }
 return original_event_has_card_reward(s)?1:0;
}
bool world_take_card_reward_alternative(State& s,unsigned bit){
 if(bit!=1||!(world_card_reward_alternatives(s)&bit))return false;
 int at=w_first(s,WorldRelicPaelsWing,true);if(at<0)return false;
 auto&w=s.world_relics;
 if(s.phase==Phase::Reward)s.reward_claimed|=2;
 else if(s.phase==Phase::WorldRelicChoice){w.rewards[w.choice_reward-1].flags|=1;w_clear_choice(s);}
 else if(!original_event_consume_card_reward(s))return false;
 auto&r=w.entries[at];r.values[0]=(r.values[0]+1)%2;world_relic_flash(s,r.id);
 if(!r.values[0]){
  const RelicId id=world_relic_pull(s,WorldRelicPool::Reward,false);
  if(id==NoWorldRelic||!gain_world_relic(s,id)){w_error(s);return false;}
 }
 w_pump(s);world_relic_resume(s);++s.revision;
 if(!world_relic_pending(s)&&s.phase!=Phase::Error&&s.phase!=Phase::Dead){
  w.offer_count=0;w.choice_group=w.choice_reward=0;original_event_resume(s);world_resume_room(s);
 }
 return s.phase!=Phase::Error;
}

bool world_relic_combat_rewards(State& s){
 if(!world_room_is_combat(s))return false;
 struct Extra {uint8_t kind;uint16_t serial;};
 Extra extras[MaxWorldRelicRewards]{};unsigned count=0;bool cards=false;
 const auto room=WorldRoom(world_room_type(s));unsigned group=0;
 if(!(room==WorldRoom::Boss&&s.world.act_index==2))for(unsigned i=0;i<encounter_returned_gold_count(s);++i){
  if(!group)group=w_new_group(s,0);if(!group)return false;
  auto* reward=w_reward_slot(s,group,4);if(!reward)return false;
  const int gold=encounter_returned_gold(s,i);reward->value=uint16_t(world_rng(s,WorldRng::Rewards).range(gold,gold+1));
 }
 auto add=[&](unsigned kind,unsigned serial,unsigned n=1){
  if(count+n>MaxWorldRelicRewards){w_error(s);return false;}
  for(unsigned i=0;i<n;++i)extras[count++]={uint8_t(kind),uint16_t(serial)};return true;
 };
 // First every modifier appends reward descriptors. Only then are added
 // rewards populated; this preserves the source RNG and hook order.
 for(unsigned i=0;i<s.world_relics.count;++i){auto& r=s.world_relics.entries[i];if(!w_present(r))continue;
  switch(r.id){
  case WorldRelicAmethystAubergine:if(!(room==WorldRoom::Boss&&s.world.act_index==2)&&!add(4,r.serial))return false;break;
  case WorldRelicPrayerWheel:if(room==WorldRoom::Monster){if(!add(1,r.serial))return false;cards=true;}break;
  case WorldRelicWhiteStar:if(room==WorldRoom::Elite){if(!add(5,r.serial))return false;cards=true;}break;
  case WorldRelicBlackStar:if(room==WorldRoom::Elite&&!add(2,r.serial))return false;break;
  case WorldRelicLavaRock:if(room==WorldRoom::Boss&&s.world.act_index==0&&!r.values[0]){if(!add(2,r.serial,2))return false;r.values[0]=1;world_relic_flash(s,r.id);}break;
  case WorldRelicWongosMysteryTicket:if(!r.values[1]&&r.values[0]>=5&&!add(2,r.serial,3))return false;break;
  default:break;
  }
 }
 if(!count&&!group)return false;if(!group)group=w_new_group(s,0);if(!group)return false;
 for(unsigned i=0;i<count;++i){const auto& extra=extras[i];
  if(extra.kind==4){auto* reward=w_reward_slot(s,group,4);if(!reward)return false;reward->value=uint16_t(world_rng(s,WorldRng::Rewards).range(15,16));}
  else if(extra.kind==2){RelicId id=random_world_relic(s);if(id==NoWorldRelic){w_error(s);return false;}w_reward_relic(s,group,id);}
  else if(!w_reward_cards(s,group,wr_room_card_options(extra.kind==5)))return false;
 }
 for(unsigned i=0;i<count;++i){bool first=true;for(unsigned j=0;j<i;++j)if(extras[j].serial==extras[i].serial)first=false;if(!first)continue;
  int at=world_relic_instance_index(s,extras[i].serial);if(at>=0){auto& r=s.world_relics.entries[at];if(r.id==WorldRelicWongosMysteryTicket)r.values[1]=1;world_relic_flash(s,r.id);}
 }
 w_schedule_group(s,group);w_pump(s);return cards;
}

void world_relic_rest_rewards(State& s){
 unsigned group=0;
 for(unsigned i=0;i<s.world_relics.count;++i){const auto& r=s.world_relics.entries[i];if(!w_present(r))continue;
  if(r.id!=WorldRelicDreamCatcher&&r.id!=WorldRelicTinyMailbox)continue;
  if(!group)group=w_new_group(s,0);if(!group)return;
  world_relic_flash(s,r.id);
  if(r.id==WorldRelicDreamCatcher){if(!w_reward_cards(s,group,wr_room_card_options()))return;}
  else{w_reward_potion(s,group);w_reward_potion(s,group);}
 }
 if(group){w_schedule_group(s,group);w_pump(s);}
}

bool world_relic_rest_select(State& s,bool cook){
 int at=w_first(s,cook?WorldRelicMeatCleaver:NoWorldRelic,true);
 if(cook&&at<0)return false;
 auto& w=s.world_relics;if(world_relic_pending(s))return false;
 w.choice_cancelable=1;w.offer_count=0;w.choice_group=w.choice_reward=0;
 w_pause(s,cook?WorldRelicChoice::Remove:WorldRelicChoice::Upgrade,
         at<0?0:w.entries[at].serial,0,cook?2:1,cook?WRemove:WUpgradeable);
 return true;
}
