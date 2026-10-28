 // Save v14 appends Defect facts after every previous version's frozen prefix.
 {
  auto&v=s.defect;static_assert(DPowerCount==24&&MaxCards==512&&MaxDeck==192&&MaxOrbs==10&&MaxAutoFrames==12,"v14 Defect dimensions are frozen");
  for(int i=0;i<24;i++)F(v.powers[i]);
  for(int i=0;i<24;i++)F(v.power_order[i]);F(v.power_count);
  for(int i=0;i<10;i++)F(v.orb_kind[i]);
  F(v.lightning_channeled);F(v.physical_started);F(v.cards_finished);
  F(v.energy_spent_turn);F(v.zero_attacks_started);F(v.feral_used);F(v.status_drawn);F(v.temporary_focus);
  for(int id=0;id<512;id++){F(v.value[id]);F(v.cost_delta[id]);F(v.turn_origin[id]);F(v.combat_origin[id]);F(v.deck_origin[id]);}
  for(int id=0;id<192;id++)F(v.genetic_deck[id]);
  for(int f=0;f<13;f++){
   F(v.storm_before[f]);F(v.subroutine_before[f]);F(v.spent[f]);F(v.scrape_waiting[f]);F(v.scrape_count[f]);
   for(int i=0;i<10;i++)F(v.scrape_drawn[f][i]);
  }
 }
