// v11 dimensions are frozen. Append a new version rather than silently changing
// these loops when the content registry grows in future.
 if(version>=11){
  for(int i=1;i<10;i++){F(s.relics.words[i]);F(s.relic_flash.words[i]);}
  for(int i=30;i<116;i++){F(s.relic_order[i]);F(s.relic_counters[i]);}
  F(s.potion_slots);F(s.potions[2]);F(s.potions[3]);
  for(unsigned i=0;i<s.effect_count;i++)F(s.effects[i].aux);
  auto&b=s.bulk;
  static_assert(BulkPowerKinds==21,"v11 power width is frozen");
  for(int i=0;i<21;i++)F(b.powers[i]);
  for(int i=0;i<16;i++){F(b.automation_instances[i]);F(b.panache_instances[i]);F(b.boulder_instances[i]);F(b.automation_counts[i]);F(b.panache_counts[i]);F(b.panache_skip[i]);}
  F(b.nonpower_started);F(b.nostalgia_return);
  for(int i=0;i<512;i++){F(b.turn_cost[i]);F(b.combat_cost[i]);F(b.replay[i]);F(b.return_turn[i]);}
  for(int i=0;i<2;i++){F(b.enemy_restore[i]);F(b.enemy_demise[i]);F(b.enemy_beetle[i]);}
  F(b.choices.count);if(b.choices.count>MaxCards){c.good=false;return;}
  for(unsigned i=0;i<b.choices.count;i++)F(b.choices.ids[i]);
  for(auto&x:b.offers){F(x.kind);F(x.upgraded);}
  F(b.choice);F(b.choice_min);F(b.choice_max);F(b.choice_free);F(b.choice_upgraded);
  F(b.depth);if(b.depth>MaxAutoDepth){c.good=false;return;}
  for(unsigned i=0;i<b.depth;i++){
   auto&f=b.frames[i];F(f.playing);F(f.spent);F(f.x_value);F(f.vigor);F(f.target);F(f.last_damage);F(f.repeat_remaining);F(f.double_attack);F(f.triple_attack);F(f.debuff_double);F(f.nostalgia_return);
  }
  F(b.triple_attack);F(b.debuff_double);F(b.enemy_turn);F(b.potion_active);
  F(b.spent);F(b.x_value);F(b.target);F(b.last_drawn);F(b.last_damage);F(b.repeat_remaining);
  F(b.cards_finished);F(b.cards_drawn);F(b.turn_cards);F(b.previous_turn_cards);
  F(b.panache_count);F(b.automation_count);F(b.hp_lost_turn);F(b.hp_lost_combat);
  F(b.nostalgia_used);F(b.skip_enemy);F(b.dead_mask);F(b.tea_ready);F(b.setup);F(b.boulder_increment);
  for(int i=0;i<16;i++){F(b.bomb_damage[i]);F(b.bomb_turns[i]);}
  for(auto&x:b.reward_upgraded)F(x);
  for(int i=0;i<116;i++){F(b.turn_flags[i]);F(b.combat_flags[i]);}
 }else if(c.in){
  s.potion_slots=2;
  if(s.phase==Phase::BulkChoice||s.relic_count>30||!s.relics.valid(30)||!s.relic_flash.valid(30)||s.deck_count>64||s.card_count>192||s.effect_count>96)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=68)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=68)c.good=false;
  for(auto p:s.potions)if((unsigned)p>16)c.good=false;
  for(auto p:s.shop_potions)if(p>16)c.good=false;
  if((unsigned)s.reward_potion>16)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++)if((unsigned)s.effects[i].kind>(unsigned)EffectKind::EscapeSkillBlock)c.good=false;
 }
