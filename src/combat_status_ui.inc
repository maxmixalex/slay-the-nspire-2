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
 if(fact>=CombatStatus::AutomationCount)return s.expansion.automation_count[fact-CombatStatus::AutomationCount];
 if(fact>=CombatStatus::Automation)return s.expansion.automation[fact-CombatStatus::Automation];
 const unsigned slot=fact-CombatStatus::CapturedCard;if(slot>=s.enemy_count)return -1;
 const unsigned card=s.enemy_world.models[slot].captured_card;
 return card&&card<=s.card_count?card_token(s.cards[card-1]):-1;
}
int App::combat_status_amount(int actor,int id,bool label)const{
 if(actor<0||actor>MaxEnemies||id<0||id>=StatusKinds)return 0;
 const auto value=ui_feedback_bound(*this)?combat_status.shown[actor*StatusKinds+id]:ui_combat_status_value(game,actor,id);
 if(label&&id==RegentStatusStart+int(RPower::Monologue)){int applied=0;for(int i=0;i<MaxPowerInstances;++i)applied+=combat_status_fact(CombatStatus::MonologueApplied,i);return applied;}
 if(label&&id==EnemyStatusStart+int(EnemyPower::Slow))return value.auxiliary*10;
 if(label&&id==EnemyStatusStart+int(EnemyPower::WitheringPresence))return value.auxiliary;
 return value.amount;
}
bool App::combat_status_counter(int actor,int id)const{
 if(id<0||id>=StatusKinds||(CombatStatus::PowerFlags[id]&8))return false;
 if(id==41||id==EnemyStatusStart+int(EnemyPower::Shrink))return combat_status_amount(actor,id)>=0;
 if(id==RegentStatusStart+int(RPower::Monologue))return combat_status_amount(actor,id,true)!=0;
 return true;
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
 uint16_t tokens[MaxPlayerPowerEntries];const unsigned n=combat_power_entries(actor,tokens,MaxPlayerPowerEntries);bool seen[StatusKinds]{};int count=0;
 for(unsigned i=0;i<n;++i){const unsigned id=power_id(tokens[i]);if(!seen[id]){seen[id]=true;if(out)out[count]=int(id);++count;}}
 // Original independent controls remain available in this optional details
 // browser. They never enter the model order or the on-creature power strip.
 if(!actor){if(game.character==Character::Necrobinder||combat_status_fact(CombatStatus::OstyMax)>0){if(out)out[count]=45;++count;}
  if(combat_status_fact(CombatStatus::OrbSlots)>0){if(out)out[count]=46;++count;}}
 return count;
}
int App::combat_power_amount(uint16_t token,bool label)const{
 const unsigned actor=power_actor(token),id=power_id(token),slot=power_instance(token);
 if(!power_instanced(id))return combat_status_amount(int(actor),int(id),label);
 if(actor)return 0;
 if(id==18+unsigned(XPower::Automation))return label?10-combat_status_fact(CombatStatus::AutomationCount,slot):combat_status_fact(CombatStatus::Automation,slot);
 if(id==18+unsigned(XPower::Panache))return label?5-combat_status_fact(CombatStatus::PanacheCards,slot):combat_status_fact(CombatStatus::Panache,slot);
 if(id==18+unsigned(XPower::RollingBoulder))return combat_status_fact(CombatStatus::Boulder,slot);
 if(id==43)return combat_status_fact(CombatStatus::BombTurns,slot);
 if(id==CharacterExtraStatusStart)return combat_status_fact(CombatStatus::NightmareActive,slot)?3:0;
 if(id==RegentStatusStart+unsigned(RPower::Monologue))return combat_status_fact(label?CombatStatus::MonologueApplied:CombatStatus::Monologue,slot);
 if(id==RegentStatusStart+unsigned(RPower::Orbit))return label?4-combat_status_fact(CombatStatus::OrbitRemainder,slot):combat_status_fact(CombatStatus::Orbit,slot);
 return 0;
}
bool App::combat_power_counter(uint16_t token)const{
 const unsigned actor=power_actor(token),id=power_id(token);
 if(id==RegentStatusStart+unsigned(RPower::Monologue))return combat_power_amount(token,true)!=0;
 return combat_status_counter(int(actor),int(id));
}
unsigned App::combat_power_entries(int actor,uint16_t*out,unsigned capacity)const{
 if(actor<0||actor>MaxEnemies)return 0;
 const auto&order=ui_feedback_bound(*this)?combat_status.shown_order:game.power_sequence;
 bool seen[StatusKinds]{};uint16_t instances[InstancedPowerFamilies]{};unsigned count=0;
 const auto append=[&](uint16_t token){const unsigned id=power_id(token),slot=power_instance(token);
  if(power_instanced(id)){const unsigned family=power_instance_family(id);const uint16_t bit=uint16_t(1)<<slot;if(instances[family]&bit)return;instances[family]|=bit;}
  else{if(seen[id])return;seen[id]=true;}
  if(out&&count<capacity)out[count]=token;++count;
 };
 for(unsigned i=0;i<order.count;++i){const auto token=order.entries[i];if(power_actor(token)==unsigned(actor)&&combat_power_amount(token))append(token);}
 // Preserve renderability of historical hand-authored/debug fixtures whose
 // direct field edits do not call model APIs. Canonical play/save tests assert
 // that every live token is already in the persistent ordered prefix.
 for(unsigned id=0;id<StatusKinds;++id){if(id==44||id==45||id==46)continue;
  if(power_instanced(id)){if(!actor)for(unsigned slot=0;slot<MaxPowerInstances;++slot){const auto token=power_token(0,id,slot);if(combat_power_amount(token))append(token);}}
  else{const auto value=ui_feedback_bound(*this)?combat_status.shown[actor*StatusKinds+id]:ui_combat_status_value(game,actor,int(id));if(value.visible)append(power_token(unsigned(actor),id));}
 }
 return count;
}
static void ui_capture_combat_status(App&a,uint32_t at,bool initial=false){
 auto&view=a.combat_status;const auto&s=a.game;
 if(initial){
  view.reset();
  if(view.visual_seed!=s.seed||view.visual_floor!=s.floor||view.visual_combat!=s.world.combat_count||view.visual_encounter!=int(s.enemy_world.encounter)){
   view.reset_visuals();view.visual_seed=s.seed;view.visual_floor=s.floor;view.visual_combat=s.world.combat_count;view.visual_encounter=int(s.enemy_world.encounter);view.star_tick=a.map_clock_ms;
  }
 }
 // A combat never shrinks enemy_count: removed actors retain their slots,
 // and a summon either reuses one or extends it. Only the initial snapshot
 // must clear every absent slot; later effects scan actual actor slots.
 unsigned actors=initial?CombatStatus::Actors:unsigned(s.enemy_count)+1;
 if(actors>CombatStatus::Actors)actors=CombatStatus::Actors;
 for(unsigned actor=0;actor<actors;++actor)for(int id=0;id<StatusKinds;++id){
  const unsigned cell=actor*StatusKinds+id;const auto value=ui_combat_status_value(s,int(actor),id);
  if(initial)view.shown[cell]=view.observed[cell]=value;
  else{
   const auto old=view.observed[cell];const unsigned flags=CombatStatus::PowerFlags[id];
   // NCreature.OnPowerIncreased creates title/icon VFX; duration ticks and
   // amount decreases do not. NCreature.OnPowerRemoved has its own VFX.
   // Negative-capable counters (Strength/Dexterity/Focus)
   // also report losses, using the power type at the resulting amount.
   const bool increased=value.amount>old.amount,negative=(flags&4)&&value.amount<old.amount;
   if((flags&1)&&value.amount&&value.visible&&(increased||negative)&&
      (actor==0?s.hp>0:actor<=unsigned(s.enemy_count)&&s.enemies[actor-1].hp>0)){
    const bool debuff=(flags&4)&&value.amount<0?true:(flags&2)!=0;
    view.popup(actor,unsigned(id),!debuff,ui_feedback_identity(s,actor),at,a.animation);
   }
   if((flags&1)&&old.amount&&old.visible&&!value.amount&&
      combat_phase(s.phase)&&(actor==0?s.hp>0:actor<=unsigned(s.enemy_count)&&s.enemies[actor-1].hp>0))
    view.popup(actor,unsigned(id),false,ui_feedback_identity(s,actor),at,a.animation,true);
   view.append(cell,value,at);
  }
 }
 for(unsigned fact=0;fact<CombatStatus::Facts;++fact){const unsigned cell=CombatStatus::FactBase+fact;
  const CombatStatus::Value value{ui_combat_status_fact(s,int(fact)),0,0,0};
  if(initial)view.shown[cell]=view.observed[cell]=value;else view.append(cell,value,at);
 }
 view.capture_order(s.power_sequence,at,initial);
}
