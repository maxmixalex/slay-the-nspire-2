// v15 only. B/H preserve bounded flags/indices; old F prefixes are untouched.
{
 auto&v=s.event_cards;
#define ECF(f) B((f).enchantment);B((f).science_type);B((f).rider);H((f).enchantment_amount);H((f).energy_offset);H((f).progress)
 for(unsigned i=0;i<MaxDeck;++i){ECF(v.deck[i]);}
 for(unsigned i=0;i<MaxCards;++i){ECF(v.cards[i]);H(v.deck_origin[i]);B(v.enchantment_disabled[i]);H(v.momentum[i]);H(v.maul_damage[i]);}
 for(unsigned i=0;i<EventCardPowerKinds;++i)H(v.powers[i]);
 B(v.power_count);for(unsigned i=0;i<EventCardPowerKinds+MaxPowerInstances;++i)B(v.power_order[i]);
 for(unsigned i=0;i<MaxPowerInstances;++i){H(v.toric_block[i]);B(v.toric_turns[i]);}
 H(v.plays_started);B(v.end_hand_count);B(v.choice_copies);for(auto&n:v.rebound_return)B(n);
#undef ECF
}
