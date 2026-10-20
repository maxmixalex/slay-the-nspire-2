static bool world_choice_phase(Phase p){return p==Phase::WorldRelicChoice||p==Phase::OriginalEventChoice;}
unsigned App::world_choice_options(uint16_t*out,unsigned capacity)const{
 if(game.phase==Phase::WorldRelicChoice)return world_relic_eligible(game,out,capacity);
 if(game.phase==Phase::OriginalEventChoice)return original_event_eligible(game,out,capacity);
 return 0;
}
unsigned App::world_choice_min()const{return game.phase==Phase::WorldRelicChoice?game.world_relics.choice_min:game.original_events.select_min;}
unsigned App::world_choice_max()const{return game.phase==Phase::WorldRelicChoice?game.world_relics.choice_max:game.original_events.select_max;}
void App::toggle_world_choice(int id){
 if(!world_choice_phase(game.phase)||id<0)return;sync_selection();
 uint16_t ids[MaxWorldUiChoices];unsigned n=world_choice_options(ids,MaxWorldUiChoices);bool eligible=false;
 for(unsigned i=0;i<n&&i<MaxWorldUiChoices;i++)if(ids[i]==id)eligible=true;if(!eligible)return;
 const bool reward=game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::OfferReward:game.original_events.selection==OriginalEventSelection::Rewards;
 if(reward){uint16_t selected=uint16_t(id);bool done=game.phase==Phase::WorldRelicChoice?world_relic_select(game,&selected,1):original_event_select(game,&selected,1);
  if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();}return;
 }
 for(unsigned i=0;i<choice_selected_count;i++)if(choice_selected[i]==id){
  for(unsigned j=i+1;j<choice_selected_count;j++)choice_selected[j-1]=choice_selected[j];--choice_selected_count;return;
 }
 unsigned maximum=world_choice_max();if(maximum==1)choice_selected_count=0;
 if(choice_selected_count<maximum&&choice_selected_count<MaxDeck)choice_selected[choice_selected_count++]=uint16_t(id);
}
void App::confirm_world_choice(){
 if(!world_choice_phase(game.phase))return;sync_selection();
 if(choice_selected_count<world_choice_min()||choice_selected_count>world_choice_max())return;
 bool done=game.phase==Phase::WorldRelicChoice?world_relic_select(game,choice_selected,choice_selected_count):
  original_event_select(game,choice_selected,choice_selected_count);
 if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();}
}
void App::world_choice_detail(int id){
 if(id<0)return;
 if(game.phase==Phase::WorldRelicChoice){
  const auto&w=game.world_relics;
  if((w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer)&&id<w.offer_count)open_card_offer(card_from_token(w.offers[id]),w.offer_facts[id]);
  else if(w.choice==WorldRelicChoice::OfferBundle&&id*3<w.offer_count)open_card_offer(card_from_token(w.offers[id*3]),w.offer_facts[id*3]);
  else if(w.choice==WorldRelicChoice::Hand&&id<game.card_count)open_choice_detail(id);
  else if(w.choice==WorldRelicChoice::Potion&&id<int(potion_slot_count(game))&&potion_at(game,id)!=Potion::Empty){detail_item=WorldRelicKinds+int(potion_at(game,id))-1;detail_scroll=0;}
  else if(w.choice==WorldRelicChoice::OfferReward&&id<w.offer_count){const auto&r=w.rewards[w.offers[id]];if(r.kind==2)detail_item=r.value;else if(r.kind==3)detail_item=WorldRelicKinds+r.value-1;else if(r.kind==5)open_card_offer(card_from_token(r.value),r.facts[0],r.tokens[0],r.tokens[1]);detail_owned=-1;detail_scroll=0;}
  else if(w.choice!=WorldRelicChoice::OfferReward&&id<game.deck_count)open_card_detail(game.deck[id],id);
 }else if(game.phase==Phase::OriginalEventChoice){
  const auto&e=game.original_events;
  if(e.selection==OriginalEventSelection::OfferCard&&id<e.offer_count)open_card_offer(card_from_token(e.offers[id]),e.offer_facts[id]);
  else if(e.selection==OriginalEventSelection::Potion&&id<int(potion_slot_count(game))&&potion_at(game,id)!=Potion::Empty){detail_item=WorldRelicKinds+int(potion_at(game,id))-1;detail_scroll=0;}
  else if(e.selection==OriginalEventSelection::Relic&&id<e.offer_count){detail_item=e.offers[id];detail_owned=-1;detail_scroll=0;}
  else if(e.selection==OriginalEventSelection::Rewards&&id<e.reward_count){const auto&r=e.rewards[id];if(r.kind==3)detail_item=r.model;else if(r.kind==2)detail_item=WorldRelicKinds+r.model-1;detail_owned=-1;detail_scroll=0;}
  else if(e.selection!=OriginalEventSelection::Relic&&e.selection!=OriginalEventSelection::Crystal&&e.selection!=OriginalEventSelection::Rewards&&id<game.deck_count)open_card_detail(game.deck[id],id);
 }
}
bool App::world_choice_input(Key key){
 if(title||paused||help||panel||!world_choice_phase(game.phase))return false;
 sync_selection();
 if(game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Crystal){
  cursor=cursor<0?0:cursor>=CrystalCells?CrystalCells-1:cursor;
  if(key==Key::Left&&cursor%11)cursor--;
  else if(key==Key::Right&&cursor%11<10)cursor++;
  else if(key==Key::Up&&cursor>=11)cursor-=11;
  else if(key==Key::Down&&cursor+11<CrystalCells)cursor+=11;
  else if(key==Key::Confirm)original_event_crystal_click(game,cursor%11,cursor/11);
  else if(key==Key::EndTurn)original_event_crystal_tool(game,game.original_events.crystal_tool!=2);
  else return false;
  return true;
 }
 uint16_t ids[MaxWorldUiChoices];unsigned count=world_choice_options(ids,MaxWorldUiChoices);
 if(count>MaxWorldUiChoices)return false;if(cursor<0||cursor>=int(count))cursor=0;
 if(key==Key::Left&&count)cursor=(cursor+int(count)-1)%int(count);
 else if(key==Key::Right&&count)cursor=(cursor+1)%int(count);
 else if(key==Key::Up&&count)cursor=cursor>=5?cursor-5:cursor;
 else if(key==Key::Down&&count)cursor=cursor+5<int(count)?cursor+5:cursor;
 else if(key==Key::Confirm&&count)toggle_world_choice(ids[cursor]);
 else if(key==Key::EndTurn)confirm_world_choice();
 else if(key==Key::Detail&&count)world_choice_detail(ids[cursor]);
 else return false;
 return true;
}
