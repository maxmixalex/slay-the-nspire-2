// Full single-player Ironclad rules, locked to v0.111.0 / 41cef1ea.
// Every operation is a bounded, serializable continuation in the shared queue.
static int ipower(const State&s,IPower p){return s.ironclad.powers[(unsigned)p];}
static void ironclad_gain_power(State&s,IPower p,int n){
 auto&i=s.ironclad;unsigned k=(unsigned)p;int old=i.powers[k];
 int value=max(0,min(9999,old+n));
 if(p==IPower::Barricade||p==IPower::Corruption||p==IPower::Hellraiser)value=min(1,value);
 if(!old&&value){if(i.power_count==IPowerCount){fail(s);return;}i.power_order[i.power_count++]=(uint8_t)k;}
 if(old&&!value)for(unsigned at=0;at<i.power_count;at++)if(i.power_order[at]==k){for(unsigned j=at+1;j<i.power_count;j++)i.power_order[j-1]=i.power_order[j];i.power_order[--i.power_count]=0;break;}
 i.powers[k]=(int16_t)value;
}
static bool ironclad_choice(XChoice c){return c==XChoice::IUpgrade;}
static bool choose_ironclad(State&s,const uint16_t*ids,unsigned n){
 auto&x=s.expansion;
 if(s.phase!=Phase::ExtendedChoice||x.choice!=XChoice::IUpgrade||n!=1||!ids)return false;
 unsigned id=ids[0];bool found=false;for(unsigned j=0;j<x.choices.count;j++)if(x.choices.ids[j]==id)found=true;
 if(!found||id>=s.card_count||s.zones[id]!=Zone::Hand||s.cards[id].upgraded)return false;
 s.cards[id].upgraded=1;record(s,LogKind::Upgrade,s.cards[id].kind);clear_extended_choice(s);s.phase=Phase::Combat;
 resolve_pending(s);++s.revision;return true;
}
static void ironclad_card_reset(State&s,int id){
 if(id<0||id>=MaxCards)return;auto&i=s.ironclad;i.damage_bonus[id]=0;
 i.stomp_discount[id]=i.stomp_turn_origin[id]=i.stomp_combat_origin[id]=0;i.forced_exhaust[id]=0;
}
static void ironclad_card_created(State&s,int id){
 if(id<0||id>=s.card_count)return;ironclad_card_reset(s,id);
 if(s.cards[id].kind==Stomp)s.ironclad.stomp_discount[id]=s.ironclad.attacks_finished;
}
static void ironclad_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&i=s.ironclad;
 i.damage_bonus[to]=i.damage_bonus[from];i.stomp_discount[to]=i.stomp_discount[from];
 i.stomp_turn_origin[to]=i.stomp_turn_origin[from];i.stomp_combat_origin[to]=i.stomp_combat_origin[from];
 i.forced_exhaust[to]=0; // CreateClone explicitly clears ExhaustOnNextPlay.
}
static void ironclad_temporary_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.turn_cost[id])s.ironclad.stomp_turn_origin[id]=s.ironclad.stomp_discount[id];
}
static void ironclad_combat_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.combat_cost[id])s.ironclad.stomp_combat_origin[id]=s.ironclad.stomp_discount[id];
}
static int ironclad_cost(const State&s,int id,int price){
 if(id<0||id>=s.card_count||price<0)return price;Card c=s.cards[id];const auto&i=s.ironclad;
 if(c.kind==Stomp){int origin=s.expansion.turn_cost[id]?i.stomp_turn_origin[id]:s.expansion.combat_cost[id]?i.stomp_combat_origin[id]:0;price=max(0,price-(i.stomp_discount[id]-origin));}
 if(ipower(s,IPower::Corruption)&&definitions[c.kind].type==CardType::Skill)return 0;
 if(ipower(s,IPower::FreeAttack)&&definitions[c.kind].type==CardType::Attack&&(s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo))return 0;
 return price;
}
static int ironclad_base_damage(const State&s,Card c,int target,int instance,int base){
 const auto&i=s.ironclad;
 if(c.kind==AshenStrike)base=6+(c.upgraded?4:3)*s.exhaust.count;
 if(c.kind==Bully)base=4+(c.upgraded?3:2)*(target>=0&&target<s.enemy_count?s.enemies[target].vulnerable:0);
 if((c.kind==Rampage||c.kind==Thrash)&&instance>=0&&instance<s.card_count)base+=i.damage_bonus[instance];
 return min(9999,base);
}
static int ironclad_vulnerable_multiplier(const State&s){return 150+ipower(s,IPower::Cruelty);}
static int ironclad_enemy_damage(const State&s,int enemy,int amount){
 return enemy>=0&&enemy<s.enemy_count&&s.enemies[enemy].vulnerable&&ipower(s,IPower::Colossus)?amount/2:amount;
}
static int ironclad_block(const State&s,Card,int amount){
 const auto&i=s.ironclad;unsigned d=s.expansion.depth;
 if(ipower(s,IPower::Unmovable)&&i.block_events_turn-i.block_events_play[d]<ipower(s,IPower::Unmovable))amount=min(9999,amount*2);
 return amount;
}
static void ironclad_after_block(State&s,int amount,bool card_powered){
 if(amount<=0||!combat_phase(s.phase))return;auto&i=s.ironclad;
 if(card_powered&&s.playing>=0){i.block_events_turn=(uint16_t)min(9999,i.block_events_turn+1);i.block_events_play[s.expansion.depth]=(uint16_t)min(9999,i.block_events_play[s.expansion.depth]+1);}
 if(ipower(s,IPower::Juggernaut))queue_extension(s,XOp::IUnpoweredRandom,ipower(s,IPower::Juggernaut),0,true);
}
static int ironclad_replays(State&s,Card c){
 if(definitions[c.kind].type==CardType::Attack&&ipower(s,IPower::OneTwoPunch)){ironclad_gain_power(s,IPower::OneTwoPunch,-1);return 1;}return 0;
}
static bool ironclad_exhausts(const State&s,int id){
 return id>=0&&id<s.card_count&&(s.ironclad.forced_exhaust[id]||(ipower(s,IPower::Corruption)&&definitions[s.cards[id].kind].type==CardType::Skill));
}
static void ironclad_before_card(State&s,Card c){
 auto&i=s.ironclad;unsigned depth=s.expansion.depth;i.block_events_play[depth]=0;i.rupture_pending[depth]=0;
 i.rupture_registered[depth]=(uint8_t)(!s.expansion.enemy_side&&ipower(s,IPower::Rupture)>0);
 if(definitions[c.kind].type==CardType::Attack){
  i.attacks_started=(uint16_t)min(9999,i.attacks_started+1);
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==Stomp)i.stomp_discount[id]=(uint16_t)min(9999,i.stomp_discount[id]+1);
  if(ipower(s,IPower::FreeAttack))ironclad_gain_power(s,IPower::FreeAttack,-1);
  if(i.attacks_started==3&&ipower(s,IPower::Juggling))queue_extension(s,XOp::IClone,s.playing,ipower(s,IPower::Juggling));
 }
}
static void ironclad_after_card(State&s,Card c){
 auto&i=s.ironclad;unsigned depth=s.expansion.depth;
 if(definitions[c.kind].type==CardType::Attack)i.attacks_finished=(uint16_t)min(9999,i.attacks_finished+1);
 // Power listeners retain acquisition order. Rupture caused by this card is
 // deliberately delivered after its attack, including on each replay.
 for(int at=i.power_count-1;at>=0;at--){IPower p=(IPower)i.power_order[at];
  if(p==IPower::Rage&&definitions[c.kind].type==CardType::Attack)enqueue(s,EffectKind::Block,ipower(s,p),0,true);
  if(p==IPower::Rupture&&i.rupture_pending[depth])enqueue(s,EffectKind::Strength,i.rupture_pending[depth],0,true);
 }
 i.rupture_pending[depth]=0;i.rupture_registered[depth]=0;
}
static void ironclad_after_play_group(State&s,int id){
 if(id>=0&&id<s.card_count)s.ironclad.forced_exhaust[id]=0;
 unsigned d=s.expansion.depth;s.ironclad.block_events_play[d]=0;s.ironclad.pillage_waiting[d]=0;s.ironclad.pillage_drawn[d]=0;
}
static void ironclad_after_draw(State&s,int id){
 auto&i=s.ironclad;unsigned d=s.expansion.depth;
 if(i.pillage_waiting[d]){i.pillage_drawn[d]=(int16_t)id;i.pillage_waiting[d]=0;}
 if(id>=0&&id<s.card_count&&card_strike_tag(s.cards[id].kind)&&ipower(s,IPower::Hellraiser))queue_extension(s,XOp::AutoInstance,id,0,true);
}
static void ironclad_after_exhaust(State&s,int id){
 auto&i=s.ironclad;i.exhausted_turn=(uint16_t)min(9999,i.exhausted_turn+1);
 if(id>=0&&id<s.card_count&&s.cards[id].kind==DrumOfBattle)enqueue(s,EffectKind::Energy,s.cards[id].upgraded?3:2,0,true);
}
static void ironclad_hp_lost(State&s,int loss,int card_source){
 if(loss<=0||!combat_phase(s.phase))return;auto&i=s.ironclad;
 i.hp_losses_combat=(uint16_t)min(9999,i.hp_losses_combat+1);i.hp_losses_turn=(uint16_t)min(9999,i.hp_losses_turn+1);
 if(s.expansion.enemy_side)return;
 int source_depth=-1;
 if(card_source>=0){if(card_source==s.playing)source_depth=s.expansion.depth;else for(unsigned j=0;j<s.expansion.depth;j++)if(s.expansion.frames[j].playing==card_source){source_depth=j;break;}}
 for(int at=i.power_count-1;at>=0;at--){IPower p=(IPower)i.power_order[at];
  if(p==IPower::Inferno)queue_extension(s,XOp::AllDamage,ipower(s,p),0,true);
  if(p==IPower::Rupture){
   if(source_depth>=0&&i.rupture_registered[source_depth])i.rupture_pending[source_depth]=(int16_t)min(9999,i.rupture_pending[source_depth]+ipower(s,p));
   else enqueue(s,EffectKind::Strength,ipower(s,p),0,true);
  }
 }
}
static void ironclad_after_vulnerable(State&s,int enemy,int added){
 if(enemy>=0&&enemy<s.enemy_count&&added>0&&ipower(s,IPower::Vicious))queue_extension(s,XOp::Draw,ipower(s,IPower::Vicious),0,true);
}
static bool ironclad_retains_block(const State&s){return ipower(s,IPower::Barricade)>0;}
static int ironclad_max_energy(const State&s){return ipower(s,IPower::Pyre);}
static void ironclad_turn_start(State&s){
 auto&i=s.ironclad;i.attacks_started=i.attacks_finished=i.hp_losses_turn=i.exhausted_turn=i.block_events_turn=0;
 for(unsigned d=0;d<=MaxAutoFrames;d++){i.block_events_play[d]=0;i.rupture_pending[d]=0;i.rupture_registered[d]=0;}
 for(unsigned id=0;id<s.card_count;id++)i.stomp_discount[id]=i.stomp_turn_origin[id]=i.stomp_combat_origin[id]=0;
 if(ipower(s,IPower::Aggression))queue_extension(s,XOp::IAggression,ipower(s,IPower::Aggression),0,true);
}
static void ironclad_after_hand_draw(State&s){
 auto&i=s.ironclad;for(int at=i.power_count-1;at>=0;at--){IPower p=(IPower)i.power_order[at];if(p==IPower::CrimsonMantle||p==IPower::Inferno)queue_extension(s,XOp::IStartPower,(int)p,0,true);}
}
static void ironclad_turn_end(State&s){
 // Creature powers are listeners before combat cards: Stampede completes
 // before a Howl card tests whether it is now in the exhaust pile.
 queue_extension(s,XOp::IHowls,0,0,true);
 if(ipower(s,IPower::Stampede))queue_extension(s,XOp::IStampede,ipower(s,IPower::Stampede),0,true);
}
static void ironclad_player_side_end(State&s){
 auto&i=s.ironclad;ironclad_gain_power(s,IPower::Rage,-ipower(s,IPower::Rage));ironclad_gain_power(s,IPower::OneTwoPunch,-ipower(s,IPower::OneTwoPunch));
 if(i.setup_strength){s.strength=(int16_t)max(-9999,s.strength-i.setup_strength);i.setup_strength=0;}
 for(unsigned id=0;id<s.card_count;id++)i.stomp_discount[id]=i.stomp_turn_origin[id]=i.stomp_combat_origin[id]=0;
}
static void ironclad_enemy_side_end(State&s){if(ipower(s,IPower::Colossus))ironclad_gain_power(s,IPower::Colossus,-1);}
static void ironclad_combat_end(State&s){
 auto&i=s.ironclad;for(unsigned d=0;d<=MaxAutoFrames;d++){i.rupture_pending[d]=i.pillage_drawn[d]=i.block_events_play[d]=0;i.rupture_registered[d]=i.pillage_waiting[d]=0;}
 for(unsigned id=0;id<s.card_count;id++)i.forced_exhaust[id]=0;
}
static void ironclad_copy_to_hand(State&s,int original){
 if(original<0||original>=s.card_count)return;int id=create_to_hand(s,s.cards[original]);if(id<0)return;
 copy_card_modifiers(s,id,original);
}
static bool queue_ironclad(State&s,Card c,int target,int energy){
 if((c.kind<Aggression||c.kind>Vicious)&&c.kind!=GiantRock)return false;
 const auto&d=definitions[c.kind];int hit=card_base_damage(s,c,target,s.playing),b=d.block+(c.upgraded?d.block_up:0),p=d.power+(c.upgraded?d.power_up:0);
 record(s,LogKind::Play,c.kind,c.upgraded);if(d.type==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp o,int a=0,int v=0){queue_extension(s,o,a,v);};
 auto power=[&](IPower what,int n){op(XOp::IPower,(int)what,n);};
 auto attack=[&](int n){enqueue(s,EffectKind::Damage,n,target);};
 switch(c.kind){
 case Aggression:power(IPower::Aggression,1);break;
 case Armaments:op(XOp::IBlock,b);op(XOp::IUpgrade,c.upgraded);break;
 case AshenStrike:case Bully:case GiantRock:attack(hit);break;
 case Barricade:power(IPower::Barricade,1);break;
 case BloodWall:enqueue(s,EffectKind::LoseHp,2);op(XOp::IBlock,b);break;
 case Brand:enqueue(s,EffectKind::LoseHp,1);enqueue(s,EffectKind::ChooseExhaust);enqueue(s,EffectKind::Strength,p);break;
 case Break:attack(hit);enqueue(s,EffectKind::Vulnerable,p,target);break;
 case Breakthrough:enqueue(s,EffectKind::LoseHp,1);op(XOp::IAllHits,hit,1);break;
 case Cascade:if(energy+c.upgraded>0)op(XOp::AutoTop,min(MaxCards,energy+c.upgraded));break;
 case Cinder:attack(hit);enqueue(s,EffectKind::RandomExhaust);break;
 case Colossus:op(XOp::IBlock,b);power(IPower::Colossus,1);break;
 case Conflagration:op(XOp::IAllHits,hit,p);break;
 case Corruption:power(IPower::Corruption,1);break;
 case CrimsonMantle:power(IPower::CrimsonMantle,p);break;
 case Cruelty:power(IPower::Cruelty,p);break;
 case Dismantle:op(XOp::IHits,hit,target>=0&&s.enemies[target].vulnerable?2:1);break;
 case Dominate:enqueue(s,EffectKind::Vulnerable,p,target);op(XOp::IDominateStrength,target);break;
 case DrumOfBattle:op(XOp::Draw,2);break;
 case EvilEye:op(XOp::IBlock,b);if(s.ironclad.exhausted_turn)op(XOp::IBlock,b);break;
 case ExpectAFight:op(XOp::IBlock,b,1);break;
 case Feed:op(XOp::IFeed,hit,p);break;
 case FightMe:op(XOp::IHits,hit,2);enqueue(s,EffectKind::Strength,p);op(XOp::IEnemyStrength,target,1);break;
 case ForgottenRitual:enqueue(s,EffectKind::Energy,c.upgraded?4:3);break;
 case Havoc:op(XOp::IHavoc);break;
 case Hellraiser:power(IPower::Hellraiser,1);break;
 case HowlFromBeyond:op(XOp::IAllHits,hit,1);break;
 case InfernalBlade:op(XOp::Generate,1,1);break;
 case Inferno:power(IPower::Inferno,p);break;
 case Juggernaut:power(IPower::Juggernaut,p);break;
 case Juggling:power(IPower::Juggling,1);break;
 case Mangle:attack(hit);op(XOp::StrengthLoss,target,p);break;
 case MoltenFist:attack(hit);op(XOp::IDoubleVulnerable,target);break;
 case NotYet:op(XOp::IHeal,p);break;
 case OneTwoPunch:power(IPower::OneTwoPunch,p);break;
 case PactsEnd:if(s.exhaust.count>=3)op(XOp::IAllHits,hit,1);break;
 case Pillage:attack(hit);op(XOp::IPillageDraw);break;
 case PrimalForce:op(XOp::IPrimalForce,c.upgraded);break;
 case Pyre:power(IPower::Pyre,p);break;
 case Rage:power(IPower::Rage,p);break;
 case Rampage:attack(hit);op(XOp::IIncreaseDamage,s.playing,p);break;
 case Rupture:power(IPower::Rupture,p);break;
 case SecondWind:op(XOp::ISecondWind,b);break;
 case SetupStrike:attack(hit);op(XOp::ITemporaryStrength,p);break;
 case Spite:op(XOp::IHits,hit,s.ironclad.hp_losses_turn?p:1);break;
 case Stampede:power(IPower::Stampede,1);break;
 case Stoke:op(XOp::IStoke,c.upgraded);break;
 case Stomp:op(XOp::IAllHits,hit,1);break;
 case StoneArmor:op(XOp::Power,(int)XPower::Plating,p);break;
 case Taunt:op(XOp::IBlock,b);enqueue(s,EffectKind::Vulnerable,p,target);break;
 case TearAsunder:attack(hit);if(s.ironclad.hp_losses_combat)op(XOp::IHits,hit,s.ironclad.hp_losses_combat);break;
 case Thrash:op(XOp::IHits,hit,2);op(XOp::IThrash,s.playing);break;
 case Tremble:enqueue(s,EffectKind::Vulnerable,p,target);break;
 case Unmovable:power(IPower::Unmovable,1);break;
 case Unrelenting:attack(hit);power(IPower::FreeAttack,1);break;
 case Vicious:power(IPower::Vicious,p);break;
 default:return false;
 }
 enqueue(s,EffectKind::Finish);return true;
}
static bool ironclad_effect(State&s,Effect e){
 XOp op=(XOp)e.target;int a=e.value,b=e.aux;auto&i=s.ironclad;auto&x=s.expansion;
 switch(op){
 case XOp::IPower:{
  IPower p=(IPower)a;ironclad_gain_power(s,p,b);
  if(p==IPower::CrimsonMantle)i.crimson_self_damage=(int16_t)min(9999,i.crimson_self_damage+1);
  if(p==IPower::Inferno)i.inferno_self_damage=(int16_t)min(9999,i.inferno_self_damage+1);
  break;}
 case XOp::IBlock:{
  Card c=s.playing>=0?s.cards[s.playing]:Card{Defend,0};
  if(b)a+=(c.upgraded?8:5)*s.strength;
  enqueue_card_block(s,powered_block(s,c,max(0,min(9999,a))),true);break;}
 case XOp::IUpgrade:{
  Pile eligible{};for(unsigned at=0;at<s.hand.count;at++){int id=s.hand.ids[at];if(!s.cards[id].upgraded)eligible.ids[eligible.count++]=(uint16_t)id;}
  if(a)for(unsigned at=0;at<eligible.count;at++){s.cards[eligible.ids[at]].upgraded=1;record(s,LogKind::Upgrade,s.cards[eligible.ids[at]].kind);}
  else set_extended_choice(s,XChoice::IUpgrade,eligible,1,1);break;}
 case XOp::IDominateStrength:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp)enqueue(s,EffectKind::Strength,s.enemies[a].vulnerable,0,true);break;
 case XOp::IDoubleVulnerable:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&s.enemies[a].vulnerable)enqueue(s,EffectKind::Vulnerable,s.enemies[a].vulnerable,a,true);break;
 case XOp::IFeed:
  if(x.target>=0&&x.target<s.enemy_count&&s.enemies[x.target].hp){queue_extension(s,XOp::IFeedReward,x.target,b,true);enqueue(s,EffectKind::Damage,a,x.target,true);}break;
 case XOp::IFeedReward:
  if(a>=0&&a<s.enemy_count&&!s.enemies[a].hp){int n=min(999-s.max_hp,b);if(n>0){s.max_hp+=(int16_t)n;heal(s,n);}}break;
 case XOp::IEnemyStrength:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp)s.enemies[a].strength=(int16_t)min(9999,s.enemies[a].strength+b);break;
 case XOp::IIncreaseDamage:
  if(a>=0&&a<s.card_count)i.damage_bonus[a]=(int16_t)min(9999,i.damage_bonus[a]+b);break;
 case XOp::IPillageDraw:{
  unsigned depth=x.depth;i.pillage_drawn[depth]=-1;i.pillage_waiting[depth]=1;
  queue_extension(s,XOp::IPillageCheck,depth,0,true);queue_extension(s,XOp::Draw,1,0,true);break;}
 case XOp::IPillageCheck:{
  int id=i.pillage_drawn[a];i.pillage_drawn[a]=0;i.pillage_waiting[a]=0;
  if(id>=0&&id<s.card_count&&definitions[s.cards[id].kind].type==CardType::Attack&&s.hand.count<MaxHand&&!s.no_draw&&(s.draw.count||s.discard.count))queue_extension(s,XOp::IPillageDraw,0,0,true);break;}
 case XOp::IPrimalForce:
  for(unsigned at=0;at<s.hand.count;at++){int id=s.hand.ids[at];if(definitions[s.cards[id].kind].type!=CardType::Attack)continue;
   s.cards[id]={GiantRock,(uint8_t)a};reset_card_modifiers(s,id);record(s,LogKind::Transform,GiantRock);
  }break;
 case XOp::ISecondWind:{
  uint16_t ids[MaxHand];unsigned n=0;for(unsigned at=0;at<s.hand.count;at++)if(definitions[s.cards[s.hand.ids[at]].kind].type!=CardType::Attack)ids[n++]=s.hand.ids[at];
  if(s.effect_count+2*n>MaxEffects){fail(s);break;}
  for(unsigned at=n;at>0;at--){queue_extension(s,XOp::IBlock,a,0,true);enqueue(s,EffectKind::ExhaustCard,ids[at-1],0,true);}break;}
 case XOp::IStoke:{
  unsigned n=s.hand.count;if(s.card_count+n>MaxCards||s.effect_count+2*n>MaxEffects){fail(s);break;}
  uint16_t ids[MaxHand];for(unsigned at=0;at<n;at++)ids[at]=s.hand.ids[at];
  for(unsigned at=0;at<n;at++)queue_extension(s,XOp::Generate,0,a?2:0,true);
  for(unsigned at=n;at>0;at--)enqueue(s,EffectKind::ExhaustCard,ids[at-1],0,true);break;}
 case XOp::IThrash:{
  uint16_t ids[MaxHand];unsigned n=0;for(unsigned at=0;at<s.hand.count;at++)if(definitions[s.cards[s.hand.ids[at]].kind].type==CardType::Attack)ids[n++]=s.hand.ids[at];
  if(!n)break;int id=ids[random_below(s.rng_cards,n)];Card c=s.cards[id];
  // ModifyDamageHookType.All with the victim as card source and no enemy
  // target. The shared preview excludes captures consumed by this Thrash.
  int gained=damage(s,c,-1,id);
  i.damage_bonus[a]=(int16_t)min(9999,i.damage_bonus[a]+gained);
  enqueue(s,EffectKind::ExhaustCard,id,0,true);break;}
 case XOp::IHavoc:{
  if(!alive(s))break;
  if(!s.draw.count){if(!a&&s.discard.count){queue_extension(s,XOp::IHavoc,1,0,true);shuffle(s);}break;}
  if(x.depth>=MaxAutoFrames){fail(s);break;}
  int id=s.draw.ids[s.draw.count-1];i.forced_exhaust[id]=1;
  if(!start_instance(s,id,-1,true))i.forced_exhaust[id]=0;break;}
 case XOp::IAggression:{
  uint16_t ids[MaxCards];unsigned n=0;for(unsigned at=0;at<s.discard.count;at++)if(definitions[s.cards[s.discard.ids[at]].kind].type==CardType::Attack)ids[n++]=s.discard.ids[at];
  unsigned count=(unsigned)min(a,n);
  for(unsigned at=0;at<count;at++){unsigned pick=at+random_below(s.rng_cards,n-at);uint16_t id=ids[pick];ids[pick]=ids[at];ids[at]=id;bool room=s.hand.count<MaxHand;
   if(move_instance(s,id,s.discard,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard)&&!s.cards[id].upgraded){s.cards[id].upgraded=1;record(s,LogKind::Upgrade,s.cards[id].kind);}
  }break;}
 case XOp::IStartPower:{
  IPower p=(IPower)a;if(!ipower(s,p))break;
  if(p==IPower::CrimsonMantle)enqueue(s,EffectKind::Block,ipower(s,p),0,true);
  player_hp_loss(s,p==IPower::CrimsonMantle?i.crimson_self_damage:i.inferno_self_damage,false,false,-1);break;}
 case XOp::IStampede:{
  if(a<=0||!alive(s))break;
  uint16_t ids[MaxHand];unsigned n=0;for(unsigned at=0;at<s.hand.count;at++)if(definitions[s.cards[s.hand.ids[at]].kind].type==CardType::Attack)ids[n++]=s.hand.ids[at];
  if(!n)break;if(a>1)queue_extension(s,XOp::IStampede,a-1,0,true);
  int id=ids[random_below(s.rng_cards,n)];start_instance(s,id,-1,true);break;}
 case XOp::IHowls:
  if(alive(s))for(int id=a;id<s.card_count;id++)if(s.cards[id].kind==HowlFromBeyond&&s.zones[id]==Zone::Exhaust){queue_extension(s,XOp::IHowls,id+1,0,true);start_instance(s,id,-1,true);break;}break;
 case XOp::IClone:
  if(a>=0&&a<s.card_count){if(s.card_count+b>MaxCards){fail(s);break;}for(int n=0;n<b;n++)ironclad_copy_to_hand(s,a);}break;
 case XOp::IHits:
  if(b>0&&alive(s)){if(b>1)queue_extension(s,XOp::IHits,a,b-1,true);enqueue(s,EffectKind::Damage,a,x.target,true);}break;
 case XOp::IAllHits:
  if(b>0&&alive(s)){if(b>1)queue_extension(s,XOp::IAllHits,a,b-1,true);for(int t=s.enemy_count-1;t>=0;t--)if(s.enemies[t].hp)enqueue(s,EffectKind::Damage,a,t,true);}break;
 case XOp::ITemporaryStrength:{
  int actual=min(9999-s.strength,a);s.strength+=(int16_t)actual;i.setup_strength=(int16_t)min(9999,i.setup_strength+actual);record(s,LogKind::Strength,actual);break;}
 case XOp::IHeal:heal(s,a);break;
 case XOp::IUnpoweredRandom:{int targets[MaxEnemies],n=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp)targets[n++]=t;if(n)damage_unpowered(s,targets[random_below(s.rng_cards,n)],a);break;}
 default:return false;
 }
 return true;
}
static bool ironclad_word(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
bool ironclad_description_value(const State&s,Card c,int target,int instance,const char*key,int&out){
 if((c.kind<Aggression||c.kind>Vicious)&&c.kind!=GiantRock)return false;
 if(ironclad_word(key,"Damage")||ironclad_word(key,"CalculatedDamage")){out=damage(s,c,target,instance);return true;}
 if(ironclad_word(key,"Block")||ironclad_word(key,"CalculatedBlock")){
  const auto&d=definitions[c.kind];int n=d.block+(c.upgraded?d.block_up:0);
  if(c.kind==ExpectAFight)n+=(c.upgraded?8:5)*s.strength;
  out=min(9999,silent_modify_block(s,ironclad_block(s,c,powered_block(s,c,n))));return true;
 }
 if(ironclad_word(key,"CalculatedHits")&&c.kind==TearAsunder){out=1+s.ironclad.hp_losses_combat;return true;}
 return false;
}
