// Appended to the unpublished v15 world suffix. No pointer or workspace enters
// a save, and no historical v3-v14 byte is reinterpreted.
{
 auto&w=s.world;
#define WCF(f) B((f).enchantment);B((f).science_type);B((f).rider);H((f).enchantment_amount);H((f).energy_offset);H((f).progress)
 B(w.room_type);B(w.current_point);B(w.room_open);B(w.event_combat_parent);B(w.parent_room_type);B(w.final_event);B(w.finished);B(w.point_had_shop);B(w.finish_pending);
 for(auto&v:w.map_ready)B(v);
 for(auto&m:w.maps){
  B(m.count);B(m.root);B(m.boss);B(m.second_boss);B(m.error);
  B(m.config.rows);B(m.config.rests);B(m.config.shops);B(m.config.elites);B(m.config.unknowns);
  B(m.config.replace_treasure);B(m.config.second_boss);B(m.config.prune);
  B(m.config.variant);
  for(auto&p:m.points){
   B(p.col);B(p.row);B(p.type);B(p.active);B(p.modifiable);
   B(p.parents.used);for(auto&id:p.parents.slots)B(id);
   B(p.children.used);for(auto&id:p.children.slots)B(id);
  }
  for(auto&col:m.grid)for(auto&id:col)B(id);
  for(auto&calls:m.stage_calls)F(calls);
  for(auto&word:m.final_rng.words)field64(word,c);F(m.final_rng.calls);B(m.final_rng.good);
 }
 for(auto&q:w.rooms){
  B(q.ready);B(q.region);B(q.encounter_count);H(q.encounter_cursor);B(q.elite_count);H(q.elite_cursor);
  B(q.event_count);H(q.event_cursor);B(q.boss_count);H(q.boss_cursor);B(q.ancient);
  for(auto&id:q.encounters)B(id);for(auto&id:q.elites)B(id);for(auto&id:q.bosses)B(id);for(auto&id:q.events)B(id);
 }
 for(auto&act:w.visited)for(auto&v:act)B(v);for(auto&v:w.visited_events)B(v);
 F(w.unknown_monster_bits);F(w.unknown_elite_bits);F(w.unknown_shop_bits);F(w.unknown_treasure_bits);F(w.potion_drop_bits);
 B(w.reward_count);B(w.reward_relic_claimed);
 for(unsigned i=0;i<MaxWorldRewardCards;++i){H(w.reward_cards[i].kind);B(w.reward_cards[i].upgraded);WCF(w.reward_facts[i]);}
 H(w.reward_relic);H(w.treasure_relic);
 for(auto&id:w.shop_relics)H(id);
 for(unsigned i=0;i<ShopCardCount;++i){H(w.shop_cards[i].kind);B(w.shop_cards[i].upgraded);WCF(w.shop_facts[i]);}
 for(auto&v:w.shop_base_prices)H(v);
 B(w.shop_sale);B(w.treasure_gold_claimed);H(w.treasure_gold);
 B(w.shop_pending_item);B(w.shop_auto_active);B(w.shop_auto_cursor);B(w.shop_free_remove);H(w.shop_pending_price);
 B(w.rest_count);for(auto&v:w.rest_options)B(v);B(w.rest_pending);F(w.rest_used);H(w.rest_deck_before);
#undef WCF
}
