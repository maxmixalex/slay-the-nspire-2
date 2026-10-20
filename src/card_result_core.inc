// CardModel.OnPlayWrapper: capture the base location, apply the ordered
// modifiers, notify only modifiers that changed it, then generate play count.
// This is a physical-play snapshot; repeated/nested bodies do not recalculate it.
static bool card_result_actor_active(const State&s,CardResultActor actor){
 switch(actor){
 case CardResultActor::Feral:return dpower(s,DPower::Feral)>0;
 case CardResultActor::Corruption:return ipower(s,IPower::Corruption)>0;
 case CardResultActor::Nostalgia:return power_value(s,XPower::Nostalgia)>0;
 case CardResultActor::Rebound:return ec_power(s,EventCardPower::Rebound)>0;
 default:return false;
 }
}
static bool saved_combat_phase(const State&s){
 // A relic's own selection can suspend a combat card or combat choice. Keep
 // its physical frames while the separately validated relic choice is open.
 return combat_phase(s.phase)||(s.phase==Phase::WorldRelicChoice&&s.world_relics.active&&
  s.world_relics.choice!=WorldRelicChoice::None&&combat_phase(Phase(s.world_relics.return_phase)));
}
static void card_result_register(State&s,CardResultActor actor){
 const unsigned value=unsigned(actor);if(!value||value>=unsigned(CardResultActor::Count)){fail(s);return;}
 for(auto&entry:s.expansion.result_order){if(entry==value)return;if(!entry){entry=uint8_t(value);return;}}
 fail(s);
}
static void card_result_unregister(State&s,CardResultActor actor){
 auto&order=s.expansion.result_order;
 for(unsigned i=0;i<CardResultActors;++i)if(order[i]==unsigned(actor)){
  for(unsigned j=i+1;j<CardResultActors;++j)order[j-1]=order[j];order[CardResultActors-1]=0;return;
 }
}
static uint8_t card_result_base(const State&s,int id){
 const Card c=s.cards[id];const auto type=card_type(s,c,id);
 if(s.world_relics.card_is_dupe[id]||type==CardType::Power)return ResultNone;
 if(s.ironclad.forced_exhaust[id]||card_exhausts(s,c,id))return ResultExhaust;
 if(c.kind==RegentParticleWall)return ResultHand;
 if(c.kind==RegentShiningStrike)return ResultDraw|ResultTop;
 return ResultDiscard;
}
struct CardResultEvaluation {uint8_t location=ResultDiscard,changed[CardResultActors]{};unsigned count=0;};
static CardResultEvaluation card_result_evaluate(const State&s,int id,int resource_energy){
 CardResultEvaluation result;result.location=card_result_base(s,id);
 const auto type=card_type(s,s.cards[id],id);
 for(unsigned entry:s.expansion.result_order){
  if(!entry)break;const auto actor=CardResultActor(entry);if(!card_result_actor_active(s,actor))continue;
  const auto before=result.location;const unsigned pile=before&7;
  switch(actor){
  case CardResultActor::Feral:
   if(type==CardType::Attack&&resource_energy==0&&!s.world_relics.card_is_dupe[id]&&s.defect.feral_used<dpower(s,DPower::Feral))result.location=ResultHand|ResultTop;
   break;
  case CardResultActor::Corruption:
   if(type==CardType::Skill)result.location=uint8_t((before&ResultTop)|ResultExhaust);
   break;
  case CardResultActor::Nostalgia:
   if((type==CardType::Attack||type==CardType::Skill)&&pile==ResultDiscard&&s.expansion.nonpower_started<power_value(s,XPower::Nostalgia))result.location=ResultDraw|ResultTop;
   break;
  case CardResultActor::Rebound:if(pile==ResultDiscard)result.location=ResultDraw|ResultTop;break;
  default:break;
  }
  if(before!=result.location)result.changed[result.count++]=uint8_t(entry);
 }
 return result;
}
static void card_result_capture(State&s,int id,int resource_energy){
 auto&x=s.expansion;const unsigned depth=x.depth;
 const auto result=card_result_evaluate(s,id,resource_energy);
 // Base GetResultLocation consumes ExhaustOnNextPlay before any callbacks;
 // Dupe/Power takes its earlier None branch and does not consume that flag.
 if(!s.world_relics.card_is_dupe[id]&&card_type(s,s.cards[id],id)!=CardType::Power&&
    (s.ironclad.forced_exhaust[id]||card_exhausts(s,s.cards[id],id)))s.ironclad.forced_exhaust[id]=0;
 x.result_location[depth]=result.location;x.feral_result[depth]=1;
 s.event_cards.rebound_return[depth]=0;x.returning=uint8_t((result.location&7)==ResultDraw);
 // Snapshot the changed listeners before Rebound's decrement can remove its
 // entry from the live order. Every callback receives the final location.
 for(unsigned i=0;i<result.count;++i)switch(CardResultActor(result.changed[i])){
 case CardResultActor::Feral:++s.defect.feral_used;x.feral_result[depth]=2;break;
 case CardResultActor::Rebound:ec_gain_power(s,EventCardPower::Rebound,-1);s.event_cards.rebound_return[depth]=1;break;
 default:break; // The other two original callbacks only flash their power.
 }
}
static uint8_t card_result_legacy(State&s,int id){
 // v3-v14 did not save a final result or cross-module acquisition order.
 // Retain their stored Nostalgia decision and old pending-play policy once;
 // every physical play started after migration uses the ordered capture.
 const auto type=card_type(s,s.cards[id],id);const auto&x=s.expansion;
 if(s.world_relics.card_is_dupe[id]||type==CardType::Power)return ResultNone;
 if(regent_returns_to_hand(s,id))return ResultHand;
 if(x.feral_result[x.depth]?x.feral_result[x.depth]==2:defect_returns_to_hand(s,id))return ResultHand|ResultTop;
 if(card_exhausts(s,s.cards[id],id)||ironclad_exhausts(s,id))return ResultExhaust;
 return x.returning||event_card_returns_to_draw(s,id)?ResultDraw|ResultTop:ResultDiscard;
}
static uint8_t card_result_legacy_preview(const State&s,int id,unsigned depth,bool returning){
 const auto type=card_type(s,s.cards[id],id);const auto&x=s.expansion;
 if(s.world_relics.card_is_dupe[id]||type==CardType::Power)return ResultNone;
 if(s.cards[id].kind==RegentParticleWall&&!returning)return ResultHand;
 const bool feral=x.feral_result[depth]?x.feral_result[depth]==2:
  type==CardType::Attack&&!s.defect.spent[depth]&&s.defect.feral_used<dpower(s,DPower::Feral);
 if(feral)return ResultHand|ResultTop;
 if(card_exhausts(s,s.cards[id],id)||ironclad_exhausts(s,id))return ResultExhaust;
 return returning||s.event_cards.rebound_return[depth]?ResultDraw|ResultTop:ResultDiscard;
}
static void card_result_put(State&s,int id,uint8_t result){
 unsigned pile=result&7;const bool top=(result&ResultTop)!=0;
 Zone zone=pile==ResultNone?Zone::Power:pile==ResultDraw?Zone::Draw:pile==ResultHand?Zone::Hand:Zone::Discard;
 if(zone==Zone::Hand&&s.hand.count>=MaxHand)zone=Zone::Discard;
 Pile&to=zone==Zone::Power?s.powers:zone==Zone::Draw?s.draw:zone==Zone::Hand?s.hand:s.discard;
 if(!put(s,to,id,zone))return;
 // Draw is represented in reverse order. Other piles preserve original list
 // order, including Feral Top versus ParticleWall Bottom and full-hand spill.
 if(zone!=Zone::Power&&top==(zone!=Zone::Draw)){
  for(unsigned i=to.count-1;i>0;--i)to.ids[i]=to.ids[i-1];to.ids[0]=uint16_t(id);
 }
}
bool card_play_result(const State&s,int id,Zone&pile,bool&top){
 if(id<0||id>=s.card_count||s.cards[id].kind>=CardKinds||s.expansion.depth>MaxAutoFrames)return false;
 uint8_t location=0;const auto&x=s.expansion;
 if(s.playing==id)location=x.result_location[x.depth]?x.result_location[x.depth]:card_result_legacy_preview(s,id,x.depth,x.returning);
 else for(unsigned i=0;i<x.depth;++i)if(x.frames[i].playing==id){
  location=x.result_location[i]?x.result_location[i]:card_result_legacy_preview(s,id,i,x.frames[i].returning);break;
 }
 if(!location){
  if(cost(s.cards[id])< -1)location=s.world_relics.card_is_dupe[id]?ResultNone:
      s.ironclad.forced_exhaust[id]||card_exhausts(s,s.cards[id],id)?ResultExhaust:ResultDiscard;
  else{const int price=cost(s,id);location=card_result_evaluate(s,id,price<0?s.energy:price).location;}
 }
 top=(location&ResultTop)!=0;
 switch(location&7){
 case ResultNone:pile=Zone::Power;break;case ResultDiscard:pile=Zone::Discard;break;case ResultDraw:pile=Zone::Draw;break;
 case ResultHand:pile=Zone::Hand;break;case ResultExhaust:pile=Zone::Exhaust;break;default:return false;
 }
 return true;
}
static bool card_result_state_valid(const State&s){
 if(s.expansion.depth>MaxAutoFrames)return false;
 bool seen[unsigned(CardResultActor::Count)]{},gap=false;
 for(unsigned entry:s.expansion.result_order){
  if(!entry){gap=true;continue;}
  if(gap||entry>=unsigned(CardResultActor::Count)||seen[entry]||!card_result_actor_active(s,CardResultActor(entry)))return false;
  seen[entry]=true;
 }
 for(unsigned i=1;i<unsigned(CardResultActor::Count);++i)if(seen[i]!=card_result_actor_active(s,CardResultActor(i)))return false;
 for(unsigned i=0;i<=MaxAutoFrames;++i){const unsigned result=s.expansion.result_location[i];if(!result)continue;
  if(result!=ResultNone&&result!=ResultDiscard&&result!=ResultHand&&result!=ResultExhaust&&
     result!=(ResultDraw|ResultTop)&&result!=(ResultHand|ResultTop)&&result!=(ResultExhaust|ResultTop))return false;
  if(i>s.expansion.depth||!saved_combat_phase(s))return false;
  if(!s.expansion.feral_result[i]||(s.expansion.feral_result[i]==2&&result!=(ResultHand|ResultTop)))return false;
  const int id=i<s.expansion.depth?s.expansion.frames[i].playing:s.playing;
  if(id<0||id>=s.card_count)return false;
 }
 return true;
}
static void card_result_migrate_order(State&s){
 // Rebound was absent from historical files. The remaining pairings have no
 // order-dependent consumption; preserve a declared, stable migration order.
 for(auto&entry:s.expansion.result_order)entry=0;
 for(unsigned i=1;i<unsigned(CardResultActor::Count);++i)if(card_result_actor_active(s,CardResultActor(i)))card_result_register(s,CardResultActor(i));
}
