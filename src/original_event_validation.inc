// Validate all saved bounds before UI or queue code can index their arrays.
bool valid_original_event_state(const State&s){const auto&e=s.original_events;
 if(unsigned(e.id)>=unsigned(OriginalEventId::Count)||e.active>1||e.finished>1||e.act_index>2||e.option_count>MaxOriginalEventOptions)return false;
 if(e.active&&e.id==OriginalEventId::None)return false;
 if(!terminated(e.page_key,sizeof e.page_key))return false;
 for(const auto&key:e.option_keys)if(!terminated(key,sizeof key))return false;
 if(e.step_count>MaxOriginalEventSteps||e.step_cursor>e.step_count)return false;
 for(const auto&step:e.steps){if(unsigned(step.op)>=unsigned(OriginalEventOp::Count))return false;
  if(step.op==OriginalEventOp::MaxHp&&(step.a< -9999||step.a>9999))return false;
  if(step.op==OriginalEventOp::KillPlayer&&(step.a<0||step.a>1))return false;
  if(step.op==OriginalEventOp::ProcurePotion&&(step.a<=0||step.a>PotionKinds||step.b<0||step.b>=int(MaxPotionSlots)||step.c<0||step.c>1))return false;
 }
 if(unsigned(e.selection)>=unsigned(OriginalEventSelection::Count)||unsigned(e.filter)>=unsigned(OriginalEventFilter::Count))return false;
 if(e.select_min>e.select_max||e.select_max>MaxDeck||e.select_cancelable>1||e.select_enchantment>=EventEnchantmentKinds||e.select_amount<0||e.select_amount>9999||e.select_extra<0||e.select_extra>CardKinds)return false;
 if(e.offer_count>MaxOriginalEventOffers||e.selected_count>MaxDeck||e.selected_cursor>e.selected_count||e.selection_committed>1)return false;
 if(e.selection==OriginalEventSelection::None&&(e.selection_committed||e.selected_count||e.selected_cursor))return false;
 if(e.selection_committed&&(e.selected_count<e.select_min||e.selected_count>e.select_max))return false;
 if(!e.selection_committed&&e.selected_cursor)return false;
 if(s.phase==Phase::OriginalEventChoice&&(!e.active||e.selection==OriginalEventSelection::None||e.selection_committed))return false;
 if(e.selection!=OriginalEventSelection::None&&!e.active)return false;
 for(unsigned i=0;i<e.selected_count;i++){
  if(e.selection==OriginalEventSelection::OfferCard||e.selection==OriginalEventSelection::Relic){if(e.selected[i]>=e.offer_count)return false;}
  else if(e.selected[i]>=MaxDeck)return false;
  for(unsigned j=0;j<i;j++)if(e.selected[i]==e.selected[j])return false;
 }
 for(const auto&snapshot:e.selected_cards){
  if(snapshot.card.kind>=CardKinds||snapshot.card.upgraded>1||!event_card_facts_valid(snapshot.card,snapshot.facts)||snapshot.genetic>9999||snapshot.scythe>9999)return false;
 }
 for(unsigned i=0;i<e.offer_count;i++){
  if(e.selection==OriginalEventSelection::OfferCard){Card card=card_from_token(e.offers[i]);if(card.kind>=CardKinds||card_token(card)!=e.offers[i]||!event_card_facts_valid(card,e.offer_facts[i]))return false;}
  else if(e.selection==OriginalEventSelection::Relic&&e.offers[i]>=WorldRelicKinds)return false;
  else if(e.selection==OriginalEventSelection::Potion&&(e.offers[i]==0||e.offers[i]>PotionKinds))return false;
 }
 for(auto seen:e.remembered)if(seen>1)return false;
 for(auto price:e.choice_prices)if(price<0||price>9999)return false;
 if(e.combat_pending>1||e.resume_after_combat>1||e.encounter>=unsigned(EncounterId::Count))return false;
 if(e.combat_pending&&(!e.active||!e.encounter||e.selection!=OriginalEventSelection::None))return false;
 if(!e.combat_pending&&(e.resume_after_combat||e.encounter))return false;
 if(e.reward_active>1||e.reward_count>MaxOriginalEventRewards||e.reward_card_count>MaxOriginalEventRewardCards||(e.reward_current!=NoOriginalEventReward&&e.reward_current>=e.reward_count))return false;
 if(e.selection==OriginalEventSelection::Rewards&&!e.reward_active)return false;
 for(unsigned i=0;i<e.reward_count;i++){const auto&r=e.rewards[i];if(r.kind<1||r.kind>4||r.claimed>1)return false;
  if(r.kind==2&&(r.model==0||r.model>PotionKinds))return false;if(r.kind==3&&r.model>=WorldRelicKinds)return false;
  if(r.kind==4&&(!r.count||r.count>MaxOriginalEventOffers||r.first+r.count>e.reward_card_count))return false;
  if(r.flags>31||r.original_count>MaxOriginalEventOffers||r.zero_energy>1||r.pool_count>11||r.odds>unsigned(WorldCardOdds::Uniform)||r.source>unsigned(WorldCardSource::Other))return false;
  if((r.flags&1)&&(r.kind!=4||!r.original_count||!r.pool_count))return false;
  if((r.flags&4)&&(r.flags&8))return false;
  for(unsigned j=0;j<r.pool_count;j++)if(r.pools[j]>unsigned(WorldCardPool::Own))return false;
 }
 for(unsigned i=0;i<e.reward_card_count;i++){Card card=card_from_token(e.reward_cards[i]);if(card.kind>=CardKinds||card_token(card)!=e.reward_cards[i]||!event_card_facts_valid(card,e.reward_facts[i]))return false;}
 if(e.crystal_tool<1||e.crystal_tool>2||e.crystal_item_count>MaxCrystalItems||e.crystal_revealed_count>MaxCrystalReveals)return false;
 for(unsigned i=0;i<CrystalCells;i++)if(e.crystal_clear[i]>1||e.crystal_owner[i]>e.crystal_item_count)return false;
 for(unsigned i=0;i<e.crystal_item_count;i++){
  const auto&item=e.crystal_items[i];
  if(item.x>=11||item.y>=11||item.w==0||item.h==0||item.w>11-item.x||item.h>11-item.y||item.revealed>1||item.kind>4||item.subscriptions>10)return false;
 }
 for(unsigned i=0;i<e.crystal_revealed_count;i++){
  if(e.crystal_revealed[i]>=e.crystal_item_count)return false;
 }
 if(s.event_run.freed_repy>1||s.event_run.completion_recorded>1)return false;
 return true;
}
