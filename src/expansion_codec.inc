// Appended after the frozen v3-v10 prefix. Save v11 dimensions are explicit.
 if(version>=11){
  for(unsigned i=0;i<s.effect_count;i++)F(s.effects[i].aux);
  auto&x=s.expansion;static_assert(XPowerCount==22,"v11 power width is frozen");
  static_assert((int)XOp::OrbsEnd==52&&(int)XOp::BoulderTick==53,"v11 operation IDs are frozen");
  for(int i=0;i<22;i++)F(x.powers[i]);
  for(int i=0;i<16;i++){F(x.automation[i]);F(x.panache[i]);F(x.boulder[i]);F(x.automation_count[i]);F(x.panache_count[i]);F(x.panache_skip[i]);}
  for(int i=0;i<512;i++){F(x.turn_cost[i]);F(x.combat_cost[i]);F(x.replay[i]);F(x.return_turn[i]);F(x.blade_damage[i]);}
  for(int i=0;i<2;i++){F(x.restore_strength[i]);F(x.demise[i]);F(x.beetle[i]);F(x.doom[i]);}
  F(x.choices.count);if(x.choices.count>MaxCards){c.good=false;return;}
  for(unsigned i=0;i<x.choices.count;i++)F(x.choices.ids[i]);
  for(auto&o:x.offers){F(o.kind);F(o.upgraded);}
  F(x.choice);F(x.choice_min);F(x.choice_max);F(x.choice_free);F(x.depth);
  if(x.depth>MaxAutoFrames){c.good=false;return;}
  for(unsigned i=0;i<x.depth;i++){auto&f=x.frames[i];F(f.playing);F(f.target);F(f.x);F(f.last_damage);F(f.vigor);F(f.remaining);F(f.double_attack);F(f.triple);F(f.returning);}
  F(x.target);F(x.x);F(x.last_damage);F(x.last_drawn);F(x.remaining);F(x.cards_finished);F(x.nonpower_started);
  F(x.triple);F(x.returning);F(x.enemy_side);F(x.potion_active);F(x.skip_enemy);
  for(int i=0;i<16;i++){F(x.bomb_damage[i]);F(x.bomb_turns[i]);}
  F(x.stars);F(x.osty_hp);F(x.osty_max_hp);F(x.orb_slots);F(x.orb_count);for(int i=0;i<10;i++)F(x.dark[i]);
 }else if(c.in){
  if(s.phase==Phase::ExtendedChoice||s.deck_count>64||s.card_count>192||s.effect_count>96)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=68)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=68)c.good=false;
  for(auto k:s.shop_cards)if(k>=68)c.good=false;for(auto k:s.reward)if(k>=68)c.good=false;
  for(auto p:s.potions)if((unsigned)p>16)c.good=false;
  for(auto p:s.shop_potions)if(p>16)c.good=false;if((unsigned)s.reward_potion>16)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++)if((unsigned)s.effects[i].kind>(unsigned)EffectKind::EscapeSkillBlock)c.good=false;
 }

 // v12 appends the acquisition boundary without altering any v11 field.
 if(version>=12){F(s.expansion.entropy_boulder_prefix);}
 else if(c.in&&version==11){
  // Old writers did not record acquisition order. Preserve their previous
  // all-Boulders-before-Entropy policy instead of inventing missing history.
  auto&x=s.expansion;
  if(x.powers[(int)XPower::Entropy])for(auto n:x.boulder)if(n)++x.entropy_boulder_prefix;
  for(unsigned i=0;i<s.effect_count;i++)if(s.effects[i].kind==EffectKind::Extended&&
      s.effects[i].target>=(int)XOp::BoulderTick)c.good=false;
 }
