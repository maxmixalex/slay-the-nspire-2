 // Save v13 appends explicit Silent fields, retaining every older prefix byte.
 {
  auto&v=s.silent;static_assert(SPowerCount==22&&MaxNightmares==16,"v13 Silent dimensions are frozen");
  for(int n=0;n<22;n++)F(v.powers[n]);
  for(int n=0;n<38;n++)F(v.power_order[n]);F(v.power_count);
  F(v.attacks_finished);F(v.skills_finished);F(v.discarded_turn);F(v.drawn_combat);F(v.shiv_finished);
  for(int n=0;n<512;n++){
   F(v.sly[n]);F(v.retain[n]);F(v.inky[n]);F(v.bullet_free[n]);
   F(v.discount[n]);F(v.turn_origin[n]);F(v.combat_origin[n]);F(v.bullet_origin[n]);
  }
  for(int t=0;t<2;t++){F(v.strangle[t]);F(v.enemy_artifact[t]);}
  for(int f=0;f<13;f++){
   F(v.afterimage_before[f]);F(v.serpent_before[f]);for(int t=0;t<2;t++)F(v.strangle_before[f][t]);
   F(v.retain_waiting[f]);F(v.retain_drawn[f]);
  }
  for(int slot=0;slot<16;slot++){
   F(v.nightmare_active[slot]);auto&n=v.nightmares[slot];
   F(n.card.kind);F(n.card.upgraded);F(n.turn_cost);F(n.combat_cost);F(n.replay);F(n.return_turn);F(n.blade_damage);
   F(n.sly);F(n.retain);F(n.inky);F(n.bullet_free);F(n.discount);F(n.turn_origin);F(n.combat_origin);F(n.bullet_origin);
   F(n.iron_damage);F(n.stomp_discount);F(n.stomp_turn_origin);F(n.stomp_combat_origin);
  }
  F(v.extra_rewards);
 }
