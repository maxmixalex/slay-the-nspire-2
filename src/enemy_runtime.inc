// All monster actions use the same bounded, serializable continuation queue as
// cards. A knowledge choice can therefore suspend a multi-enemy turn safely.
static void enemy_queue_turn(State&s,int ordinal){
 auto&w=s.enemy_world;if(!w.active||ordinal<0||ordinal>=w.turn_count)return;
 int t=w.turn_order[ordinal];auto&m=w.models[t];
 if(!enemy_slot_exists(s,t)||m.combat_id!=w.turn_ids[ordinal])return;
 if(s.enemies[t].hp<=0&&!(m.flags&EnemyDown))return;
 w.actor=(uint8_t)t;record(s,LogKind::Enemy,t,m.node);
 queue_extension(s,XOp::EnemyMoveFinish,t,m.node,true);
 queue_extension(s,XOp::EnemyMove,t,m.node,true);
}
static void enemy_generate_card(State&s,CardKind kind,int count,int pile){
 if(count<=0)return;if(count>1)queue_extension(s,XOp::EnemyGenerateCard,(int)kind,count-1+1024*pile,true);
 int id=make_card(s,{kind,0});if(id<0)return;
 if(pile==0||pile==4){
  Pile&dest=pile==0?s.draw:s.discard;unsigned old=dest.count;
  unsigned original=world_rng(s,WorldRng::Shuffle).below(old+1),at=pile==0?old-original:original;
  if(!put(s,dest,id,pile==0?Zone::Draw:Zone::Discard))return;
  for(unsigned i=old;i>at;--i)dest.ids[i]=dest.ids[i-1];dest.ids[at]=(uint16_t)id;
 }else if(pile==1&&s.hand.count<MaxHand)put(s,s.hand,id,Zone::Hand);
 else if(pile==3)put(s,s.exhaust,id,Zone::Exhaust);
 else put(s,s.discard,id,Zone::Discard);
 record(s,LogKind::Create,1,kind);notify_card_generated(s,id);
}
static void enemy_knowledge_choice(State&s,int t){
 if(!enemy_slot_alive(s,t))return;auto&w=s.enemy_world;auto&m=w.models[t];int stage=m.data[0];
 if(stage<0||stage>=3){fail(s);return;}
 const CardKind alternative[]={EventCardMindRot,EventCardSloth,EventCardWasteAway};
 s.expansion.offers[0]={EventCardDisintegration,0};s.expansion.offers[1]={alternative[stage],0};s.expansion.offers[2]={Strike,0};
 Pile pool{};pool.count=2;pool.ids[0]=MaxCards;pool.ids[1]=MaxCards+1;
 w.choice_enemy=(uint8_t)t;w.choice_stage=(uint8_t)stage;set_extended_choice(s,XChoice::EnemyKnowledge,pool,1,1);
}
static bool enemy_choice(State&s,int token){
 auto&w=s.enemy_world;auto&x=s.expansion;
 if(s.phase!=Phase::ExtendedChoice||x.choice!=XChoice::EnemyKnowledge||!w.active||w.choice_enemy>=s.enemy_count||w.choice_stage>=3||
    (token!=MaxCards&&token!=MaxCards+1)||x.choices.count!=2||x.choices.ids[0]!=MaxCards||x.choices.ids[1]!=MaxCards+1||s.effect_count>=MaxEffects)return false;
 int t=w.choice_enemy,stage=w.choice_stage;
 if(!enemy_slot_exists(s,t)||w.models[t].kind!=EnemyId::KnowledgeDemon||w.models[t].data[0]!=stage)return false;
 EnemyPower p=token==MaxCards?EnemyPower::Disintegration:stage==0?EnemyPower::MindRot:stage==1?EnemyPower::Sloth:EnemyPower::WasteAway;
 int amount=token==MaxCards?6+stage:stage==1?3:1;
 clear_extended_choice(s);s.phase=Phase::Combat;w.choice_enemy=255;w.choice_stage=0;++w.models[t].data[0];
 queue_extension(s,XOp::EnemyApplyPower,amount,enemy_power_operand(-1,p,t),true);
 resolve_pending(s);++s.revision;return true;
}
static bool enemy_effect(State&s,Effect e){
 XOp op=(XOp)e.target;int a=e.value,b=e.aux;auto&w=s.enemy_world;
 switch(op){
 case XOp::EnemyMove:
  if(enemy_slot_exists(s,a)&&w.models[a].node==b){w.models[a].flags|=EnemyActed;enemy_visual_move_begin(s,a,b);enemy_queue_move(s,a,b);}break;
 case XOp::EnemyMoveFinish:
  if(enemy_slot_exists(s,a)){
   auto&m=w.models[a];enemy_visual_move_end(s,a,b);if(m.age<65535)++m.age;
   if(m.node==b){m.flags|=EnemyActed;m.flags&=(uint8_t)~EnemyImmediate;}
  }break;
 case XOp::EnemyAttackStart:
  if(enemy_slot_alive(s,a)){auto&m=w.models[a];m.attack_unblocked_hits=m.attack_total_damage=m.data[10]=0;m.attack_vigor=(int16_t)enemy_power_value(s,a,EnemyPower::Vigor);}break;
 case XOp::EnemyHit:{
  int t=b%8,remaining=b/8;if(!enemy_slot_alive(s,t)||remaining<=0||s.hp<=0)break;
  if(remaining>1)queue_extension(s,XOp::EnemyHit,a,t+8*(remaining-1),true);
  int hit=enemy_attack_value(s,t,a);
  if(s.thorns)damage_unpowered(s,t,s.thorns);
  int old_pet=s.expansion.osty_hp,blocked=min(s.block,hit);
  int player_loss=player_hp_loss(s,hit,true,true,-1,t);if(s.phase==Phase::Dead)break;
  auto&m=w.models[t];int pet_loss=max(0,old_pet-s.expansion.osty_hp);
  if(player_loss>0)m.attack_unblocked_hits=(int16_t)min(9999,m.attack_unblocked_hits+1);
  if(player_loss>0||pet_loss>0)m.data[10]=(int16_t)min(9999,m.data[10]+1);
  m.attack_total_damage=(int16_t)min(9999,m.attack_total_damage+player_loss+pet_loss);
  if(enemy_power_value(s,t,EnemyPower::Imbalanced)&&blocked>=hit&&player_loss==0&&pet_loss==0)m.data[0]=1;
  regent_after_blocked_attack(s,t,blocked);
  if(s.flame_barrier)damage_unpowered(s,t,s.flame_barrier);
  break;}
 case XOp::EnemyAttackEnd:
  if(enemy_slot_exists(s,a)){auto&m=w.models[a];int wounds=enemy_power_value(s,a,EnemyPower::PainfulStabs)*m.attack_unblocked_hits;
   if(wounds)queue_extension(s,XOp::EnemyGenerateCard,Wound,min(MaxCards,wounds)+2048,true);
   int suck=enemy_power_value(s,a,EnemyPower::Suck)*m.data[10];if(suck)enemy_apply_power(s,a,EnemyPower::Strength,suck,a);
   if(m.attack_vigor)enemy_clear_power(s,a,EnemyPower::Vigor);m.attack_vigor=0;
  }
  world_relic_hook(s,WorldRelicHook::AfterAttack,a,-1,-1);break;
 case XOp::EnemyGainBlock:enemy_gain_block(s,b%8,a,b>=8);break;
 case XOp::EnemyApplyPower:enemy_apply_power(s,(b/80)%9-1,(EnemyPower)(b%80),a,b/720-1);break;
 case XOp::EnemyRemovePower:enemy_clear_power(s,a,(EnemyPower)b);break;
 case XOp::EnemyHeal:
  if(b<0)heal(s,a);else if(enemy_slot_exists(s,b)){s.enemies[b].hp=min(s.enemies[b].max_hp,s.enemies[b].hp+a);if(s.enemies[b].hp>0)w.models[b].flags&=(uint8_t)~EnemyDown;}break;
 case XOp::EnemyPlayerDamage:player_hp_loss(s,a,false,(b&1)!=0,-1);break;
 case XOp::EnemyLoseMaxHp:player_lose_max_hp(s,a);break;
 case XOp::EnemyGenerateCard:enemy_generate_card(s,(CardKind)a,b%1024,b/1024);break;
 case XOp::EnemySpawn:{int slot=b%8,owner=b/8-1;bool occupied=false;for(int t=0;t<s.enemy_count;++t)if(enemy_slot_exists(s,t)&&w.models[t].slot==slot)occupied=true;if(!occupied)enemy_spawn(s,(EnemyId)a,slot,owner);break;}
 case XOp::EnemyEscape:enemy_escape(s,a);break;
 case XOp::EnemyDie:
  if(b==1&&enemy_slot_exists(s,a))s.enemies[a].hp=0;
  if(b==2)enemy_die(s,a);
  else if(enemy_slot_exists(s,a)&&s.enemies[a].hp==0){
   w.models[a].flags|=EnemyDying;enemy_visual_snapshot_death(s,a);
   // Swipe.BeforeDeath registers an optional exact-card reward before any
   // AfterDeath listeners; claiming it later is the only deck insertion.
   enemy_before_death(s,a);if(s.phase==Phase::Error)break;
   queue_extension(s,XOp::EnemyDie,a,2,true);
   world_relic_hook(s,WorldRelicHook::AfterEnemyDeath,a,enemy_power_value(s,a,EnemyPower::Minion)!=0);
  }break;
 case XOp::EnemyTransform:
  if(enemy_slot_exists(s,a)){auto&m=w.models[a];m.kind=(EnemyId)b;int hp=enemy_unique_hp(s,m.kind,a);s.enemies[a].hp=s.enemies[a].max_hp=hp;enemy_initial_powers(s,a);enemy_set_move(s,a,enemy_initial_node(s,a));m.previous=m.node;}break;
 case XOp::EnemySideHook:if(b)enemy_side_end(s,a!=0);else enemy_side_start(s,a!=0,a!=2);break;
 case XOp::EnemyPowerHook:enemy_power_hook(s,a,(EnemyPower)(b%EnemyPowerCount),b/EnemyPowerCount);break;
 case XOp::EnemyCardHook:if(b==0)enemy_card_drawn(s,a);else if(b==1)enemy_card_generated(s,a);else if(b==2)enemy_card_after_body(s,a);break;
 case XOp::EnemyKnowledgeChoice:enemy_knowledge_choice(s,a);break;
 case XOp::EnemyFranticEscape:
  for(int t=0;t<s.enemy_count;++t)if(enemy_slot_alive(s,t)&&enemy_power_value(s,t,EnemyPower::Sandpit)){enemy_apply_power(s,t,EnemyPower::Sandpit,1,t);break;}
  w.card_value[a]=(int16_t)min(9999,w.card_value[a]+1);break;
 case XOp::EnemyWither:w.card_value[a]=(int16_t)min(9999,w.card_value[a]+1);break;
 case XOp::EnemyCardDamage:player_hp_loss(s,a,false,(b&1)==0,b>>1);break;
 case XOp::EnemyAwaken:enemy_stun_to(s,a,b);break;
 case XOp::EnemyActionSpecial:enemy_execute_special(s,a,b);break;
 default:return false;
 }
 return true;
}
