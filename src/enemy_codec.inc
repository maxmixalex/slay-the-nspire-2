// Original encounter state is a v15 suffix. Keep the historical two-enemy
// prefix in state_codec.inc unchanged; every suspended command lives in the
// ordinary effect queue and stores only stable actor/card identifiers.
{
 auto&w=s.enemy_world;
 B(w.active);B(w.ascension);B(w.side);B(w.actor);B(w.timed_out);
 H(w.next_creature_id);B(w.slot_limit);B(w.turn_count);B(w.encounter);
 for(unsigned i=0;i<MaxEnemies;++i){B(w.turn_order[i]);H(w.turn_ids[i]);}
 for(auto&v:w.composition_rng.words)field64(v,c);
 F(w.composition_rng.calls);B(w.composition_rng.good);
 for(auto&m:w.models){
  B(m.kind);for(auto&v:m.rng.words)field64(v,c);F(m.rng.calls);B(m.rng.good);
  H(m.combat_id);B(m.flags);B(m.node);B(m.previous);B(m.consecutive);
  B(m.owner);B(m.slot);B(m.resume_node);B(m.spawned_this_side);H(m.age);
  for(auto&v:m.seen_moves)H(v);for(auto&v:m.history)B(v);B(m.history_count);
  for(auto&v:m.data)H(v);for(auto&v:m.power)H(v);for(auto&v:m.power_aux)H(v);
  for(auto&v:m.power_order)B(v);B(m.power_count);
  for(auto&v:m.duration_skip)B(v);for(auto&v:m.source)B(v);
  H(m.captured_card);H(m.stolen_gold);H(m.attack_unblocked_hits);
  H(m.attack_total_damage);H(m.attack_vigor);
  H(m.captured_deck.card.kind);B(m.captured_deck.card.upgraded);
  B(m.captured_deck.facts.enchantment);B(m.captured_deck.facts.science_type);B(m.captured_deck.facts.rider);
  H(m.captured_deck.facts.enchantment_amount);H(m.captured_deck.facts.energy_offset);H(m.captured_deck.facts.progress);
  H(m.captured_deck.genetic);H(m.captured_deck.scythe);
 }
 for(auto&v:w.player_power)H(v);for(auto&v:w.player_aux)H(v);
 for(auto&v:w.player_power_order)B(v);B(w.player_power_count);
 for(auto&v:w.player_duration_skip)B(v);for(auto&v:w.player_source)B(v);
 for(unsigned i=0;i<MaxCards;++i){B(w.affliction[i]);B(w.dampen_restore[i]);H(w.affliction_amount[i]);H(w.card_value[i]);}
 for(auto&v:w.spawned_models)B(v);
 B(w.dampen_casters);B(w.bound_played);B(w.used_types);
 H(w.escaped_count);H(w.spawned_model_count);H(w.cards_played_turn);H(w.bound_draws_turn);
 B(w.facing);B(w.choice_enemy);B(w.choice_stage);
 for(auto&v:w.turn_damage)H(v);for(auto&v:w.turn_attacks)H(v);
}
