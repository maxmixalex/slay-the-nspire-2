// Reimplemented from the pinned model metadata and command traces. No original
// assembly, localization payload, artwork, or disassembly belongs in this file.
static int npower(const State&s,NPower p){return s.necro.powers[(unsigned)p];}
static void necro_unregister(State&s,unsigned p){
 auto&v=s.necro;for(unsigned i=0;i<v.power_count;i++)if(v.power_order[i]==p){
  for(unsigned j=i+1;j<v.power_count;j++)v.power_order[j-1]=v.power_order[j];
  v.power_order[--v.power_count]=0;return;
 }
}
static void necro_gain_power(State&s,NPower p,int amount){
 auto&v=s.necro;auto&n=v.powers[(unsigned)p];int old=n;
 n=(int16_t)max(0,min(9999,n+amount));
 if(!old&&n){if(v.power_count==NPowerCount){fail(s);return;}v.power_order[v.power_count++]=(uint8_t)p;}
 else if(old&&!n)necro_unregister(s,(unsigned)p);
 unsigned actor=p==NPower::DevourLife?(unsigned)PostPlayActor::NecroDevourLife:p==NPower::Haunt?(unsigned)PostPlayActor::NecroHaunt:0;
 if(actor){if(!old&&n)after_play_register(s,actor);else if(old&&!n)after_play_unregister(s,actor);}
}
static void necro_combat_start(State&s){
 uint16_t saved[MaxDeck];for(int i=0;i<MaxDeck;i++)saved[i]=s.necro.scythe_deck[i];
 s.necro=NecroState{};for(int i=0;i<MaxDeck;i++)s.necro.scythe_deck[i]=saved[i];
}
static void necro_card_reset(State&s,int id){
 if(id<0||id>=MaxCards)return;auto&v=s.necro;
 v.cost_delta[id]=v.turn_origin[id]=v.combat_origin[id]=0;v.ethereal[id]=v.fetch_played[id]=0;v.scythe_value[id]=0;v.deck_origin[id]=0;v.flatten_zero[id]=0;v.flatten_origin[id]=0;
}
static void necro_card_created(State&s,int id){
 if(id<0||id>=s.card_count)return;necro_card_reset(s,id);auto&v=s.necro;
 if(s.cards[id].kind==NecroBansheesCry)v.cost_delta[id]=(int16_t)-min(9999,2*v.ethereal_played);
 if(s.cards[id].kind==NecroFlatten&&v.osty_attacks_turn){s.expansion.turn_cost[id]=1;temporary_cost_set(s,id);v.flatten_zero[id]=1;v.flatten_origin[id]=v.cost_delta[id];}
}
static void necro_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&v=s.necro;
 v.ethereal[to]=v.ethereal[from];v.cost_delta[to]=v.cost_delta[from];
 v.turn_origin[to]=v.turn_origin[from];v.combat_origin[to]=v.combat_origin[from];
 // Fetch's history belongs to the physical instance, never its clone.
 v.fetch_played[to]=0;v.scythe_value[to]=v.scythe_value[from];v.deck_origin[to]=0;v.flatten_zero[to]=v.flatten_zero[from];v.flatten_origin[to]=v.flatten_origin[from];
}
static void necro_temporary_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.turn_cost[id])s.necro.turn_origin[id]=s.necro.cost_delta[id];
}
static void necro_combat_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.combat_cost[id]){s.necro.combat_origin[id]=s.necro.cost_delta[id];s.necro.flatten_zero[id]=0;s.necro.flatten_origin[id]=0;}
}
static bool necro_ethereal(const State&s,int id){
 if(id<0||id>=s.card_count)return false;Card c=s.cards[id];
 return s.necro.ethereal[id]||((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&8);
}
static int necro_cost(const State&s,int id,int current){
 if(id<0||id>=s.card_count||current<0)return current;const auto&v=s.necro;
 int origin=s.expansion.turn_cost[id]?v.turn_origin[id]:s.expansion.combat_cost[id]?v.combat_origin[id]:0;
 if(v.flatten_zero[id]&&!s.expansion.turn_cost[id]){current=0;origin=v.flatten_origin[id];}
 current=max(0,min(9999,current+v.cost_delta[id]-origin+npower(s,NPower::BorrowedTime)));
 if(npower(s,NPower::Veilpiercer)&&necro_ethereal(s,id)&&(s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo))return 0;
 return current;
}
static bool necro_can_play(const State&s,Card c){return c.kind!=NecroHighFive||s.expansion.osty_hp>0;}
static bool necro_osty_attack(Card c){
 switch(c.kind){
 case NecroBoneShards:case NecroFetch:case NecroFlatten:case NecroHighFive:
 case NecroPoke:case NecroProtector:case NecroRattle:case NecroRightHandHand:
 case NecroSicEm:case NecroSnap:case NecroSqueeze:case NecroUnleash:
 case SweepingGaze:return true;
 default:return false;
 }
}
static int necro_osty_bonus(const State&s){return npower(s,NPower::Calcify);}
static int necro_base_damage(const State&s,Card c,int t,int id,int current){
 const auto&v=s.necro;const auto&x=s.expansion;bool up=c.upgraded!=0;
 switch(c.kind){
 case NecroTheScythe:return min(9999,13+(id>=0&&id<s.card_count?v.scythe_value[id]:0));
 case NecroDeathMarch:return min(9999,(up?9:8)+(up?6:4)*v.drawn_turn);
 case NecroProtector:return min(9999,(up?15:10)+x.osty_max_hp);
 case NecroUnleash:return min(9999,(up?9:6)+x.osty_hp);
 case NecroSoulStorm:{int n=0;for(unsigned i=0;i<s.exhaust.count;i++)if(s.cards[s.exhaust.ids[i]].kind==Soul)++n;return min(9999,9+(up?6:4)*n);}
 case NecroSqueeze:{
  int n=0;for(unsigned i=0;i<s.card_count;i++)if((int)i!=id&&necro_osty_attack(s.cards[i]))++n;
  return min(9999,(up?30:25)+(up?6:5)*n);
 }
 case NecroTimesUp:return t>=0&&t<s.enemy_count?x.doom[t]:0;
 default:return current;
 }
}
static int necro_base_block(const State&s,Card c,int,int current){
 if(c.kind==NecroSacrifice)return s.expansion.osty_hp>0?min(9999,3*s.expansion.osty_max_hp):0;
 return current;
}
static int necro_damage_multiplier(const State&s,Card c,int t,int id){
 int n=100;bool active=id>=0&&id==s.playing;
 if(npower(s,NPower::Lethality)&&(active?s.necro.first_attack[s.expansion.depth]!=0:s.necro.attacks_started_turn==0))n+=npower(s,NPower::Lethality);
 if(c.kind==NecroHang&&t>=0&&t<s.enemy_count&&s.necro.hang[t])n=min(9999999,n*s.necro.hang[t]);
 return n;
}
static int necro_vulnerable_multiplier(const State&s,int t,int percent){
 return t>=0&&t<s.enemy_count&&s.necro.debilitate[t]?max(0,2*percent-100):percent;
}
static int necro_weak_multiplier(const State&s,int t,int percent){
 return t>=0&&t<s.enemy_count&&s.necro.debilitate[t]?max(0,2*percent-100):percent;
}
static void necro_start_instance(State&s,int,int energy){
 auto&v=s.necro;unsigned f=s.expansion.depth;
 v.play_index[f]=0;v.resource_energy[f]=(uint16_t)max(0,min(9999,energy));v.first_attack[f]=0;
}
static void necro_before_play(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.necro;unsigned f=s.expansion.depth;Card c=s.cards[id];
 v.attack_total[f]=0;
 for(int t=0;t<MaxEnemies;t++)v.oblivion_before[f][t]=v.oblivion[t];
 v.first_attack[f]=(uint8_t)(definitions[c.kind].type==CardType::Attack&&!v.attacks_started_turn&&!v.play_index[f]);
 if(definitions[c.kind].type==CardType::Attack)v.attacks_started_turn=(uint16_t)min(9999,v.attacks_started_turn+1);
 v.play_index[f]=(uint16_t)min(9999,v.play_index[f]+1);
 // The model dispatches powers in acquisition order, before OnPlay. Veilpiercer
 // consumes a layer at its own place in that ordering.
 uint8_t order[NPowerCount];unsigned count=v.power_count;for(unsigned i=0;i<count;i++)order[i]=v.power_order[i];
 for(unsigned i=0;i<count;i++){
  NPower p=(NPower)order[i];int n=npower(s,p);
  if(p==NPower::DanseMacabre&&cost(s,id)>=2)enqueue(s,EffectKind::Block,n);
  else if(p==NPower::SpiritOfAsh&&necro_ethereal(s,id))enqueue(s,EffectKind::Block,n);
  else if(p==NPower::Veilpiercer&&necro_ethereal(s,id)&&n)necro_gain_power(s,p,-1);
 }
}
static void necro_after_play(State&s,int id){
 if(id<0||id>=s.card_count)return;
 if(s.cards[id].kind==NecroFetch)s.necro.fetch_played[id]=1;
}
static void necro_after_play_listener(State&s,unsigned actor){
 if(s.playing<0||s.playing>=s.card_count)return;auto&v=s.necro;unsigned f=s.expansion.depth;Card c=s.cards[s.playing];
 if(actor==(unsigned)PostPlayActor::NecroDevourLife){if(c.kind==Soul&&npower(s,NPower::DevourLife))queue_extension(s,XOp::NSummon,npower(s,NPower::DevourLife),1,true);}
 else if(actor==(unsigned)PostPlayActor::NecroHaunt){if(c.kind==Soul&&npower(s,NPower::Haunt))queue_extension(s,XOp::NRandomHpLoss,npower(s,NPower::Haunt),0,true);}
 else if(actor==(unsigned)PostPlayActor::Cards&&necro_ethereal(s,s.playing)){
  v.ethereal_played=(uint16_t)min(9999,v.ethereal_played+1);
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroBansheesCry)v.cost_delta[id]=(int16_t)max(-9999,v.cost_delta[id]-2);
 }else if(actor==(unsigned)PostPlayActor::Enemies){
  for(int t=MaxEnemies-1;t>=0;t--){int n=v.oblivion_before[f][t];v.oblivion_before[f][t]=0;if(n)queue_extension(s,XOp::NDoom,t,n,true);}
 }
}
static void necro_after_play_late(State&s,int){
 if(s.necro.resource_energy[s.expansion.depth]<2)return;
 // AllCards traverses Hand, Draw, Discard, Exhaust, then Play. Only Discard
 // candidates qualify, so preserve that pile's order when capacity is limited.
 for(unsigned i=0;i<s.discard.count&&s.hand.count<MaxHand;){int id=s.discard.ids[i];
  if(s.cards[id].kind==NecroRightHandHand)move_instance(s,id,s.discard,s.hand,Zone::Hand);else ++i;
 }
}
static void necro_after_play_group(State&s,int){
 auto&v=s.necro;unsigned f=s.expansion.depth;v.play_index[f]=v.resource_energy[f]=v.attack_total[f]=0;v.first_attack[f]=0;
 for(auto&n:v.oblivion_before[f])n=0;
}
static void necro_card_drawn(State&s,int id){
 if(!s.expansion.enemy_side)s.necro.drawn_turn=(uint16_t)min(9999,s.necro.drawn_turn+1);
 if(necro_ethereal(s,id)&&npower(s,NPower::Pagestorm))queue_extension(s,XOp::Draw,npower(s,NPower::Pagestorm),0,true);
}
static void necro_debuff_applied(State&s,int t,int change,bool doom){
 if(doom&&change>0)s.necro.doom_applied_turn=1;
 auto&v=s.necro;
 for(unsigned i=v.power_count;i>0;i--){NPower p=(NPower)v.power_order[i-1];
  if(p==NPower::Shroud&&doom)enqueue(s,EffectKind::Block,npower(s,p),0,true);
  else if(p==NPower::SleightOfFlesh&&change&&t>=0&&t<s.enemy_count)queue_extension(s,XOp::NDebuffDamage,npower(s,p),t,true);
 }
}
static void necro_apply_doom(State&s,int t,int amount){
 if(amount<=0)return;
 if(t==-1){
  if(s.hp<=0)return;int n=min(9999-s.necro.player_doom,amount);s.necro.player_doom+=(int16_t)n;
  necro_debuff_applied(s,-1,n,true);return;
 }
 if(t<0||t>=s.enemy_count||s.enemies[t].hp<=0||silent_blocks_debuff(s,t))return;
 int n=min(9999-s.expansion.doom[t],amount);s.expansion.doom[t]+=(int16_t)n;necro_note_debuff(s,t,NDebuff::Doom);necro_debuff_applied(s,t,n,true);
}
static void necro_attack_damage(State&s,int t,int total,bool pet){
 if(t<0||t>=s.enemy_count)return;
 s.necro.attack_total[s.expansion.depth]=(int16_t)min(9999,s.necro.attack_total[s.expansion.depth]+max(0,total));
 // Player AfterDamageGiven powers precede the target's SicEm reaction.
 if(pet&&s.necro.sic_em[t])queue_extension(s,XOp::NSummon,s.necro.sic_em[t],1,true);
 if(total>0&&npower(s,NPower::ReaperForm))queue_extension(s,XOp::NDoom,t,min(9999,total*npower(s,NPower::ReaperForm)),true);
}
static void necro_death_event(State&s){
 auto&v=s.necro;v.creature_deaths=(uint16_t)min(9999,v.creature_deaths+1);
 for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroMelancholy)v.cost_delta[id]=(int16_t)max(-9999,v.cost_delta[id]-1);
}
static void necro_observe_deaths(State&s){
 auto&v=s.necro;
 for(int t=0;t<s.enemy_count;t++){
  unsigned bit=1u<<t;
  if(s.enemies[t].hp>0)v.enemy_dead_mask&=(uint8_t)~bit;
  else if(!(v.enemy_dead_mask&bit)){v.enemy_dead_mask|=(uint8_t)bit;necro_death_event(s);}
 }
 if(s.expansion.osty_hp>0)v.osty_was_alive=1;
 else if(v.osty_was_alive){v.osty_was_alive=0;necro_death_event(s);}
 necro_prune_debuffs(s);
}
static void necro_summon(State&s,int n){
 if(n<=0)return;auto&x=s.expansion;n=min(9999,n);
 if(x.osty_hp>0){int gained=min(9999-x.osty_max_hp,n);x.osty_max_hp+=(int16_t)gained;x.osty_hp=(int16_t)min(x.osty_max_hp,x.osty_hp+gained);}
 else{x.osty_max_hp=x.osty_hp=(int16_t)n;}
 s.necro.osty_was_alive=1;
}
static void necro_osty_hp_lost(State&s,int old_hp){
 int loss=old_hp-s.expansion.osty_hp;
 if(loss>0&&npower(s,NPower::NecroMastery))queue_extension(s,XOp::NAllHpLoss,min(9999,loss*npower(s,NPower::NecroMastery)),0,true);
 necro_observe_deaths(s);
}
static void necro_turn_start(State&s){
 auto&v=s.necro;v.drawn_turn=v.osty_attacks_turn=v.attacks_started_turn=0;v.doom_applied_turn=0;
 for(unsigned id=0;id<s.card_count;id++){v.fetch_played[id]=0;v.turn_origin[id]=0;}
 // BoundPhylactery's first summon is the CombatStart relic hook. Later
 // summons occur after energy reset, before BeforeHandDraw.
 if((s.relics&BoundPhylactery)&&s.turn>1)necro_summon(s,1);
 int energy=npower(s,NPower::EnergyNextTurn);if(energy){s.energy=(int16_t)min(999,s.energy+energy);necro_gain_power(s,NPower::EnergyNextTurn,-energy);}
 int pet=npower(s,NPower::SummonNextTurn);if(pet){necro_summon(s,pet);necro_gain_power(s,NPower::SummonNextTurn,-pet);}
}
static int necro_max_energy(const State&s){return min(999,npower(s,NPower::Demesne)+npower(s,NPower::Friendship));}
static int necro_hand_draw_count(const State&s){return min(MaxCards,npower(s,NPower::Demesne));}
static void necro_before_hand_draw(State&s){
 const auto&v=s.necro;for(unsigned i=v.power_count;i>0;i--){NPower p=(NPower)v.power_order[i-1];
  if(p==NPower::CallOfTheVoid)queue_extension(s,XOp::NCallVoid,npower(s,p),0,true);
  else if(p==NPower::SentryMode)queue_extension(s,XOp::NSentry,npower(s,p),0,true);
 }
}
static void necro_turn_ready(State&s){
 const auto&v=s.necro;for(unsigned i=v.power_count;i>0;i--){NPower p=(NPower)v.power_order[i-1];
  if(p==NPower::Countdown||p==NPower::Neurosurge)queue_extension(s,XOp::NPowerTurn,(int)p,0,true);
 }
}
static void necro_player_side_end(State&s){
 auto&v=s.necro;necro_gain_power(s,NPower::BorrowedTime,-npower(s,NPower::BorrowedTime));
 // Oblivion expires after the player's side; SicEm/Debilitate after the
 // target's own side. Doom's execution precedes those expiry callbacks.
 for(auto&n:v.oblivion)n=0;
 for(unsigned id=0;id<s.card_count;id++){v.flatten_zero[id]=0;v.flatten_origin[id]=0;}
 if(s.hp>0&&v.player_doom>=s.hp&&v.player_doom){s.hp=0;if(!revive(s))death_cleanup(s);}
}
static void necro_enemy_side_end(State&s){
 auto&v=s.necro;
 for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0&&s.expansion.doom[t]>=s.enemies[t].hp&&s.expansion.doom[t])s.enemies[t].hp=0;
 necro_observe_deaths(s);
 for(int t=0;t<MaxEnemies;t++){if(v.debilitate[t])--v.debilitate[t];v.sic_em[t]=0;}
}
static void necro_combat_end(State&s){
 auto&v=s.necro;if(s.hp>0)v.grimoire_pending=(uint16_t)npower(s,NPower::ForbiddenGrimoire);
 for(unsigned f=0;f<=MaxAutoFrames;f++){
  v.play_index[f]=v.resource_energy[f]=v.attack_total[f]=0;v.first_attack[f]=0;
  for(auto&n:v.oblivion_before[f])n=0;
 }
}
bool necro_grimoire_choice(State&s,int deck_index){
 if(s.phase!=Phase::Reward||!s.necro.grimoire_pending||deck_index<-1)return false;
 if(deck_index>=0){
  if(deck_index>=s.deck_count||s.deck_count<=1||!card_removable(s.deck[deck_index]))return false;
  record(s,LogKind::Remove,s.deck[deck_index].kind);
  notify_deck_removed(s,deck_index);
  for(int i=deck_index+1;i<s.deck_count;i++)s.deck[i-1]=s.deck[i];
  s.deck[--s.deck_count]={};
 }
 --s.necro.grimoire_pending;++s.revision;return true;
}
static bool queue_necro(State&s,Card c,int target,int energy){
 if((c.kind<NecroAfterlife||c.kind>NecroWisp)&&c.kind!=SweepingGaze)return false;
 record(s,LogKind::Play,c.kind,c.upgraded);const auto&d=definitions[c.kind];bool up=c.upgraded!=0;
 if(d.type==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};
 auto pw=[&](NPower p,int n){op(XOp::NPower,(int)p,n);};
 auto block=[&](int n){enqueue_card_block(s,powered_block(s,c,n));};
 auto hit=[&](int n,int t){if(enqueue(s,EffectKind::Damage,n,t)&&necro_osty_attack(c))s.effects[s.effect_count-1].aux=2;};
 auto single=[&](int n){hit(n,target);};
 auto all=[&](int n){for(int t=0;t<s.enemy_count;t++)hit(n,t);};
 auto doom=[&](int n){op(XOp::NDoom,target,n);};
 auto alldoom=[&](int n){for(int t=0;t<s.enemy_count;t++)op(XOp::NDoom,t,n);};
 auto summon=[&](int n,int times=1){op(XOp::NSummon,n,times);};
 auto soul=[&](int n,int zone=0,bool upgraded=false){op(XOp::NMakeSoul,n,zone*2+(upgraded?1:0));};
 auto petdone=[&](){op(XOp::NFinishOsty,1);};
 int damage_base=card_base_damage(s,c,target,s.playing);
 // All thirteen Osty attacks test his existence before any of their OnPlay
 // commands. A missing pet also suppresses Fetch's draw and Snap's selection.
 if(necro_osty_attack(c)&&s.expansion.osty_hp<=0){enqueue(s,EffectKind::Finish);return true;}
 switch(c.kind){
 case NecroAfterlife:summon(up?9:6);break;
 case NecroBansheesCry:all(33);break;
 case NecroBlightStrike:single(up?10:8);op(XOp::NDoomFromDamage,target);break;
 case NecroBodyguard:summon(up?7:5);break;
 case NecroBoneShards:all(up?12:9);petdone();block(up?12:9);op(XOp::NKillOsty);break;
 case NecroBorrowedTime:enqueue(s,EffectKind::Energy,up?6:4);pw(NPower::BorrowedTime,1);break;
 case NecroBury:single(up?63:52);break;
 case NecroCalcify:pw(NPower::Calcify,up?6:4);break;
 case NecroCallOfTheVoid:pw(NPower::CallOfTheVoid,1);break;
 case NecroCaptureSpirit:op(XOp::NEnemyHpLoss,up?4:3,target);soul(up?4:3);break;
 case NecroCleanse:summon(up?5:3);op(XOp::NChooseCleanse);break;
 case NecroCountdown:pw(NPower::Countdown,up?9:6);break;
 case NecroDanseMacabre:pw(NPower::DanseMacabre,up?6:4);break;
 case NecroDeathMarch:single(damage_base);break;
 case NecroDeathbringer:alldoom(up?26:21);for(int t=0;t<s.enemy_count;t++)enqueue(s,EffectKind::Weak,1,t);break;
 case NecroDeathsDoor:op(XOp::NDeathDoor,up?7:6);break;
 case NecroDebilitate:single(up?12:10);op(XOp::NEnemyPower,target,up?3:2);break;
 case NecroDefendNecrobinder:block(up?8:5);break;
 case NecroDefile:single(up?17:13);break;
 case NecroDefy:block(up?9:6);enqueue(s,EffectKind::Weak,1,target);break;
 case NecroDelay:block(up?13:11);pw(NPower::EnergyNextTurn,up?2:1);break;
 case NecroDemesne:pw(NPower::Demesne,1);break;
 case NecroDevourLife:pw(NPower::DevourLife,up?2:1);break;
 case NecroDirge:summon(up?4:3,energy);soul(energy,0,up);break;
 case NecroDrainPower:single(up?12:10);op(XOp::NUpgradeDiscard,up?3:2);break;
 case NecroDredge:op(XOp::NChooseDredge,3);break;
 case NecroEidolon:op(XOp::NEidolon);break;
 case NecroEndOfDays:alldoom(up?37:29);op(XOp::NCheckDoom);break;
 case NecroEnfeeblingTouch:op(XOp::StrengthLoss,target,up?11:8);break;
 case NecroEradicate:op(XOp::NRepeatedHit,up?14:11,energy);break;
 case NecroFear:single(up?8:7);enqueue(s,EffectKind::Vulnerable,up?2:1,target);break;
 case NecroFetch:single(up?6:3);petdone();op(XOp::NFetchDraw,s.playing);break;
 case NecroFlatten:single(up?16:12);petdone();break;
 case NecroForbiddenGrimoire:pw(NPower::ForbiddenGrimoire,1);break;
 case NecroFriendship:op(XOp::Strength,-(up?1:2));pw(NPower::Friendship,1);break;
 case NecroGraveWarden:block(up?11:8);soul(1);break;
 case NecroGraveblast:single(up?6:4);op(XOp::NChooseDredge,1);break;
 case NecroHang:single(up?13:10);op(XOp::NEnemyPower,MaxEnemies+target,0);break;
 case NecroHaunt:pw(NPower::Haunt,up?9:7);break;
 case NecroHighFive:all(up?13:11);petdone();for(int t=0;t<s.enemy_count;t++)enqueue(s,EffectKind::Vulnerable,up?3:2,t);break;
 case NecroInvoke:pw(NPower::SummonNextTurn,up?3:2);pw(NPower::EnergyNextTurn,up?3:2);break;
 case NecroLethality:pw(NPower::Lethality,up?75:50);break;
 case NecroMelancholy:block(up?17:13);break;
 case NecroMisery:single(up?9:7);op(XOp::NMisery,target);break;
 case NecroNecroMastery:summon(up?8:5);pw(NPower::NecroMastery,1);break;
 case NecroNegativePulse:block(up?6:5);alldoom(up?11:7);break;
 case NecroNeurosurge:enqueue(s,EffectKind::Energy,up?4:3);op(XOp::Draw,2);pw(NPower::Neurosurge,3);break;
 case NecroNoEscape:doom(min(9999,(up?15:10)+5*((target>=0&&target<s.enemy_count)?s.expansion.doom[target]/10:0)));break;
 case NecroOblivion:op(XOp::NEnemyPower,2*MaxEnemies+target,up?4:3);break;
 case NecroPagestorm:pw(NPower::Pagestorm,1);break;
 case NecroParse:op(XOp::Draw,up?4:3);break;
 case NecroPoke:single(up?9:6);petdone();break;
 case NecroProtector:single(damage_base);petdone();break;
 case NecroPullAggro:summon(up?5:4);block(up?9:7);break;
 case NecroPullFromBelow:op(XOp::NRepeatedHit,up?7:5,s.necro.ethereal_played);break;
 case NecroPutrefy:enqueue(s,EffectKind::Weak,up?3:2,target);enqueue(s,EffectKind::Vulnerable,up?3:2,target);break;
 case NecroRattle:op(XOp::NRepeatedHit,up?9:7,min(9999,1+s.necro.osty_attacks_turn));petdone();break;
 case NecroReanimate:summon(up?25:20);break;
 case NecroReap:single(up?33:27);break;
 case NecroReaperForm:pw(NPower::ReaperForm,1);break;
 case NecroReave:single(up?13:10);soul(1,0,up);break;
 case NecroRightHandHand:single(up?6:4);petdone();break;
 case NecroSacrifice:{int n=necro_base_block(s,c,s.playing,0);if(s.expansion.osty_hp>0){op(XOp::NKillOsty);block(n);}break;}
 case NecroScourge:doom(up?16:13);op(XOp::Draw,up?2:1);break;
 case NecroSculptingStrike:single(up?12:9);op(XOp::NChooseEthereal);break;
 case NecroSeance:op(XOp::NChooseSeance);break;
 case NecroSentryMode:pw(NPower::SentryMode,1);break;
 case NecroSeverance:single(up?18:13);soul(1,0);soul(1,2);soul(1,1);break;
 case NecroSharedFate:op(XOp::Strength,-2);op(XOp::NEnemyPower,4*MaxEnemies+target,up?3:2);break;
 case NecroShroud:pw(NPower::Shroud,up?4:3);break;
 case NecroSicEm:single(up?6:5);petdone();op(XOp::NEnemyPower,3*MaxEnemies+target,up?4:3);break;
 case NecroSleightOfFlesh:pw(NPower::SleightOfFlesh,up?13:9);break;
 case NecroSnap:single(up?10:7);petdone();op(XOp::NChooseRetain);break;
 case NecroSoulStorm:single(damage_base);break;
 case NecroSow:all(up?11:8);break;
 case NecroSpiritOfAsh:pw(NPower::SpiritOfAsh,up?5:4);break;
 case NecroSpur:summon(up?5:3);op(XOp::NHealOsty,up?7:5);break;
 case NecroSqueeze:single(damage_base);petdone();break;
 case NecroStrikeNecrobinder:single(up?9:6);break;
 case NecroTheScythe:single(damage_base);op(XOp::NScytheGrow,s.playing,up?7:5);break;
 case NecroTimesUp:single(damage_base);break;
 case NecroTransfigure:op(XOp::NChooseTransfigure);break;
 case NecroUndeath:block(up?9:7);op(XOp::NUndeathClone,s.playing);break;
 case NecroUnleash:single(damage_base);petdone();break;
 case NecroVeilpiercer:single(up?13:10);pw(NPower::Veilpiercer,1);break;
 case NecroWisp:enqueue(s,EffectKind::Energy,1);break;
 case SweepingGaze:{
  int ids[MaxEnemies],n=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0)ids[n++]=t;
  if(n)hit(up?15:10,ids[random_below(s.rng_cards,n)]);petdone();break;
 }
 default:fail(s);return true;
 }
 enqueue(s,EffectKind::Finish);return true;
}
static void necro_deck_card_created(State&s,int id,int deck){
 if(id<0||id>=s.card_count||deck<0||deck>=s.deck_count)return;
 if(s.cards[id].kind==NecroTheScythe){s.necro.deck_origin[id]=(uint16_t)(deck+1);s.necro.scythe_value[id]=(int16_t)s.necro.scythe_deck[deck];}
}
static void necro_deck_removed(State&s,int at){
 if(at<0||at>=MaxDeck)return;auto&v=s.necro;
 for(int i=at+1;i<MaxDeck;i++)v.scythe_deck[i-1]=v.scythe_deck[i];v.scythe_deck[MaxDeck-1]=0;
 for(auto&n:v.deck_origin){if(n==(unsigned)at+1)n=0;else if(n>(unsigned)at+1)--n;}
}
static void necro_deck_replaced(State&s,int at){
 if(at<0||at>=MaxDeck)return;s.necro.scythe_deck[at]=0;
 for(auto&n:s.necro.deck_origin)if(n==(unsigned)at+1)n=0;
}
static void necro_deck_copied(State&s,int to,int from){
 if(to>=0&&to<MaxDeck&&from>=0&&from<s.deck_count)s.necro.scythe_deck[to]=s.necro.scythe_deck[from];
}
static int necro_debuff_amount(const State&s,int t,NDebuff p){
 if(t<0||t>=s.enemy_count)return 0;const auto&e=s.enemies[t];const auto&x=s.expansion;const auto&v=s.necro;
 switch(p){
 case NDebuff::Weak:return e.weak;case NDebuff::Vulnerable:return e.vulnerable;case NDebuff::Poison:return e.poison;
 case NDebuff::Strength:return max(0,-e.strength);case NDebuff::Beetle:return x.beetle[t];case NDebuff::Demise:return x.demise[t];
 case NDebuff::Doom:return x.doom[t];case NDebuff::Strangle:return s.silent.strangle[t];case NDebuff::Debilitate:return v.debilitate[t];
 case NDebuff::Hang:return v.hang[t];case NDebuff::Oblivion:return v.oblivion[t];case NDebuff::SicEm:return v.sic_em[t];
 default:return 0;
 }
}
static void necro_note_debuff(State&s,int t,NDebuff p){
 if(t<0||t>=s.enemy_count||(unsigned)p>=NDebuffCount)return;auto&v=s.necro;
 uint8_t entry=(uint8_t)((unsigned)p+1);for(unsigned i=0;i<v.debuff_count[t];i++)if(v.debuff_order[t][i]==entry)return;
 if(v.debuff_count[t]==NDebuffCount){fail(s);return;}v.debuff_order[t][v.debuff_count[t]++]=entry;
}
static void necro_prune_debuffs(State&s){
 auto&v=s.necro;for(int t=0;t<s.enemy_count;t++){
  unsigned n=0;for(unsigned i=0;i<v.debuff_count[t];i++){auto p=(NDebuff)(v.debuff_order[t][i]-1);if(necro_debuff_amount(s,t,p)>0)v.debuff_order[t][n++]=v.debuff_order[t][i];}
  for(unsigned i=n;i<NDebuffCount;i++)v.debuff_order[t][i]=0;v.debuff_count[t]=(uint8_t)n;
 }
}
static void necro_apply_copied_debuff(State&s,int t,NDebuff p,int amount){
 if(t<0||t>=s.enemy_count||s.enemies[t].hp<=0||amount<=0)return;
 // Common applications own their Artifact and secondary power callbacks.
 if(p==NDebuff::Weak){apply_weak(s,t,amount);return;}
 if(p==NDebuff::Vulnerable){apply_vulnerable(s,t,amount);return;}
 if(p==NDebuff::Poison){apply_poison(s,t,amount);return;}
 if(p==NDebuff::Doom){necro_apply_doom(s,t,amount);return;}
 if(silent_blocks_debuff(s,t))return;
 int n=0;auto&v=s.necro;auto&x=s.expansion;
 if(p==NDebuff::Strength){n=min(amount,9999+s.enemies[t].strength);s.enemies[t].strength-=(int16_t)n;}
 else{
  int16_t*value=p==NDebuff::Beetle?&x.beetle[t]:p==NDebuff::Demise?&x.demise[t]:p==NDebuff::Strangle?&s.silent.strangle[t]:p==NDebuff::Debilitate?&v.debilitate[t]:p==NDebuff::Hang?&v.hang[t]:p==NDebuff::Oblivion?&v.oblivion[t]:p==NDebuff::SicEm?&v.sic_em[t]:nullptr;
  if(!value)return;n=min(9999-*value,amount);*value+=(int16_t)n;
 }
 necro_note_debuff(s,t,p);necro_debuff_applied(s,t,n,false);
}
static void necro_copy_debuffs(State&s,int from){
 if(from<0||from>=s.enemy_count)return;necro_prune_debuffs(s);
 // Snapshot every amount before the first copied debuff can cause damage or
 // kill a creature. Unknown legacy acquisition order has a stable fallback.
 uint8_t order[NDebuffCount]{};int amounts[NDebuffCount]{};unsigned count=0;bool seen[NDebuffCount]{};
 for(unsigned i=0;i<s.necro.debuff_count[from];i++){
  unsigned p=s.necro.debuff_order[from][i]-1;int n=necro_debuff_amount(s,from,(NDebuff)p);
  if(n>0){order[count]=(uint8_t)p;amounts[count++]=n;seen[p]=true;}
 }
 for(unsigned p=0;p<NDebuffCount;p++)if(!seen[p]){int n=necro_debuff_amount(s,from,(NDebuff)p);if(n>0){order[count]=(uint8_t)p;amounts[count++]=n;}}
 for(int t=s.enemy_count-1;t>=0;t--)if(t!=from&&s.enemies[t].hp>0)
  for(unsigned i=count;i>0;i--)queue_extension(s,XOp::NApplyCopiedDebuff,(int)order[i-1]*MaxEnemies+t,amounts[i-1],true);
}
static void necro_hp_loss_enemy(State&s,int t,int n){
 if(t<0||t>=s.enemy_count||s.enemies[t].hp<=0||n<=0)return;
 int lost=min(s.enemies[t].hp,n);s.enemies[t].hp-=(int16_t)lost;record(s,LogKind::LoseHp,lost,t+1);necro_observe_deaths(s);
}
static int necro_random_enemy(State&s){
 int ids[MaxEnemies],count=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0)ids[count++]=t;
 return count?ids[random_below(s.rng_cards,count)]:-1;
}
static void necro_insert_draw(State&s,int id){
 if(id<0||id>=s.card_count||s.draw.count>=MaxCards){fail(s);return;}
 unsigned at=random_below(s.rng_cards,s.draw.count+1);
 for(unsigned i=s.draw.count;i>at;i--)s.draw.ids[i]=s.draw.ids[i-1];
 s.draw.ids[at]=(uint16_t)id;++s.draw.count;s.zones[id]=Zone::Draw;
}
static bool necro_has_upgrade(Card c){
 return card_upgradeable(c);
}
static void necro_choice_begin(State&s,XChoice which,int count){
 Pile eligible{};
 if(which==XChoice::NCleanse||which==XChoice::NSeance)eligible=s.draw;
 else if(which==XChoice::NDredge)eligible=s.discard;
 else for(unsigned i=0;i<s.hand.count;i++){
  int id=s.hand.ids[i];bool allowed=true;
  if(which==XChoice::NEthereal)allowed=!necro_ethereal(s,id);
  else if(which==XChoice::NRetain)allowed=!card_retains(s.cards[id])&&!s.silent.retain[id];
  if(allowed)eligible.ids[eligible.count++]=(uint16_t)id;
 }
 if(which==XChoice::NDredge)count=min(count,MaxHand-s.hand.count);
 count=min(count,eligible.count);if(count<=0)return;
 set_extended_choice(s,which,eligible,count,count);
}
static bool necro_choose(State&s,const uint16_t*ids,unsigned count){
 auto&x=s.expansion;XChoice k=x.choice;
 if(s.phase!=Phase::ExtendedChoice||k<XChoice::NCleanse||k>XChoice::NTransfigure||!ids||!count||count!=x.choice_min||count!=x.choice_max)return false;
 Zone zone=k==XChoice::NCleanse||k==XChoice::NSeance?Zone::Draw:k==XChoice::NDredge?Zone::Discard:Zone::Hand;
 for(unsigned i=0;i<count;i++){
  int id=ids[i];if(id<0||id>=s.card_count||s.zones[id]!=zone)return false;
  for(unsigned j=0;j<i;j++)if(ids[j]==id)return false;
  bool found=false;for(unsigned j=0;j<x.choices.count;j++)if(x.choices.ids[j]==id)found=true;if(!found)return false;
  if(k==XChoice::NEthereal&&necro_ethereal(s,id))return false;
  if(k==XChoice::NRetain&&(card_retains(s.cards[id])||s.silent.retain[id]))return false;
 }
 if(k==XChoice::NDredge&&s.hand.count+count>MaxHand)return false;
 // Validate the complete choice before mutating a pile or keyword.
 clear_extended_choice(s);s.phase=Phase::Combat;
 for(unsigned i=0;i<count;i++){
  int id=ids[i];
  if(k==XChoice::NCleanse){for(unsigned j=0;j<s.draw.count;j++)if(s.draw.ids[j]==id){remove(s.draw,j);exhaust_id(s,id);break;}}
  else if(k==XChoice::NDredge)move_instance(s,id,s.discard,s.hand,Zone::Hand);
  else if(k==XChoice::NEthereal)s.necro.ethereal[id]=1;
  else if(k==XChoice::NRetain)s.silent.retain[id]|=1;
  else if(k==XChoice::NSeance){s.cards[id]={Soul,0};reset_card_modifiers(s,id);}
  else if(k==XChoice::NTransfigure){
   if(cost(s.cards[id])>=0&&cost(s,id)>=0)s.necro.cost_delta[id]=(int16_t)min(9999,s.necro.cost_delta[id]+1);
   s.expansion.replay[id]=(uint8_t)min(255,s.expansion.replay[id]+1);
  }
 }
 resolve_pending(s);++s.revision;return true;
}
static bool necro_effect(State&s,Effect e){
 XOp op=(XOp)e.target;int a=e.value,b=e.aux;auto&v=s.necro;auto&x=s.expansion;
 switch(op){
 case XOp::NPower:necro_gain_power(s,(NPower)a,b);break;
 case XOp::NSummon:
  if(b>1)queue_extension(s,XOp::NSummon,a,b-1,true);if(b>0)necro_summon(s,a);break;
 case XOp::NHealOsty:if(x.osty_hp>0)x.osty_hp=(int16_t)min(x.osty_max_hp,x.osty_hp+a);break;
 case XOp::NKillOsty:{int old=x.osty_hp;x.osty_hp=0;necro_osty_hp_lost(s,old);break;}
 case XOp::NFinishOsty:
  v.osty_attacks_turn=(uint16_t)min(9999,v.osty_attacks_turn+1);
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroFlatten){x.turn_cost[id]=1;temporary_cost_set(s,id);v.flatten_zero[id]=1;v.flatten_origin[id]=v.cost_delta[id];}break;
 case XOp::NDoom:necro_apply_doom(s,a,b);break;
 case XOp::NDoomFromDamage:necro_apply_doom(s,a,v.attack_total[x.depth]);break;
 case XOp::NCheckDoom:
  for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0&&x.doom[t]>=s.enemies[t].hp&&x.doom[t])s.enemies[t].hp=0;
  necro_observe_deaths(s);break;
 case XOp::NEnemyPower:{
  int kind=a/MaxEnemies,t=a%MaxEnemies;NDebuff p=kind==0?NDebuff::Debilitate:kind==1?NDebuff::Hang:kind==2?NDebuff::Oblivion:kind==3?NDebuff::SicEm:NDebuff::Strength;
  necro_apply_copied_debuff(s,t,p,kind==1?max(2,v.hang[t]):b);break;
 }
 case XOp::NApplyCopiedDebuff:necro_apply_copied_debuff(s,a%MaxEnemies,(NDebuff)(a/MaxEnemies),b);break;
 case XOp::NEnemyHpLoss:necro_hp_loss_enemy(s,b,a);break;
 case XOp::NRandomHpLoss:necro_hp_loss_enemy(s,necro_random_enemy(s),a);break;
 case XOp::NAllHpLoss:for(int t=0;t<s.enemy_count;t++)necro_hp_loss_enemy(s,t,a);break;
 case XOp::NDebuffDamage:damage_unpowered(s,b,a);necro_observe_deaths(s);break;
 case XOp::NMakeSoul:
  if(a>MaxCards-s.card_count){fail(s);break;}
  for(int i=0;i<a;i++){
   Card c{Soul,(uint8_t)(b&1)};int zone=b/2;
   if(zone==1){if(create_to_hand(s,c)<0)break;}
   else{int id=make_card(s,c);if(id<0)break;if(zone==0)necro_insert_draw(s,id);else put(s,s.discard,id,Zone::Discard);}
  }break;
 case XOp::NChooseCleanse:necro_choice_begin(s,XChoice::NCleanse,1);break;
 case XOp::NChooseDredge:necro_choice_begin(s,XChoice::NDredge,a);break;
 case XOp::NChooseEthereal:necro_choice_begin(s,XChoice::NEthereal,1);break;
 case XOp::NChooseRetain:necro_choice_begin(s,XChoice::NRetain,1);break;
 case XOp::NChooseSeance:necro_choice_begin(s,XChoice::NSeance,1);break;
 case XOp::NChooseTransfigure:necro_choice_begin(s,XChoice::NTransfigure,1);break;
 case XOp::NEidolon:{
  uint16_t ids[MaxCards];unsigned n=0;
  for(unsigned i=0;i<s.exhaust.count;i++)if(necro_ethereal(s,s.exhaust.ids[i]))ids[n++]=s.exhaust.ids[i];
  if(s.effect_count+n>MaxEffects){fail(s);break;}
  // Capture identities without removing them: early SoulStorm must still see
  // Souls whose scheduled autoplay has not started. A lethal play leaves the
  // remaining candidates in their original piles.
  for(unsigned i=n;i>0;i--)queue_extension(s,XOp::AutoInstance,ids[i-1],0,true);break;
 }
 case XOp::NUpgradeDiscard:{
  uint16_t ids[MaxCards];unsigned n=0;for(unsigned i=0;i<s.discard.count;i++)if(necro_has_upgrade(s.cards[s.discard.ids[i]]))ids[n++]=s.discard.ids[i];
  for(int i=0;i<a&&n;i++){unsigned at=random_below(s.rng_cards,n);int id=ids[at];ids[at]=ids[--n];s.cards[id].upgraded=1;record(s,LogKind::Upgrade,id);}break;
 }
 case XOp::NMisery:necro_copy_debuffs(s,a);break;
 case XOp::NFetchDraw:if(a>=0&&a<s.card_count&&!v.fetch_played[a])queue_extension(s,XOp::Draw,1,0,true);break;
 case XOp::NRepeatedHit:
  if(b>0&&x.target>=0&&x.target<s.enemy_count&&s.enemies[x.target].hp>0){
   if(b>1)queue_extension(s,XOp::NRepeatedHit,a,b-1,true);
   if(enqueue(s,EffectKind::Damage,a,x.target,true)&&s.playing>=0&&necro_osty_attack(s.cards[s.playing]))s.effects[0].aux=2;
  }break;
 case XOp::NDeathDoor:{
  int count=v.doom_applied_turn?3:1;Card c=s.playing>=0?s.cards[s.playing]:Card{NecroDeathsDoor,0};
  while(count--)enqueue_card_block(s,powered_block(s,c,a),true);break;
 }
 case XOp::NScytheGrow:
  if(a>=0&&a<s.card_count&&s.cards[a].kind==NecroTheScythe){
   int gain=min(9986-v.scythe_value[a],b);v.scythe_value[a]+=(int16_t)gain;
   unsigned origin=v.deck_origin[a];if(origin&&origin<=s.deck_count&&s.deck[origin-1].kind==NecroTheScythe)v.scythe_deck[origin-1]=(uint16_t)min(9986,v.scythe_deck[origin-1]+gain);
  }break;
 case XOp::NUndeathClone:
  if(a>=0&&a<s.card_count){int id=make_card(s,s.cards[a]);if(id>=0){copy_card_modifiers(s,id,a);put(s,s.discard,id,Zone::Discard);}}break;
 case XOp::NPowerTurn:
  if(a==(int)NPower::Countdown){int t=necro_random_enemy(s);if(t>=0)necro_apply_doom(s,t,npower(s,NPower::Countdown));}
  else if(a==(int)NPower::Neurosurge)necro_apply_doom(s,-1,npower(s,NPower::Neurosurge));break;
 case XOp::NCallVoid:{
  uint16_t candidates[CardKinds];unsigned n=0;for(unsigned k=0;k<CardKinds;k++)if(card_in_pool(k,s.character)&&card_can_generate(k))candidates[n++]=(uint16_t)k;
  for(int i=0;i<a&&n;i++){int id=create_to_hand(s,{(CardKind)candidates[random_below(s.rng_cards,n)],0});if(id<0)break;v.ethereal[id]=1;}break;
 }
 case XOp::NSentry:for(int i=0;i<a;i++)if(create_to_hand(s,{SweepingGaze,0})<0)break;break;
 default:return false;
 }
 necro_prune_debuffs(s);return true;
}

static bool necro_var_name(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return !*a&&!*b;}
static bool necro_description_value(const State&s,Card c,int target,int instance,const char*name,int&out){
 if((c.kind<NecroAfterlife||c.kind>NecroWisp)&&c.kind!=SweepingGaze)return false;
 if(necro_var_name(name,"IsOstyAlive")){out=s.expansion.osty_hp>0;return true;}
 if(necro_var_name(name,"IsTargeting")){out=target>=0&&target<s.enemy_count;return true;}
 if(necro_var_name(name,"CalculatedHits")){out=c.kind==NecroRattle?min(9999,1+s.necro.osty_attacks_turn):s.necro.ethereal_played;return true;}
 if(necro_var_name(name,"CalculatedDoom")){out=min(9999,(c.upgraded?15:10)+5*((target>=0&&target<s.enemy_count)?s.expansion.doom[target]/10:0));return true;}
 if(necro_var_name(name,"CalculatedBlock")||necro_var_name(name,"Block")){out=card_block(s,c,instance);return true;}
 if(necro_var_name(name,"CalculatedDamage")||necro_var_name(name,"OstyDamage")||(necro_var_name(name,"Damage")&&definitions[c.kind].type==CardType::Attack)){out=damage(s,c,target,instance);return true;}
 struct Entry{CardKind kind;const char*name;int16_t base,up;};
 static const Entry entries[]={
  {NecroAfterlife,"Summon",6,9},
  {NecroBansheesCry,"Damage",33,33},
  {NecroBansheesCry,"Energy",2,2},
  {NecroBlightStrike,"Damage",8,10},
  {NecroBodyguard,"Summon",5,7},
  {NecroBoneShards,"OstyDamage",9,12},
  {NecroBoneShards,"Block",9,12},
  {NecroBorrowedTime,"Energy",4,6},
  {NecroBorrowedTime,"ExtraCost",1,1},
  {NecroBury,"Damage",52,63},
  {NecroCalcify,"CalcifyPower",4,6},
  {NecroCallOfTheVoid,"Cards",1,1},
  {NecroCaptureSpirit,"Damage",3,4},
  {NecroCaptureSpirit,"Cards",3,4},
  {NecroCleanse,"Summon",3,5},
  {NecroCountdown,"CountdownPower",6,9},
  {NecroDanseMacabre,"DanseMacabrePower",4,6},
  {NecroDanseMacabre,"Energy",2,2},
  {NecroDeathMarch,"CalculationBase",8,9},
  {NecroDeathMarch,"ExtraDamage",4,6},
  {NecroDeathbringer,"DoomPower",21,26},
  {NecroDeathbringer,"WeakPower",1,1},
  {NecroDeathsDoor,"Block",6,7},
  {NecroDeathsDoor,"Repeat",2,2},
  {NecroDebilitate,"Damage",10,12},
  {NecroDebilitate,"DebilitatePower",2,3},
  {NecroDefendNecrobinder,"Block",5,8},
  {NecroDefile,"Damage",13,17},
  {NecroDefy,"Block",6,9},
  {NecroDefy,"WeakPower",1,1},
  {NecroDelay,"Block",11,13},
  {NecroDelay,"Energy",1,2},
  {NecroDemesne,"Energy",1,1},
  {NecroDemesne,"Cards",1,1},
  {NecroDevourLife,"DevourLifePower",1,2},
  {NecroDirge,"Summon",3,4},
  {NecroDrainPower,"Damage",10,12},
  {NecroDrainPower,"Cards",2,3},
  {NecroDredge,"Cards",3,3},
  {NecroEndOfDays,"DoomPower",29,37},
  {NecroEnfeeblingTouch,"StrengthLoss",8,11},
  {NecroEradicate,"Damage",11,14},
  {NecroFear,"Damage",7,8},
  {NecroFear,"VulnerablePower",1,2},
  {NecroFetch,"OstyDamage",3,6},
  {NecroFetch,"Cards",1,1},
  {NecroFlatten,"OstyDamage",12,16},
  {NecroFriendship,"StrengthPower",2,1},
  {NecroFriendship,"Energy",1,1},
  {NecroGraveWarden,"Block",8,11},
  {NecroGraveWarden,"Cards",1,1},
  {NecroGraveblast,"Damage",4,6},
  {NecroHang,"Damage",10,13},
  {NecroHaunt,"HpLoss",7,9},
  {NecroHighFive,"OstyDamage",11,13},
  {NecroHighFive,"VulnerablePower",2,3},
  {NecroInvoke,"Summon",2,3},
  {NecroInvoke,"Energy",2,3},
  {NecroLethality,"LethalityPower",50,75},
  {NecroMelancholy,"Block",13,17},
  {NecroMelancholy,"Energy",1,1},
  {NecroMisery,"Damage",7,9},
  {NecroNecroMastery,"Summon",5,8},
  {NecroNegativePulse,"Block",5,6},
  {NecroNegativePulse,"DoomPower",7,11},
  {NecroNeurosurge,"NeurosurgePower",3,3},
  {NecroNeurosurge,"Energy",3,4},
  {NecroNeurosurge,"Cards",2,2},
  {NecroNoEscape,"DoomThreshold",10,10},
  {NecroNoEscape,"CalculationBase",10,15},
  {NecroNoEscape,"CalculationExtra",5,5},
  {NecroOblivion,"DoomPower",3,4},
  {NecroPagestorm,"Cards",1,1},
  {NecroParse,"Cards",3,4},
  {NecroPoke,"OstyDamage",6,9},
  {NecroProtector,"CalculationBase",10,15},
  {NecroProtector,"ExtraDamage",1,1},
  {NecroPullAggro,"Summon",4,5},
  {NecroPullAggro,"Block",7,9},
  {NecroPullFromBelow,"Damage",5,7},
  {NecroPullFromBelow,"CalculationBase",0,0},
  {NecroPullFromBelow,"CalculationExtra",1,1},
  {NecroPutrefy,"Power",2,3},
  {NecroRattle,"OstyDamage",7,9},
  {NecroRattle,"CalculationBase",0,0},
  {NecroRattle,"CalculationExtra",1,1},
  {NecroReanimate,"Summon",20,25},
  {NecroReap,"Damage",27,33},
  {NecroReave,"Damage",10,13},
  {NecroReave,"Cards",1,1},
  {NecroRightHandHand,"OstyDamage",4,6},
  {NecroRightHandHand,"Energy",2,2},
  {NecroSacrifice,"CalculationBase",0,0},
  {NecroSacrifice,"CalculationExtra",1,1},
  {NecroScourge,"DoomPower",13,16},
  {NecroScourge,"Cards",1,2},
  {NecroSculptingStrike,"Damage",9,12},
  {NecroSeance,"Cards",1,1},
  {NecroSentryMode,"SentryModePower",1,1},
  {NecroSeverance,"Damage",13,18},
  {NecroSharedFate,"EnemyStrengthLoss",2,3},
  {NecroSharedFate,"PlayerStrengthLoss",2,2},
  {NecroShroud,"Block",3,4},
  {NecroSicEm,"OstyDamage",5,6},
  {NecroSicEm,"SicEmPower",3,4},
  {NecroSleightOfFlesh,"SleightOfFleshPower",9,13},
  {NecroSnap,"OstyDamage",7,10},
  {NecroSoulStorm,"CalculationBase",9,9},
  {NecroSoulStorm,"ExtraDamage",4,6},
  {NecroSow,"Damage",8,11},
  {NecroSpiritOfAsh,"BlockOnExhaust",4,5},
  {NecroSpur,"Summon",3,5},
  {NecroSpur,"Heal",5,7},
  {NecroSqueeze,"CalculationBase",25,30},
  {NecroSqueeze,"ExtraDamage",5,6},
  {NecroStrikeNecrobinder,"Damage",6,9},
  {NecroTheScythe,"Damage",13,13},
  {NecroTheScythe,"Increase",5,7},
  {NecroTimesUp,"CalculationBase",0,0},
  {NecroTimesUp,"ExtraDamage",1,1},
  {NecroTransfigure,"Energy",1,1},
  {NecroUndeath,"Block",7,9},
  {NecroUnleash,"CalculationBase",6,9},
  {NecroUnleash,"ExtraDamage",1,1},
  {NecroVeilpiercer,"Damage",10,13},
  {NecroWisp,"Energy",1,1},
  {SweepingGaze,"OstyDamage",10,15}
 };
 for(const auto&e:entries)if(e.kind==c.kind&&necro_var_name(name,e.name)){out=c.upgraded?e.up:e.base;return true;}
 return false;
}
