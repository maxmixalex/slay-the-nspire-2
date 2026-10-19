static bool validate_extension(const State&s){
 const auto&x=s.expansion;
 bool after_seen[(unsigned)PostPlayActor::Count]{};bool after_gap=false;
 for(unsigned actor:x.after_play_order){
  if(!actor){after_gap=true;continue;}
  if(after_gap||actor>=(unsigned)PostPlayActor::Count||
     (actor>=(unsigned)PostPlayActor::Relics&&actor<=(unsigned)PostPlayActor::Enemies)||after_seen[actor]||!post_play_actor_active(s,actor))return false;
  after_seen[actor]=true;
 }
 for(unsigned actor=1;actor<(unsigned)PostPlayActor::Count;actor++){
  if(actor>=(unsigned)PostPlayActor::Relics&&actor<=(unsigned)PostPlayActor::Enemies)continue;
  if(after_seen[actor]!=post_play_actor_active(s,actor))return false;
 }
 if(x.depth>MaxAutoFrames||x.triple>1||x.returning>1||x.enemy_side>1||x.potion_active>1)return false;
 if(x.target< -1||x.target>=MaxEnemies||x.x<0||x.x>9999||x.last_damage<0||x.last_damage>9999||x.last_drawn< -1||x.last_drawn>=s.card_count)return false;
 if(x.cards_finished>9999||x.nonpower_started>9999||x.choices.count>MaxCards)return false;
 for(unsigned p=0;p<XPowerCount;p++)if(x.powers[p]<(p==(unsigned)XPower::Focus?-9999:0)||x.powers[p]>9999)return false;
 int auto_sum=0,pan_sum=0,boulder_sum=0,boulder_count=0;
 bool boulder_gap=false;
 for(int i=0;i<MaxPowerInstances;i++){
  if(x.automation[i]<0||x.automation[i]>9999||x.panache[i]<0||x.panache[i]>9999||x.boulder[i]<0||x.boulder[i]>9999||x.automation_count[i]>9||x.panache_count[i]>4||x.panache_skip[i]>1)return false;
  if(!x.automation[i]&&x.automation_count[i])return false;
  if(!x.panache[i]&&(x.panache_count[i]||x.panache_skip[i]))return false;
  if(x.boulder[i]){if(boulder_gap)return false;++boulder_count;}else boulder_gap=true;
  auto_sum+=x.automation[i];pan_sum+=x.panache[i];boulder_sum+=x.boulder[i];
 }
 if(x.entropy_boulder_prefix>boulder_count||(!x.powers[(int)XPower::Entropy]&&x.entropy_boulder_prefix))return false;
 if(x.powers[(int)XPower::Automation]!=min(9999,auto_sum)||x.powers[(int)XPower::Panache]!=min(9999,pan_sum)||x.powers[(int)XPower::RollingBoulder]!=min(9999,boulder_sum))return false;
 for(unsigned i=0;i<MaxCards;i++){
  if(x.star_turn_free[i]>1||x.star_combat_free[i]>1||(i>=s.card_count&&(x.star_turn_free[i]||x.star_combat_free[i])))return false;
  if(x.turn_cost[i]>100||x.combat_cost[i]>100||x.blade_damage[i]<0||x.blade_damage[i]>9999)return false;
  if(i>=s.card_count&&(x.turn_cost[i]||x.combat_cost[i]||x.replay[i]||x.return_turn[i]||x.blade_damage[i]))return false;
  if(x.return_turn[i]&&(x.return_turn[i]<(unsigned)s.turn||x.return_turn[i]>20001))return false;
  if(i<s.card_count&&x.blade_damage[i]&&s.cards[i].kind!=SovereignBlade)return false;
 }
 for(int i=0;i<MaxEnemies;i++)if(x.restore_strength[i]<0||x.restore_strength[i]>9999||x.demise[i]<0||x.demise[i]>9999||x.beetle[i]<0||x.beetle[i]>9999||x.doom[i]<0||x.doom[i]>9999)return false;
 for(int i=0;i<MaxBombs;i++)if(x.bomb_turns[i]>3||x.bomb_damage[i]<0||x.bomb_damage[i]>9999||(!x.bomb_turns[i]&&x.bomb_damage[i]))return false;
 if(x.stars<0||x.stars>9999||x.osty_hp<0||x.osty_hp>x.osty_max_hp||x.osty_max_hp>9999||x.orb_slots>MaxOrbs||x.orb_count>x.orb_slots)return false;
 for(int i=0;i<MaxOrbs;i++)if(x.dark[i]<0||x.dark[i]>9999||(i>=x.orb_count&&x.dark[i]))return false;
 for(unsigned i=0;i<x.depth;i++){
  const auto&f=x.frames[i];if(f.playing< -1||f.playing>=s.card_count||f.target< -1||f.target>=MaxEnemies||f.x<0||f.x>9999||f.last_damage<0||f.last_damage>9999||f.vigor<0||f.vigor>9999||f.double_attack>1||f.triple>1||f.returning>1)return false;
 }
 if((unsigned)x.choice>=(unsigned)XChoice::Count)return false;
 if(s.phase==Phase::ExtendedChoice){
  if(x.choice==XChoice::None||!x.choices.count||!x.choice_max||x.choice_min>x.choice_max||x.choice_max>MaxHand||x.choice_max>x.choices.count||x.choice_free>1)return false;
  for(unsigned i=0;i<x.choices.count;i++){
   int id=x.choices.ids[i];for(unsigned j=0;j<i;j++)if(id==x.choices.ids[j])return false;
   if(x.choice==XChoice::Offer){if(id<MaxCards||id>=MaxCards+3||x.offers[id-MaxCards].kind>=CardKinds||x.offers[id-MaxCards].upgraded>1)return false;}
   else {if(id<0||id>=s.card_count)return false;
    // Each character module validates its own choice source and operands.
    if((unsigned)x.choice>(unsigned)XChoice::Transform)continue;
    Zone z=x.choice==XChoice::Draw?Zone::Draw:x.choice==XChoice::Discard?Zone::Discard:Zone::Hand;if(s.zones[id]!=z)return false;}
  }
 }else if(x.choice!=XChoice::None||x.choices.count||x.choice_min||x.choice_max||x.choice_free)return false;
 if(!choice_phase(s.phase)&&s.phase!=Phase::Error&&(x.depth||x.potion_active))return false;
 return true;
}
