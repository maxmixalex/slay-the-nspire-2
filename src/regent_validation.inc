static bool valid_regent_state(const State&s){
 const auto&r=s.regent;
 if(r.power_count>RPowerCount||r.cards_created>9999||r.stars_gained_turn>9999||r.cards_finished_turn>9999||r.skills_finished_turn>r.cards_finished_turn||r.void_played_turn>9999||r.pale_activated>1||r.end_turn_requested>1)return false;
 bool seen[RPowerCount]{};
 for(unsigned at=0;at<r.power_count;at++){unsigned p=r.power_order[at];if(p>=RPowerCount||seen[p])return false;seen[p]=true;}
 for(unsigned at=r.power_count;at<RPowerCount;at++)if(r.power_order[at])return false;
 for(int p=0;p<RPowerCount;p++)if(r.powers[p]<0||r.powers[p]>9999||seen[p]!=(r.powers[p]>0))return false;
 if(r.powers[(int)RPower::SeekingEdge]>1||(r.pale_activated&&!r.powers[(int)RPower::PaleBlueDot]))return false;
 int orbit=0,monologue=0;
 for(int slot=0;slot<MaxPowerInstances;slot++){
  if(r.orbit[slot]<0||r.orbit[slot]>9999||r.monologue[slot]<0||r.monologue[slot]>9999||r.monologue_applied[slot]<0||r.monologue_applied[slot]>9999||r.orbit_remainder[slot]>3)return false;
  if((!r.orbit[slot]&&r.orbit_remainder[slot])||(!r.monologue[slot]&&r.monologue_applied[slot]))return false;
  orbit+=r.orbit[slot];monologue+=r.monologue[slot];
  for(int f=0;f<=MaxAutoFrames;f++)if(r.monologue_capture[slot][f]<0||r.monologue_capture[slot][f]>9999||(!r.monologue[slot]&&r.monologue_capture[slot][f]))return false;
 }
 if(r.powers[(int)RPower::Orbit]!=min(9999,orbit)||r.powers[(int)RPower::Monologue]!=min(9999,monologue))return false;
 for(unsigned id=0;id<MaxCards;id++){
  if(r.kick_discount[id]>9999||r.kick_turn_origin[id]>r.kick_discount[id]||r.kick_combat_origin[id]>r.kick_discount[id]||r.punch_damage[id]<0||r.punch_damage[id]>9999)return false;
  if(id>=s.card_count){if(r.kick_discount[id]||r.kick_turn_origin[id]||r.kick_combat_origin[id]||r.punch_damage[id])return false;continue;}
  if((r.kick_discount[id]||r.kick_turn_origin[id]||r.kick_combat_origin[id])&&s.cards[id].kind!=RegentKinglyKick)return false;
  if(r.punch_damage[id]&&s.cards[id].kind!=RegentKinglyPunch)return false;
 }
 for(int t=0;t<MaxEnemies;t++)if(r.hits_turn[t]>9999||r.conqueror[t]<0||r.conqueror[t]>9999||(t>=s.enemy_count&&(r.hits_turn[t]||r.conqueror[t])))return false;
 for(int f=0;f<=MaxAutoFrames;f++){
  if(r.stars_spent[f]<0||r.stars_spent[f]>9999||r.star_x[f]<0||r.star_x[f]>9999||r.automatic[f]>1)return false;
  bool pending=r.stars_spent[f]||r.star_x[f]||r.automatic[f];for(int slot=0;slot<MaxPowerInstances;slot++)pending=pending||r.monologue_capture[slot][f];
  if(pending&&(f>s.expansion.depth||!combat_phase(s.phase)||(f==s.expansion.depth&&s.playing<0)))return false;
 }
 if(r.end_turn_requested&&!combat_phase(s.phase))return false;
 if(s.phase==Phase::ExtendedChoice&&regent_choice(s.expansion.choice)){
  const auto&x=s.expansion;XChoice k=x.choice;
  if(k!=XChoice::RTransformHand&&k!=XChoice::RTransformDraw&&k!=XChoice::RTransformGuards&&x.choice_free)return false;
  if(k==XChoice::RTransformGuards){if(x.choice_min||x.choice_max!=min(MaxHand,x.choices.count))return false;}
  else if(k==XChoice::RTransformDraw){if(x.choice_min!=min(2,x.choices.count)||x.choice_min!=x.choice_max)return false;}
  else if(k==XChoice::RForegone||k==XChoice::RTyranny){if(!x.choice_min||x.choice_min!=x.choice_max)return false;}
  else if(x.choice_min!=1||x.choice_max!=1)return false;
  Zone zone=(k==XChoice::RTransformDraw||k==XChoice::RForegone)?Zone::Draw:k==XChoice::RDiscardTop?Zone::Discard:Zone::Hand;
  for(unsigned i=0;i<x.choices.count;i++){
   int id=x.choices.ids[i];if(id<0||id>=s.card_count||s.zones[id]!=zone)return false;
   if(k==XChoice::RDecisions&&definitions[s.cards[id].kind].type!=CardType::Skill)return false;
   if(k==XChoice::RHeirloom&&!card_colorless(s.cards[id].kind))return false;
  }
 }
 return true;
}
static bool valid_regent_effect(const State&s,const Effect&e){
 int a=e.value,b=e.aux;bool amount=a>=0&&a<=9999,instance=a>=0&&a<s.card_count,enemy=a>=0&&a<s.enemy_count;
 switch((XOp)e.target){
 case XOp::RPower:return a>=0&&a<RPowerCount&&b!=0&&b>=-9999&&b<=9999&&(b>0||a==(int)RPower::EnergyNextTurn||a==(int)RPower::StarsNextTurn||a==(int)RPower::ForegoneConclusion);
 case XOp::RStars:case XOp::RUnpoweredBlock:case XOp::RForgeHits:case XOp::RVigor:return amount&&b==0;
 case XOp::RBlock:return amount&&b>=0&&b<=9999;
 case XOp::RHits:return amount&&b>=0&&b<=9999;
 case XOp::RAllHits:return amount&&b>=-1&&b<=9999;
 case XOp::RTransform:return a>=0&&a<=2&&b>=0&&b<=1;
 case XOp::RTransformCard:{Card c=card_from_token(b);return instance&&b>=0&&card_token(c)==b&&(c.kind==MinionStrike||c.kind==MinionDiveBomb||c.kind==MinionSacrifice);}
 case XOp::RDecisions:case XOp::RHeirloom:case XOp::RDiscardTop:case XOp::REndTurn:case XOp::RReturnBlade:return a==0&&b==0;
 case XOp::RClone:return instance&&card_colorless(s.cards[a].kind)&&b==0;
 case XOp::RConqueror:case XOp::RKnockoutReward:case XOp::RStrengthLoss:return enemy&&b>0&&b<=9999;
 case XOp::RStartPower:return (a==(int)RPower::Genesis||a==(int)RPower::StarsNextTurn||a==(int)RPower::EnergyNextTurn||a==(int)RPower::ForegoneConclusion||a==(int)RPower::SpectrumShift)&&b==0;
 case XOp::RForegone:return a>0&&a<=9999&&b>=0&&b<=1;
 case XOp::RTyranny:return a>0&&a<=9999&&b==0;
 case XOp::RBombardments:return (!a&&!b)||(instance&&s.cards[a].kind==RegentBombardment&&b==1);
 case XOp::RInvincible:return (!a&&!b)||(instance&&s.cards[a].kind==RegentIAmInvincible&&b==1);
 case XOp::RUnpoweredDamage:return amount&&b>=0&&b<s.enemy_count;
 case XOp::RMonologueStrength:return a>=0&&a<MaxPowerInstances&&s.regent.monologue[a]>0&&b>0&&b<=9999;
 case XOp::RDebris:return a>=0&&a<=MaxHand&&b==0;
 default:return false;
 }
}
