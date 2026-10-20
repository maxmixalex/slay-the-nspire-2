// Death is a saved continuation. Marking every pending death first prevents a
// temporary empty board from ending Stock, Infested, or multi-phase encounters.
static void enemy_face_remaining(State&s){
 if(!enemy_power_value(s,-1,EnemyPower::Surrounded))return;
 bool left=false,right=false;
 for(int t=0;t<s.enemy_count;++t)if(enemy_allow_target(s,t)){
  left|=enemy_power_value(s,t,EnemyPower::BackAttackLeft)!=0;
  right|=enemy_power_value(s,t,EnemyPower::BackAttackRight)!=0;
 }
 if(left!=right)s.enemy_world.facing=(uint8_t)left;
}
static void enemy_release_captured_instance(State&s,int t,bool restore){
 auto&m=s.enemy_world.models[t];if(!m.captured_card)return;
 const int id=m.captured_card-1;
 if(restore)event_restore_deck_card(s,m.captured_deck);
 // A stolen card was removed from combat. Keep its instance in the terminal
 // pile once its holder leaves; it is never returned to the hand or draw pile.
 if(id>=0&&id<s.card_count&&s.zones[id]==Zone::Limbo)put(s,s.exhaust,id,Zone::Exhaust);
 m.captured_card=0;m.captured_deck=EventDeckSnapshot{};
}
static void enemy_player_died(State&s){for(int t=0;t<s.enemy_count;++t)enemy_release_captured_instance(s,t,false);}
static void enemy_applier_died(State&s,int t){
 auto&w=s.enemy_world;
 const EnemyPower remove_on_applier_death[]={EnemyPower::Constrict,EnemyPower::Hex,EnemyPower::Shrink};
 for(EnemyPower p:remove_on_applier_death)
  if(enemy_source(s,-1,p)==t+1)enemy_clear_power(s,-1,p);
 if(w.dampen_casters&(1u<<t)){
  w.dampen_casters&=(uint8_t)~(1u<<t);
  if(!w.dampen_casters)enemy_clear_power(s,-1,EnemyPower::Dampen);
 }
 const EnemyPower possess[]={EnemyPower::PossessSpeed,EnemyPower::PossessStrength};
 for(EnemyPower p:possess){
  int stolen=enemy_aux(s,t,p);if(stolen<0){
   enemy_apply_power(s,-1,p==EnemyPower::PossessSpeed?EnemyPower::Dexterity:EnemyPower::Strength,-stolen,-1);
   enemy_aux(s,t,p)=0;
  }
 }
}
static void enemy_escape(State&s,int t){
 if(!enemy_slot_exists(s,t))return;auto&m=s.enemy_world.models[t];
 enemy_release_captured_instance(s,t,false);
 m.flags|=EnemyRemoved|EnemyEscaped;m.flags&=(uint8_t)~(EnemyDown|EnemyDying);s.enemies[t].hp=0;
 if(s.enemy_world.escaped_count<65534)++s.enemy_world.escaped_count;
 enemy_face_remaining(s);
}
static void enemy_die(State&s,int t){
 if(!enemy_slot_exists(s,t)||s.enemies[t].hp>0)return;
 auto&w=s.enemy_world;auto&m=w.models[t];
 const EnemyId kind=m.kind;const int named_slot=m.slot;
 const int stock=enemy_power_value(s,t,EnemyPower::Stock),infested=enemy_power_value(s,t,EnemyPower::Infested);
 const bool surprise=enemy_power_value(s,t,EnemyPower::Surprise)!=0;
 const int stolen=m.stolen_gold;
 const bool illusion=enemy_power_value(s,t,EnemyPower::Illusion)!=0;
 const bool adaptable=enemy_power_value(s,t,EnemyPower::Adaptable)!=0;
 const bool reattach=enemy_power_value(s,t,EnemyPower::Reattach)!=0;
 const bool steam=enemy_power_value(s,t,EnemyPower::SteamEruption)!=0;
 if(enemy_power_value(s,t,EnemyPower::Heist)&&stolen>0){
  if(w.returned_gold_count>=32){fail(s);return;}
  w.returned_gold[w.returned_gold_count++]=(uint16_t)stolen;
 }
 enemy_release_captured_instance(s,t,true);enemy_applier_died(s,t);
 // Death observers run even when the defeated creature will revive.
 int ordered[MaxEnemies];unsigned n=enemy_ordered_slots(s,ordered);
 for(unsigned at=0;at<n;++at){int other=ordered[at];if(other==t||!enemy_slot_alive(s,other))continue;
  int ravenous=enemy_power_value(s,other,EnemyPower::Ravenous);
  if(ravenous){enemy_stun(s,other);enemy_apply_power(s,other,EnemyPower::Strength,ravenous,other);}
  if(enemy_power_value(s,other,EnemyPower::CrabRage)){
   enemy_apply_power(s,other,EnemyPower::Strength,6,other);
   enemy_gain_block(s,other,99,true);
   enemy_clear_power(s,other,EnemyPower::CrabRage);
  }
  auto&observer=w.models[other];
  if(kind==EnemyId::TorchHeadAmalgam&&observer.kind==EnemyId::Queen){observer.data[0]=1;if(observer.node==2)enemy_set_move(s,other,5,true);}
 }
 if(illusion){m.resume_node=m.previous;m.flags|=EnemyDown;enemy_set_move(s,t,30,true);}
 else if(adaptable){m.flags|=EnemyDown;enemy_set_move(s,t,0,true);}
 else if(reattach){m.flags|=EnemyDown;enemy_set_move(s,t,3,true);}
 else if(steam){m.data[2]=1;s.enemies[t].hp=s.enemies[t].max_hp=999999999;enemy_set_move(s,t,6,true);}
 const bool retained=illusion||adaptable||reattach||steam;
 // Illusions retain buffs and temporary powers. Other revivers preserve only
 // their explicitly persistent powers, as the original death hooks require.
 for(unsigned p=0;p<EnemyPowerCount;++p){EnemyPower power=(EnemyPower)p;bool keep=false;
  if(illusion)keep=!enemy_power_meta[p].debuff||power==EnemyPower::TemporaryStrength;
  else keep=power==EnemyPower::Adaptable||power==EnemyPower::PainfulStabs||power==EnemyPower::Reattach||power==EnemyPower::SteamEruption;
  if(!keep)enemy_clear_power(s,t,power);
 }
 if(!illusion){s.enemies[t].poison=0;s.expansion.doom[t]=0;s.expansion.demise[t]=0;s.silent.strangle[t]=0;s.necro.hang[t]=s.necro.oblivion[t]=s.necro.sic_em[t]=0;}
 m.flags&=(uint8_t)~EnemyDying;
 if(!retained)m.flags|=EnemyRemoved;
 // Replacement actors receive new creature identities. The turn snapshot
 // contains identities, so reusing a physical slot cannot grant an extra turn.
 if(stock>0){
  int replacement=enemy_create(s,EnemyId::Axebot,named_slot,-1,10*(3-stock));
  if(replacement>=0){auto&fresh=w.models[replacement];fresh.data[0]=(int16_t)(3-stock);enemy_initial_powers(s,replacement);
   if(w.side)fresh.data[11]=1;else{enemy_set_move(s,replacement,enemy_initial_node(s,replacement));fresh.previous=fresh.node;}
   world_relic_hook(s,WorldRelicHook::AfterEnemyAdded,replacement);}
 }
 if(infested)for(int slot=1;slot<=4;++slot)enemy_spawn(s,EnemyId::Wriggler,slot,t);
 if(surprise){
  // Fat is created first in the original, before Sneaky is attached.
  int fat=enemy_create(s,EnemyId::FatGremlin,1,t);
  if(fat>=0){w.models[fat].stolen_gold=(int16_t)stolen;enemy_initial_powers(s,fat);if(stolen)enemy_apply_power(s,fat,EnemyPower::Heist,stolen,t);}
  enemy_spawn(s,EnemyId::SneakyGremlin,0,t);
  if(fat>=0){if(w.side)w.models[fat].data[11]=1;else{enemy_set_move(s,fat,enemy_initial_node(s,fat));w.models[fat].previous=w.models[fat].node;}
   world_relic_hook(s,WorldRelicHook::AfterEnemyAdded,fat);}
 }
 enemy_face_remaining(s);
}
static void enemy_check_deaths(State&s){
 if(!s.enemy_world.active||s.phase==Phase::Dead||s.phase==Phase::Error)return;
 int order[MaxEnemies];unsigned n=enemy_ordered_slots(s,order);
 for(unsigned i=n;i>0;--i){int t=order[i-1];auto&m=s.enemy_world.models[t];
  if(s.enemies[t].hp==0&&!(m.flags&(EnemyDown|EnemyDying))){m.flags|=EnemyDying;queue_extension(s,XOp::EnemyDie,t,0,true);}
 }
}
