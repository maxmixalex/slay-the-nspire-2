// Original solo enemy simulation. Every delayed action uses the shared saved
// continuation queue; no callback, allocation or original source is embedded.
#include "enemy_definitions.inc"
#include "enemy_move_definitions.inc"
#include "enemy_fixed_actions.inc"
#include "enemy_graphs.inc"
#include "enemy_power_definitions.inc"
static int enemy_asc(const State&s,int ordinary,int tough,int deadly){return s.enemy_world.ascension>=9?deadly:s.enemy_world.ascension>=8?tough:ordinary;}
#include "enemy_numbers.inc"
static bool enemy_slot_exists(const State&s,int t){return t>=0&&t<s.enemy_count&&(s.enemy_world.models[t].flags&EnemyPresent)&&!(s.enemy_world.models[t].flags&EnemyRemoved);}
static bool enemy_slot_alive(const State&s,int t){return enemy_slot_exists(s,t)&&s.enemies[t].hp>0;}
static unsigned enemy_teammates(const State&s,int except=-1){unsigned n=0;for(int t=0;t<s.enemy_count;t++)if(t!=except&&enemy_slot_alive(s,t))++n;return n;}
static int16_t& enemy_raw_power(State&s,int t,EnemyPower p){return t<0?s.enemy_world.player_power[(unsigned)p]:s.enemy_world.models[t].power[(unsigned)p];}
static int16_t& enemy_aux(State&s,int t,EnemyPower p){return t<0?s.enemy_world.player_aux[(unsigned)p]:s.enemy_world.models[t].power_aux[(unsigned)p];}
static uint8_t& enemy_skip(State&s,int t,EnemyPower p){return t<0?s.enemy_world.player_duration_skip[(unsigned)p]:s.enemy_world.models[t].duration_skip[(unsigned)p];}
static uint8_t& enemy_source(State&s,int t,EnemyPower p){return t<0?s.enemy_world.player_source[(unsigned)p]:s.enemy_world.models[t].source[(unsigned)p];}
int enemy_power_value(const State&s,int t,EnemyPower p){
 if(t< -1||t>=s.enemy_count||(unsigned)p>=EnemyPowerCount)return 0;
 if(t>=0){switch(p){case EnemyPower::Strength:return s.enemies[t].strength;case EnemyPower::Weak:return s.enemies[t].weak;case EnemyPower::Vulnerable:return s.enemies[t].vulnerable;case EnemyPower::Artifact:return s.silent.enemy_artifact[t];case EnemyPower::Debilitate:return s.necro.debilitate[t];default:break;}return s.enemy_world.models[t].power[(unsigned)p];}
 switch(p){case EnemyPower::Strength:return s.strength;case EnemyPower::Dexterity:return s.dexterity;case EnemyPower::Thorns:return s.thorns;case EnemyPower::Vigor:return s.vigor;case EnemyPower::Plating:return power_value(s,XPower::Plating);case EnemyPower::Ritual:return power_value(s,XPower::Ritual);case EnemyPower::Intangible:return power_value(s,XPower::Intangible);default:return s.enemy_world.player_power[(unsigned)p];}
}
static void enemy_write_power(State&s,int t,EnemyPower p,int amount){
 if(t>=0){switch(p){case EnemyPower::Strength:s.enemies[t].strength=(int16_t)amount;return;case EnemyPower::Weak:s.enemies[t].weak=(int16_t)amount;return;case EnemyPower::Vulnerable:s.enemies[t].vulnerable=(int16_t)amount;return;case EnemyPower::Artifact:s.silent.enemy_artifact[t]=(int16_t)amount;return;case EnemyPower::Debilitate:s.necro.debilitate[t]=(int16_t)amount;return;default:break;}}
 else switch(p){case EnemyPower::Strength:world_relic_gain_strength(s,amount-s.strength);return;case EnemyPower::Dexterity:world_relic_gain_dexterity(s,amount-s.dexterity);return;case EnemyPower::Thorns:s.thorns=(int16_t)amount;return;case EnemyPower::Vigor:s.vigor=(int16_t)amount;return;case EnemyPower::Plating:gain_power(s,XPower::Plating,amount-power_value(s,XPower::Plating));return;case EnemyPower::Ritual:gain_power(s,XPower::Ritual,amount-power_value(s,XPower::Ritual));return;case EnemyPower::Intangible:gain_power(s,XPower::Intangible,amount-power_value(s,XPower::Intangible));return;default:break;}
 enemy_raw_power(s,t,p)=(int16_t)amount;
}
static void enemy_power_order_add(State&s,int t,EnemyPower p){
 auto&w=s.enemy_world;uint8_t*order=t<0?w.player_power_order:w.models[t].power_order;uint8_t&n=t<0?w.player_power_count:w.models[t].power_count;
 for(unsigned i=0;i<n;i++)if(order[i]==(unsigned)p)return;
 if(n>=EnemyPowerCount){fail(s);return;}order[n++]=(uint8_t)p;
}
static void enemy_power_order_remove(State&s,int t,EnemyPower p){
 auto&w=s.enemy_world;uint8_t*order=t<0?w.player_power_order:w.models[t].power_order;uint8_t&n=t<0?w.player_power_count:w.models[t].power_count;
 for(unsigned i=0;i<n;i++)if(order[i]==(unsigned)p){for(unsigned j=i+1;j<n;j++)order[j-1]=order[j];order[--n]=0;break;}
}
static int enemy_power_operand(int target,EnemyPower p,int source=-1){return (int)p+80*(target+1)+720*(source+1);}
struct EnemyCommands{
 State&s;int actor;Effect values[48]{};unsigned count=0;
 void op(XOp kind,int a=0,int b=0){if(count>=48||a< -9999||a>9999||b< -9999||b>9999){fail(s);return;}values[count++]={EffectKind::Extended,(int16_t)a,(int16_t)kind,(int16_t)b};}
 void attack(int amount,int hits=1){op(XOp::EnemyAttackStart,actor,hits);op(XOp::EnemyHit,amount,actor+8*hits);op(XOp::EnemyAttackEnd,actor);}
 void power(int target,EnemyPower p,int amount){op(XOp::EnemyApplyPower,amount,enemy_power_operand(target,p,actor));}
 void remove(int target,EnemyPower p){op(XOp::EnemyRemovePower,target,(int)p);}
 void block(int target,int amount,bool powered=true){op(XOp::EnemyGainBlock,amount,target+8*powered);}
 void heal(int target,int amount){op(XOp::EnemyHeal,amount,target);}
 void card(CardKind kind,int count,int pile=2){op(XOp::EnemyGenerateCard,(int)kind,count+1024*pile);}
 void special(int step){op(XOp::EnemyActionSpecial,actor,step);}
 void flush(){for(unsigned i=count;i>0;i--)queue_extension(s,(XOp)values[i-1].target,values[i-1].value,values[i-1].aux,true);}
};
static bool enemy_must_perform(const EnemyModelState&m){return m.node==31||m.node==30||(m.kind==EnemyId::TestSubject&&m.node==0)||(m.kind==EnemyId::WaterfallGiant&&m.node==6)||(m.kind>=EnemyId::DecimillipedeSegmentBack&&m.kind<=EnemyId::DecimillipedeSegmentMiddle&&m.node==4);}
static void enemy_log_move(State&,int,int);
static void enemy_set_move(State&s,int t,int node,bool immediate=false){
 auto&m=s.enemy_world.models[t];m.node=(uint8_t)node;m.flags&=(uint8_t)~EnemyActed;if(immediate)m.flags|=EnemyImmediate;
 if(!immediate)enemy_log_move(s,t,node);
 const auto*def=enemy_move_spec(m.kind,(unsigned)node);s.enemies[t].move=(uint8_t)(def&&(def->flags&EnemyIntentAttack)?0:2);
}
static void enemy_log_move(State&s,int t,int node){
 auto&m=s.enemy_world.models[t];if(node<0||node>=MaxEnemyMoves){fail(s);return;}
 if(m.history_count==MaxEnemyMoveHistory){for(unsigned i=1;i<MaxEnemyMoveHistory;i++)m.history[i-1]=m.history[i];--m.history_count;}
 m.history[m.history_count++]=(uint8_t)node;m.previous=(uint8_t)node;if(m.seen_moves[node]<65535)++m.seen_moves[node];
}
void enemy_stun(State&s,int t){if(!enemy_slot_alive(s,t))return;auto&m=s.enemy_world.models[t];if(enemy_must_perform(m)&&!(m.flags&EnemyActed))return;m.resume_node=m.previous;m.flags|=EnemyStunned;enemy_set_move(s,t,31,true);}
static void enemy_stun_to(State&s,int t,int next){if(!enemy_slot_alive(s,t))return;auto&m=s.enemy_world.models[t];if(enemy_must_perform(m)&&!(m.flags&EnemyActed))return;enemy_stun(s,t);if(m.node==31)m.resume_node=(uint8_t)next;}
bool enemy_allow_target(const State&s,int t){if(!s.enemy_world.active)return t>=0&&t<s.enemy_count&&s.enemies[t].hp>0;return enemy_slot_alive(s,t)&&!(s.enemy_world.models[t].flags&EnemyDown);}
bool enemy_keeps_combat_alive(const State&s){
 if(!s.enemy_world.active){for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0)return true;return false;}
 for(int t=0;t<s.enemy_count;t++){
  if(!enemy_slot_exists(s,t))continue;const auto&m=s.enemy_world.models[t];
  if(m.flags&EnemyDying)return true;
  if(s.enemies[t].hp>0&&!enemy_power_value(s,t,EnemyPower::Minion))return true;
  if((m.flags&EnemyDown)&&enemy_power_value(s,t,EnemyPower::Adaptable))return true;
 }
 return false;
}
bool enemy_encounter_timed_out(const State&s){return s.enemy_world.timed_out!=0;}
void enemy_note_ally_created(State&s){if(s.enemy_world.next_creature_id<65535)++s.enemy_world.next_creature_id;}
static void enemy_clear_matching_afflictions(State&s,EnemyAffliction a){for(int id=0;id<s.card_count;id++)if(s.enemy_world.affliction[id]==(unsigned)a)enemy_clear_affliction(s,id);}
static bool enemy_afflict_card(State&s,int id,EnemyAffliction a,int amount){
 if(id<0||id>=s.card_count||a==EnemyAffliction::None||amount<=0)return false;auto&w=s.enemy_world;
 if(a==EnemyAffliction::Tainted&&definitions[s.cards[id].kind].type!=CardType::Skill)return false;
 if(w.affliction[id]){if(w.affliction[id]!=(unsigned)a||(a!=EnemyAffliction::Tainted&&a!=EnemyAffliction::Galvanized))return false;w.affliction_amount[id]=(int16_t)min(9999,w.affliction_amount[id]+amount);return true;}
 w.affliction[id]=(uint8_t)a;w.affliction_amount[id]=(int16_t)min(9999,amount);return true;
}
static void enemy_clear_affliction(State&s,int id){if(id>=0&&id<s.card_count){s.enemy_world.affliction[id]=0;s.enemy_world.affliction_amount[id]=0;}}
static bool enemy_blocks_debuff(State&s,int t){int amount=enemy_power_value(s,t,EnemyPower::Artifact);if(!amount)return false;enemy_write_power(s,t,EnemyPower::Artifact,amount-1);if(amount==1)enemy_power_order_remove(s,t,EnemyPower::Artifact);return true;}
static void enemy_clear_power(State&s,int t,EnemyPower p){
 if(t< -1||t>=s.enemy_count||(unsigned)p>=EnemyPowerCount)return;int old=enemy_power_value(s,t,p);if(!old)return;
 enemy_write_power(s,t,p,0);enemy_power_order_remove(s,t,p);enemy_skip(s,t,p)=0;
 if(t<0){
  if(p==EnemyPower::Tender)after_play_unregister(s,(unsigned)PostPlayActor::EnemyTender);
  if(p==EnemyPower::Smoggy){after_play_unregister(s,(unsigned)PostPlayActor::EnemySmoggy);enemy_clear_matching_afflictions(s,EnemyAffliction::Smog);}
  if(p==EnemyPower::Dampen){for(int id=0;id<s.card_count;id++)if(s.enemy_world.dampen_restore[id]){s.cards[id].upgraded=s.enemy_world.dampen_restore[id];s.enemy_world.dampen_restore[id]=0;}s.enemy_world.dampen_casters=0;}
  else if(p==EnemyPower::Hex)enemy_clear_matching_afflictions(s,EnemyAffliction::Hexed);
  else if(p==EnemyPower::Ringing)enemy_clear_matching_afflictions(s,EnemyAffliction::Ringing);
  else if(p==EnemyPower::Tangled)enemy_clear_matching_afflictions(s,EnemyAffliction::Entangled);
 }else{
  if(p==EnemyPower::Burrowed)s.enemies[t].block=0;
  else if(p==EnemyPower::VitalSpark)enemy_clear_matching_afflictions(s,EnemyAffliction::Tainted);
  else if(p==EnemyPower::Sandpit&&enemy_slot_alive(s,t)&&s.hp>0){s.expansion.osty_hp=0;s.hp=0;death_cleanup(s);}
 }
 enemy_aux(s,t,p)=0;enemy_source(s,t,p)=0;
}
static void enemy_apply_power(State&s,int t,EnemyPower p,int amount,int source,bool internal){
 if(t< -1||t>=s.enemy_count||(unsigned)p>=EnemyPowerCount||!amount||(t>=0&&!enemy_slot_alive(s,t)))return;
 const auto&meta=enemy_power_meta[(unsigned)p];int old=enemy_power_value(s,t,p);
 if(t<0&&p==EnemyPower::Dampen&&old){if(source>=0)s.enemy_world.dampen_casters|=(uint8_t)(1u<<source);return;}
 bool negative=meta.debuff||(amount<0&&(p==EnemyPower::Strength||p==EnemyPower::Dexterity));
 if(negative&&!internal&&enemy_blocks_debuff(s,t))return;
 amount=wr_debuff_amount(s,t,p,amount,source<0?s.playing:-1,internal);
 int next=meta.single?(old?old:amount):old+amount;next=max(meta.negative?-9999:0,min(9999,next));
 if(!next){enemy_clear_power(s,t,p);return;}enemy_write_power(s,t,p,next);next=enemy_power_value(s,t,p);
 if(!old){enemy_power_order_add(s,t,p);enemy_source(s,t,p)=(uint8_t)(source+1);if(t<0&&meta.debuff)enemy_skip(s,t,p)=1;}
 int change=next-old;
 if(p==EnemyPower::TemporaryStrength)enemy_apply_power(s,t,EnemyPower::Strength,change,source,true);
 if(t>=0){
  if(p==EnemyPower::Weak){necro_note_debuff(s,t,NDebuff::Weak);necro_debuff_applied(s,t,change);}
  else if(p==EnemyPower::Vulnerable){ironclad_after_vulnerable(s,t,change);necro_note_debuff(s,t,NDebuff::Vulnerable);necro_debuff_applied(s,t,change);}
  else if(p==EnemyPower::Strength){necro_note_debuff(s,t,NDebuff::Strength);necro_prune_debuffs(s);}
  if(!old&&p==EnemyPower::Illusion)enemy_apply_power(s,t,EnemyPower::Minion,1,-1);
  if(!old&&p==EnemyPower::Ritual)enemy_aux(s,t,p)=1;
  if(!old&&p==EnemyPower::WitheringPresence)enemy_aux(s,t,p)=6;
  if(p==EnemyPower::VitalSpark)for(int id=0;id<s.card_count;id++)if(s.enemy_world.affliction[id]==(unsigned)EnemyAffliction::Tainted)s.enemy_world.affliction_amount[id]=(int16_t)next;
 }else{
  if(!old&&p==EnemyPower::Tender)after_play_register(s,(unsigned)PostPlayActor::EnemyTender);
  if(!old&&p==EnemyPower::Smoggy)after_play_register(s,(unsigned)PostPlayActor::EnemySmoggy);
  if(source>=0&&change<0){EnemyPower stolen=p==EnemyPower::Dexterity?EnemyPower::PossessSpeed:EnemyPower::PossessStrength;if((p==EnemyPower::Strength||p==EnemyPower::Dexterity)&&enemy_power_value(s,source,stolen))enemy_aux(s,source,stolen)=(int16_t)max(-9999,enemy_aux(s,source,stolen)+change);}
  if(p==EnemyPower::Dampen){if(source>=0)s.enemy_world.dampen_casters|=(uint8_t)(1u<<source);if(!old)for(int id=0;id<s.card_count;id++)if(s.cards[id].upgraded){s.enemy_world.dampen_restore[id]=s.cards[id].upgraded;s.cards[id].upgraded=0;}}
  if(!old&&(p==EnemyPower::Hex||p==EnemyPower::Ringing||p==EnemyPower::Tangled))for(int id=0;id<s.card_count;id++){
   if(p==EnemyPower::Tangled&&definitions[s.cards[id].kind].type!=CardType::Attack)continue;
   enemy_afflict_card(s,id,p==EnemyPower::Hex?EnemyAffliction::Hexed:p==EnemyPower::Ringing?EnemyAffliction::Ringing:EnemyAffliction::Entangled,next);
  }
 }
}
static void enemy_tick(State&s,int t,EnemyPower p,bool duration=false){if(!enemy_power_value(s,t,p))return;if(duration&&enemy_skip(s,t,p)){enemy_skip(s,t,p)=0;return;}int n=enemy_power_value(s,t,p);if(n==1)enemy_clear_power(s,t,p);else enemy_write_power(s,t,p,n-1);}
static int enemy_player_weak_percent(const State&s,bool pet){if(pet||!enemy_power_value(s,-1,EnemyPower::Weak))return 100;return enemy_power_value(s,-1,EnemyPower::Debilitate)?50:75;}
static int enemy_player_shrink_percent(const State&s,bool pet){return !pet&&enemy_power_value(s,-1,EnemyPower::Shrink)?70:100;}
static int enemy_target_halves(const State&s,int t){return int(enemy_power_value(s,t,EnemyPower::Flutter)>0)+int(enemy_power_value(s,t,EnemyPower::Soar)>0);}
static int enemy_target_slow_tenths(const State&s,int t){return t>=0&&t<s.enemy_count&&enemy_power_value(s,t,EnemyPower::Slow)?10+s.enemy_world.models[t].power_aux[(unsigned)EnemyPower::Slow]:10;}
static int enemy_player_attack_multiplier(const State&s,int n,bool pet){return n*enemy_player_weak_percent(s,pet)*enemy_player_shrink_percent(s,pet)/10000;}
static int enemy_damage_multiplier(const State&s,int t,int n,bool attack,bool){if(!attack)return n;return n*enemy_target_slow_tenths(s,t)/(10<<enemy_target_halves(s,t));}
static int enemy_player_block_multiplier(const State&s,int n){return enemy_power_value(s,-1,EnemyPower::Frail)?n*3/4:n;}
static int enemy_player_energy(const State&s,int n){return max(0,n-enemy_power_value(s,-1,EnemyPower::WasteAway));}
static int enemy_player_draw(const State&s,int n){return max(0,n-enemy_power_value(s,-1,EnemyPower::MindRot));}
static int enemy_enemy_block_value(const State&s,int t,int n,bool powered){if(powered)n+=enemy_power_value(s,t,EnemyPower::Dexterity);if(powered&&enemy_power_value(s,t,EnemyPower::Frail))n=n*3/4;return max(0,n);}
static void enemy_gain_block(State&s,int t,int amount,bool powered=false){
 if(!enemy_slot_alive(s,t))return;int modified=enemy_enemy_block_value(s,t,amount,powered);if(!modified)return;
 s.enemies[t].block=(int16_t)min(9999,s.enemies[t].block+modified);
 world_relic_hook(s,WorldRelicHook::AfterBlock,modified,t,-1);
}
static int enemy_damage_cap(const State&s,int t,int n,bool){
 if(enemy_power_value(s,t,EnemyPower::HardToKill))n=min(n,enemy_power_value(s,t,EnemyPower::HardToKill));
 if(enemy_power_value(s,t,EnemyPower::Intangible))n=min(n,1);
 return max(0,n);
}
static int enemy_hp_loss_cap(const State&s,int t,int n){
 if(enemy_power_value(s,t,EnemyPower::Intangible)||enemy_power_value(s,t,EnemyPower::Slippery))n=min(n,1);
 if(t>=0&&enemy_power_value(s,t,EnemyPower::HardenedShell))n=min(n,max(0,enemy_power_value(s,t,EnemyPower::HardenedShell)-s.enemy_world.models[t].power_aux[(unsigned)EnemyPower::HardenedShell]));
 return max(0,n);
}
static bool enemy_has_named_slots(const State&);
static unsigned enemy_ordered_slots(const State&s,int*out){
 const bool named=enemy_has_named_slots(s);unsigned n=0;
 for(int t=0;t<s.enemy_count;++t)if(enemy_slot_exists(s,t)){
  const auto&m=s.enemy_world.models[t];const unsigned key=named?m.slot:m.combat_id;unsigned at=n;
  while(at){const auto&prior=s.enemy_world.models[out[at-1]];if((named?prior.slot:prior.combat_id)<=key)break;out[at]=out[at-1];--at;}
  out[at]=t;++n;
 }return n;
}
bool enemy_infinite_hp(const State&s,int t){return enemy_slot_exists(s,t)&&s.enemy_world.models[t].kind==EnemyId::WaterfallGiant&&s.enemy_world.models[t].data[2]!=0;}
bool enemy_death_is_fatal(const State&s,int t){
 if(!s.enemy_world.active)return true;if(!enemy_slot_exists(s,t)&&!(t>=0&&t<s.enemy_count))return false;
 if(enemy_power_value(s,t,EnemyPower::Minion))return false;
 if(enemy_power_value(s,t,EnemyPower::Reattach))for(int other=0;other<s.enemy_count;other++)if(other!=t&&enemy_slot_alive(s,other)&&enemy_power_value(s,other,EnemyPower::Reattach))return false;
 return true;
}
static uint64_t enemy_round_significand(uint64_t n,unsigned bits){
 unsigned length=0;for(uint64_t v=n;v;v>>=1)++length;if(length<=bits)return n;
 unsigned d=length-bits;uint64_t q=n>>d,rem=n&((UINT64_C(1)<<d)-1),half=UINT64_C(1)<<(d-1);if(rem>half||(rem==half&&(q&1)))++q;return q<<d;
}
// Exact binary64 multiply followed by binary32 rounding for original NextFloat.
// Values use units of 2^-27; all nonzero branch weights are at least 1/12.
static uint32_t enemy_float_sample(reference::Random&r,uint32_t sum){
 uint64_t m=r.next()>>11,lo=(m&UINT64_C(0xffffffff))*sum,hi=(m>>32)*sum+(lo>>32);
 unsigned bits=0;for(uint64_t v=hi;v;v>>=1)++bits;bits=bits?bits+32:0;if(!bits)for(uint64_t v=uint32_t(lo);v;v>>=1)++bits;
 unsigned shift=bits>53?bits-53:0;uint64_t q=0;
 if(shift){q=(hi<<(32-shift))|(uint32_t(lo)>>shift);uint64_t mask=(UINT64_C(1)<<shift)-1,rem=lo&mask,half=UINT64_C(1)<<(shift-1);if(rem>half||(rem==half&&(q&1)))++q;}
 else q=(hi<<32)|uint32_t(lo);
 q=enemy_round_significand(q,24);int d=53-(int)shift;return d>=64?0:d>=0?(uint32_t)(q>>d):(uint32_t)(q<<-d);
}
static int enemy_open_slot(const State&s,bool last=false){
 int found=-1;for(unsigned name=0;name<s.enemy_world.slot_limit;name++){
  bool used=false;for(int t=0;t<s.enemy_count;t++)if(enemy_slot_exists(s,t)&&s.enemy_world.models[t].slot==name){used=true;break;}
  if(!used){found=(int)name;if(!last)return found;}
 }return found;
}
static bool enemy_rat_can_summon(const State&s,int t){
 const auto&m=s.enemy_world.models[t];if(m.data[0]>0||m.data[1]>=3||enemy_open_slot(s)<0)return false;
 for(int j=0;j<s.enemy_count;j++)if(j!=t&&enemy_slot_alive(s,j)&&s.enemy_world.models[j].kind==EnemyId::TwoTailedRat&&s.enemy_world.models[j].node==3)return false;return true;
}
static int enemy_random_branch(State&s,int t,int branch){
 auto&m=s.enemy_world.models[t];unsigned n=0;const auto*entries=enemy_branches(m.kind,branch,n);if(!entries||!n){fail(s);return 0;}
 uint32_t weights[8]{},sum=0;bool rat=enemy_rat_can_summon(s,t);
 for(unsigned i=0;i<n;i++){
  const auto&e=entries[i];bool eligible=true;
  if(e.max_repeats==255&&m.seen_moves[e.node])eligible=false;
  else if(e.max_repeats&&e.max_repeats!=255&&m.history_count>=e.max_repeats){bool all=true;for(unsigned j=0;j<e.max_repeats;j++)if(m.history[m.history_count-1-j]!=e.node)all=false;if(all)eligible=false;}
  if(e.cooldown)for(unsigned j=0;j<e.cooldown&&j<m.history_count;j++)if(m.history[m.history_count-1-j]==e.node)eligible=false;
  uint32_t w=134217728u;if(m.kind==EnemyId::Fogmog)w=e.node==2?53687092u:80530640u;
  if(m.kind==EnemyId::TwoTailedRat)w=e.node==3?(rat?100663296u:0u):(rat?11184811u:134217728u);
  weights[i]=eligible?w:0;sum=(uint32_t)enemy_round_significand(uint64_t(sum)+weights[i],24);
 }
 if(!sum){fail(s);return 0;}uint32_t roll=enemy_float_sample(world_rng(s,WorldRng::MonsterAi),sum);
 for(unsigned i=0;i<n;i++){if(roll<=weights[i])return entries[i].node;roll=(uint32_t)enemy_round_significand(uint64_t(roll-weights[i]),24);}
 fail(s);return 0;
}
static int enemy_resolve_node(State&s,int t,int node){
 auto&m=s.enemy_world.models[t];for(unsigned depth=0;depth<8;depth++){
  if(node<128)return node;
  if(node<192){node=enemy_random_branch(s,t,node-128);continue;}
  switch(m.kind){
   case EnemyId::BowlbugRock:node=m.data[0]?1:0;break;
   case EnemyId::Exoskeleton:node=m.slot<3?m.slot:128;break;
   case EnemyId::Fabricator:node=enemy_teammates(s)<4?128:2;break;
   case EnemyId::FrogKnight:node=m.data[0]||s.enemies[t].hp>=s.enemies[t].max_hp/2?2:3;break;
   case EnemyId::KnowledgeDemon:node=m.data[0]<3?0:1;break;
   case EnemyId::LagavulinMatriarch:node=enemy_power_value(s,t,EnemyPower::Asleep)?0:1;break;
   case EnemyId::LivingShield:node=enemy_teammates(s,t)?0:1;break;
   case EnemyId::Myte:node=m.slot==0?0:2;break;
   case EnemyId::Nibbit:node=m.data[0]?0:m.data[1]?1:2;break;
   case EnemyId::Ovicopter:node=enemy_teammates(s)<4?0:3;break;
   case EnemyId::PhantasmalGardener:{const uint8_t first[]={2,0,1,3};node=first[m.slot%4];break;}
   case EnemyId::Queen:node=m.data[0]?3:2;break;
   case EnemyId::SlumberingBeetle:node=enemy_power_value(s,t,EnemyPower::Slumber)?0:1;break;
   case EnemyId::TestSubject:node=m.data[0]<2?3:4;break;
   case EnemyId::Toadpole:node=m.data[0]?2:1;break;
   case EnemyId::Wriggler:node=(m.slot-(s.enemy_world.encounter==EncounterId::PhrogParasiteElite?1:0))%2?1:0;break;
   default:fail(s);return 0;
  }
 }fail(s);return 0;
}
static int enemy_initial_node(State&s,int t){
 const auto&m=s.enemy_world.models[t];int node=enemy_graphs[(unsigned)m.kind].start;
 switch(m.kind){
  case EnemyId::Axebot:node=m.data[0]?0:2;break;
  case EnemyId::Chomper:node=m.data[0]?1:0;break;
  case EnemyId::CorpseSlug:case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:case EnemyId::ScrollOfBiting:node=m.data[3]%3;break;
  case EnemyId::Inklet:node=m.data[0]?1:0;break;
  case EnemyId::KinFollower:node=m.data[0]?2:0;break;
  case EnemyId::PunchConstruct:node=m.data[0]?2:0;break;
  case EnemyId::TwoTailedRat:node=m.data[3]<0?128:m.data[3]%3;break;
  case EnemyId::Wriggler:node=m.data[0]?2:192;break;
  default:break;
 }
 return enemy_resolve_node(s,t,node);
}
static int enemy_next_node(State&s,int t){const auto&m=s.enemy_world.models[t];if(m.node==30||m.node==31)return m.resume_node;const auto&g=enemy_graphs[(unsigned)m.kind];if(m.node>=g.count)return 0;return enemy_resolve_node(s,t,g.next[m.node]);}
static int enemy_raw_move_damage(const State&s,int t,int node){
 const auto&m=s.enemy_world.models[t];const auto*spec=enemy_move_spec(m.kind,(unsigned)node);int amount=spec?(s.enemy_world.ascension>=9?spec->damage_9:spec->damage):0;
 auto span=enemy_fixed_span(m.kind,(unsigned)node);if(span.count!=255)for(unsigned i=0;i<span.count;i++){const auto&a=enemy_fixed_actions[span.first+i];if(a.op==EnemyFixedOp::Attack){amount=enemy_asc(s,a.base,a.tough,a.deadly);break;}}
 if(m.kind==EnemyId::TheForgotten&&node==1)amount+=enemy_power_value(s,t,EnemyPower::Dexterity);
 if(m.kind==EnemyId::WaterfallGiant&&node==4)amount=m.data[0];if(m.kind==EnemyId::WaterfallGiant&&node==7)amount=m.data[1];
 if(m.kind==EnemyId::GremlinMerc){const int ordinary[]={7,6,8};amount=ordinary[node]+(s.enemy_world.ascension>=8);}
 return amount;
}
static int enemy_attack_value(const State&s,int t,int base){
 if(t<0||t>=s.enemy_count)return 0;const auto&m=s.enemy_world.models[t];int vigor=m.attack_vigor?m.attack_vigor:enemy_power_value(s,t,EnemyPower::Vigor);
 int n=max(0,base+s.enemies[t].strength+vigor+enemy_power_value(s,-1,EnemyPower::Tainted));
 int weak=s.enemies[t].weak?necro_weak_multiplier(s,t,75):100;
 int vulnerable=enemy_power_value(s,-1,EnemyPower::Vulnerable)?150+enemy_power_value(s,t,EnemyPower::Cruelty):100;
 if(vulnerable!=100&&enemy_power_value(s,-1,EnemyPower::Debilitate))vulnerable=vulnerable*2-100;
 bool back=enemy_power_value(s,-1,EnemyPower::Surrounded)&&enemy_power_value(s,t,s.enemy_world.facing?EnemyPower::BackAttackRight:EnemyPower::BackAttackLeft);
 uint64_t numerator=(uint64_t)n*(unsigned)max(0,weak)*(unsigned)max(0,vulnerable)*(s.expansion.beetle[t]?70u:100u)*(back?3u:2u);
 // Bounded integer division avoids platform floating-point differences.
 uint64_t q=numerator/2000000u;return ironclad_enemy_damage(s,t,(int)(q>9999999u?9999999u:q));
}
EnemyIntent enemy_intent(const State&s,int t){
 if(t<0||t>=s.enemy_count)return {"",0,0,0};const auto&m=s.enemy_world.models[t];
 if(!s.enemy_world.active||m.kind==EnemyId::LegacyNibbit){bool attack=s.enemies[t].move!=2;return {attack?"ATTACK":"HISS",(uint16_t)(attack?EnemyIntentAttack:EnemyIntentBuff),(int16_t)(attack?enemy_damage(s.enemies[t]):0),(int16_t)(attack?1:0)};}
 if(!enemy_slot_exists(s,t))return {"",0,0,0};if(m.node==31)return {"STUNNED",EnemyIntentStun,0,0};if(m.node==30)return {"REVIVE_MOVE",EnemyIntentHeal,0,0};
 const auto*d=enemy_move_spec(m.kind,m.node);if(!d)return {"",0,0,0};int hits=d->hits;
 if(m.kind==EnemyId::TestSubject&&m.node==3)hits=min(999,3+m.data[1]);
 if(s.enemy_world.ascension>=9&&((m.kind==EnemyId::Entomancer&&m.node==1)||(m.kind==EnemyId::Exoskeleton&&m.node==0)||(m.kind==EnemyId::TrackerRubyRaider&&m.node==1)))++hits;
 return {d->key,d->flags,(int16_t)min(9999,(d->flags&EnemyIntentAttack)?enemy_attack_value(s,t,enemy_raw_move_damage(s,t,m.node)):0),(int16_t)hits};
}
#include "enemy_encounters.inc"
#include "enemy_visuals.inc"
#include "enemy_moves.inc"
#include "enemy_death.inc"
#include "enemy_hooks.inc"
#include "enemy_runtime.inc"
