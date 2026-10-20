// v15: fixed-width, pointer-free acquisition and suspended hook continuations.
{
 auto&w=s.world_relics;
#define WRF(f) B((f).enchantment);B((f).science_type);B((f).rider);H((f).enchantment_amount);H((f).energy_offset);H((f).progress)
 B(w.active);B(w.bags_ready);H(w.count);H(w.last_serial);
 for(auto&r:w.entries){
  H(r.id);H(r.floor);H(r.serial);B(r.flags);B(r.skin);
  for(auto&v:r.values)F(v);for(auto&v:r.marked_points)B(v);
  for(auto&v:r.card_tokens)H(v);for(auto&f:r.card_facts){WRF(f);}
 }
 for(unsigned b=0;b<2;b++)for(unsigned r=0;r<4;r++){
  H(w.bag_sizes[b][r]);for(auto&id:w.bags[b][r])H(id);
 }
 B(w.step_count);B(w.step_cursor);B(w.running);
 for(auto&v:w.steps){B(v.op);H(v.owner);F(v.a);F(v.b);F(v.c);}
 B(w.choice);B(w.return_phase);B(w.choice_filter);B(w.choice_cancelable);
 H(w.choice_owner);H(w.choice_min);H(w.choice_max);B(w.choice_enchantment);H(w.choice_amount);
 H(w.offer_count);for(auto&v:w.offers)H(v);for(auto&f:w.offer_facts){WRF(f);}
 H(w.selected_count);for(auto&v:w.selected)H(v);
 B(w.reward_count);B(w.group_count);B(w.choice_group);B(w.choice_reward);
 for(unsigned i=0;i<MaxWorldRelicRewardGroups;i++){B(w.group_active[i]);B(w.group_skip[i]);H(w.group_owner[i]);}
 for(auto&r:w.rewards){
  B(r.group);B(r.kind);B(r.flags);B(r.count);H(r.value);
  for(auto&v:r.tokens)H(v);for(auto&f:r.facts){WRF(f);}
  H(r.rarity_mask);H(r.creation_flags);for(auto&v:r.pools)B(v);
  B(r.pool_count);B(r.odds);B(r.source);
 }
 B(w.prepared_count);for(auto&r:w.prepared){H(r.id);for(auto&v:r.values)H(v);}
 B(w.tooth_count);for(auto&r:w.tooth_cards){H(r.token);H(r.genetic);H(r.scythe);WRF(r.facts);}
 F(w.play_serial_counter);for(auto&v:w.play_serial)F(v);H(w.manual_plays_turn);
 H(w.self_forming_clay);H(w.reptile_trinket);H(w.helical_dart);
 B(w.byrdpip_present);B(w.legion_present);H(w.last_flash);F(w.flash_revision);
 for(auto&f:w.hook_frames){
  B(f.hook);B(f.active);H(f.count);H(f.cursor);
  for(auto&v:f.serials)H(v);for(auto&v:f.args)F(v);
 }
#undef WRF
}
