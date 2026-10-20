// Saved power order mirrors acquisition order; hooks never iterate a changing
// collection while an earlier listener is awaiting a card or player choice.
static void enemy_power_hook(State&,int,EnemyPower,int);
static bool enemy_card_can_play(const State&s,int id,bool){
 if(!s.enemy_world.active||id<0||id>=s.card_count)return true;const auto&w=s.enemy_world;
 EnemyAffliction a=(EnemyAffliction)w.affliction[id];
 if(a==EnemyAffliction::Bound&&w.bound_played)return false;
 if(a==EnemyAffliction::Smog)return false;
 if(a==EnemyAffliction::Ringing&&w.cards_played_turn)return false;
 int limit=enemy_power_value(s,-1,EnemyPower::Sloth);return !limit||w.cards_played_turn<(unsigned)limit;
}
static bool enemy_card_ethereal(const State&s,int id){return s.enemy_world.active&&id>=0&&id<s.card_count&&s.enemy_world.affliction[id]==(unsigned)EnemyAffliction::Hexed;}
static int enemy_card_energy_cost(const State&s,int id,int base){
 if(!s.enemy_world.active||id<0||id>=s.card_count||base<0)return base;
 if(s.enemy_world.affliction[id]==(unsigned)EnemyAffliction::Entangled)base+=enemy_power_value(s,-1,EnemyPower::Tangled);
 if(s.cards[id].kind==EventCardFranticEscape)base+=s.enemy_world.card_value[id];
 return min(999,base);
}
static void enemy_before_potion(State&s,int target){
 if(!s.enemy_world.active||!enemy_allow_target(s,target)||!enemy_power_value(s,-1,EnemyPower::Surrounded))return;
 if(enemy_power_value(s,target,EnemyPower::BackAttackLeft))s.enemy_world.facing=1;
 else if(enemy_power_value(s,target,EnemyPower::BackAttackRight))s.enemy_world.facing=0;
}
static void enemy_before_card_play(State&s,int id,bool replay){
 if(!s.enemy_world.active||id<0||id>=s.card_count)return;auto&w=s.enemy_world;
 if(!replay&&w.affliction[id]==(unsigned)EnemyAffliction::Bound)w.bound_played=1;
 if(w.cards_played_turn<65535)++w.cards_played_turn;
 if(card_type(s,s.cards[id],id)<=CardType::Power)w.used_types|=(uint8_t)(1u<<(unsigned)card_type(s,s.cards[id],id));
 enemy_before_potion(s,s.expansion.target);
 for(unsigned t=0;t<MaxEnemies;++t)w.turn_attacks[t]=0;
}
static void enemy_card_drawn(State&s,int id){
 if(!s.enemy_world.active||s.enemy_world.side||id<0||id>=s.card_count)return;
 int limit=enemy_power_value(s,-1,EnemyPower::ChainsOfBinding);
 if(s.enemy_world.bound_draws_turn<(unsigned)limit&&enemy_afflict_card(s,id,EnemyAffliction::Bound,limit))++s.enemy_world.bound_draws_turn;
}
static void enemy_card_created(State&s,int id){
 if(!s.enemy_world.active||id<0||id>=s.card_count)return;
 auto&w=s.enemy_world;CardType type=card_type(s,s.cards[id],id);
 // Player powers are visited before enemy powers in the original hook set.
 for(unsigned i=0;i<w.player_power_count;++i){auto p=(EnemyPower)w.player_power_order[i];int amount=enemy_power_value(s,-1,p);
  if(p==EnemyPower::Hex)enemy_afflict_card(s,id,EnemyAffliction::Hexed,amount);
  else if(p==EnemyPower::Ringing)enemy_afflict_card(s,id,EnemyAffliction::Ringing,amount);
  else if(p==EnemyPower::Tangled&&type==CardType::Attack)enemy_afflict_card(s,id,EnemyAffliction::Entangled,amount);
  else if(p==EnemyPower::Smoggy&&type==CardType::Skill&&(w.used_types&(1u<<(unsigned)CardType::Skill)))enemy_afflict_card(s,id,EnemyAffliction::Smog,amount);
 }
 int order[MaxEnemies];unsigned n=enemy_ordered_slots(s,order);
 for(unsigned i=0;i<n;++i){int t=order[i];if(!enemy_slot_alive(s,t))continue;auto&m=w.models[t];
  for(unsigned j=0;j<m.power_count;++j){EnemyPower p=(EnemyPower)m.power_order[j];int amount=enemy_power_value(s,t,p);
   if(p==EnemyPower::Galvanic&&type==CardType::Power)enemy_afflict_card(s,id,EnemyAffliction::Galvanized,amount);
   else if(p==EnemyPower::VitalSpark&&type==CardType::Skill)enemy_afflict_card(s,id,EnemyAffliction::Tainted,amount);
  }
 }
}
static void enemy_card_generated(State&s,int id){
 if(!s.enemy_world.active||id<0||id>=s.card_count)return;
 if(s.cards[id].kind==EventCardWither){int upgrades=0;for(int t=0;t<s.enemy_count;++t)if(enemy_slot_alive(s,t)&&s.enemy_world.models[t].kind==EnemyId::Aeonglass)upgrades+=s.enemy_world.models[t].data[1];s.enemy_world.card_value[id]=(int16_t)min(9999,upgrades);}
}
static void enemy_card_reset(State&s,int id){if(id<0||id>=MaxCards)return;enemy_clear_affliction(s,id);s.enemy_world.card_value[id]=s.enemy_world.dampen_restore[id]=0;enemy_card_created(s,id);}
static void enemy_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&w=s.enemy_world;
 w.affliction[to]=w.affliction[from];w.affliction_amount[to]=w.affliction_amount[from];w.card_value[to]=w.card_value[from];w.dampen_restore[to]=w.dampen_restore[from];
}
static void enemy_card_after_body(State&s,int id){
 // Afflictions in the pinned build are markers. Their caster's AfterCardPlayed
 // hook owns the damage/power application, after player and relic listeners.
 (void)s;(void)id;
}
static void enemy_player_after_play_listener(State&s,unsigned actor){
 if(!s.enemy_world.active||s.playing<0||s.playing>=s.card_count)return;
 if(actor==(unsigned)PostPlayActor::EnemyTender&&enemy_power_value(s,-1,EnemyPower::Tender)){
  enemy_aux(s,-1,EnemyPower::Tender)=(int16_t)min(9999,enemy_aux(s,-1,EnemyPower::Tender)+1);
  int source=enemy_source(s,-1,EnemyPower::Tender)-1;
  queue_extension(s,XOp::EnemyApplyPower,-1,enemy_power_operand(-1,EnemyPower::Dexterity,source),true);
  queue_extension(s,XOp::EnemyApplyPower,-1,enemy_power_operand(-1,EnemyPower::Strength,source),true);
 }else if(actor==(unsigned)PostPlayActor::EnemySmoggy&&card_type(s,s.cards[s.playing],s.playing)==CardType::Skill){
  for(int id=0;id<s.card_count;++id)if(card_type(s,s.cards[id],id)==CardType::Skill)enemy_afflict_card(s,id,EnemyAffliction::Smog,1);
 }
}
static void enemy_after_card_play(State&s,int id){
 if(!s.enemy_world.active||id<0||id>=s.card_count)return;
 int order[MaxEnemies];unsigned n=enemy_ordered_slots(s,order);
 for(unsigned at=n;at>0;--at){int t=order[at-1];if(!enemy_slot_alive(s,t))continue;const auto&m=s.enemy_world.models[t];
  for(unsigned i=m.power_count;i>0;--i)queue_extension(s,XOp::EnemyPowerHook,t,m.power_order[i-1]+EnemyPowerCount*4,true);
 }
}
static void enemy_cards_turn_end(State&s){
 if(!s.enemy_world.active)return;
 s.enemy_world.bound_played=0;enemy_clear_matching_afflictions(s,EnemyAffliction::Bound);
 for(unsigned at=s.hand.count;at>0;--at)enemy_token_turn_end(s,s.hand.ids[at-1]);
}
static bool enemy_description_value(const State&s,Card c,int,int id,const char*key,int&out){
 if(c.kind==EventCardDisintegration&&enemy_text_equal(key,"DisintegrationPower")){out=6+s.enemy_world.choice_stage;return true;}
 if(c.kind==EventCardWither&&id>=0&&id<s.card_count&&enemy_text_equal(key,"Damage")){out=3+3*s.enemy_world.card_value[id];return true;}
 return false;
}
static void enemy_note_legacy_power(State&s,int t,NDebuff d){
 if(!s.enemy_world.active||t<0||t>=s.enemy_count)return;
 EnemyPower p=d==NDebuff::Weak?EnemyPower::Weak:d==NDebuff::Vulnerable?EnemyPower::Vulnerable:EnemyPower::Strength;
 if(d!=NDebuff::Weak&&d!=NDebuff::Vulnerable&&d!=NDebuff::Strength)return;
 if(enemy_power_value(s,t,p))enemy_power_order_add(s,t,p);else enemy_power_order_remove(s,t,p);
}
static void enemy_prune_legacy_powers(State&s){
 if(!s.enemy_world.active)return;
 constexpr EnemyPower mirrored[]={EnemyPower::Strength,EnemyPower::Weak,EnemyPower::Vulnerable,EnemyPower::Artifact,EnemyPower::Debilitate};
 for(int t=0;t<s.enemy_count;++t)for(auto p:mirrored)if(!enemy_power_value(s,t,p)){
  enemy_power_order_remove(s,t,p);enemy_aux(s,t,p)=0;enemy_skip(s,t,p)=0;enemy_source(s,t,p)=0;
 }
}
static void enemy_before_damage(State&s,int t,int,bool attack,bool pet){
 if(!s.enemy_world.active||!enemy_slot_alive(s,t)||(!attack&&(s.playing<0||s.cards[s.playing].kind!=Omnislice)))return;
 int thorns=enemy_power_value(s,t,EnemyPower::Thorns);if(!thorns)return;
 if(pet&&s.expansion.osty_hp){int old=s.expansion.osty_hp;s.expansion.osty_hp=(int16_t)max(0,old-thorns);necro_osty_hp_lost(s,old);}
 else player_hp_loss(s,thorns,false,true,-1);
}
static void enemy_after_damage(State&s,int t,int blocked,int loss,bool attack,bool){
 if(!s.enemy_world.active||t<0||t>=s.enemy_count||!enemy_slot_exists(s,t))return;
 auto&w=s.enemy_world;auto&m=w.models[t];
 if(loss>0){w.turn_damage[t]=(int16_t)min(9999,w.turn_damage[t]+loss);if(attack)w.turn_attacks[t]=(int16_t)min(9999,w.turn_attacks[t]+loss);}
 uint8_t order[EnemyPowerCount];unsigned n=m.power_count;for(unsigned i=0;i<n;++i)order[i]=m.power_order[i];
 EnemyCommands q{s,t};
 for(unsigned i=0;i<n;++i){EnemyPower p=(EnemyPower)order[i];int amount=enemy_power_value(s,t,p);if(!amount)continue;
  switch(p){
  case EnemyPower::PersonalHive:if(attack)q.card(Dazed,amount,0);break;
  case EnemyPower::CurlUp:if(attack&&s.playing>=0&&!enemy_aux(s,t,p))enemy_aux(s,t,p)=(int16_t)(s.playing+1);break;
  case EnemyPower::HardenedShell:enemy_aux(s,t,p)=(int16_t)min(9999,enemy_aux(s,t,p)+loss);break;
  case EnemyPower::Slippery:if(loss>0)enemy_tick(s,t,p);break;
  case EnemyPower::Flutter:if(loss>0&&attack){enemy_tick(s,t,p);if(!enemy_power_value(s,t,p)&&enemy_slot_alive(s,t))enemy_stun_to(s,t,enemy_next_node(s,t));}break;
  case EnemyPower::Burrowed:if(blocked>0&&!s.enemies[t].block){enemy_stun_to(s,t,0);enemy_clear_power(s,t,p);}break;
  case EnemyPower::Asleep:if(loss>0){enemy_clear_power(s,t,EnemyPower::Plating);enemy_stun_to(s,t,1);enemy_clear_power(s,t,p);}break;
  case EnemyPower::Slumber:if(loss>0){enemy_tick(s,t,p);if(!enemy_power_value(s,t,p))enemy_stun_to(s,t,1);}break;
  case EnemyPower::Plow:if(loss>0&&s.enemies[t].hp<=amount){enemy_clear_power(s,t,EnemyPower::TemporaryStrength);enemy_clear_power(s,t,EnemyPower::Strength);enemy_stun_to(s,t,3);enemy_clear_power(s,t,p);}break;
  case EnemyPower::Shriek:if(loss>0&&s.enemies[t].hp<=amount){enemy_stun_to(s,t,3);enemy_clear_power(s,t,p);}break;
  default:break;
  }
 }
 (void)blocked;q.flush();
}
static void enemy_after_player_attack(State&s){
 if(!s.enemy_world.active)return;
 for(int t=s.enemy_count-1;t>=0;--t){int amount=enemy_power_value(s,t,EnemyPower::Skittish);
  if(enemy_slot_alive(s,t)&&amount&&s.enemy_world.turn_attacks[t]>0&&!enemy_aux(s,t,EnemyPower::Skittish)){
   enemy_aux(s,t,EnemyPower::Skittish)=1;queue_extension(s,XOp::EnemyGainBlock,amount,t,true);
  }
  s.enemy_world.turn_attacks[t]=0;
 }
}
static void enemy_side_start(State&s,bool player){
 if(!s.enemy_world.active)return;auto&w=s.enemy_world;w.side=(uint8_t)!player;w.actor=255;
 int order[MaxEnemies];unsigned n=enemy_ordered_slots(s,order);
 for(unsigned i=0;i<n;++i){int t=order[i];enemy_aux(s,t,EnemyPower::HardenedShell)=0;w.turn_damage[t]=w.turn_attacks[t]=0;}
 if(player){w.cards_played_turn=w.bound_draws_turn=0;w.bound_played=w.used_types=0;
  for(unsigned i=0;i<n;++i){int t=order[i];auto&m=w.models[t];
   if(m.data[11]){m.data[11]=0;enemy_set_move(s,t,enemy_initial_node(s,t));m.previous=m.node;}
   else if(m.age&&(!enemy_must_perform(m)||(m.flags&EnemyActed))){int next=enemy_next_node(s,t);m.flags&=(uint8_t)~(EnemyStunned|EnemyImmediate);enemy_set_move(s,t,next);}
  }
 }else{
  w.turn_count=(uint8_t)n;
  for(unsigned i=0;i<n;++i){int t=order[i];w.turn_order[i]=(uint8_t)t;w.turn_ids[i]=w.models[t].combat_id;
   if(!enemy_power_value(s,t,EnemyPower::Burrowed))s.enemies[t].block=0;
   enemy_aux(s,t,EnemyPower::Slow)=0;
  }
 }
 for(unsigned at=n;at>0;--at){int t=order[at-1];const auto&m=w.models[t];
  for(unsigned i=m.power_count;i>0;--i)queue_extension(s,XOp::EnemyPowerHook,t,m.power_order[i-1]+EnemyPowerCount*(player?0:1),true);
 }
}
static void enemy_side_end(State&s,bool player){
 if(!s.enemy_world.active)return;auto&w=s.enemy_world;
 int order[MaxEnemies];unsigned n=enemy_ordered_slots(s,order);
 // The late damage pass follows ordinary AfterSideTurnEnd listeners.
 if(player&&enemy_power_value(s,-1,EnemyPower::Disintegration))queue_extension(s,XOp::EnemyPowerHook,-1,(int)EnemyPower::Disintegration+EnemyPowerCount*2,true);
 for(unsigned at=n;at>0;--at){int t=order[at-1];const auto&m=w.models[t];
  for(unsigned i=m.power_count;i>0;--i)queue_extension(s,XOp::EnemyPowerHook,t,m.power_order[i-1]+EnemyPowerCount*(player?2:3),true);
 }
 for(unsigned i=w.player_power_count;i>0;--i){EnemyPower p=(EnemyPower)w.player_power_order[i-1];if(p==EnemyPower::Disintegration)continue;
  queue_extension(s,XOp::EnemyPowerHook,-1,(int)p+EnemyPowerCount*(player?2:3),true);
 }
 if(!player){
  // Asleep removes Plating before its early-end block callback on the final
  // sleeping turn, independent of the order in which powers were acquired.
  for(unsigned at=n;at>0;--at){int t=order[at-1];if(enemy_power_value(s,t,EnemyPower::Asleep)==1)enemy_clear_power(s,t,EnemyPower::Plating);
   int plating=enemy_power_value(s,t,EnemyPower::Plating);if(plating&&enemy_slot_alive(s,t))queue_extension(s,XOp::EnemyGainBlock,plating,t,true);
  }
 }
}
static void enemy_power_hook(State&s,int t,EnemyPower p,int stage){
 if(t>=0&&!enemy_slot_exists(s,t))return;int amount=enemy_power_value(s,t,p);if(!amount)return;
 const bool own=(t<0?stage==2:stage==3);const bool living=t<0?s.hp>0:enemy_slot_alive(s,t);if(!living)return;
 if(stage==4){
  int id=s.playing;if(id<0||id>=s.card_count)return;CardType type=card_type(s,s.cards[id],id);
  switch(p){
  case EnemyPower::Slow:enemy_aux(s,t,p)=(int16_t)min(9999,enemy_aux(s,t,p)+1);break;
  case EnemyPower::Enrage:if(type==CardType::Skill)enemy_apply_power(s,t,EnemyPower::Strength,amount,t);break;
  case EnemyPower::CurlUp:if(enemy_aux(s,t,p)==id+1){enemy_gain_block(s,t,amount);s.enemy_world.models[t].data[0]=1;enemy_clear_power(s,t,p);}break;
  case EnemyPower::Galvanic:if(s.enemy_world.affliction[id]==(unsigned)EnemyAffliction::Galvanized)queue_extension(s,XOp::EnemyPlayerDamage,amount,1,true);break;
  case EnemyPower::VitalSpark:if(s.enemy_world.affliction[id]==(unsigned)EnemyAffliction::Tainted)queue_extension(s,XOp::EnemyApplyPower,amount,enemy_power_operand(-1,EnemyPower::Tainted),true);break;
  case EnemyPower::WitheringPresence:if(--enemy_aux(s,t,p)<=0){enemy_aux(s,t,p)=6;queue_extension(s,XOp::EnemyGenerateCard,EventCardWither,1+1024,true);}break;
  default:break;
  }return;
 }
 if(stage==0){
  if(p==EnemyPower::Plating&&t>=0&&s.turn==1)enemy_gain_block(s,t,amount);
  if(p==EnemyPower::Rampart)for(int target=0;target<s.enemy_count;++target)if(enemy_slot_alive(s,target)&&s.enemy_world.models[target].kind==EnemyId::TurretOperator)enemy_gain_block(s,target,amount);
  return;
 }
 if(stage==1){
  if(p==EnemyPower::Plating&&t>=0&&s.turn>1)enemy_tick(s,t,p);
  if(p==EnemyPower::Sandpit&&t>=0)enemy_tick(s,t,p);
  return;
 }
 if(stage==3&&(p==EnemyPower::Weak||p==EnemyPower::Vulnerable||p==EnemyPower::Frail)){enemy_tick(s,t,p,true);return;}
 if(stage==3&&p==EnemyPower::Intangible&&t>=0){enemy_tick(s,t,p);return;}
 if(stage==3&&p==EnemyPower::Tainted){enemy_clear_power(s,t,p);return;}
 if(stage==2&&p==EnemyPower::Skittish&&t>=0){enemy_aux(s,t,p)=0;return;}
 if(!own)return;
 switch(p){
 case EnemyPower::BattlewornDummyTimeLimit:if(amount==1){s.enemy_world.timed_out=1;enemy_escape(s,t);}else enemy_tick(s,t,p);break;
 case EnemyPower::Constrict:case EnemyPower::Disintegration:
  if(t<0)queue_extension(s,XOp::EnemyPlayerDamage,amount,1,true);else damage_unpowered(s,t,amount);break;
 case EnemyPower::Debilitate:if(t<0)enemy_tick(s,t,p);break;
 case EnemyPower::EscapeArtist:if(amount>1)enemy_tick(s,t,p);break;
 case EnemyPower::Hatch:enemy_tick(s,t,p);break;
 case EnemyPower::HighVoltage:case EnemyPower::Territorial:enemy_apply_power(s,t,EnemyPower::Strength,amount,t);break;
 case EnemyPower::Ritual:if(t>=0){if(enemy_aux(s,t,p))enemy_aux(s,t,p)=0;else enemy_apply_power(s,t,EnemyPower::Strength,amount,t);}break;
 case EnemyPower::Asleep:enemy_tick(s,t,p);break;
 case EnemyPower::Slumber:enemy_tick(s,t,p);if(!enemy_power_value(s,t,p))enemy_clear_power(s,t,EnemyPower::Plating);break;
 case EnemyPower::Nemesis:enemy_aux(s,t,p)^=1;if(enemy_aux(s,t,p))enemy_apply_power(s,t,EnemyPower::Intangible,1,t);else enemy_clear_power(s,t,EnemyPower::Intangible);break;
 case EnemyPower::Shrink:if(amount>0)enemy_tick(s,t,p);break;
 case EnemyPower::TemporaryStrength:enemy_clear_power(s,t,p);enemy_apply_power(s,t,EnemyPower::Strength,-amount,t);break;
 case EnemyPower::Ringing:case EnemyPower::Tangled:enemy_clear_power(s,t,p);break;
 case EnemyPower::Smoggy:enemy_clear_matching_afflictions(s,EnemyAffliction::Smog);break;
 case EnemyPower::Tender:{int count=enemy_aux(s,t,p);enemy_aux(s,t,p)=0;enemy_apply_power(s,t,EnemyPower::Strength,count,enemy_source(s,t,p)-1);enemy_apply_power(s,t,EnemyPower::Dexterity,count,enemy_source(s,t,p)-1);break;}
 default:break;
 }
}
