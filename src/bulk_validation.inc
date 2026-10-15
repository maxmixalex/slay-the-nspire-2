static bool validate_bulk(const State&s){
 const auto&b=s.bulk;
 if(s.potion_slots<2||s.potion_slots>MaxPotionSlots)return false;
 for(unsigned i=s.potion_slots;i<MaxPotionSlots;i++)if(s.potions[i]!=Potion::Empty)return false;
 if(b.nonpower_started>9999||b.nostalgia_return>1)return false;
 for(int i=0;i<MaxPowerInstances;i++)if(b.automation_instances[i]<0||b.automation_instances[i]>9999||b.panache_instances[i]<0||b.panache_instances[i]>9999||b.boulder_instances[i]<0||b.boulder_instances[i]>9999||b.automation_counts[i]>9||b.panache_counts[i]>4||b.panache_skip[i]>1)return false;
 int auto_sum=0,panache_sum=0,boulder_sum=0;
 for(int i=0;i<MaxPowerInstances;i++){
  auto_sum+=b.automation_instances[i];panache_sum+=b.panache_instances[i];boulder_sum+=b.boulder_instances[i];
  if(!b.automation_instances[i]&&b.automation_counts[i])return false;
  if(!b.panache_instances[i]&&(b.panache_counts[i]||b.panache_skip[i]))return false;
 }
 if(b.powers[(int)BulkPower::Automation]!=min(9999,auto_sum)||b.powers[(int)BulkPower::Panache]!=min(9999,panache_sum)||b.powers[(int)BulkPower::RollingBoulder]!=min(9999,boulder_sum))return false;
 for(auto p:b.powers)if(p<0||p>9999)return false;
 for(unsigned i=0;i<MaxCards;i++){
  if(b.turn_cost[i]>100||b.combat_cost[i]>100)return false;
  if(i>=s.card_count&&(b.turn_cost[i]||b.combat_cost[i]||b.replay[i]||b.return_turn[i]))return false;
  if(b.return_turn[i]&&b.return_turn[i]<(unsigned)s.turn)return false;
 }
 if(b.depth>MaxAutoDepth||b.triple_attack>1||b.debuff_double>1||b.enemy_turn>1||b.potion_active>1||b.tea_ready>1||b.setup>1)return false;
 if(b.spent<0||b.spent>999||b.x_value<0||b.x_value>1001||b.target< -1||b.target>=MaxEnemies||b.last_damage<0||b.last_damage>9999)return false;
 if(b.last_drawn< -1||b.last_drawn>=s.card_count||b.choices.count>MaxCards)return false;
 if(b.cards_finished>9999||b.cards_drawn>9999||b.turn_cards>9999||b.previous_turn_cards>9999||b.hp_lost_turn>9999||b.hp_lost_combat>9999||b.panache_count>4||b.automation_count>9)return false;
 if(b.boulder_increment<0||b.boulder_increment>9999||b.dead_mask>=(1u<<MaxEnemies))return false;
 for(auto v:b.reward_upgraded)if(v>1)return false;
 for(int i=0;i<MaxBombs;i++)if(b.bomb_turns[i]>3||b.bomb_damage[i]<0||b.bomb_damage[i]>9999||(!b.bomb_turns[i]&&b.bomb_damage[i]))return false;
 for(int i=0;i<MaxEnemies;i++)if(b.enemy_restore[i]<0||b.enemy_restore[i]>9999||b.enemy_demise[i]<0||b.enemy_demise[i]>9999||b.enemy_beetle[i]<0||b.enemy_beetle[i]>9999)return false;
 for(unsigned i=0;i<b.depth;i++){
  const auto&f=b.frames[i];
  if(f.playing< -1||f.playing>=s.card_count||f.spent<0||f.spent>999||f.x_value<0||f.x_value>1001||f.vigor<0||f.vigor>9999||f.target< -1||f.target>=MaxEnemies||f.last_damage<0||f.last_damage>9999||f.double_attack>1||f.triple_attack>1||f.debuff_double>1||f.nostalgia_return>1)return false;
 }
 if((unsigned)b.choice>=(unsigned)BulkChoice::Count)return false;
 if(s.phase==Phase::BulkChoice){
  if(b.choice==BulkChoice::None||!b.choices.count||b.choice_min>b.choice_max||b.choice_max>MaxHand||b.choice_max>b.choices.count||b.choice_free>1||b.choice_upgraded>1)return false;
  for(unsigned i=0;i<b.choices.count;i++){
   int id=b.choices.ids[i];
   for(unsigned j=0;j<i;j++)if(b.choices.ids[j]==id)return false;
   if(b.choice==BulkChoice::Offer){if(id<MaxCards||id>=MaxCards+3||b.offers[id-MaxCards].kind>=CardKinds||b.offers[id-MaxCards].upgraded>1)return false;}
   else{
    if(id<0||id>=s.card_count)return false;
    Zone expected=b.choice==BulkChoice::DrawToHand?Zone::Draw:b.choice==BulkChoice::DiscardToHand?Zone::Discard:Zone::Hand;
    if(s.zones[id]!=expected)return false;
   }
  }
 }else if(b.choice!=BulkChoice::None||b.choices.count||b.choice_min||b.choice_max||b.choice_free||b.choice_upgraded)return false;
 return true;
}
