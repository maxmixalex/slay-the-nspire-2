#include "crystal_input.inc"
static bool world_choice_phase(Phase p){return p==Phase::WorldRelicChoice||p==Phase::OriginalEventChoice;}
unsigned App::world_choice_options(uint16_t*out,unsigned capacity)const{
 if(game.phase==Phase::WorldRelicChoice)return world_relic_eligible(game,out,capacity);
 if(game.phase==Phase::OriginalEventChoice){
  const unsigned count=original_event_eligible(game,out,capacity);
  if(out&&capacity>=count&&neow_modifier_selection(game,RunModifier::SealedDeck)){
   const auto&e=game.original_events;
   for(unsigned i=1;i<count;++i){const uint16_t value=out[i];const Card card=card_from_token(e.offers[value]);unsigned j=i;
    while(j){const Card prior=card_from_token(e.offers[out[j-1]]);const int rarity=card_rarity(prior.kind)-card_rarity(card.kind);
     if(rarity<0||(!rarity&&localized_card_rank(prior.kind)<=localized_card_rank(card.kind)))break;out[j]=out[j-1];--j;}
    out[j]=value;
   }
  }return count;
 }
 return 0;
}
unsigned App::world_choice_min()const{return game.phase==Phase::WorldRelicChoice?game.world_relics.choice_min:game.original_events.select_min;}
unsigned App::world_choice_max()const{return game.phase==Phase::WorldRelicChoice?game.world_relics.choice_max:game.original_events.select_max;}
bool App::world_choice_is_potion_reward()const{
 return game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::Potion:
  game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Potion;
}
Potion App::world_choice_potion(int id)const{
 if(id<0)return Potion::Empty;
 if(game.phase==Phase::WorldRelicChoice){const auto&w=game.world_relics;
  if(w.choice==WorldRelicChoice::Potion&&w.offer_count)return Potion(w.offers[0]);
  if(w.choice==WorldRelicChoice::OfferReward&&id<w.offer_count){const auto&r=w.rewards[w.offers[id]];if(r.kind==3)return Potion(r.value);}
 }else if(game.phase==Phase::OriginalEventChoice){const auto&e=game.original_events;
  if(e.selection==OriginalEventSelection::Potion&&e.offer_count)return Potion(e.offers[0]);
  if(e.selection==OriginalEventSelection::Rewards&&id<e.reward_count&&e.rewards[id].kind==2)return Potion(e.rewards[id].model);
 }
 return Potion::Empty;
}
bool App::world_choice_is_card_reward()const{
 if(game.phase==Phase::OriginalEventChoice&&neow_modifier_selection(game,RunModifier::Draft))return true;
 if(original_event_has_card_reward(game))return true;
 if(game.phase!=Phase::WorldRelicChoice)return false;const auto&w=game.world_relics;
 if(w.choice!=WorldRelicChoice::OfferCard||!w.choice_reward||w.choice_reward>w.reward_count)return false;
 const auto&r=w.rewards[w.choice_reward-1];return r.group&&r.kind==1&&!(r.flags&1)&&w.choice_max==1;
}
unsigned App::world_choice_page_size()const{
 if(world_choice_is_card_reward())return 3;
 if(game.phase==Phase::WorldRelicChoice){if(game.world_relics.choice==WorldRelicChoice::OfferReward)return 4;if(game.world_relics.choice==WorldRelicChoice::OfferBundle)return 2;}
 if(game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Rewards)return 4;
 return 10;
}
void App::toggle_world_choice(int id){
 if(choice_peeking||!world_choice_phase(game.phase)||id<0||!combat_choice_ready())return;sync_selection();
 uint16_t ids[MaxWorldUiChoices];unsigned n=world_choice_options(ids,MaxWorldUiChoices);bool eligible=false;
 for(unsigned i=0;i<n&&i<MaxWorldUiChoices;i++)if(ids[i]==id)eligible=true;if(!eligible)return;
 const bool reward=game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::OfferReward:game.original_events.selection==OriginalEventSelection::Rewards;
 if(reward||world_choice_is_card_reward()||world_choice_is_potion_reward()){const bool potion=world_choice_potion(id)!=Potion::Empty;continue_combat_choice();uint16_t selected=uint16_t(id);bool done=game.phase==Phase::WorldRelicChoice?world_relic_select(game,&selected,1):original_event_select(game,&selected,1);
  if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();}else if(potion)potion_obtain_failed();return;
 }
 for(unsigned i=0;i<choice_selected_count;i++)if(choice_selected[i]==id){
  for(unsigned j=i+1;j<choice_selected_count;j++)choice_selected[j-1]=choice_selected[j];--choice_selected_count;return;
 }
 unsigned maximum=world_choice_max();if(maximum==1)choice_selected_count=0;
 if(choice_selected_count<maximum&&choice_selected_count<MaxDeck)choice_selected[choice_selected_count++]=uint16_t(id);
}
void App::confirm_world_choice(){
 if(choice_peeking||!world_choice_phase(game.phase)||!combat_choice_ready())return;sync_selection();
 if(choice_selected_count<world_choice_min()||choice_selected_count>world_choice_max())return;
 continue_combat_choice();
 bool done=game.phase==Phase::WorldRelicChoice?world_relic_select(game,choice_selected,choice_selected_count):
  original_event_select(game,choice_selected,choice_selected_count);
 if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();}
}
void App::world_choice_detail(int id){
 if(choice_peeking||id<0||!combat_choice_ready())return;
 if(game.phase==Phase::WorldRelicChoice){
  const auto&w=game.world_relics;
  if((w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer)&&id<w.offer_count)open_card_offer(card_from_token(w.offers[id]),w.offer_facts[id]);
  else if(w.choice==WorldRelicChoice::OfferBundle&&id*3<w.offer_count)open_card_offer(card_from_token(w.offers[id*3]),w.offer_facts[id*3]);
  else if(w.choice==WorldRelicChoice::Hand&&id<game.card_count)open_choice_detail(id);
  else if(w.choice==WorldRelicChoice::Potion&&w.offer_count){detail_item=WorldRelicKinds+w.offers[0]-1;detail_owned=-1;detail_scroll=0;}
  else if(w.choice==WorldRelicChoice::OfferReward&&id<w.offer_count){const auto&r=w.rewards[w.offers[id]];if(r.kind==2)detail_item=r.value;else if(r.kind==3)detail_item=WorldRelicKinds+r.value-1;else if(r.kind==5)open_card_offer(card_from_token(r.value),r.facts[0],r.tokens[0],r.tokens[1]);detail_owned=-1;detail_scroll=0;}
  else if(w.choice!=WorldRelicChoice::OfferReward&&id<game.deck_count)open_card_detail(game.deck[id],id);
 }else if(game.phase==Phase::OriginalEventChoice){
  const auto&e=game.original_events;
  if(e.selection==OriginalEventSelection::OfferCard&&id<e.offer_count)open_card_offer(card_from_token(e.offers[id]),e.offer_facts[id]);
  else if(e.selection==OriginalEventSelection::Potion&&e.offer_count){detail_item=WorldRelicKinds+e.offers[0]-1;detail_owned=-1;detail_scroll=0;}
  else if(e.selection==OriginalEventSelection::Relic&&id<e.offer_count){detail_item=e.offers[id];detail_owned=-1;detail_scroll=0;}
  else if(e.selection==OriginalEventSelection::Rewards&&id<e.reward_count){const auto&r=e.rewards[id];if(r.kind==3)detail_item=r.model;else if(r.kind==2)detail_item=WorldRelicKinds+r.model-1;detail_owned=-1;detail_scroll=0;}
  else if(e.selection!=OriginalEventSelection::Relic&&e.selection!=OriginalEventSelection::Crystal&&e.selection!=OriginalEventSelection::Rewards&&id<game.deck_count)open_card_detail(game.deck[id],id);
 }
}
bool App::world_choice_input(Key key){
 if(choice_peeking||title||paused||help||panel||!world_choice_phase(game.phase)||!combat_choice_ready())return false;
 sync_selection();
 if(game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Crystal)return crystal_input(key);
 uint16_t ids[MaxWorldUiChoices];unsigned count=world_choice_options(ids,MaxWorldUiChoices);
 if(count>MaxWorldUiChoices)return false;if(cursor<0||cursor>=int(count))cursor=0;
 const bool rewards=game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::OfferReward:game.original_events.selection==OriginalEventSelection::Rewards;
 const bool potion_reward=world_choice_is_potion_reward();
 if(potion_reward&&(key==Key::EndTurn||key==Key::Back)){choice_selected_count=0;confirm_world_choice();return true;}
 const bool card_reward=world_choice_is_card_reward();const int columns=rewards?1:card_reward?0:5;
 if(key==Key::Left&&count)cursor=(cursor+int(count)-1)%int(count);
 else if(key==Key::Right&&count)cursor=(cursor+1)%int(count);
 else if(key==Key::Up&&count)cursor=cursor>=columns?cursor-columns:cursor;
 else if(key==Key::Down&&count)cursor=cursor+columns<int(count)?cursor+columns:cursor;
 else if(key==Key::Confirm&&count)toggle_world_choice(ids[cursor]);
 else if(key==Key::EndTurn&&rewards)ui_action(UiAction::LeaveWorldRewards);
 else if(key==Key::EndTurn||(key==Key::Back&&card_reward&&world_choice_min()==0)){if(card_reward)choice_selected_count=0;confirm_world_choice();}
 else if(key==Key::Detail&&count)world_choice_detail(ids[cursor]);
 else return false;
 return true;
}
