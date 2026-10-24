// A model's first application appends it; amount changes retain its place;
// removing and later applying it appends a new position. No rendering clock,
// RNG, effect or pointer is stored. Tokens also distinguish instanced powers.
void power_order_set(State&s,unsigned actor,unsigned id,bool present,unsigned instance){
 if(actor>MaxEnemies||id>=StatusKinds||instance>=MaxPowerInstances||(!power_instanced(id)&&instance)||
    (actor&&power_instanced(id))||id==44||id==45||id==46)return;
 auto&order=s.power_sequence;const uint16_t token=power_token(actor,id,instance);
 unsigned at=0;while(at<order.count&&order.entries[at]!=token)++at;
 if(present){if(at==order.count&&order.count<MaxPowerOrderEntries)order.entries[order.count++]=token;}
 else if(at<order.count){for(unsigned i=at+1;i<order.count;++i)order.entries[i-1]=order.entries[i];order.entries[--order.count]=0;}
}
void power_order_clear_actor(State&s,unsigned actor){
 auto&order=s.power_sequence;unsigned n=0;
 for(unsigned i=0;i<order.count;++i)if(power_actor(order.entries[i])!=actor)order.entries[n++]=order.entries[i];
 for(unsigned i=n;i<order.count;++i)order.entries[i]=0;order.count=uint16_t(n);
}
void power_order_prune(State&s){
 auto&order=s.power_sequence;unsigned n=0;
 for(unsigned i=0;i<order.count;++i)if(power_token_present(s,order.entries[i]))order.entries[n++]=order.entries[i];
 for(unsigned i=n;i<order.count;++i)order.entries[i]=0;order.count=uint16_t(n);
}
static void power_order_append_current(State&s,unsigned actor,unsigned id){
 if(actor>unsigned(s.enemy_count)||id>=StatusKinds)return;
 if(power_instanced(id)){if(!actor)for(unsigned instance=0;instance<MaxPowerInstances;++instance)
   if(power_instance_amount(s,id,instance))power_order_set(s,0,id,true,instance);
 }else if(power_amount(s,int(actor),int(id),true))power_order_set(s,actor,id,true);
}
void power_order_reconcile(State&s){
 power_order_prune(s);
 // This bounded pass is for migration/save validation and old hand-authored
 // fixtures. Runtime model commands register at their actual mutation site.
 for(unsigned actor=0;actor<=unsigned(s.enemy_count);++actor)
  for(unsigned id=0;id<StatusKinds;++id)power_order_append_current(s,actor,id);
}
void power_order_migrate(State&s){
 s.power_sequence={};
 // Old saves retain these per-family ordered lists, but never wrote their
 // cross-family interleaving. Keep all recoverable local order; explicitly
 // migrate the unavailable interleaving by this stable directory order.
 for(unsigned i=0;i<s.power_count;++i)power_order_append_current(s,0,s.power_order[i]?8:7);
 for(unsigned i=0;i<s.ironclad.power_count;++i)power_order_append_current(s,0,IroncladStatusStart+s.ironclad.power_order[i]);
 for(unsigned i=0;i<s.silent.power_count;++i){const unsigned value=s.silent.power_order[i];
  if(value<SPowerCount)power_order_append_current(s,0,silent_power_id(value));
  else if(value<SPowerCount+MaxNightmares&&s.silent.nightmare_active[value-SPowerCount])power_order_set(s,0,CharacterExtraStatusStart,true,value-SPowerCount);
 }
 for(unsigned i=0;i<s.defect.power_count;++i)power_order_append_current(s,0,DefectStatusStart+s.defect.power_order[i]);
 for(unsigned i=0;i<s.regent.power_count;++i)power_order_append_current(s,0,RegentStatusStart+s.regent.power_order[i]);
 for(unsigned i=0;i<s.necro.power_count;++i)power_order_append_current(s,0,NecroStatusStart+s.necro.power_order[i]);
 for(unsigned i=0;i<s.event_cards.power_count;++i)if(s.event_cards.power_order[i]<EventCardPowerKinds)
  power_order_append_current(s,0,EventCardStatusStart+s.event_cards.power_order[i]);
 for(unsigned actor=0;actor<=unsigned(s.enemy_count);++actor){
  const uint8_t*order=actor?s.enemy_world.models[actor-1].power_order:s.enemy_world.player_power_order;
  const unsigned count=actor?s.enemy_world.models[actor-1].power_count:s.enemy_world.player_power_count;
  for(unsigned i=0;i<count;++i)power_order_append_current(s,actor,enemy_power_id(actor,EnemyPower(order[i])));
  if(actor)for(unsigned i=0;i<s.necro.debuff_count[actor-1];++i)
   power_order_append_current(s,actor,debuff_power_id(NDebuff(s.necro.debuff_order[actor-1][i]-1)));
 }
 power_order_reconcile(s);
}
bool valid_power_order(const State&s){
 const auto&order=s.power_sequence;if(order.count>MaxPowerOrderEntries)return false;
 // Directly authored test fixtures may change amounts without emitting model
 // commands. Validate identity/uniqueness here; the save copy reconciles live
 // membership without mutating the caller, RNG, or historical fields.
 uint32_t seen[MaxEnemies+1][(StatusKinds+31)/32]{};
 uint16_t instances[InstancedPowerFamilies]{};
 for(unsigned i=0;i<order.count;++i){const uint16_t token=order.entries[i];const unsigned actor=power_actor(token),id=power_id(token),slot=power_instance(token);
  if(actor>MaxEnemies||id>=StatusKinds||id==44||id==45||id==46)return false;
  if(power_instanced(id)){const unsigned family=power_instance_family(id);if(actor||(instances[family]&(uint16_t(1)<<slot)))return false;instances[family]|=uint16_t(1)<<slot;}
  else{const uint32_t bit=uint32_t(1)<<(id%32);if(slot||(seen[actor][id/32]&bit))return false;seen[actor][id/32]|=bit;}
 }
 for(unsigned i=order.count;i<MaxPowerOrderEntries;++i)if(order.entries[i])return false;
 return true;
}
