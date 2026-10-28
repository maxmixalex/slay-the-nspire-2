static bool valid_ironclad_effect(const State&s,const Effect&e){
 if(e.kind!=EffectKind::Extended)return false;int a=e.value,b=e.aux;XOp op=(XOp)e.target;
 if(a<0||b<0)return false;
 switch(op){
 case XOp::IPower:return a<IPowerCount&&b>0&&b<=9999;
 case XOp::IBlock:return a<=9999&&b<=1;
 case XOp::IUpgrade:case XOp::IPrimalForce:case XOp::IStoke:case XOp::IHavoc:return a<=1&&!b;
 case XOp::IDominateStrength:case XOp::IDoubleVulnerable:return a<s.enemy_count&&!b;
 case XOp::IFeed:return a<=9999&&(b==3||b==4);
 case XOp::IFeedReward:return a<s.enemy_count&&(b==3||b==4);
 case XOp::IEnemyStrength:return a<s.enemy_count&&b<=9999;
 case XOp::IIncreaseDamage:return a<s.card_count&&(s.cards[a].kind==Rampage||s.cards[a].kind==Thrash)&&b<=9999;
 case XOp::IPillageDraw:return !a&&!b;
 case XOp::IPillageCheck:return a<=MaxAutoFrames&&!b;
 case XOp::ISecondWind:return a<=9999&&!b;
 case XOp::IThrash:return a<s.card_count&&s.cards[a].kind==Thrash&&!b;
 case XOp::IAggression:case XOp::IStampede:return a>0&&a<=9999&&!b;
 case XOp::IStartPower:return (a==(int)IPower::CrimsonMantle||a==(int)IPower::Inferno)&&!b;
 case XOp::IHowls:return a<=MaxCards&&!b;
 case XOp::IClone:return a<s.card_count&&b>0&&b<=9999;
 case XOp::IHits:case XOp::IAllHits:return a<=9999&&b>0&&b<=9999;
 case XOp::ITemporaryStrength:case XOp::IHeal:case XOp::IUnpoweredRandom:return a<=9999&&!b;
 default:return false;
 }
}
static bool valid_ironclad_state(const State&s){
 const auto&i=s.ironclad;if(i.power_count>IPowerCount)return false;bool seen[IPowerCount]{};
 for(unsigned n=0;n<i.power_count;n++){unsigned p=i.power_order[n];if(p>=IPowerCount||seen[p]||i.powers[p]<=0)return false;seen[p]=true;}
 for(unsigned n=i.power_count;n<IPowerCount;n++)if(i.power_order[n])return false;
 for(unsigned p=0;p<IPowerCount;p++)if(i.powers[p]<0||i.powers[p]>9999||(i.powers[p]>0)!=seen[p])return false;
 if(ipower(s,IPower::Barricade)>1||ipower(s,IPower::Corruption)>1||ipower(s,IPower::Hellraiser)>1)return false;
 if(i.attacks_started>9999||i.attacks_finished>i.attacks_started||i.hp_losses_turn>i.hp_losses_combat||i.hp_losses_combat>9999||i.exhausted_turn>9999||i.block_events_turn>9999)return false;
 if(i.setup_strength<0||i.setup_strength>9999||i.crimson_self_damage<0||i.crimson_self_damage>9999||i.inferno_self_damage<0||i.inferno_self_damage>9999)return false;
 if((i.crimson_self_damage>0)!=(ipower(s,IPower::CrimsonMantle)>0)||(i.inferno_self_damage>0)!=(ipower(s,IPower::Inferno)>0))return false;
 bool check[MaxAutoFrames+1]{};
 for(unsigned n=0;n<s.effect_count;n++)if(s.effects[n].kind==EffectKind::Extended&&s.effects[n].target==(int)XOp::IPillageCheck){int d=s.effects[n].value;if(d<0||d>MaxAutoFrames||check[d])return false;check[d]=true;}
 for(unsigned d=0;d<=MaxAutoFrames;d++){
  if(i.block_events_play[d]>i.block_events_turn||i.rupture_registered[d]>1||i.rupture_pending[d]<0||i.rupture_pending[d]>9999||i.pillage_waiting[d]>1)return false;
  if(!i.rupture_registered[d]&&i.rupture_pending[d])return false;
  if(check[d]){if(d>s.expansion.depth||i.pillage_drawn[d]<-1||i.pillage_drawn[d]>=s.card_count)return false;}
  else if(i.pillage_drawn[d]||i.pillage_waiting[d])return false;
  if(d>s.expansion.depth&&(i.rupture_registered[d]||i.rupture_pending[d]||i.block_events_play[d]))return false;
 }
 for(unsigned id=0;id<MaxCards;id++){
  if(i.damage_bonus[id]<0||i.damage_bonus[id]>9999||i.stomp_discount[id]>9999||i.stomp_turn_origin[id]>i.stomp_discount[id]||i.stomp_combat_origin[id]>i.stomp_discount[id]||i.forced_exhaust[id]>1)return false;
  if(id>=s.card_count){if(i.damage_bonus[id]||i.stomp_discount[id]||i.stomp_turn_origin[id]||i.stomp_combat_origin[id]||i.forced_exhaust[id])return false;continue;}
  if(i.damage_bonus[id]&&s.cards[id].kind!=Rampage&&s.cards[id].kind!=Thrash)return false;
  if((i.stomp_discount[id]||i.stomp_turn_origin[id]||i.stomp_combat_origin[id])&&s.cards[id].kind!=Stomp)return false;
 }
 if(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::IUpgrade){
  if(s.expansion.choice_min!=1||s.expansion.choice_max!=1||s.expansion.choice_free||!s.expansion.choices.count)return false;
  for(unsigned at=0;at<s.expansion.choices.count;at++){int id=s.expansion.choices.ids[at];if(id<0||id>=s.card_count||s.zones[id]!=Zone::Hand||s.cards[id].upgraded)return false;}
 }
 return true;
}
