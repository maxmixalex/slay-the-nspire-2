// Full Silent model effects use the ordinary serialized continuation queue.
static int spower(const State&s,SPower p){return s.silent.powers[(unsigned)p];}
static void silent_unregister(State&s,unsigned entry){
 auto&v=s.silent;for(unsigned i=0;i<v.power_count;i++)if(v.power_order[i]==entry){
  for(unsigned j=i+1;j<v.power_count;j++)v.power_order[j-1]=v.power_order[j];
  v.power_order[--v.power_count]=0;return;
 }
}
static void silent_register(State&s,unsigned entry){
 auto&v=s.silent;for(unsigned i=0;i<v.power_count;i++)if(v.power_order[i]==entry)return;
 if(v.power_count>=SPowerCount+MaxNightmares){fail(s);return;}
 v.power_order[v.power_count++]=(uint8_t)entry;
}
static void silent_gain_power(State&s,SPower p,int amount){
 auto&n=s.silent.powers[(unsigned)p];int old=n;n=(int16_t)max(0,min(9999,n+amount));
 if(!old&&n)silent_register(s,(unsigned)p);else if(old&&!n)silent_unregister(s,(unsigned)p);
}
static void silent_combat_start(State&s){s.silent=SilentState{};}
static void silent_card_reset(State&s,int id){
 if(id<0||id>=MaxCards)return;auto&v=s.silent;
 v.sly[id]=v.retain[id]=v.inky[id]=v.bullet_free[id]=0;
 v.discount[id]=v.turn_origin[id]=v.combat_origin[id]=v.bullet_origin[id]=0;
}
static void silent_card_created(State&s,int id){
 if(id<0||id>=s.card_count)return;silent_card_reset(s,id);
 if(s.cards[id].kind==Pinpoint)s.silent.discount[id]=s.silent.skills_finished;
 if(s.cards[id].kind==Shiv&&spower(s,SPower::PhantomBlades))s.silent.retain[id]=1;
}
static void silent_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&v=s.silent;
 v.sly[to]=v.sly[from];v.retain[to]=v.retain[from];v.inky[to]=v.inky[from];v.bullet_free[to]=v.bullet_free[from];
 v.discount[to]=v.discount[from];v.turn_origin[to]=v.turn_origin[from];v.combat_origin[to]=v.combat_origin[from];v.bullet_origin[to]=v.bullet_origin[from];
 if(s.cards[to].kind==Shiv&&spower(s,SPower::PhantomBlades))v.retain[to]|=1;
}
static void silent_temporary_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.turn_cost[id])s.silent.turn_origin[id]=s.silent.discount[id];
}
static void silent_combat_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.combat_cost[id]){
  s.silent.combat_origin[id]=s.silent.discount[id];s.silent.bullet_free[id]=0;s.silent.bullet_origin[id]=0;
 }
}
static int silent_cost(const State&s,int id,int current){
 if(id<0||id>=s.card_count||current<0)return current;const auto&v=s.silent;
 int origin=s.expansion.turn_cost[id]?v.turn_origin[id]:s.expansion.combat_cost[id]?v.combat_origin[id]:0;
 if(!s.expansion.turn_cost[id]&&v.bullet_free[id]){current=0;origin=v.bullet_origin[id];}
 current=max(0,current-max(0,(int)v.discount[id]-origin));
 if(spower(s,SPower::FreeSkill)&&definitions[s.cards[id].kind].type==CardType::Skill&&
    (s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo))return 0;
 return current;
}
static bool silent_can_play(const State&s,Card c){return c.kind!=GrandFinale||!s.draw.count;}
static bool silent_is_sly(const State&s,int id){
 if(id<0||id>=s.card_count)return false;Card c=s.cards[id];
 return s.silent.sly[id]||((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&16);
}
static bool silent_retains(const State&s,int id){
 return id>=0&&id<s.card_count&&(s.silent.retain[id]||spower(s,SPower::WellLaidPlans)||
        (s.cards[id].kind==Shiv&&spower(s,SPower::PhantomBlades)));
}
static int silent_extra_replays(State&s,Card c){
 if(definitions[c.kind].type!=CardType::Skill||!spower(s,SPower::Burst))return 0;
 silent_gain_power(s,SPower::Burst,-1);return 1;
}
static void silent_before_play(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.silent;unsigned f=s.expansion.depth;
 v.afterimage_before[f]=(int16_t)spower(s,SPower::Afterimage);
 v.serpent_before[f]=(int16_t)spower(s,SPower::SerpentForm);
 for(int t=0;t<MaxEnemies;t++)v.strangle_before[f][t]=v.strangle[t];
 if(definitions[s.cards[id].kind].type==CardType::Skill&&spower(s,SPower::FreeSkill))silent_gain_power(s,SPower::FreeSkill,-1);
}
static void silent_after_play(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.silent;unsigned f=s.expansion.depth;Card c=s.cards[id];
 // Hooks are prepended in reverse order so each captured trigger completes
 // before the card's result-pile movement, including suspended nested draws.
 for(int t=MaxEnemies-1;t>=0;t--)if(v.strangle_before[f][t])queue_extension(s,XOp::SStrangle,t,v.strangle_before[f][t],true);
 if(v.serpent_before[f])queue_extension(s,XOp::SUnpoweredRandom,v.serpent_before[f],0,true);
 if(v.afterimage_before[f])enqueue(s,EffectKind::Block,v.afterimage_before[f],0,true);
 v.afterimage_before[f]=v.serpent_before[f]=0;for(auto&n:v.strangle_before[f])n=0;
 if(definitions[c.kind].type==CardType::Attack)v.attacks_finished=(uint16_t)min(9999,v.attacks_finished+1);
 if(definitions[c.kind].type==CardType::Skill){
  v.skills_finished=(uint16_t)min(9999,v.skills_finished+1);
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind==Pinpoint)v.discount[i]=(uint16_t)min(9999,v.discount[i]+1);
  if(spower(s,SPower::MasterPlanner))v.sly[id]|=1;
 }
 if(c.kind==Shiv)v.shiv_finished=1;
}
static void silent_card_drawn(State&s,int id){
 auto&v=s.silent;v.drawn_combat=(uint16_t)min(9999,v.drawn_combat+1);unsigned f=s.expansion.depth;
 if(v.retain_waiting[f]){v.retain_waiting[f]=0;v.retain_drawn[f]=(int16_t)(id+1);}
 if(!s.expansion.enemy_side&&spower(s,SPower::Speedster))queue_extension(s,XOp::AllDamage,spower(s,SPower::Speedster),0,true);
 for(int t=s.enemy_count-1;t>=0;t--)if(spower(s,SPower::CorrosiveWave))enqueue(s,EffectKind::Poison,spower(s,SPower::CorrosiveWave),t,true);
}
static void silent_card_discarded(State&s,int id){
 s.silent.discarded_turn=(uint16_t)min(9999,s.silent.discarded_turn+1);
 if(silent_is_sly(s,id))queue_extension(s,XOp::AutoInstance,id,0,true);
}
static bool silent_blocks_debuff(State&s,int t){
 if(t<0||t>=s.enemy_count||!s.silent.enemy_artifact[t])return false;
 --s.silent.enemy_artifact[t];return true;
}
static void silent_attack_damage(State&s,int t,int unblocked){
 if(unblocked>0&&spower(s,SPower::Envenom))apply_poison(s,t,spower(s,SPower::Envenom));
}
static void silent_attack_killed(State&s,int){
 if(s.playing>=0&&s.cards[s.playing].kind==TheHunt)s.silent.extra_rewards=(uint16_t)min(9999,s.silent.extra_rewards+1);
}
static bool silent_next_reward(State&s){
 if(!s.silent.extra_rewards)return false;--s.silent.extra_rewards;
 for(int i=0;i<3;i++){int k;bool duplicate;do{k=(int)random_reward_card(s);duplicate=false;for(int j=0;j<i;j++)if(s.reward[j]==k)duplicate=true;}while(duplicate);s.reward[i]=(uint8_t)k;}
 s.reward_claimed&=(uint8_t)~2;return true;
}
static int silent_hand_type_count(const State&s,CardType type){int n=0;for(unsigned i=0;i<s.hand.count;i++)if(definitions[s.cards[s.hand.ids[i]].kind].type==type)++n;return n;}
static int silent_shiv_count(const State&s){int n=0;for(unsigned i=0;i<s.exhaust.count;i++)if(s.cards[s.exhaust.ids[i]].kind==Shiv)++n;return n;}
static int silent_poison_total(const State&s){int n=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp)n+=s.enemies[t].poison;return min(9999,n);}
static int silent_base_damage(const State&s,Card c,int,int instance,int current){
 if(c.kind==MementoMori)return min(9999,(c.upgraded?11:9)+(c.upgraded?5:4)*s.silent.discarded_turn);
 if(c.kind==Murder)return min(9999,1+s.silent.drawn_combat);
 if(c.kind==PreciseCut){int other=s.hand.count;if(instance>=0&&instance<s.card_count&&s.zones[instance]==Zone::Hand)--other;return max(0,(c.upgraded?16:13)-2*other);}
 if(c.kind==Shiv&&!s.silent.shiv_finished)return min(9999,current+spower(s,SPower::PhantomBlades));
 return current;
}
static int silent_attack_factor(const State&s,int t,int amount){
 // The original command multiplies decimal factors before converting damage
 // to an integer. Keep Vulnerable and Tracking in the same integer fraction.
 int64_t n=amount;if(spower(s,SPower::DoubleDamage))n*=2;
 int vulnerable=100,tracking=100;
 if(t>=0&&t<s.enemy_count){if(s.enemies[t].vulnerable)vulnerable=ironclad_vulnerable_multiplier(s);if(s.enemies[t].weak)tracking+=spower(s,SPower::Tracking);}
 n=n*vulnerable*tracking/10000;
 return (int)(n>9999999?9999999:n);
}
static int silent_modify_block(const State&s,int n){
 if(n<=0)return 0;int layers=spower(s,SPower::Shadowmeld);
 while(layers--&&n<9999)n=min(9999,n*2);return n;
}
static bool silent_keeps_block(State&s){
 return spower(s,SPower::Blur)>0;
}
static void silent_turn_start(State&s){
 auto&v=s.silent;v.attacks_finished=v.skills_finished=v.discarded_turn=0;v.shiv_finished=0;
 if(spower(s,SPower::EnergyNextTurn)){s.energy=(int16_t)min(999,s.energy+spower(s,SPower::EnergyNextTurn));silent_gain_power(s,SPower::EnergyNextTurn,-spower(s,SPower::EnergyNextTurn));}
}
static int silent_hand_draw_count(const State&s){return min(MaxCards,spower(s,SPower::ToolsOfTheTrade));}
static void silent_before_hand_draw(State&s){
 auto&v=s.silent;bool blades=false;
 for(unsigned i=v.power_count;i>0;i--){unsigned p=v.power_order[i-1];
  if(p>=SPowerCount)queue_extension(s,XOp::SNightmareCopies,p-SPowerCount,0,true);
  else if(p==(unsigned)SPower::InfiniteBlades){blades=true;if(s.infinite_blades)queue_extension(s,XOp::Create,s.infinite_blades,Shiv,true);}
 }
 if(!blades&&s.infinite_blades)queue_extension(s,XOp::Create,s.infinite_blades,Shiv,true);
}
static void silent_turn_ready(State&s){
 auto&v=s.silent;bool noxious=false;
 if(spower(s,SPower::ToolsOfTheTrade))enqueue(s,EffectKind::ChooseDiscard,min(MaxHand,spower(s,SPower::ToolsOfTheTrade)),0,true);
 for(unsigned i=v.power_count;i>0;i--){unsigned p=v.power_order[i-1];
  if(p==(unsigned)SPower::NoxiousFumes)noxious=true;
  if(p==(unsigned)SPower::NoxiousFumes||p==(unsigned)SPower::WraithForm||p==(unsigned)SPower::ShadowStep||p==(unsigned)SPower::Blur)queue_extension(s,XOp::SPowerTurn,p,0,true);
 }
 if(!noxious&&s.noxious_fumes)queue_extension(s,XOp::SPowerTurn,(int)SPower::NoxiousFumes,0,true);
}
static void silent_player_side_end(State&s){
 auto&v=s.silent;const SPower ended[]={SPower::Burst,SPower::CorrosiveWave,SPower::Shadowmeld};
 for(auto p:ended)silent_gain_power(s,p,-spower(s,p));
 if(spower(s,SPower::DoubleDamage))silent_gain_power(s,SPower::DoubleDamage,-1);
 for(auto&n:v.strangle)n=0;
 for(unsigned id=0;id<s.card_count;id++){
  v.sly[id]&=1;v.retain[id]&=1;v.bullet_free[id]=0;v.bullet_origin[id]=v.turn_origin[id]=0;
  if(s.cards[id].kind==Pinpoint)v.discount[id]=v.combat_origin[id]=0;
 }
}
static void silent_enemy_poison(State&s){
 for(int t=0;t<s.enemy_count;t++){
  auto&e=s.enemies[t];int ticks=min(e.poison,1+spower(s,SPower::Accelerant));
  while(ticks--&&e.hp&&e.poison){int n=e.poison;e.hp=(int16_t)max(0,e.hp-n);if(e.hp)--e.poison;record(s,LogKind::PoisonDamage,n,t);}
 }
}
static void silent_combat_end(State&s){
 auto&v=s.silent;for(unsigned f=0;f<=MaxAutoFrames;f++){v.afterimage_before[f]=v.serpent_before[f]=v.retain_drawn[f]=0;v.retain_waiting[f]=0;for(auto&n:v.strangle_before[f])n=0;}
}
static SilentSnapshot silent_capture(const State&s,int id){
 const auto&v=s.silent;const auto&x=s.expansion;const auto&i=s.ironclad;SilentSnapshot n{};
 n.card=s.cards[id];n.turn_cost=x.turn_cost[id];n.combat_cost=x.combat_cost[id];n.replay=x.replay[id];n.return_turn=x.return_turn[id];n.blade_damage=x.blade_damage[id];
 n.sly=v.sly[id];n.retain=v.retain[id];n.inky=v.inky[id];n.bullet_free=v.bullet_free[id];n.discount=v.discount[id];n.turn_origin=v.turn_origin[id];n.combat_origin=v.combat_origin[id];n.bullet_origin=v.bullet_origin[id];
 n.iron_damage=i.damage_bonus[id];n.stomp_discount=i.stomp_discount[id];n.stomp_turn_origin=i.stomp_turn_origin[id];n.stomp_combat_origin=i.stomp_combat_origin[id];return n;
}
static int silent_clone(State&s,const SilentSnapshot&n){
 int id=create_to_hand(s,n.card);if(id<0)return -1;auto&v=s.silent;auto&x=s.expansion;auto&i=s.ironclad;
 x.turn_cost[id]=n.turn_cost;x.combat_cost[id]=n.combat_cost;x.replay[id]=n.replay;
 // A past scheduled return must not become an invalid stale turn index.
 x.return_turn[id]=n.return_turn>=(unsigned)s.turn?n.return_turn:0;x.blade_damage[id]=n.blade_damage;
 v.sly[id]=n.sly;v.retain[id]=n.retain;v.inky[id]=n.inky;v.bullet_free[id]=n.bullet_free;v.discount[id]=n.discount;v.turn_origin[id]=n.turn_origin;v.combat_origin[id]=n.combat_origin;v.bullet_origin[id]=n.bullet_origin;
 if(n.card.kind==Shiv&&spower(s,SPower::PhantomBlades))v.retain[id]|=1;
 i.damage_bonus[id]=n.iron_damage;i.stomp_discount[id]=n.stomp_discount;i.stomp_turn_origin[id]=n.stomp_turn_origin;i.stomp_combat_origin[id]=n.stomp_combat_origin;i.forced_exhaust[id]=0;return id;
}
static bool silent_choose(State&s,const uint16_t*ids,unsigned count){
 auto&x=s.expansion;XChoice kind=x.choice;
 if(s.phase!=Phase::ExtendedChoice||(kind!=XChoice::SlySkill&&kind!=XChoice::NightmareCard)||!ids||count!=1||x.choice_min!=1||x.choice_max!=1)return false;
 int id=ids[0];bool found=false;for(unsigned i=0;i<x.choices.count;i++)if(x.choices.ids[i]==id)found=true;
 if(!found||id<0||id>=s.card_count||s.zones[id]!=Zone::Hand)return false;
 if(kind==XChoice::SlySkill&&(definitions[s.cards[id].kind].type!=CardType::Skill||silent_is_sly(s,id)))return false;
 int slot=0;if(kind==XChoice::NightmareCard){while(slot<MaxNightmares&&s.silent.nightmare_active[slot])++slot;if(slot==MaxNightmares||s.silent.power_count>=SPowerCount+MaxNightmares)return false;}
 if(kind==XChoice::SlySkill)s.silent.sly[id]|=2;
 else{s.silent.nightmares[slot]=silent_capture(s,id);s.silent.nightmare_active[slot]=1;silent_register(s,SPowerCount+slot);}
 clear_extended_choice(s);s.phase=Phase::Combat;resolve_pending(s);++s.revision;return true;
}
static bool queue_silent(State&s,Card c,int target,int energy){
 if(!card_in_pool(c.kind,Character::Silent)&&c.kind!=Shiv)return false;
 const auto&d=definitions[c.kind];int hit=card_base_damage(s,c,target,s.playing),block=d.block+(c.upgraded?d.block_up:0),p=d.power+(c.upgraded?d.power_up:0),draw=d.draw+(c.upgraded?d.draw_up:0);
 record(s,LogKind::Play,c.kind,c.upgraded);if(d.type==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};
 auto power=[&](SPower which,int n){op(XOp::SPower,(int)which,n);};
 auto attack=[&](int n){enqueue(s,EffectKind::Damage,n,target);};
 auto all=[&](int n){for(int t=0;t<s.enemy_count;t++)enqueue(s,EffectKind::Damage,n,t);};
 auto defend=[&](int n){enqueue_card_block(s,powered_block(s,c,n));};
 auto poison=[&](int n){enqueue(s,EffectKind::Poison,n,target);};
 auto weak=[&](int n){enqueue(s,EffectKind::Weak,n,target);};
 switch(c.kind){
 case StrikeSilent:case Backstab:case Slice:attack(hit);break;
 case DefendSilent:case Deflect:defend(block);break;
 case Neutralize:case SuckerPunch:attack(hit);weak(p);break;
 case Survivor:defend(block);enqueue(s,EffectKind::ChooseDiscard,1);break;
 case Acrobatics:op(XOp::Draw,draw);enqueue(s,EffectKind::ChooseDiscard,1);break;
 case Backflip:defend(block);op(XOp::Draw,draw);break;
 case DaggerThrow:attack(hit);op(XOp::Draw,1);enqueue(s,EffectKind::ChooseDiscard,1);break;
 case DeadlyPoison:poison(p);break;
 case PoisonedStab:attack(hit);poison(p);break;
 case CloakAndDagger:defend(block);op(XOp::SCreateShivs,c.upgraded?2:1);break;
 case BladeDance:op(XOp::SCreateShivs,c.upgraded?4:3);break;
 case DaggerSpray:all(hit);all(hit);break;
 case Dash:defend(block);attack(hit);break;
 case LegSweep:weak(p);defend(block);break;
 case Footwork:enqueue(s,EffectKind::Dexterity,p);break;
 case NoxiousFumes:power(SPower::NoxiousFumes,p);break;
 case BouncingFlask:for(int j=0;j<(c.upgraded?4:3);j++)enqueue(s,EffectKind::RandomPoison,p);break;
 case Prepared:op(XOp::Draw,draw);enqueue(s,EffectKind::ChooseDiscard,draw);break;
 case Adrenaline:enqueue(s,EffectKind::Energy,c.upgraded?2:1);op(XOp::Draw,2);break;
 case Accuracy:enqueue(s,EffectKind::Accuracy,p);break;
 case LeadingStrike:attack(hit);op(XOp::SCreateShivs,2);break;
 case DodgeAndRoll:enqueue(s,EffectKind::DodgeBlock,powered_block(s,c,block));break;
 case InfiniteBlades:power(SPower::InfiniteBlades,1);break;
 case EscapePlan:op(XOp::SEscapeDraw,powered_block(s,c,block));break;
 case Predator:attack(hit);enqueue(s,EffectKind::DrawNextTurn,2);break;
 case Shiv:
  if(spower(s,SPower::FanOfKnives))all(hit);else attack(hit);
  if(s.silent.inky[s.playing])op(XOp::SInky,s.playing,target);break;
 case Abrasive:op(XOp::SAbrasive,p);break;
 case Accelerant:power(SPower::Accelerant,p);break;
 case Afterimage:power(SPower::Afterimage,p);break;
 case Anticipate:op(XOp::SAnticipate,p);break;
 case Assassinate:attack(hit);enqueue(s,EffectKind::Vulnerable,p,target);break;
 case BladeOfInk:op(XOp::SCreateShivs,c.upgraded?3:2,2);break;
 case Blur:defend(block);power(SPower::Blur,1);break;
 case BubbleBubble:if(target>=0&&target<s.enemy_count&&s.enemies[target].poison)poison(p);break;
 case BulletTime:op(XOp::SBulletTime);break;
 case Burst:power(SPower::Burst,p);break;
 case CalculatedGamble:op(XOp::SDiscardHand,1);break;
 case CorrosiveWave:power(SPower::CorrosiveWave,p);break;
 case EchoingSlash:op(XOp::SEchoSlash,hit);break;
 case Envenom:power(SPower::Envenom,p);break;
 case Expertise:op(XOp::SRetainDraw,draw);break;
 case Expose:op(XOp::SExpose,target,p);break;
 case FanOfKnives:power(SPower::FanOfKnives,1);op(XOp::SCreateShivs,c.upgraded?5:4);break;
 case Finisher:op(XOp::SHits,hit,s.silent.attacks_finished);break;
 case Flechettes:op(XOp::SHits,hit,silent_hand_type_count(s,CardType::Skill));break;
 case FlickFlack:all(hit);break;
 case Sidestep:power(SPower::EnergyNextTurn,c.upgraded?2:1);break;
 case GrandFinale:all(hit);break;
 case HandTrick:defend(block);op(XOp::SSlyChoice);break;
 case Haze:for(int t=0;t<s.enemy_count;t++){enqueue(s,EffectKind::Poison,p,t);enqueue(s,EffectKind::Weak,c.upgraded?2:1,t);}break;
 case HiddenDaggers:enqueue(s,EffectKind::ChooseDiscard,2);op(XOp::SCreateShivs,2,c.upgraded?1:0);break;
 case KnifeTrap:
  for(unsigned i=0;i<s.exhaust.count;i++){int id=s.exhaust.ids[i];if(s.cards[id].kind==Shiv)op(XOp::SKnifeShiv,id,target+(c.upgraded?MaxEnemies:0));}break;
 case Malaise:op(XOp::SMalaise,target,min(9999,energy+(c.upgraded?1:0)));break;
 case MasterPlanner:power(SPower::MasterPlanner,1);break;
 case MementoMori:case Murder:case PreciseCut:case Pinpoint:attack(hit);break;
 case Mirage:defend(silent_poison_total(s));break;
 case Nightmare:op(XOp::SNightmareChoice);break;
 case Outbreak:for(int t=0;t<s.enemy_count;t++)enqueue(s,EffectKind::Poison,p,t);op(XOp::SPoisonTrigger);break;
 case PhantomBlades:power(SPower::PhantomBlades,p);break;
 case PiercingWail:for(int t=0;t<s.enemy_count;t++)op(XOp::StrengthLoss,t,p);break;
 case Pounce:attack(hit);power(SPower::FreeSkill,1);break;
 case Reflex:op(XOp::Draw,draw);break;
 case Ricochet:op(XOp::RandomHits,hit,p);break;
 case SerpentForm:power(SPower::SerpentForm,p);break;
 case ShadowStep:op(XOp::SDiscardHand);power(SPower::ShadowStep,1);break;
 case Shadowmeld:power(SPower::Shadowmeld,1);break;
 case Skewer:op(XOp::SHits,hit,energy);break;
 case Snakebite:poison(p);break;
 case Speedster:power(SPower::Speedster,p);break;
 case StormOfSteel:op(XOp::SDiscardHand,c.upgraded?3:2);break;
 case Strangle:attack(hit);op(XOp::SPower,(int)SPower::Count+target,p);break;
 case Suppress:attack(hit);weak(p);break;
 case Tactician:enqueue(s,EffectKind::Energy,c.upgraded?2:1);break;
 case TheHunt:attack(hit);break;
 case ToolsOfTheTrade:power(SPower::ToolsOfTheTrade,1);break;
 case Tracking:power(SPower::Tracking,50);break;
 case Untouchable:defend(block);break;
 case UpMySleeve:op(XOp::SCreateShivs,c.upgraded?4:3);op(XOp::SUpMySleeve,s.playing);break;
 case WellLaidPlans:power(SPower::WellLaidPlans,1);break;
 case WraithForm:op(XOp::Power,(int)XPower::Intangible,p);power(SPower::WraithForm,1);break;
 default:return false;
 }
 enqueue(s,EffectKind::Finish);return true;
}
static bool silent_effect(State&s,Effect e){
 const auto op=(XOp)e.target;int a=e.value,b=e.aux;auto&v=s.silent;
 switch(op){
 case XOp::SPower:
  if(a>=SPowerCount){int t=a-SPowerCount;if(t>=0&&t<s.enemy_count&&s.enemies[t].hp&&!silent_blocks_debuff(s,t))v.strangle[t]=(int16_t)min(9999,v.strangle[t]+b);break;}
  silent_gain_power(s,(SPower)a,b);
  if(a==(int)SPower::InfiniteBlades)s.infinite_blades=(int16_t)min(MaxCards,s.infinite_blades+b);
  if(a==(int)SPower::NoxiousFumes){s.noxious_fumes=(int16_t)min(9999,s.noxious_fumes+b);record(s,LogKind::Noxious,b);}
  if(a==(int)SPower::PhantomBlades)for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==Shiv)v.retain[id]|=1;
  break;
 case XOp::SAbrasive:s.dexterity=(int16_t)min(9999,s.dexterity+1);s.thorns=(int16_t)min(9999,s.thorns+a);record(s,LogKind::Dexterity,1);break;
 case XOp::SAnticipate:{int n=min(a,min(9999-s.dexterity,9999-s.temporary_dexterity));s.dexterity+=(int16_t)n;s.temporary_dexterity+=(int16_t)n;record(s,LogKind::Dexterity,n);break;}
 case XOp::SBulletTime:
  for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(cost(s.cards[id])>=0){s.expansion.turn_cost[id]=0;v.bullet_free[id]=1;v.bullet_origin[id]=v.discount[id];}}
  s.no_draw=1;record(s,LogKind::NoDraw);break;
 case XOp::SCreateShivs:
  if(s.card_count+a>MaxCards){fail(s);break;}
  for(int n=0;n<a;n++){int id=create_to_hand(s,{Shiv,(uint8_t)(b&1)});if(id<0)break;if(b&2)v.inky[id]=1;}break;
 case XOp::SDiscardHand:{
  uint16_t ids[MaxHand];unsigned n=s.hand.count;if(!n)break;
  if(s.effect_count+n+2>MaxEffects){fail(s);break;}
  for(unsigned i=0;i<n;i++)ids[i]=s.hand.ids[i];
  while(s.hand.count){int id=remove(s.hand,0);put(s,s.discard,id,Zone::Discard);record(s,LogKind::Discard,s.cards[id].kind);}
  if(a>=2)queue_extension(s,XOp::SCreateShivs,n,a==3?1:0,true);
  for(unsigned i=n;i>0;i--)silent_card_discarded(s,ids[i-1]);
  if(a==1)queue_extension(s,XOp::Draw,n,0,true);break;
 }
 case XOp::SEchoSlash:{
  int mask=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp)mask|=1<<t;
  if(!mask)break;queue_extension(s,XOp::SEchoRepeat,a,mask,true);
  for(int t=s.enemy_count-1;t>=0;t--)if(mask&(1<<t))enqueue(s,EffectKind::Damage,a,t,true);break;
 }
 case XOp::SEchoRepeat:{
  int kills=0;for(int t=0;t<s.enemy_count;t++)if((b&(1<<t))&&!s.enemies[t].hp)++kills;
  for(int n=0;n<kills;n++)queue_extension(s,XOp::SEchoSlash,a,0,true);break;
 }
 case XOp::SExpose:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp){v.enemy_artifact[a]=0;s.enemies[a].block=0;enqueue(s,EffectKind::Vulnerable,b,a,true);}break;
 case XOp::SSlyChoice:{
  Pile eligible{};for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(definitions[s.cards[id].kind].type==CardType::Skill&&!silent_is_sly(s,id))eligible.ids[eligible.count++]=(uint16_t)id;}
  set_extended_choice(s,XChoice::SlySkill,eligible,1,1);break;
 }
 case XOp::SNightmareChoice:set_extended_choice(s,XChoice::NightmareCard,s.hand,1,1);break;
 case XOp::SNightmareCopies:
  if(a>=0&&a<MaxNightmares&&v.nightmare_active[a]){
   if(s.card_count+3>MaxCards){fail(s);break;}SilentSnapshot copy=v.nightmares[a];
   v.nightmare_active[a]=0;v.nightmares[a]=SilentSnapshot{};silent_unregister(s,SPowerCount+a);
   for(int n=0;n<3;n++)if(silent_clone(s,copy)<0)break;
  }break;
 case XOp::SRetainDraw:{
  if(a<=0)break;unsigned f=s.expansion.depth;v.retain_waiting[f]=1;v.retain_drawn[f]=0;
  queue_extension(s,XOp::SRetainDrawEnd,a-1,0,true);queue_extension(s,XOp::Draw,1,0,true);break;
 }
 case XOp::SRetainDrawEnd:{
  unsigned f=s.expansion.depth;v.retain_waiting[f]=0;int id=v.retain_drawn[f]-1;v.retain_drawn[f]=0;
  if(id>=0&&id<s.card_count)v.retain[id]|=2;
  if(a>0)queue_extension(s,XOp::SRetainDraw,a,0,true);break;
 }
 case XOp::SEscapeDraw:{
  unsigned f=s.expansion.depth;v.retain_waiting[f]=1;v.retain_drawn[f]=0;
  queue_extension(s,XOp::SEscapeBlock,a,0,true);queue_extension(s,XOp::Draw,1,0,true);break;
 }
 case XOp::SEscapeBlock:{
  unsigned f=s.expansion.depth;v.retain_waiting[f]=0;int id=v.retain_drawn[f]-1;v.retain_drawn[f]=0;
  if(id>=0&&id<s.card_count&&definitions[s.cards[id].kind].type==CardType::Skill)enqueue_card_block(s,a,true);
  break;
 }
 case XOp::SKnifeShiv:{
  int t=b%MaxEnemies;if(a>=0&&a<s.card_count&&s.cards[a].kind==Shiv&&s.enemies[t].hp){
   if(b>=MaxEnemies)s.cards[a].upgraded=1;start_instance(s,a,t,true);
  }break;
 }
 case XOp::SMalaise:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&b>0){if(!silent_blocks_debuff(s,a))s.enemies[a].strength=(int16_t)max(-9999,s.enemies[a].strength-b);enqueue(s,EffectKind::Weak,b,a,true);}break;
 case XOp::SPoisonTrigger:silent_enemy_poison(s);break;
 case XOp::SStrangle:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp){int n=min(s.enemies[a].hp,b);s.enemies[a].hp-=(int16_t)n;record(s,LogKind::LoseHp,n,a+1);}break;
 case XOp::SUnpoweredRandom:{
  int ids[MaxEnemies],n=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp)ids[n++]=t;
  if(n)damage_unpowered(s,ids[random_below(s.rng_cards,n)],a);break;
 }
 case XOp::SUpMySleeve:
  if(a>=0&&a<s.card_count&&s.cards[a].kind==UpMySleeve)v.discount[a]=(uint16_t)min(9999,v.discount[a]+1);break;
 case XOp::SPowerTurn:
  if(a==(int)SPower::NoxiousFumes){for(int t=0;t<s.enemy_count;t++)apply_poison(s,t,s.noxious_fumes);}
  else if(a==(int)SPower::WraithForm){int n=spower(s,SPower::WraithForm);s.dexterity=(int16_t)max(-9999,s.dexterity-n);record(s,LogKind::Dexterity,-n);}
  else if(a==(int)SPower::ShadowStep){int n=spower(s,SPower::ShadowStep);silent_gain_power(s,SPower::DoubleDamage,n);silent_gain_power(s,SPower::ShadowStep,-n);}
  else if(a==(int)SPower::Blur)silent_gain_power(s,SPower::Blur,-1);
  break;
 case XOp::SHits:
  if(b>0&&s.expansion.target>=0&&s.expansion.target<s.enemy_count&&s.enemies[s.expansion.target].hp){
   if(b>1)queue_extension(s,XOp::SHits,a,b-1,true);enqueue(s,EffectKind::Damage,a,s.expansion.target,true);
  }break;
 case XOp::SInky:
  if(spower(s,SPower::FanOfKnives)){for(int t=s.enemy_count-1;t>=0;t--)enqueue(s,EffectKind::Weak,1,t,true);}
  else if(b>=0&&b<s.enemy_count)enqueue(s,EffectKind::Weak,1,b,true);break;
 default:return false;
 }
 return true;
}

static bool silent_card_targets_enemy(const State&s,Card c){return c.kind<CardKinds&&definitions[c.kind].targeted&&!(c.kind==Shiv&&spower(s,SPower::FanOfKnives));}

static bool silent_var_name(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool silent_description_value(const State&s,Card c,int target,int instance,const char*name,int&out){
 if(c.kind>=CardKinds||(!card_in_pool(c.kind,Character::Silent)&&c.kind!=Shiv))return false;
 if(silent_var_name(name,"Damage")||silent_var_name(name,"CalculatedDamage")){out=damage(s,c,target,instance);return true;}
 if(silent_var_name(name,"Block")){out=card_block(s,c,instance);return true;}
 if(c.kind==Finisher&&silent_var_name(name,"CalculatedHits")){out=s.silent.attacks_finished;return true;}
 if(c.kind==Flechettes&&silent_var_name(name,"CalculatedHits")){out=silent_hand_type_count(s,CardType::Skill);return true;}
 if(c.kind==KnifeTrap&&silent_var_name(name,"CalculatedShivs")){out=silent_shiv_count(s);return true;}
 if(c.kind==Mirage&&silent_var_name(name,"CalculatedBlock")){out=silent_modify_block(s,ironclad_block(s,c,powered_block(s,c,silent_poison_total(s)),instance));return true;}
 switch(c.kind){
 case Abrasive:
  if(silent_var_name(name,"ThornsPower")){out=(c.upgraded?6:4);return true;}
  if(silent_var_name(name,"DexterityPower")){out=1;return true;}
  break;
 case Accelerant:
  if(silent_var_name(name,"Accelerant")){out=(c.upgraded?2:1);return true;}
  break;
 case Accuracy:
  if(silent_var_name(name,"AccuracyPower")){out=(c.upgraded?6:4);return true;}
  break;
 case Acrobatics:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?4:3);return true;}
  break;
 case Adrenaline:
  if(silent_var_name(name,"Energy")){out=(c.upgraded?2:1);return true;}
  if(silent_var_name(name,"Cards")){out=2;return true;}
  break;
 case Afterimage:
  if(silent_var_name(name,"AfterimagePower")){out=1;return true;}
  break;
 case Anticipate:
  if(silent_var_name(name,"DexterityPower")){out=(c.upgraded?4:2);return true;}
  break;
 case Assassinate:
  if(silent_var_name(name,"VulnerablePower")){out=(c.upgraded?2:1);return true;}
  break;
 case Backflip:
  if(silent_var_name(name,"Cards")){out=2;return true;}
  break;
 case BladeOfInk:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?3:2);return true;}
  break;
 case BladeDance:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?4:3);return true;}
  break;
 case Blur:
  if(silent_var_name(name,"Blur")){out=1;return true;}
  break;
 case BouncingFlask:
  if(silent_var_name(name,"PoisonPower")){out=3;return true;}
  if(silent_var_name(name,"Repeat")){out=(c.upgraded?4:3);return true;}
  break;
 case BubbleBubble:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?12:9);return true;}
  break;
 case Burst:
  if(silent_var_name(name,"Skills")){out=(c.upgraded?2:1);return true;}
  break;
 case CloakAndDagger:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?2:1);return true;}
  break;
 case CorrosiveWave:
  if(silent_var_name(name,"CorrosiveWave")){out=(c.upgraded?3:2);return true;}
  break;
 case DeadlyPoison:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?7:5);return true;}
  break;
 case Envenom:
  if(silent_var_name(name,"EnvenomPower")){out=(c.upgraded?2:1);return true;}
  break;
 case Expertise:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?3:2);return true;}
  break;
 case Expose:
  if(silent_var_name(name,"Power")){out=(c.upgraded?3:2);return true;}
  break;
 case FanOfKnives:
  if(silent_var_name(name,"Shivs")){out=(c.upgraded?5:4);return true;}
  break;
 case Finisher:
  if(silent_var_name(name,"CalculationBase")){out=0;return true;}
  if(silent_var_name(name,"CalculationExtra")){out=1;return true;}
  break;
 case Flechettes:
  if(silent_var_name(name,"CalculationBase")){out=0;return true;}
  if(silent_var_name(name,"CalculationExtra")){out=1;return true;}
  break;
 case Sidestep:
  if(silent_var_name(name,"Energy")){out=(c.upgraded?2:1);return true;}
  break;
 case Footwork:
  if(silent_var_name(name,"DexterityPower")){out=(c.upgraded?3:2);return true;}
  break;
 case Haze:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?6:4);return true;}
  if(silent_var_name(name,"WeakPower")){out=(c.upgraded?2:1);return true;}
  break;
 case HiddenDaggers:
  if(silent_var_name(name,"Cards")){out=2;return true;}
  if(silent_var_name(name,"Shivs")){out=2;return true;}
  break;
 case KnifeTrap:
  if(silent_var_name(name,"CalculationBase")){out=0;return true;}
  if(silent_var_name(name,"CalculationExtra")){out=1;return true;}
  break;
 case LeadingStrike:
  if(silent_var_name(name,"Shivs")){out=2;return true;}
  break;
 case LegSweep:
  if(silent_var_name(name,"WeakPower")){out=(c.upgraded?3:2);return true;}
  break;
 case MementoMori:
  if(silent_var_name(name,"CalculationBase")){out=(c.upgraded?11:9);return true;}
  if(silent_var_name(name,"ExtraDamage")){out=(c.upgraded?5:4);return true;}
  break;
 case Mirage:
  if(silent_var_name(name,"CalculationBase")){out=0;return true;}
  if(silent_var_name(name,"CalculationExtra")){out=1;return true;}
  break;
 case Murder:
  if(silent_var_name(name,"CalculationBase")){out=1;return true;}
  if(silent_var_name(name,"ExtraDamage")){out=1;return true;}
  break;
 case Neutralize:
  if(silent_var_name(name,"WeakPower")){out=(c.upgraded?2:1);return true;}
  break;
 case NoxiousFumes:
  if(silent_var_name(name,"PoisonPerTurn")){out=(c.upgraded?3:2);return true;}
  break;
 case Outbreak:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?12:9);return true;}
  break;
 case PhantomBlades:
  if(silent_var_name(name,"PhantomBladesPower")){out=(c.upgraded?12:9);return true;}
  break;
 case PiercingWail:
  if(silent_var_name(name,"StrengthLoss")){out=(c.upgraded?8:6);return true;}
  break;
 case PoisonedStab:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?4:3);return true;}
  break;
 case PreciseCut:
  if(silent_var_name(name,"CalculationBase")){out=(c.upgraded?16:13);return true;}
  if(silent_var_name(name,"ExtraDamage")){out=2;return true;}
  break;
 case Prepared:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?2:1);return true;}
  break;
 case Reflex:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?3:2);return true;}
  break;
 case Ricochet:
  if(silent_var_name(name,"Repeat")){out=(c.upgraded?5:4);return true;}
  break;
 case SerpentForm:
  if(silent_var_name(name,"SerpentFormPower")){out=(c.upgraded?6:4);return true;}
  break;
 case ShadowStep:
  if(silent_var_name(name,"Cards")){out=3;return true;}
  break;
 case Shadowmeld:
  if(silent_var_name(name,"Power")){out=1;return true;}
  break;
 case Snakebite:
  if(silent_var_name(name,"PoisonPower")){out=(c.upgraded?10:7);return true;}
  break;
 case Speedster:
  if(silent_var_name(name,"SpeedsterPower")){out=2;return true;}
  break;
 case Strangle:
  if(silent_var_name(name,"StranglePower")){out=(c.upgraded?3:2);return true;}
  break;
 case SuckerPunch:
  if(silent_var_name(name,"WeakPower")){out=(c.upgraded?2:1);return true;}
  break;
 case Suppress:
  if(silent_var_name(name,"WeakPower")){out=(c.upgraded?5:3);return true;}
  break;
 case Tactician:
  if(silent_var_name(name,"Energy")){out=(c.upgraded?2:1);return true;}
  break;
 case UpMySleeve:
  if(silent_var_name(name,"Cards")){out=(c.upgraded?4:3);return true;}
  break;
 case WraithForm:
  if(silent_var_name(name,"IntangiblePower")){out=(c.upgraded?3:2);return true;}
  if(silent_var_name(name,"WraithFormPower")){out=1;return true;}
  break;
 default:break;
 }
 return false;
}
