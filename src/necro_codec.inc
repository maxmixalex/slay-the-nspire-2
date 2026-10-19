 // v14 appends these fields without changing the v13 prefix.
 {
  auto&v=s.necro;
  static_assert(NPowerCount==22&&NDebuffCount==20&&NTempStrengthCount==7&&MaxCards==512&&MaxDeck==192&&MaxEnemies==2&&MaxAutoFrames==12,"v14 Necrobinder save dimensions are frozen");
  for(int p=0;p<22;p++)F(v.powers[p]);
  for(int p=0;p<22;p++)F(v.power_order[p]);F(v.power_count);
  F(v.drawn_turn);F(v.ethereal_played);F(v.osty_attacks_turn);F(v.attacks_started_turn);F(v.creature_deaths);
  F(v.player_doom);F(v.doom_applied_turn);F(v.enemy_dead_mask);F(v.osty_was_alive);
  for(int id=0;id<512;id++){
   F(v.cost_delta[id]);F(v.turn_origin[id]);F(v.combat_origin[id]);F(v.ethereal[id]);F(v.fetch_played[id]);F(v.flatten_zero[id]);F(v.flatten_origin[id]);
   F(v.scythe_value[id]);F(v.deck_origin[id]);
  }
  for(int t=0;t<2;t++){
   F(v.debilitate[t]);F(v.hang[t]);F(v.oblivion[t]);F(v.sic_em[t]);
   for(int p=0;p<20;p++)F(v.debuff_order[t][p]);F(v.debuff_count[t]);
   for(int p=0;p<7;p++)F(v.temp_strength[t][p]);
  }
  for(int f=0;f<13;f++){
   for(int t=0;t<2;t++)F(v.oblivion_before[f][t]);
   F(v.attack_total[f]);F(v.play_index[f]);F(v.resource_energy[f]);F(v.first_attack[f]);
  }
  for(int i=0;i<192;i++)F(v.scythe_deck[i]);F(v.grimoire_pending);
 }
