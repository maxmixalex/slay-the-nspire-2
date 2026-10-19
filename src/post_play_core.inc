// The original listener snapshot is player powers in acquisition order,
// then relics/cards, then enemy powers. Each listener is a continuation so
// its nested effects finish before the next listener, including save/choices.
static bool post_play_actor_active(const State&s,unsigned actor){
 switch((PostPlayActor)actor){
 case PostPlayActor::Calamity:return power_value(s,XPower::Calamity)>0;
 case PostPlayActor::Rage:return s.ironclad.powers[(unsigned)IPower::Rage]>0;
 case PostPlayActor::Rupture:return s.ironclad.powers[(unsigned)IPower::Rupture]>0;
 case PostPlayActor::Afterimage:return s.silent.powers[(unsigned)SPower::Afterimage]>0;
 case PostPlayActor::SerpentForm:return s.silent.powers[(unsigned)SPower::SerpentForm]>0;
 case PostPlayActor::MasterPlanner:return s.silent.powers[(unsigned)SPower::MasterPlanner]>0;
 case PostPlayActor::SilentEnvenom:return spower(s,SPower::Envenom)>0;
 case PostPlayActor::DefectSmokestack:return dpower(s,DPower::Smokestack)>0;
 case PostPlayActor::DefectTrashToTreasure:return dpower(s,DPower::TrashToTreasure)>0;
 case PostPlayActor::DefectStorm:return dpower(s,DPower::Storm)>0;
 case PostPlayActor::DefectSubroutine:return dpower(s,DPower::Subroutine)>0;
 case PostPlayActor::NecroDevourLife:return npower(s,NPower::DevourLife)>0;
 case PostPlayActor::NecroHaunt:return npower(s,NPower::Haunt)>0;
 case PostPlayActor::NecroReaperForm:return npower(s,NPower::ReaperForm)>0;
 default:return actor>=(unsigned)PostPlayActor::PanacheStart&&actor<(unsigned)PostPlayActor::Relics&&
                s.expansion.panache[actor-(unsigned)PostPlayActor::PanacheStart]>0||regent_post_play_actor_active(s,actor);
 }
}
static void after_play_register(State&s,unsigned actor){
 if(!actor||actor>=(unsigned)PostPlayActor::Count||(actor>=(unsigned)PostPlayActor::Relics&&actor<=(unsigned)PostPlayActor::Enemies)){fail(s);return;}
 auto&order=s.expansion.after_play_order;
 for(auto&entry:order){if(entry==actor)return;if(!entry){entry=(uint8_t)actor;return;}}
 fail(s);
}
static void after_play_unregister(State&s,unsigned actor){
 auto&order=s.expansion.after_play_order;
 for(unsigned i=0;i<MaxPostPlayPowers;i++)if(order[i]==actor){
  for(unsigned j=i+1;j<MaxPostPlayPowers;j++)order[j-1]=order[j];
  order[MaxPostPlayPowers-1]=0;return;
 }
}
static void queue_post_play_listeners(State&s){
 queue_extension(s,XOp::PostPlayListener,(int)PostPlayActor::Enemies,0,true);
 queue_extension(s,XOp::PostPlayListener,(int)PostPlayActor::Cards,0,true);
 queue_extension(s,XOp::PostPlayListener,(int)PostPlayActor::Relics,0,true);
 for(int i=MaxPostPlayPowers-1;i>=0;i--)if(s.expansion.after_play_order[i])
  queue_extension(s,XOp::PostPlayListener,s.expansion.after_play_order[i],0,true);
}
static void post_play_listener(State&s,unsigned actor){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 const Card c=s.cards[s.playing];
 if(actor==(unsigned)PostPlayActor::Relics){relic_card_played(s,c);return;}
 if(actor==(unsigned)PostPlayActor::Calamity){
  if(definitions[c.kind].type==CardType::Attack)
   for(int i=0;i<power_value(s,XPower::Calamity);i++)queue_extension(s,XOp::Generate,1,0,true);
  return;
 }
 if(actor>=(unsigned)PostPlayActor::PanacheStart&&actor<(unsigned)PostPlayActor::Relics){
  auto&x=s.expansion;const unsigned slot=actor-(unsigned)PostPlayActor::PanacheStart;
  if(x.panache_skip[slot]){x.panache_skip[slot]=0;return;}
  if(x.panache[slot]&&++x.panache_count[slot]==5){x.panache_count[slot]=0;queue_extension(s,XOp::AllDamage,x.panache[slot],0,true);}
  return;
 }
 if(actor==(unsigned)PostPlayActor::Rage||actor==(unsigned)PostPlayActor::Rupture)
  ironclad_after_play_listener(s,actor);
 else if(actor==(unsigned)PostPlayActor::DefectStorm||actor==(unsigned)PostPlayActor::DefectSubroutine)defect_after_play_listener(s,actor);
 else if(regent_post_play_actor_active(s,actor))regent_after_play_listener(s,actor);
 else {silent_after_play_listener(s,actor);necro_after_play_listener(s,actor);}
}
