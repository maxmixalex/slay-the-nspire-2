bool valid_world_relic_state(const State&s){
 const auto&w=s.world_relics;
 if(w.active>1||w.bags_ready>1||w.count>WorldRelicCapacity||w.step_count>MaxWorldRelicSteps||
    w.step_cursor>w.step_count||w.running>1||unsigned(w.choice)>=unsigned(WorldRelicChoice::Count)||
    w.return_phase>unsigned(Phase::OriginalEventChoice)||w.choice_cancelable>1||
    w.choice_min>w.choice_max||w.choice_max>MaxDeck||unsigned(w.choice_enchantment)>=EventEnchantmentKinds||
    w.choice_amount<0||w.choice_amount>9999||w.offer_count>MaxWorldRelicOffers||
    w.selected_count>MaxDeck||w.reward_count>MaxWorldRelicRewards||w.group_count>MaxWorldRelicRewardGroups||
    w.choice_group>w.group_count||w.choice_reward>w.reward_count||w.prepared_count>16||w.tooth_count>5||
    w.byrdpip_present>1||w.legion_present>1||(w.last_flash!=NoWorldRelic&&w.last_flash>=WorldRelicKinds))return false;
 if(!w.active&&(w.count||w.bags_ready||w.step_count||w.choice!=WorldRelicChoice::None))return false;
 if((w.choice!=WorldRelicChoice::None)!=(s.phase==Phase::WorldRelicChoice))return false;
 auto token_valid=[](unsigned token,const EventCardFacts& facts){
  Card card=card_from_token(int(token));return card_token(card)==int(token)&&card.kind<CardKinds&&
   card.upgraded<=1&&(!card.upgraded||card_upgradeable(card))&&event_card_facts_valid(card,facts);
 };
 for(unsigned i=0;i<w.count;i++){
  const auto&r=w.entries[i];
  if(r.id>=WorldRelicKinds||!r.serial||r.serial>w.last_serial||r.flags>7||r.skin>3)return false;
  for(unsigned j=0;j<i;j++)if(w.entries[j].serial==r.serial)return false;
  for(unsigned j=0;j<2;j++)if(!token_valid(r.card_tokens[j],r.card_facts[j]))return false;
  if(r.id==WorldRelicFurCoat){
   if(r.values[0]<-1||r.values[0]>=int(WorldActCount)||r.values[1]<0||r.values[1]>1||r.values[2]<0||r.values[2]>8)return false;
   for(int p=0;p<r.values[2];p++)if(r.marked_points[p]>=reference::MaxPoints)return false;
  }
 }
 for(unsigned b=0;b<2;b++)for(unsigned r=0;r<4;r++){
  if(w.bag_sizes[b][r]>WorldRelicBagSize)return false;
  for(unsigned i=0;i<w.bag_sizes[b][r];i++){
   RelicId id=w.bags[b][r][i];if(id>=WorldRelicKinds||world_relic_definitions[id].rarity!=r+2)return false;
   for(unsigned j=0;j<i;j++)if(w.bags[b][r][j]==id)return false;
  }
 }
 for(unsigned i=0;i<w.step_count;i++){
  const auto&v=w.steps[i];if(unsigned(v.op)>=unsigned(WorldRelicOp::Count)||v.owner>w.last_serial)return false;
 }
 if(w.choice_owner>w.last_serial)return false;
 if(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::OfferBundle)
  for(unsigned i=0;i<w.offer_count;i++)if(!token_valid(w.offers[i],w.offer_facts[i]))return false;
 if(w.choice==WorldRelicChoice::OfferBundle&&w.offer_count%3)return false;
 if(w.choice==WorldRelicChoice::OfferReward)for(unsigned i=0;i<w.offer_count;i++){
  unsigned at=w.offers[i];if(at>=w.reward_count||w.rewards[at].group!=w.choice_group||(w.rewards[at].flags&1))return false;
 }
 if(w.choice==WorldRelicChoice::Hand)for(unsigned i=0;i<w.offer_count;i++)if(w.offers[i]>=s.card_count)return false;
 if(w.choice==WorldRelicChoice::Potion&&(w.offer_count!=1||!w.offers[0]||w.offers[0]>PotionKinds))return false;
 for(unsigned i=0;i<w.selected_count;i++){
  if(w.selected[i]>=MaxDeck)return false;for(unsigned j=0;j<i;j++)if(w.selected[j]==w.selected[i])return false;
 }
 for(unsigned g=0;g<MaxWorldRelicRewardGroups;g++){
  if(w.group_active[g]>1||w.group_skip[g]>1||w.group_owner[g]>w.last_serial)return false;
  if(g>=w.group_count&&w.group_active[g])return false;
 }
 for(unsigned i=0;i<w.reward_count;i++){
  const auto&r=w.rewards[i];if(!r.group)continue;
  if(r.group>w.group_count||!w.group_active[r.group-1]||r.kind<1||r.kind>3||r.count>8||r.pool_count>11||
     r.odds>unsigned(WorldCardOdds::Uniform)||r.source>unsigned(WorldCardSource::Other))return false;
  if(r.kind==2&&r.value>=WorldRelicKinds)return false;
  if(r.kind==3&&(!r.value||r.value>PotionKinds))return false;
  for(unsigned j=0;j<r.pool_count;j++)if(r.pools[j]>unsigned(WorldCardPool::Own))return false;
  if(r.kind==1)for(unsigned j=0;j<r.count;j++)if(!token_valid(r.tokens[j],r.facts[j]))return false;
 }
 for(unsigned i=0;i<w.prepared_count;i++){
  if(w.prepared[i].id>=WorldRelicKinds)return false;
  for(unsigned j=0;j<i;j++)if(w.prepared[j].id==w.prepared[i].id)return false;
 }
 for(unsigned i=0;i<w.tooth_count;i++)if(!token_valid(w.tooth_cards[i].token,w.tooth_cards[i].facts))return false;
 for(const auto&f:w.hook_frames){
  if(f.active>1||unsigned(f.hook)>=unsigned(WorldRelicHook::Count)||f.count>WorldRelicCapacity||f.cursor>f.count)return false;
  if(!f.active&&(f.count||f.cursor))return false;
  for(unsigned i=0;i<f.count;i++){
   if(!f.serials[i]||f.serials[i]>w.last_serial)return false;
   for(unsigned j=0;j<i;j++)if(f.serials[j]==f.serials[i])return false;
  }
 }
 return true;
}
