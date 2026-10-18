 // Save v13 module dimensions are explicit; no raw struct/padding is written.
 {
  auto&i=s.ironclad;static_assert(IPowerCount==18,"v13 Ironclad power width is frozen");
  for(int n=0;n<18;n++)F(i.powers[n]);
  for(int n=0;n<18;n++)F(i.power_order[n]);F(i.power_count);
  F(i.attacks_started);F(i.attacks_finished);F(i.hp_losses_turn);F(i.hp_losses_combat);F(i.exhausted_turn);F(i.block_events_turn);
  for(int n=0;n<13;n++){F(i.block_events_play[n]);F(i.rupture_pending[n]);F(i.rupture_registered[n]);F(i.pillage_drawn[n]);F(i.pillage_waiting[n]);}
  for(int n=0;n<512;n++){F(i.damage_bonus[n]);F(i.stomp_discount[n]);F(i.stomp_turn_origin[n]);F(i.stomp_combat_origin[n]);F(i.forced_exhaust[n]);}
  F(i.setup_strength);F(i.crimson_self_damage);F(i.inferno_self_damage);
 }
