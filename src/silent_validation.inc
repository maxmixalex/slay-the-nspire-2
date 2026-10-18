static bool valid_silent_snapshot(const SilentSnapshot&n,bool active){
 if(n.card.kind>=CardKinds||n.card.upgraded>1||n.turn_cost>100||n.combat_cost>100||n.sly>3||n.retain>3||n.inky>1||n.bullet_free>1)return false;
 if(n.discount>9999||n.turn_origin>n.discount||n.combat_origin>n.discount||n.bullet_origin>n.discount||n.return_turn>20001)return false;
 if(n.blade_damage<0||n.blade_damage>9999||n.iron_damage<0||n.iron_damage>9999||n.stomp_discount>9999||n.stomp_turn_origin>n.stomp_discount||n.stomp_combat_origin>n.stomp_discount)return false;
 if(n.inky&&n.card.kind!=Shiv)return false;
 if(n.blade_damage&&n.card.kind!=SovereignBlade)return false;
 if(n.discount&&n.card.kind!=Pinpoint&&n.card.kind!=UpMySleeve)return false;
 if(!active&&(n.card.kind||n.card.upgraded||n.turn_cost||n.combat_cost||n.replay||n.return_turn||n.blade_damage||n.sly||n.retain||n.inky||n.bullet_free||n.discount||n.turn_origin||n.combat_origin||n.bullet_origin||n.iron_damage||n.stomp_discount||n.stomp_turn_origin||n.stomp_combat_origin))return false;
 return true;
}
static bool valid_silent_state(const State&s){
 const auto&v=s.silent;
 if(v.power_count>SPowerCount+MaxNightmares||v.attacks_finished>9999||v.skills_finished>9999||v.discarded_turn>9999||v.drawn_combat>9999||v.shiv_finished>1||v.extra_rewards>9999)return false;
 bool seen[SPowerCount+MaxNightmares]{};
 for(unsigned i=0;i<v.power_count;i++){unsigned p=v.power_order[i];if(p>=SPowerCount+MaxNightmares||seen[p])return false;seen[p]=true;}
 for(unsigned i=v.power_count;i<SPowerCount+MaxNightmares;i++)if(v.power_order[i])return false;
 for(int p=0;p<SPowerCount;p++){if(v.powers[p]<0||v.powers[p]>9999||seen[p]!=(v.powers[p]>0))return false;}
 for(int slot=0;slot<MaxNightmares;slot++){
  if(v.nightmare_active[slot]>1||seen[SPowerCount+slot]!=(v.nightmare_active[slot]>0)||!valid_silent_snapshot(v.nightmares[slot],v.nightmare_active[slot]))return false;
 }
 for(unsigned id=0;id<MaxCards;id++){
  if(v.sly[id]>3||v.retain[id]>3||v.inky[id]>1||v.bullet_free[id]>1||v.discount[id]>9999||v.turn_origin[id]>v.discount[id]||v.combat_origin[id]>v.discount[id]||v.bullet_origin[id]>v.discount[id])return false;
  if(id>=s.card_count){if(v.sly[id]||v.retain[id]||v.inky[id]||v.bullet_free[id]||v.discount[id]||v.turn_origin[id]||v.combat_origin[id]||v.bullet_origin[id])return false;continue;}
  if(v.inky[id]&&s.cards[id].kind!=Shiv)return false;
  if(v.discount[id]&&s.cards[id].kind!=Pinpoint&&s.cards[id].kind!=UpMySleeve)return false;
  if(v.bullet_free[id]&&cost(s.cards[id])<0)return false;
 }
 for(int t=0;t<MaxEnemies;t++)if(v.strangle[t]<0||v.strangle[t]>9999||v.enemy_artifact[t]<0||v.enemy_artifact[t]>9999||(t>=s.enemy_count&&(v.strangle[t]||v.enemy_artifact[t])))return false;
 for(unsigned f=0;f<=MaxAutoFrames;f++){
  if(v.afterimage_before[f]<0||v.afterimage_before[f]>9999||v.serpent_before[f]<0||v.serpent_before[f]>9999||v.retain_waiting[f]>1||v.retain_drawn[f]<0||v.retain_drawn[f]>s.card_count)return false;
  bool pending=v.afterimage_before[f]||v.serpent_before[f]||v.retain_waiting[f]||v.retain_drawn[f];
  for(int t=0;t<MaxEnemies;t++){if(v.strangle_before[f][t]<0||v.strangle_before[f][t]>9999)return false;pending=pending||v.strangle_before[f][t];}
  if(pending&&(f>s.expansion.depth||!combat_phase(s.phase)))return false;
 }
 if(s.phase==Phase::ExtendedChoice&&(s.expansion.choice==XChoice::SlySkill||s.expansion.choice==XChoice::NightmareCard)){
  const auto&x=s.expansion;if(x.choice_min!=1||x.choice_max!=1||x.choice_free)return false;
  for(unsigned i=0;i<x.choices.count;i++){int id=x.choices.ids[i];if(id<0||id>=s.card_count||s.zones[id]!=Zone::Hand)return false;
   if(x.choice==XChoice::SlySkill&&(definitions[s.cards[id].kind].type!=CardType::Skill||silent_is_sly(s,id)))return false;
  }
 }
 bool nightmare_queued[MaxNightmares]{};
 for(unsigned i=0;i<s.effect_count;i++)if(s.effects[i].kind==EffectKind::Extended&&s.effects[i].target==(int)XOp::SNightmareCopies){int slot=s.effects[i].value;if(slot<0||slot>=MaxNightmares||nightmare_queued[slot]||!v.nightmare_active[slot])return false;nightmare_queued[slot]=true;}
 return true;
}
static bool valid_silent_effect(const State&s,const Effect&e){
 const int a=e.value,b=e.aux;bool instance=a>=0&&a<s.card_count,actor=a>=0&&a<s.enemy_count;
 switch((XOp)e.target){
 case XOp::SPower:return a>=0&&a<SPowerCount+s.enemy_count&&b>0&&b<=9999;
 case XOp::SAbrasive:case XOp::SAnticipate:return a>=0&&a<=9999&&b==0;
 case XOp::SBulletTime:case XOp::SSlyChoice:case XOp::SNightmareChoice:case XOp::SPoisonTrigger:return a==0&&b==0;
 case XOp::SDiscardHand:return a>=0&&a<=3&&b==0;
 case XOp::SCreateShivs:return a>=0&&a<=MaxCards&&b>=0&&b<=3;
 case XOp::SEchoSlash:case XOp::SUnpoweredRandom:return a>=0&&a<=9999&&b==0;
 case XOp::SEchoRepeat:return a>=0&&a<=9999&&b>0&&b<(1<<s.enemy_count);
 case XOp::SExpose:case XOp::SMalaise:case XOp::SStrangle:return actor&&b>=0&&b<=9999;
 case XOp::SNightmareCopies:return a>=0&&a<MaxNightmares&&b==0&&s.silent.nightmare_active[a];
 case XOp::SRetainDraw:case XOp::SRetainDrawEnd:return a>=0&&a<=MaxHand&&b==0;
 case XOp::SKnifeShiv:return instance&&s.cards[a].kind==Shiv&&b>=0&&b<2*MaxEnemies&&b%MaxEnemies<s.enemy_count;
 case XOp::SUpMySleeve:return instance&&s.cards[a].kind==UpMySleeve&&b==0;
 case XOp::SPowerTurn:return (a==(int)SPower::NoxiousFumes||a==(int)SPower::WraithForm||a==(int)SPower::ShadowStep||a==(int)SPower::Blur)&&b==0;
 case XOp::SInky:return instance&&s.cards[a].kind==Shiv&&b>=-1&&b<s.enemy_count;
 case XOp::SHits:return a>=0&&a<=9999&&b>=0&&b<=9999;
 case XOp::SEscapeDraw:case XOp::SEscapeBlock:return a>=0&&a<=9999&&b==0;
 default:return false;
 }
}
