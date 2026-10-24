// SaveEventOptionToHistory captures the chosen localized title and its
// variables before the event changes page or consumes the offered model.
struct HistoryEventChoice {
 char key[192]{};
 RelicId ancient_offers[3]{NoWorldRelic,NoWorldRelic,NoWorldRelic};
 uint16_t detail=0;
 uint8_t flags=0,count=0,option=0;
 RunHistoryEvent kind=RunHistoryEvent::None;
};
HistoryEventChoice history_event_choice(const State&s,unsigned option){
 HistoryEventChoice value;const auto&e=s.original_events;
 if(e.id==OriginalEventId::TheArchitect||e.id==OriginalEventId::FakeMerchant||
    (e.id==OriginalEventId::RoundTeaParty&&same(e.option_keys[option],"PICK_FIGHT.options.CONTINUE_FIGHT")))return value;
 const auto modifier=current_neow_modifier(s);
 if(e.id==OriginalEventId::Neow&&modifier!=RunModifier::Count){value.kind=RunHistoryEvent::ModifierChoice;value.detail=uint16_t(modifier);return value;}
 if(ancient(e.id)){
  value.kind=RunHistoryEvent::AncientPicked;value.option=uint8_t(option);value.count=uint8_t(smaller(3,e.offer_count));
  for(unsigned i=0;i<value.count;++i)value.ancient_offers[i]=e.offers[i];return value;
 }
 value.kind=RunHistoryEvent::EventChoice;
 if(e.id==OriginalEventId::DollRoom&&e.page&&option<e.offer_count){
  // This option overrides HistoryName with the chosen doll's title.
  const char*parts[]={world_relic_key(e.offers[option]),".title"};unsigned at=0;
  for(const char*p:parts)for(unsigned i=0;p[i]&&at+1<sizeof value.key;++i)value.key[at++]=p[i];
  value.flags=128;return value;
 }
 if(!original_event_text_key(s,int(option),false,value.key,sizeof value.key)){value.key[0]=0;return value;}
 if(e.id==OriginalEventId::GraveOfTheForgotten&&option==1){value.flags=unsigned(HistoryChoiceVariable::Relic);value.detail=WorldRelicForgottenSoul;}
 else if(e.id==OriginalEventId::RanwidTheElder){
  if(option==0){value.flags=unsigned(HistoryChoiceVariable::Potion);value.detail=uint16_t(e.values[2]);}
  else if(option==1){value.flags=unsigned(HistoryChoiceVariable::Gold);value.detail=100;}
  else{value.flags=unsigned(HistoryChoiceVariable::Relic);value.detail=uint16_t(e.values[1]);}
 }else if(e.id==OriginalEventId::TheFutureOfPotions&&option<3){
  if(const auto*d=potion_definition(potion_at(s,unsigned(e.values[option])))){value.flags=unsigned(HistoryChoiceVariable::PotionRarity);value.detail=d->rarity;}
 }
 return value;
}
void history_event_chosen(const State&s,const HistoryEventChoice&value){
 if(value.kind==RunHistoryEvent::None)return;
 if(value.kind==RunHistoryEvent::AncientPicked){for(unsigned i=0;i<value.count;++i)
  notify_history(s,i==value.option?RunHistoryEvent::AncientPicked:RunHistoryEvent::AncientSkipped,value.ancient_offers[i]);
 }else if(value.kind==RunHistoryEvent::ModifierChoice)notify_history(s,value.kind,value.detail);
 else notify_history(s,value.kind,0,value.detail,value.flags,value.key);
}
void history_skip_event_reward(const State&s,const OriginalEventReward&r){
 if(r.claimed)return;
 if(r.kind==2)notify_history(s,RunHistoryEvent::PotionSkipped,r.model);
 else if(r.kind==3)notify_history(s,RunHistoryEvent::RelicSkipped,r.model);
 else if(r.kind==4)for(unsigned i=0;i<r.count;++i)notify_history(s,RunHistoryEvent::CardSkipped,s.original_events.reward_cards[r.first+i]);
}
