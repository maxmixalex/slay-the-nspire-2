// Pinned reward UI observation. Only real acquisitions emitted by a UI input
// scope create flights; restoring a save and ordinary rendering never do.
void App::observe_deck_removing(const State&s,int index){
 // The picker already captures its selected card across reset_presentation.
 // Direct event/relic removals bypass that picker but use the same original
 // CardPileCmd removal preview. Combat-only removals may disable showPreview
 // in the source and are deliberately not inferred from a generic callback.
 if(&s!=&game||index<0||index>=s.deck_count||combat_phase(s.phase)||s.phase==Phase::Remove||grimoire_select)return;
 auto&removed=reward_presentation.removed;removed=RewardPresentation::RemovedCard{};
 removed.card=s.deck[index];if(const auto*facts=event_card_facts(s,removed.card,-1,index))removed.facts=*facts;
 removed.energy=int16_t(card_offer_cost(removed.card,&removed.facts));removed.stars=int16_t(card_star_cost(s,removed.card));
 card_description(removed.card,-1,removed.description,sizeof removed.description,-1,index,&removed.facts);
 removed.start=map_clock_ms;removed.active=true;
}
static uint8_t reward_list_mode(const App&a){
 if(a.game.phase==Phase::Reward&&!a.reward_cards&&!a.grimoire_select)return 1;
 if(a.game.phase==Phase::WorldRelicChoice&&a.game.world_relics.choice==WorldRelicChoice::OfferReward)return 2;
 if(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Rewards)return 3;
 if(a.world_choice_is_potion_reward())return 4;
 return 0;
}
bool App::reward_list_interactive()const{
 return reward_list_mode(*this)&&!title&&!paused&&!help&&!library&&!panel&&!settings_open&&!frontend_confirm&&
  !map_window.is_open&&!character_select&&detail_card<0&&detail_item<0&&status_actor<0&&!event_reader;
}
bool App::card_reward_active()const{
 return (game.phase==Phase::Reward&&reward_cards&&!grimoire_select)||world_choice_is_card_reward();
}
bool App::card_reward_interactive()const{
 return card_reward_active()&&!title&&!paused&&!help&&!library&&!panel&&!settings_open&&!frontend_confirm&&
  !map_window.is_open&&!character_select&&detail_card<0&&detail_item<0&&status_actor<0&&!event_reader;
}
void App::sync_card_reward_presentation(){
 auto&m=reward_presentation.cards;
 if(!card_reward_active()){m.observe(false,0,0,map_clock_ms);return;}
 uint32_t key=game.seed^run_number*0x9e3779b9u^unsigned(game.floor)*0x85ebca6bu^unsigned(game.phase)*0xc2b2ae35u;
 unsigned count=0;
 const auto mix=[&](unsigned value){key=(key^value)*16777619u;};
 if(game.phase==Phase::Reward){count=game.world.active?game.world.reward_count:3;
  for(unsigned i=0;i<count;++i)mix(game.world.active?card_token(game.world.reward_cards[i]):game.reward[i]);
  mix(game.world.reward_rerolled);
 }else{
  uint16_t ids[MaxWorldUiChoices];count=world_choice_options(ids,MaxWorldUiChoices);
  const bool relic=game.phase==Phase::WorldRelicChoice;
  for(unsigned i=0;i<count;++i)mix(relic?game.world_relics.offers[ids[i]]:game.original_events.offers[ids[i]]);
  mix(relic?game.world_relics.choice_reward:game.original_events.reward_active);
  // RefreshOptions has its own offer/reward progress. Unrelated potion or
  // relic changes can bump State::revision without reopening this overlay.
  mix(game.run_options.draft_remaining);mix(game.run_options.neow_current);
  if(relic){mix(game.world_relics.choice_owner);if(game.world_relics.choice_reward)mix(game.world_relics.rewards[game.world_relics.choice_reward-1].flags);}
  else{mix(game.original_events.step_cursor);mix(unsigned(game.original_events.values[14]));
   if(game.original_events.reward_current<game.original_events.reward_count)mix(game.original_events.rewards[game.original_events.reward_current].flags);}
 }
 if(m.observe(true,key,count,map_clock_ms))cursor=int(count/2);
}
bool App::card_reward_input_ready(){
 sync_card_reward_presentation();return reward_presentation.cards.clickable(map_clock_ms);
}
void App::card_reward_alternative(bool skip){
 if(!card_reward_interactive())return;
 if(skip){if(game.phase==Phase::Reward){reward_cards=false;cursor=1;sync_card_reward_presentation();}
  else if(!world_choice_min()){choice_selected_count=0;confirm_world_choice();}
  return;
 }
 const unsigned alternatives=world_card_reward_alternatives(game);
 // Source alternatives are Skip, Reroll, then relic hooks. Each normal set
 // has at most two buttons; preserve that order for its shared hotkey.
 if(alternatives&2)ui_action(UiAction::RerollReward);
 else if(alternatives&1)ui_action(UiAction::SacrificeReward);
}
RewardAcquisitionSnapshot App::reward_acquisition_snapshot()const{
 RewardAcquisitionSnapshot out;out.serial=reward_presentation.serial;out.seed=game.seed;out.run=run_number;
 if(title||paused||help||library||panel||settings_open||map_window.is_open||detail_card>=0||detail_item>=0||status_actor>=0||event_reader)return out;
 const bool cards=game.phase==Phase::Reward&&reward_cards||world_choice_is_card_reward();
 const bool direct=game.phase==Phase::Shop&&shop_open||game.phase==Phase::Event&&game.original_events.active||game.phase==Phase::OriginalEventChoice||game.phase==Phase::Treasure&&game.world.active;
 const unsigned mode=reward_list_mode(*this);if(!mode&&!cards&&!direct)return out;
 out.context=cards||direct?2:1;out.deck_count=game.deck_count;out.relic_count=uint16_t(ui_owned_relic_count(game));out.gold=game.gold;
 for(unsigned i=0;i<MaxPotionSlots;++i)out.potions[i]=potion_at(game,i);
 const HitRegion*chosen=nullptr;
 if(pointer.visible){const auto*h=hit_at(pointer.x,pointer.y);
  if(h&&(h->kind==HitKind::Choice||h->kind==HitKind::WorldChoice||h->kind==HitKind::UiAction))chosen=h;
 }
 if(!chosen){
  int id=cursor;HitKind kind=HitKind::Choice;
  if(game.phase==Phase::Treasure){id=int(game.treasure_state?UiAction::TreasureTake:UiAction::TreasureOpen);kind=HitKind::UiAction;}
  if(mode==1){UiAction actions[6];const int count=int(ui_reward_actions(game,actions));if(id<0||id>=count)return RewardAcquisitionSnapshot{};id=int(actions[id]);kind=HitKind::UiAction;}
  else if(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice){uint16_t ids[MaxWorldUiChoices];const unsigned count=world_choice_options(ids,MaxWorldUiChoices);if(cursor<0||unsigned(cursor)>=count)return RewardAcquisitionSnapshot{};id=ids[cursor];kind=HitKind::WorldChoice;}
  for(int i=0;i<hit_count;++i)if(hits[i].kind==kind&&hits[i].value==id){chosen=&hits[i];break;}
 }
 const int dy=reward_presentation.window(map_clock_ms).y;
 out.x=int16_t(chosen?chosen->x+(cards||direct?chosen->w/2:18):cards||direct?160:86);
 out.y=int16_t(chosen?chosen->y+chosen->h/2:cards||direct?125:93+dy);
 out.potion_x=out.relic_x=out.x;out.potion_y=out.relic_y=out.y;
 if(mode==1)for(int i=0;i<hit_count;++i){const auto&h=hits[i];if(h.kind!=HitKind::UiAction)continue;
  if(h.value==int(UiAction::RewardPotion)){out.potion_x=int16_t(h.x+18);out.potion_y=int16_t(h.y+h.h/2);}
  if(h.value==int(UiAction::RewardRelic)){out.relic_x=int16_t(h.x+18);out.relic_y=int16_t(h.y+h.h/2);}
 }
 return out;
}
void App::finish_reward_acquisition(const RewardAcquisitionSnapshot&old){
 auto&m=reward_presentation;
 if(!old.context||old.serial!=m.serial||old.seed!=game.seed||old.run!=run_number)return;
 bool changed=false;
 for(unsigned i=0;i<potion_slot_count(game);++i){const auto potion=potion_at(game,i);
  if(old.potions[i]==Potion::Empty&&potion!=Potion::Empty){m.append(RewardPickupKind::Potion,unsigned(potion),i,old.potion_x,old.potion_y,map_clock_ms);changed=true;}
 }
 for(unsigned i=old.relic_count;i<ui_owned_relic_count(game);++i){m.append(RewardPickupKind::Relic,ui_owned_relic_id(game,i),i,old.relic_x,old.relic_y,map_clock_ms);changed=true;}
 if(old.context==2)for(unsigned i=old.deck_count;i<game.deck_count;++i){m.append(RewardPickupKind::Card,game.deck[i].kind,i,old.x,old.y,map_clock_ms,game.deck[i]);changed=true;}
 if(old.gold!=game.gold){m.begin_gold(old.gold,game.gold,map_clock_ms);changed=true;}
 if(changed)++m.serial; // Nested pointer/key/action observers share one acquisition.
}
void App::sync_reward_presentation(){
 sync_card_reward_presentation();
 auto&m=reward_presentation;
 if(!m.gold_bound||m.gold_to!=game.gold){m.gold_bound=true;m.gold_from=m.gold_to=game.gold;m.gold_active=false;}
 for(auto&f:m.flights){
  if(!RewardPresentation::retained(f,map_clock_ms)||
     (f.kind==RewardPickupKind::Potion&&unsigned(potion_at(game,f.slot))!=f.model)||
     (f.kind==RewardPickupKind::Relic&&ui_owned_relic_id(game,f.slot)!=f.model))f.kind=RewardPickupKind::None;
 }
 uint8_t mode=reward_list_mode(*this);unsigned count=0;
 if(title||paused||help||library||(panel&&panel!=4)||settings_open||map_window.is_open||int32_t(combat_linger_until-animation)>0)mode=0;
 if(mode==1)count=ui_reward_actions(game,nullptr)-1;
 else if(mode==4)count=1;
 else if(mode)count=world_choice_options(nullptr,0);
 // The semantic room/set identity changes on a new run/room, not on hover,
 // scroll, layout or a single row disappearing from the same rewards set.
 uint32_t key=game.seed^run_number*0x9e3779b9u^unsigned(game.floor)*0x85ebca6bu;
 key^=unsigned(game.original_events.id)<<16;key^=unsigned(game.original_events.reward_active)<<8;
 m.observe_window(mode,key,count,map_clock_ms);
 m.configure_list(mode,key,count,pointer.visible?-1:cursor,map_clock_ms);
}
bool App::reward_pickup_hidden(RewardPickupKind kind,unsigned position)const{return reward_presentation.hidden(kind,position,map_clock_ms);}
int App::reward_gold()const{return reward_presentation.gold_bound?reward_presentation.displayed_gold(map_clock_ms):game.gold;}
