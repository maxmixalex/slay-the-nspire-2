 // Save v14 Regent suffix. F is the shared four-byte integer codec.
 {
  auto&r=s.regent;
  static_assert(RPowerCount==22&&MaxCards==512&&MaxPowerInstances==16&&MaxAutoFrames==12&&LegacyEnemies==2,"v14 Regent dimensions are frozen");
  for(int p=0;p<22;p++)F(r.powers[p]);
  for(int p=0;p<22;p++)F(r.power_order[p]);F(r.power_count);
  F(r.cards_created);F(r.stars_gained_turn);F(r.cards_finished_turn);F(r.skills_finished_turn);
  for(int t=0;t<2;t++)F(r.hits_turn[t]);F(r.void_played_turn);
  for(int id=0;id<512;id++){F(r.kick_discount[id]);F(r.kick_turn_origin[id]);F(r.kick_combat_origin[id]);F(r.punch_damage[id]);}
  for(int p=0;p<16;p++){
   F(r.orbit[p]);F(r.monologue[p]);F(r.monologue_applied[p]);F(r.orbit_remainder[p]);
   for(int f=0;f<13;f++)F(r.monologue_capture[p][f]);
  }
  for(int f=0;f<13;f++){F(r.stars_spent[f]);F(r.star_x[f]);F(r.automatic[f]);}
  for(int t=0;t<2;t++)F(r.conqueror[t]);F(r.pale_activated);F(r.end_turn_requested);
 }
