// v15 suffix. The v3-v14 byte prefix above still has exactly two enemy slots.
{
 B(s.potion_slots);for(unsigned p=LegacyPotionSlots;p<MaxPotionSlots;++p)B(s.potions[p]);
 for(unsigned t=LegacyEnemies;t<MaxEnemies;++t){
  auto&e=s.enemies[t];F(e.hp);F(e.max_hp);H(e.block);H(e.strength);H(e.vulnerable);B(e.move);H(e.weak);H(e.poison);
  auto&x=s.expansion;H(x.restore_strength[t]);H(x.demise[t]);H(x.beetle[t]);H(x.doom[t]);
  auto&v=s.silent;H(v.strangle[t]);H(v.enemy_artifact[t]);
  auto&r=s.regent;H(r.hits_turn[t]);H(r.conqueror[t]);
  auto&n=s.necro;H(n.debilitate[t]);H(n.hang[t]);H(n.oblivion[t]);H(n.sic_em[t]);
  B(n.debuff_count[t]);for(unsigned p=0;p<NDebuffCount;++p)B(n.debuff_order[t][p]);
  for(unsigned p=0;p<NTempStrengthCount;++p)H(n.temp_strength[t][p]);
  for(unsigned f=0;f<=MaxAutoFrames;++f){H(v.strangle_before[f][t]);H(n.oblivion_before[f][t]);}
 }
 auto&w=s.world;
 B(w.active);B(w.rng_ready);B(w.act_index);B(w.ascension);
 H(w.total_floor);H(w.rooms_completed);H(w.combat_count);H(w.map_column);H(w.map_row);
 field64(w.run_seed,c);for(auto&ch:w.seed_text)B(ch);
 for(auto&r:w.rng){for(auto&word:r.words)field64(word,c);F(r.calls);B(r.good);}
}
