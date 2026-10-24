bool event_card_type(const State&,Card,int instance,int deck_index,CardType&);
CardColor event_card_color(unsigned kind);
bool event_card_targeted(const State&,Card,int instance,int deck_index,bool&);
const EventCardFacts* event_card_facts(const State&,Card,int instance=-1,int deck_index=-1);
const char* event_card_portrait(const State&,Card,int instance=-1,int deck_index=-1);
uint8_t event_card_keywords(const State&,Card,int instance,int deck_index,uint8_t original);
bool event_card_gains_block(const State&,Card,int instance=-1,int deck_index=-1);
bool event_card_can_enchant(const State&,int deck_index,EventEnchantment);
bool event_card_can_enchant(const State&,Card,const EventCardFacts&,EventEnchantment);
bool event_card_apply_enchantment(Card&,EventCardFacts&,EventEnchantment,int amount);
bool event_card_facts_valid(Card,const EventCardFacts&);
bool event_card_enchant(State&,int deck_index,EventEnchantment,int amount);
const char* event_enchantment_key(EventEnchantment);
int event_add_deck_card(State&,Card,const EventCardFacts* facts=nullptr,bool cloned=false);
void event_deck_acquired(State&,int deck_index,bool cloned=false);
bool event_remove_deck_card(State&,int deck_index);
bool event_replace_deck_card(State&,int deck_index,Card,const EventCardFacts* facts=nullptr);
bool event_take_deck_card(State&,int deck_index,EventDeckSnapshot&);
int event_restore_deck_card(State&,const EventDeckSnapshot&);
bool event_card_hatch(State&,int deck_index);
bool event_card_spoils_complete(State&,int deck_index);
int event_card_spoils_act(const State&);
bool event_card_forces_historian(const State&,unsigned act_index);
void event_card_before_room(State&,bool unknown_point,bool first_room_at_point);
bool event_card_description_value(const State&,Card,int target,int instance,const char*,int&);
// Detached offers have no combat instance or deck slot. Damage/Block take the
// canonical value in/out; EnergyCost and CardType return their complete value.
// This applies permanent facts only and never mutates state or draws RNG.
bool event_card_offer_description_value(const State&,Card,const EventCardFacts&,const char*,int&);
bool event_card_deck_description_value(const State&,Card,int deck_index,const char*,int&);
