bool event_card_facts_valid(Card c,const EventCardFacts&f){
 if(c.kind>=CardKinds||unsigned(f.enchantment)>=EventEnchantmentKinds||f.science_type>2||f.rider>=unsigned(TinkerRider::Count)||f.enchantment_amount<0||f.enchantment_amount>9999||f.energy_offset<-9999||f.energy_offset>0||f.progress<0||f.progress>9999)return false;
 if(f.enchantment==EventEnchantment::None&&(f.enchantment_amount||f.energy_offset))return false;
 if(f.enchantment!=EventEnchantment::TezcatarasEmber&&f.energy_offset)return false;
 if(c.kind!=EventCardMadScience&&(f.science_type||f.rider))return false;
 if(c.kind==EventCardMadScience&&f.rider&&(unsigned(f.rider)-1)/3!=f.science_type)return false;
 if(c.kind!=EventCardGuilty&&c.kind!=EventCardDowsing&&c.kind!=EventCardSpoilsMap&&f.progress)return false;
 return true;
}
static bool valid_event_card_state(const State&s){
 const auto&v=s.event_cards;if(v.power_count>EventCardPowerKinds||v.plays_started>9999||v.end_hand_count>MaxHand||v.choice_copies>2)return false;
 bool seen[EventCardPowerKinds]{};for(unsigned i=0;i<v.power_count;++i){unsigned p=v.power_order[i];if(p>=EventCardPowerKinds||seen[p])return false;seen[p]=true;}
 for(unsigned i=0;i<EventCardPowerKinds;++i)if(v.powers[i]<0||v.powers[i]>9999||seen[i]!=(v.powers[i]>0))return false;
 for(unsigned id=0;id<MaxCards;++id){
  if(!event_card_facts_valid(id<s.card_count?s.cards[id]:Card{},v.cards[id])||v.deck_origin[id]<0||v.deck_origin[id]>s.deck_count||v.enchantment_disabled[id]>1||v.momentum[id]<0||v.momentum[id]>9999||v.maul_damage[id]<0||v.maul_damage[id]>9999)return false;
  if(id<s.card_count&&v.deck_origin[id]&&s.deck[v.deck_origin[id]-1].kind!=s.cards[id].kind)return false;
 }
 for(unsigned i=0;i<MaxDeck;++i)if(!event_card_facts_valid(i<s.deck_count?s.deck[i]:Card{},v.deck[i]))return false;
 for(unsigned i=0;i<MaxPowerInstances;++i)if(v.toric_turns[i]>2||v.toric_block[i]<0||v.toric_block[i]>9999||(!v.toric_turns[i]&&v.toric_block[i]))return false;
 for(auto n:v.rebound_return)if(n>1)return false;
 if(v.choice_copies&&(s.phase!=Phase::ExtendedChoice||s.expansion.choice!=XChoice::EventDualWield))return false;
 if(s.phase==Phase::ExtendedChoice&&event_card_choice(s.expansion.choice)){
  auto choice=s.expansion.choice;if(choice==XChoice::EventDualWield&&!v.choice_copies)return false;
  for(unsigned i=0;i<s.expansion.choices.count;++i){unsigned id=s.expansion.choices.ids[i];
   if(choice==XChoice::EventAbundance){if(id<MaxCards||id>=MaxCards+3)return false;continue;}
   Zone zone=choice==XChoice::EventDualWield?Zone::Hand:choice==XChoice::EventWish?Zone::Draw:Zone::Discard;
   if(id>=s.card_count||s.zones[id]!=zone)return false;
  }
 }return true;
}
static bool valid_event_card_effect(const State&s,const Effect&e){
 int a=e.value,b=e.aux;switch(XOp(e.target)){
 case XOp::EventCardSpecial:return a>=0&&a<=int(ECSpecial::Thorns)&&b>=0&&b<=9999;
 case XOp::EventEnchantAfterBody:case XOp::EventCardHandEnd:return a>=0&&a<s.card_count&&b==0;
 case XOp::EventCardToric:return a>=0&&a<=9999&&b>=0&&b<s.card_count;
 case XOp::EventCardGenerate:return a>=0&&a<=3&&b>0&&b<=MaxCards;
 case XOp::EventCardOffer:return event_card_choice(XChoice(a))&&b>=1&&b<=3;
 case XOp::EventCardMaterialize:{Card c=card_from_token(a);return a>=0&&card_token(c)==a&&c.kind<CardKinds&&b>=0&&b<=2;}
 default:return false;
 }
}
