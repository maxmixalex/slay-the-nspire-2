static CombatStatus::Value ui_combat_status_value(const State&s,int actor,int id){
 CombatStatus::Value value;if(actor<0||actor>s.enemy_count||id<0||id>=StatusKinds)return value;
 value.amount=status_amount(s,actor,id);
 const bool resource=actor==0&&((id==44&&s.character==Character::Regent)||
  (id==45&&(s.character==Character::Necrobinder||s.expansion.osty_max_hp>0))||(id==46&&s.expansion.orb_slots>0));
 value.visible=id!=44&&(value.amount!=0||resource);
 if(id>=EnemyStatusStart&&id<EventCardStatusStart){const unsigned power=id-EnemyStatusStart;
  value.auxiliary=actor?s.enemy_world.models[actor-1].power_aux[power]:s.enemy_world.player_aux[power];
  const unsigned source=actor?s.enemy_world.models[actor-1].source[power]:s.enemy_world.player_source[power];
  // Core uses zero for the player and slot+1 for an enemy applier.
  if(source&&source<=s.enemy_count)value.source=1+unsigned(s.enemy_world.models[source-1].kind);
 }
 return value;
}
static int ui_combat_status_fact(const State&s,int fact){
 switch(fact){
 case CombatStatus::CrimsonDamage:return s.ironclad.crimson_self_damage;
 case CombatStatus::InfernoDamage:return s.ironclad.inferno_self_damage;
 case CombatStatus::StarsGained:return s.regent.stars_gained_turn;
 case CombatStatus::OstyMax:return s.expansion.osty_max_hp;
 case CombatStatus::OstyAttacks:return s.necro.osty_attacks_turn;
 case CombatStatus::PhysicalStarted:return s.defect.physical_started;
 case CombatStatus::FeralUsed:return s.defect.feral_used;
 case CombatStatus::RegentCards:return s.regent.cards_finished_turn;
 case CombatStatus::PaleActivated:return s.regent.pale_activated;
 case CombatStatus::VoidPlayed:return s.regent.void_played_turn;
 case CombatStatus::EtherealPlayed:return s.necro.ethereal_played;
 case CombatStatus::Drawn:return s.necro.drawn_turn;
 case CombatStatus::OrbSlots:return s.expansion.orb_slots;
 case CombatStatus::OrbCount:return s.expansion.orb_count;
 default:break;
 }
 if(fact<CombatStatus::Panache||fact>=CombatStatus::Facts)return 0;
 if(fact<CombatStatus::PanacheCards)return s.expansion.panache[fact-CombatStatus::Panache];
 if(fact<CombatStatus::Boulder)return s.expansion.panache_count[fact-CombatStatus::PanacheCards];
 if(fact<CombatStatus::BombTurns)return s.expansion.boulder[fact-CombatStatus::Boulder];
 if(fact<CombatStatus::BombDamage)return s.expansion.bomb_turns[fact-CombatStatus::BombTurns];
 if(fact<CombatStatus::Monologue)return s.expansion.bomb_damage[fact-CombatStatus::BombDamage];
 if(fact<CombatStatus::MonologueApplied)return s.regent.monologue[fact-CombatStatus::Monologue];
 if(fact<CombatStatus::NightmareCard)return s.regent.monologue_applied[fact-CombatStatus::MonologueApplied];
 if(fact<CombatStatus::NightmareActive)return card_token(s.silent.nightmares[fact-CombatStatus::NightmareCard].card);
 if(fact<CombatStatus::Orbit)return s.silent.nightmare_active[fact-CombatStatus::NightmareActive];
 if(fact<CombatStatus::OrbitRemainder)return s.regent.orbit[fact-CombatStatus::Orbit];
 if(fact<CombatStatus::OrbKind)return s.regent.orbit_remainder[fact-CombatStatus::OrbitRemainder];
 if(fact<CombatStatus::OrbPassive)return s.defect.orb_kind[fact-CombatStatus::OrbKind];
 if(fact<CombatStatus::CapturedCard){int passive=0,evoke=0;const bool is_evoke=fact>=CombatStatus::OrbEvoke;
  orb_values(s,fact-(is_evoke?CombatStatus::OrbEvoke:CombatStatus::OrbPassive),passive,evoke);return is_evoke?evoke:passive;}
 const unsigned slot=fact-CombatStatus::CapturedCard;if(slot>=s.enemy_count)return -1;
 const unsigned card=s.enemy_world.models[slot].captured_card;
 return card&&card<=s.card_count?card_token(s.cards[card-1]):-1;
}
int App::combat_status_amount(int actor,int id,bool label)const{
 if(actor<0||actor>MaxEnemies||id<0||id>=StatusKinds)return 0;
 const auto value=ui_feedback_bound(*this)?combat_status.shown[actor*StatusKinds+id]:ui_combat_status_value(game,actor,id);
 if(label&&id==EnemyStatusStart+int(EnemyPower::Slow))return value.auxiliary*10;
 if(label&&id==EnemyStatusStart+int(EnemyPower::WitheringPresence))return value.auxiliary;
 return value.amount;
}
int App::combat_status_auxiliary(int actor,int power)const{
 if(actor<0||actor>MaxEnemies||power<0||power>=EnemyPowerCount)return 0;
 return ui_feedback_bound(*this)?combat_status.shown[actor*StatusKinds+EnemyStatusStart+power].auxiliary:
  ui_combat_status_value(game,actor,EnemyStatusStart+power).auxiliary;
}
EnemyId App::combat_status_source(int actor,int power)const{
 if(actor<0||actor>MaxEnemies||power<0||power>=EnemyPowerCount)return EnemyId::Count;
 const unsigned source=ui_feedback_bound(*this)?combat_status.shown[actor*StatusKinds+EnemyStatusStart+power].source:
  ui_combat_status_value(game,actor,EnemyStatusStart+power).source;
 return source?EnemyId(source-1):EnemyId::Count;
}
int App::combat_status_fact(int fact,int slot)const{
 fact+=slot;if(fact<0||fact>=CombatStatus::Facts)return 0;
 return ui_feedback_bound(*this)?combat_status.shown[CombatStatus::FactBase+fact].amount:ui_combat_status_fact(game,fact);
}
int App::combat_statuses(int actor,int*out)const{
 if(actor<0||actor>MaxEnemies)return 0;
 if(!ui_feedback_bound(*this))return collect_statuses(game,actor,out);
 int count=0;for(int id=0;id<StatusKinds;++id)if(combat_status.shown[actor*StatusKinds+id].visible){if(out)out[count]=id;++count;}return count;
}
static void ui_capture_combat_status(App&a,uint32_t at,bool initial=false){
 auto&view=a.combat_status;const auto&s=a.game;
 if(initial){
  view.reset();
  if(view.visual_seed!=s.seed||view.visual_floor!=s.floor||view.visual_combat!=s.world.combat_count||view.visual_encounter!=int(s.enemy_world.encounter)){
   view.reset_visuals();view.visual_seed=s.seed;view.visual_floor=s.floor;view.visual_combat=s.world.combat_count;view.visual_encounter=int(s.enemy_world.encounter);view.star_tick=a.map_clock_ms;
  }
 }
 static constexpr uint8_t visual_flags[]={
#include "combat_power_visuals.inc"
 };
 static_assert(sizeof visual_flags==StatusKinds,"Every displayed power needs its original VFX classification");
 for(unsigned actor=0;actor<CombatStatus::Actors;++actor)for(int id=0;id<StatusKinds;++id){
  const unsigned cell=actor*StatusKinds+id;const auto value=ui_combat_status_value(s,int(actor),id);
  if(initial)view.shown[cell]=view.observed[cell]=value;
  else{
   const auto old=view.observed[cell];const unsigned flags=visual_flags[id];
   // NCreature.OnPowerIncreased creates title/icon VFX; duration ticks and
   // removals do not. Negative-capable counters (Strength/Dexterity/Focus)
   // also report losses, using the power type at the resulting amount.
   const bool increased=value.amount>old.amount,negative=(flags&4)&&value.amount<old.amount&&value.amount<0;
   if((flags&1)&&value.amount&&value.visible&&(increased||negative)&&
      (actor==0?s.hp>0:actor<=unsigned(s.enemy_count)&&s.enemies[actor-1].hp>0)){
    const bool debuff=(flags&4)&&value.amount<0?true:(flags&2)!=0;
    view.popup(actor,unsigned(id),!debuff,ui_feedback_identity(s,actor),at,a.animation);
   }
   view.append(cell,value,at);
  }
 }
 for(unsigned fact=0;fact<CombatStatus::Facts;++fact){const unsigned cell=CombatStatus::FactBase+fact;
  const CombatStatus::Value value{ui_combat_status_fact(s,int(fact)),0,0,0};
  if(initial)view.shown[cell]=view.observed[cell]=value;else view.append(cell,value,at);
 }
}
