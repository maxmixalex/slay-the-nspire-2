// Stateful move bodies use saved continuation commands, including multi-hits.
static void enemy_execute_special(State&,int,int);
static void enemy_die(State&,int);
static void enemy_escape(State&,int);
static void enemy_generate_card(State&,CardKind,int,int);
static void enemy_queue_move(State&s,int t,int node){
 auto&m=s.enemy_world.models[t];EnemyCommands q{s,t};auto value=[&](EnemyValue v){return enemy_number(s,t,v);};
 if(node==30){q.heal(t,s.enemies[t].max_hp);q.flush();return;}
 if(node==31){if(m.kind==EnemyId::BowlbugRock)m.data[0]=0;if(m.kind==EnemyId::SlumberingBeetle)q.remove(t,EnemyPower::Plating);q.flush();return;}
 auto fixed=enemy_fixed_span(m.kind,(unsigned)node);
 if(fixed.count!=255){
  for(unsigned i=0;i<fixed.count;++i){const auto&a=enemy_fixed_actions[fixed.first+i];int target=a.target==-2?t:a.target,amount=enemy_asc(s,a.base,a.tough,a.deadly);
   switch(a.op){
   case EnemyFixedOp::Attack:{int hits=a.extra;if((m.kind==EnemyId::Entomancer&&node==1)||(m.kind==EnemyId::Exoskeleton&&node==0)||(m.kind==EnemyId::TrackerRubyRaider&&node==1))hits+=s.enemy_world.ascension>=9;q.attack(amount,hits);if((m.kind==EnemyId::CeremonialBeast&&node==1)||(m.kind==EnemyId::CrossbowRubyRaider&&node==0))q.special(254);break;}
   case EnemyFixedOp::Power:q.power(target,(EnemyPower)a.extra,amount);break;
   case EnemyFixedOp::Block:q.block(target,amount,a.extra!=0);break;
   case EnemyFixedOp::Heal:q.heal(target,amount);break;
   case EnemyFixedOp::Card:q.card((CardKind)a.extra,amount,a.target);break;
   }
  }
  if(m.kind==EnemyId::FrogKnight&&node==3)m.data[0]=1;
  if(m.kind==EnemyId::BowlbugRock&&node==1)m.data[0]=0;
  q.flush();return;
 }
 switch(m.kind){
 case EnemyId::Aeonglass:q.special(0);break;
 case EnemyId::Axebot:q.block(t,value(EnemyValue::BootUpBlock));q.power(t,EnemyPower::Strength,value(EnemyValue::BootUpStrGain)*m.data[0]);break;
 case EnemyId::BowlbugRock:q.attack(value(EnemyValue::HeadbuttDamage));q.special(0);break;
 case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:q.special(0);break;
 case EnemyId::Entomancer:{int hive=enemy_power_value(s,t,EnemyPower::PersonalHive);if(hive&&hive<3)q.power(t,EnemyPower::PersonalHive,1);q.power(t,EnemyPower::Strength,hive&&hive<3?1:2);break;}
 case EnemyId::Fabricator:if(node==0)q.special(0);else q.attack(value(EnemyValue::FabricatingStrikeDamage));q.special(1);break;
 case EnemyId::FatGremlin:q.op(XOp::EnemyEscape,t);break;
 case EnemyId::Fogmog:q.op(XOp::EnemySpawn,(int)EnemyId::EyeWithTeeth,8*(t+1));break;
 case EnemyId::GasBomb:m.data[0]=1;q.attack(value(EnemyValue::ExplodeDamage));q.op(XOp::EnemyDie,t,1);break;
 case EnemyId::GremlinMerc:q.attack(enemy_raw_move_damage(s,t,node),node==2?1:2);q.special(0);if(node==1)q.power(-1,EnemyPower::Weak,2);if(node==2)q.power(t,EnemyPower::Strength,2);break;
 case EnemyId::Guardbot:for(int i=0;i<s.enemy_count;++i)if(enemy_slot_alive(s,i)&&s.enemy_world.models[i].kind==EnemyId::Fabricator)q.block(i,15,false);break;
 case EnemyId::KnowledgeDemon:q.op(XOp::EnemyKnowledgeChoice,t,0);break;
 case EnemyId::LivingFog:q.special(0);q.attack(value(EnemyValue::BloatDamage));break;
 case EnemyId::MagiKnight:q.power(-1,EnemyPower::Dampen,1);break;
 case EnemyId::Noisebot:q.card(Dazed,1,2);q.card(Dazed,1,0);break;
 case EnemyId::Ovicopter:q.special(0);break;
 case EnemyId::OwlMagistrate:q.attack(value(EnemyValue::VerdictDamage));q.power(-1,EnemyPower::Vulnerable,4);q.remove(t,EnemyPower::Soar);break;
 case EnemyId::Queen:for(int i=0;i<s.enemy_count;++i)if(i!=t&&enemy_slot_alive(s,i))q.power(i,EnemyPower::Strength,1);q.block(t,20);break;
 case EnemyId::SoulFysh:if(node==0){q.card(EventCardBeckon,1,0);q.card(EventCardBeckon,1,2);}else{q.attack(value(EnemyValue::GazeDamage));q.card(EventCardBeckon,1,2);}break;
 case EnemyId::TestSubject:if(node==0)q.special(0);else{q.attack(value(EnemyValue::MultiClawDamage),min(999,3+m.data[1]));q.special(1);}break;
 case EnemyId::TheForgotten:q.attack(enemy_raw_move_damage(s,t,node));break;
 case EnemyId::TheInsatiable:q.power(t,EnemyPower::Sandpit,4);q.card(EventCardFranticEscape,3,0);q.card(EventCardFranticEscape,3,4);m.data[0]=1;break;
 case EnemyId::TheObscura:if(node==0)q.op(XOp::EnemySpawn,(int)EnemyId::Parafright,8*(t+1));else for(int i=0;i<s.enemy_count;++i)if(enemy_slot_alive(s,i))q.power(i,EnemyPower::Strength,3);break;
 case EnemyId::ThievingHopper:if(node==0){q.special(0);q.attack(value(EnemyValue::TheftDamage));}else q.op(XOp::EnemyEscape,t);break;
 case EnemyId::ToughEgg:q.special(0);break;
 case EnemyId::TwoTailedRat:if(node<3){m.data[0]=(int16_t)max(-9999,m.data[0]-1);if(node==2)q.power(-1,EnemyPower::Frail,1);else q.attack(value(node==0?EnemyValue::ScratchDamage:EnemyValue::DiseaseBiteDamage));}else q.special(0);break;
 case EnemyId::WaterfallGiant:if(node==4){q.attack(m.data[0]);q.special(0);}else if(node==6)q.special(1);else{q.attack(m.data[1]);q.op(XOp::EnemyDie,t,1);}break;
 default:fail(s);return;
 }
 q.flush();
}
static void enemy_remove_instance(State&s,int id){
 Pile* piles[]={&s.draw,&s.hand,&s.discard,&s.exhaust,&s.powers};for(auto*p:piles)for(unsigned j=0;j<p->count;++j)if(p->ids[j]==id){remove(*p,(int)j);s.zones[id]=Zone::Limbo;return;}
}
static void enemy_take_card(State&s,int t){
 auto&m=s.enemy_world.models[t];if(m.captured_card)return;
 uint16_t candidates[MaxCards];unsigned n=0;int best=5;
 // Original draw order is top first; the bounded engine stores top last.
 for(int pile=0;pile<2;++pile){const auto&p=pile?s.discard:s.draw;for(unsigned i=0;i<p.count;++i){int id=p.ids[pile?i:p.count-1-i];if(s.event_cards.deck_origin[id]<=0)continue;
  unsigned rarity=card_rarity(s.cards[id].kind);bool imbued=s.event_cards.cards[id].enchantment==EventEnchantment::Imbued;
  int priority=imbued?3:rarity==3?0:(rarity==2||rarity==4||rarity==6)?1:(rarity==1||rarity==10)?2:rarity==5?3:4;
  if(priority<best){best=priority;n=0;}if(priority==best)candidates[n++]=(uint16_t)id;
 }}
 if(!n)return;int id=candidates[world_rng(s,WorldRng::CombatCardGeneration).below(n)],deck=s.event_cards.deck_origin[id]-1;
 if(!event_take_deck_card(s,deck,m.captured_deck))return;enemy_remove_instance(s,id);m.captured_card=(uint16_t)(id+1);enemy_apply_power(s,t,EnemyPower::Swipe,1,t);
}
static void enemy_execute_special(State&s,int t,int step){
 if(!enemy_slot_exists(s,t))return;if(step==254){enemy_visual_attack_end(s,t);return;}auto&m=s.enemy_world.models[t];EnemyCommands q{s,t};auto value=[&](EnemyValue v){return enemy_number(s,t,v);};
 switch(m.kind){
 case EnemyId::Aeonglass:
  for(int id=0;id<s.card_count;++id)if(s.cards[id].kind==EventCardWither)s.enemy_world.card_value[id]=(int16_t)min(9999,s.enemy_world.card_value[id]+1);
  m.data[1]=(int16_t)min(9999,m.data[1]+1);q.card(EventCardWither,value(EnemyValue::WitherAmount));q.power(t,EnemyPower::Strength,value(EnemyValue::IncreasingIntensityBaseStrength)+m.data[0]);m.data[0]=(int16_t)min(9999,m.data[0]+1);break;
 case EnemyId::BowlbugRock:if(m.data[0])enemy_stun_to(s,t,0);break;
 case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:
  if(enemy_teammates(s,t)){m.flags&=(uint8_t)~EnemyDown;q.heal(t,enemy_power_value(s,t,EnemyPower::Reattach));}break;
 case EnemyId::Fabricator:{
  EnemyId pool[2]={step==0?EnemyId::Guardbot:EnemyId::Zapbot,step==0?EnemyId::Noisebot:EnemyId::Stabbot};EnemyId candidates[2];unsigned n=0;for(auto k:pool)if((int)k!=m.data[0])candidates[n++]=k;
  int slot=enemy_open_slot(s);if(slot>=0&&n){EnemyId kind=candidates[world_rng(s,WorldRng::MonsterAi).below(n)];m.data[0]=(int16_t)kind;int spawned=enemy_spawn(s,kind,slot,t);if(spawned>=0)enemy_apply_power(s,spawned,EnemyPower::Minion,1,t);}break;}
 case EnemyId::GremlinMerc:{int gold=min(s.gold,enemy_power_value(s,t,EnemyPower::Thievery));s.gold-=(int16_t)gold;m.stolen_gold=(int16_t)min(9999,m.stolen_gold+gold);record(s,LogKind::Gold,-gold);break;}
 case EnemyId::LivingFog:{int slot=enemy_open_slot(s);if(slot>=0)enemy_spawn(s,EnemyId::GasBomb,slot,t);break;}
 case EnemyId::Ovicopter:for(int i=0;i<3;++i){int slot=enemy_open_slot(s,true);if(slot>=0){int spawned=enemy_spawn(s,EnemyId::ToughEgg,slot,t);if(spawned>=0)enemy_apply_power(s,spawned,EnemyPower::Minion,1,t);}}break;
 case EnemyId::TestSubject:if(step==1)m.data[1]=(int16_t)min(996,m.data[1]+1);else{
  ++m.data[0];m.flags&=(uint8_t)~EnemyDown;int hp=value(m.data[0]==1?EnemyValue::SecondFormHp:EnemyValue::ThirdFormHp);s.enemies[t].max_hp=s.enemies[t].hp=hp;
  if(m.data[0]==1)q.power(t,EnemyPower::PainfulStabs,1);else{q.power(t,EnemyPower::Nemesis,1);q.remove(t,EnemyPower::Adaptable);q.remove(t,EnemyPower::PainfulStabs);}
 }break;
 case EnemyId::ThievingHopper:enemy_take_card(s,t);break;
 case EnemyId::ToughEgg:{
  m.data[0]=1;for(unsigned p=0;p<EnemyPowerCount;++p)if(p!=unsigned(EnemyPower::Minion))enemy_clear_power(s,t,EnemyPower(p));
  int hp=world_rng(s,WorldRng::Niche).range(value(EnemyValue::HatchlingMinHp),value(EnemyValue::HatchlingMaxHp)+1);s.enemies[t].hp=s.enemies[t].max_hp=hp;break;}
 case EnemyId::TwoTailedRat:{
  int slot=enemy_open_slot(s,true);if(slot>=0)enemy_spawn(s,EnemyId::TwoTailedRat,slot,t);
  int max_count=0;for(int i=0;i<s.enemy_count;++i)if(enemy_slot_exists(s,i)&&s.enemy_world.models[i].kind==EnemyId::TwoTailedRat)max_count=max(max_count,s.enemy_world.models[i].data[1]+1);
  for(int i=0;i<s.enemy_count;++i)if(enemy_slot_exists(s,i)&&s.enemy_world.models[i].kind==EnemyId::TwoTailedRat)s.enemy_world.models[i].data[1]=(int16_t)max_count;break;}
 case EnemyId::WaterfallGiant:if(step==0){m.data[0]=(int16_t)min(9999,m.data[0]+value(EnemyValue::PressureGunIncrease));q.power(t,EnemyPower::SteamEruption,3);}else{m.data[1]=(int16_t)enemy_power_value(s,t,EnemyPower::SteamEruption);q.remove(t,EnemyPower::SteamEruption);}break;
 default:fail(s);return;
 }
 q.flush();
}
