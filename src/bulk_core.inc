// Bounded, serializable continuation operations shared by cards, potions and
// relics. No C++ call stack or presentation clock is required to resume a choice.
int bulk_power(const State&s,BulkPower p){return s.bulk.powers[(unsigned)p];}
void bulk_power_add(State&s,BulkPower p,int n){
 auto&v=s.bulk.powers[(unsigned)p];v=(int16_t)max(0,min(9999,v+n));
}
void bulk_enqueue(State&s,BulkOp op,int a,int b,bool first){
 if(a< -9999||a>9999||b< -9999||b>9999){fail(s);return;}
 if(enqueue(s,EffectKind::Bulk,a,(int)op,first))s.effects[first?0:s.effect_count-1].aux=(int16_t)b;
}
int cost(const State&s,int id){
 if(id<0||id>=s.card_count)return 99;
 if(s.bulk.turn_cost[id])return s.bulk.turn_cost[id]-1;
 if(s.bulk.combat_cost[id])return s.bulk.combat_cost[id]-1;
 return cost(s.cards[id]);
}
bool card_exhausts(Card c){
 if(c.kind>=CardKinds)return false;
 const auto&d=definitions[c.kind];return d.exhausts||((c.upgraded?d.upgraded_keywords:d.keywords)&1);
}
bool card_defend_tag(unsigned k){return k==Defend||k==DefendSilent||k==UltimateDefend;}
bool card_can_generate(unsigned k){return k<CardKinds&&!card_generated(k)&&k!=Alchemize;}
static void bulk_flash(State&s,unsigned id){s.relic_flash|=id;++s.relic_revision;}
static void bulk_gain_block(State&s,int n){n=max(0,min(n,9999-s.block));s.block+=(int16_t)n;record(s,LogKind::Block,n);}
static bool bulk_matching(const State&s,unsigned kind,int filter){
 if(!card_can_generate(kind))return false;
 bool own=card_in_pool(kind,s.character),neutral=card_colorless(kind);
 if(filter==4)return neutral;
 if(filter==5)return !neutral&&!own&&definitions[kind].type==CardType::Attack;
 if(!(own||neutral))return false;
 if(filter==6)return cost({(uint8_t)kind,0})==0;
 return !filter||(int)definitions[kind].type==filter-1;
}
static void bulk_make_generated(State&s,int filter,int flags){
 uint8_t pool[CardKinds];unsigned n=0;
 for(unsigned k=0;k<CardKinds;k++)if(bulk_matching(s,k,filter))pool[n++]=(uint8_t)k;
 if(!n)return;
 int id=make_card(s,{pool[random_below(s.rng_cards,n)],(uint8_t)((flags&2)!=0)});
 if(id<0)return;
 put(s,s.hand.count<MaxHand?s.hand:s.discard,id,s.hand.count<MaxHand?Zone::Hand:Zone::Discard);
 if(flags&1)s.bulk.turn_cost[id]=1;
 record(s,LogKind::Create,1,s.cards[id].kind);
}
static void bulk_clear_choice(State&s){
 s.bulk.choice=BulkChoice::None;s.bulk.choices.count=0;
 s.bulk.choice_min=s.bulk.choice_max=s.bulk.choice_free=s.bulk.choice_upgraded=0;
}
static void bulk_set_choice(State&s,BulkChoice kind,const Pile&pool,unsigned low,unsigned high,int flags=0){
 if(!alive(s))return;
 high=min((int)high,min(MaxHand,(int)pool.count));low=min((int)low,(int)high);
 if(!pool.count||!high)return;
 s.bulk.choices=pool;s.bulk.choice=kind;s.bulk.choice_min=(uint8_t)low;s.bulk.choice_max=(uint8_t)high;
 if(!s.effect_count)bulk_enqueue(s,BulkOp::ChoiceDone);
 s.bulk.choice_free=flags&1;s.bulk.choice_upgraded=(flags&2)!=0;s.phase=Phase::BulkChoice;
}
Card choice_card(const State&s,int token){
 if(s.phase==Phase::BulkChoice&&s.bulk.choice==BulkChoice::Offer&&token>=MaxCards&&token<MaxCards+3)return s.bulk.offers[token-MaxCards];
 return token>=0&&token<s.card_count?s.cards[token]:Card{Strike,0};
}
static bool bulk_move_from(State&s,int id,Pile&from,Pile&to,Zone zone){
 for(unsigned i=0;i<from.count;i++)if(from.ids[i]==id){remove(from,i);return put(s,to,id,zone);}return false;
}
bool choose_bulk(State&s,const uint16_t*ids,unsigned count){
 if(s.phase!=Phase::BulkChoice||s.bulk.choice==BulkChoice::None||count<s.bulk.choice_min||count>s.bulk.choice_max||(count&&!ids))return false;
 uint16_t chosen[MaxHand];
 for(unsigned i=0;i<count;i++){
  bool found=false;for(unsigned j=0;j<s.bulk.choices.count;j++)if(s.bulk.choices.ids[j]==ids[i])found=true;
  if(!found)return false;for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;chosen[i]=ids[i];
 }
 const auto kind=s.bulk.choice;const bool free=s.bulk.choice_free;bulk_clear_choice(s);s.phase=Phase::Combat;
 for(unsigned i=0;i<count;i++){
  int id=chosen[i];
  if(kind==BulkChoice::Offer){
   Card c=s.bulk.offers[id-MaxCards];id=make_card(s,c);if(id<0)break;
   put(s,s.hand.count<MaxHand?s.hand:s.discard,id,s.hand.count<MaxHand?Zone::Hand:Zone::Discard);
   if(free)s.bulk.turn_cost[id]=1;
  }else if(kind==BulkChoice::DrawToHand||kind==BulkChoice::DiscardToHand){
   if(s.hand.count>=MaxHand)break;
   if(bulk_move_from(s,id,kind==BulkChoice::DrawToHand?s.draw:s.discard,s.hand,Zone::Hand)&&free)s.bulk.turn_cost[id]=1;
  }else if(kind==BulkChoice::HandToDraw)bulk_move_from(s,id,s.hand,s.draw,Zone::Draw);
  else if(kind==BulkChoice::FreeCombat)s.bulk.combat_cost[id]=1;
  else if(kind==BulkChoice::DiscardRedraw){
   if(bulk_move_from(s,id,s.hand,s.discard,Zone::Discard)){record(s,LogKind::Discard,s.cards[id].kind);bulk_after_discard(s,id);}
  }else if(kind==BulkChoice::ExhaustHand){
   // Snapshot the selected instances. Exhaust/draw hooks may grow the hand.
   // Commands are scheduled in the original selection order below.
  }else if(kind==BulkChoice::TransformHand){
   uint8_t pool[CardKinds];unsigned n=0;
   for(unsigned k=0;k<CardKinds;k++)if(k!=s.cards[id].kind&&bulk_matching(s,k,0))pool[n++]=(uint8_t)k;
   if(n){s.cards[id]={pool[random_below(s.rng_cards,n)],0};s.bulk.turn_cost[id]=s.bulk.combat_cost[id]=s.bulk.replay[id]=0;s.bulk.return_turn[id]=0;}
  }
 }
 if(kind==BulkChoice::ExhaustHand)for(unsigned i=count;i>0;i--)enqueue(s,EffectKind::ExhaustCard,chosen[i-1],0,true);
 if(kind==BulkChoice::DiscardRedraw)bulk_enqueue(s,BulkOp::Draw,(int)count,0,true);
 resolve(s);++s.revision;return true;
}
static void bulk_offer(State&s,int filter,int flags){
 if(s.hand.count>=MaxHand)return;
 uint8_t pool[CardKinds];unsigned n=0;for(unsigned k=0;k<CardKinds;k++)if(bulk_matching(s,k,filter))pool[n++]=(uint8_t)k;
 Pile choices{};
 for(unsigned i=0;i<3&&n;i++){
  unsigned j=random_below(s.rng_cards,n);s.bulk.offers[i]={pool[j],(uint8_t)((flags&2)!=0)};
  pool[j]=pool[--n];choices.ids[choices.count++]=(uint16_t)(MaxCards+i);
 }
 bulk_set_choice(s,BulkChoice::Offer,choices,1,1,flags);
}
static void bulk_draw_choice(State&s,int filter,int sample=0,int count=1){
 if(s.hand.count>=MaxHand)return;
 Pile eligible{};
 for(unsigned i=0;i<s.draw.count;i++){
  int id=s.draw.ids[i];if(!filter||(int)definitions[s.cards[id].kind].type==filter-1)eligible.ids[eligible.count++]=(uint16_t)id;
 }
 if(sample&&eligible.count>(unsigned)sample){
  for(unsigned i=0;i<(unsigned)sample;i++){unsigned j=i+random_below(s.rng_cards,eligible.count-i);auto t=eligible.ids[i];eligible.ids[i]=eligible.ids[j];eligible.ids[j]=t;}
  eligible.count=(uint16_t)sample;
 }
 bulk_set_choice(s,BulkChoice::DrawToHand,eligible,count,min(count,MaxHand-s.hand.count));
}
static void bulk_restore_frame(State&s){
 if(!s.bulk.depth){s.playing=-1;s.bulk.repeat_remaining=0;return;}
 const auto f=s.bulk.frames[--s.bulk.depth];s.bulk.frames[s.bulk.depth]=AutoFrame{};
 s.playing=f.playing;s.bulk.spent=f.spent;s.bulk.x_value=f.x_value;s.attack_vigor=f.vigor;
 s.bulk.target=f.target;s.bulk.last_damage=f.last_damage;s.bulk.repeat_remaining=f.repeat_remaining;
 s.double_attack=f.double_attack;s.bulk.triple_attack=f.triple_attack;s.bulk.debuff_double=f.debuff_double;s.bulk.nostalgia_return=f.nostalgia_return;
}
static void bulk_finish_card(State&s){
 if(s.playing<0)return;
 int id=s.playing;Card card=s.cards[id];const auto type=definitions[card.kind].type;
 // Count after a complete execution, never per damage hit or open choice.
 s.bulk.cards_finished=(uint16_t)min(9999,s.bulk.cards_finished+1);
 s.bulk.turn_cards=(uint16_t)min(9999,s.bulk.turn_cards+1);
 if(type==CardType::Attack)s.attacked_this_turn=1;
 // Schedule result movement after any newly prepended relic commands.
 bulk_enqueue(s,BulkOp::AfterPlay,id,0,true);
 relic_card_played(s,card);
 if(type==CardType::Attack&&bulk_power(s,BulkPower::Calamity))for(int i=0;i<bulk_power(s,BulkPower::Calamity);i++)bulk_enqueue(s,BulkOp::RandomGenerated,1,0,true);
 for(int i=MaxPowerInstances-1;i>=0;i--)if(s.bulk.panache_instances[i]){
  if(s.bulk.panache_skip[i]){s.bulk.panache_skip[i]=0;continue;}
  if(++s.bulk.panache_counts[i]>=5){s.bulk.panache_counts[i]=0;bulk_enqueue(s,BulkOp::UnpoweredAll,s.bulk.panache_instances[i],0,true);}
 }
}
static void bulk_after_play(State&s,int id){
 if(id!=s.playing||id<0||id>=s.card_count){fail(s);return;}
 if(s.bulk.repeat_remaining&&alive(s)){
  --s.bulk.repeat_remaining;relic_attack_end(s);s.bulk.triple_attack=0;if(definitions[s.cards[id].kind].type!=CardType::Power)s.bulk.nonpower_started=(uint16_t)min(9999,s.bulk.nonpower_started+1);bulk_queue_current(s);return;
 }
 Card card=s.cards[id];bool returning=false;
 if(!card_exhausts(card)&&definitions[card.kind].type!=CardType::Power&&s.bulk.nostalgia_return){
  ++s.bulk.nostalgia_used;returning=true;
 }
 // Restore the caller before scheduling exhaustion hooks, so generated/autoplay
 // cards cannot overwrite the identity of the card just moved out of limbo.
 if(definitions[card.kind].type==CardType::Power)put(s,s.powers,id,Zone::Power);
 else if(returning)put(s,s.draw,id,Zone::Draw);
 else if(!card_exhausts(card))put(s,s.discard,id,Zone::Discard);
 const bool exhausts=card_exhausts(card)&&definitions[card.kind].type!=CardType::Power;
 relic_attack_end(s);bulk_restore_frame(s);
 if(exhausts)exhaust_id(s,id);
 if(s.relics&UnceasingTop)bulk_enqueue(s,BulkOp::UnceasingTop,0,0,true);
}
static void bulk_queue_current(State&s){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 const Card c=s.cards[s.playing];const Definition&d=definitions[c.kind];int n=s.effect_count;
 s.bulk.last_damage=0;
 if((s.relics&IntimidatingHelmet)&&s.bulk.spent>=2){bulk_gain_block(s,4);bulk_flash(s,IntimidatingHelmet);}
 if(c.kind<Alchemize){bulk_queue_legacy(s,c,s.bulk.target,s.bulk.x_value);}
 else{
  record(s,LogKind::Play,c.kind,c.upgraded);
  if(d.type==CardType::Attack)relic_attack_begin(s);
  int hit=card_base_damage(s,c),block=d.block+(c.upgraded?d.block_up:0),power=d.power+(c.upgraded?d.power_up:0),draw=d.draw+(c.upgraded?d.draw_up:0);
  auto op=[&](BulkOp o,int a=0,int b=0){bulk_enqueue(s,o,a,b);};
  auto pw=[&](BulkPower p,int a){op(BulkOp::GrantPower,(int)p,a);};
  if(block)enqueue(s,EffectKind::Block,bulk_card_block(s,c,block));
  if(d.type==CardType::Attack){
   if(c.kind==DramaticEntrance){for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);}
   else if(c.kind==Volley)op(BulkOp::RandomHits,hit,s.bulk.x_value);
   else{
    if(c.kind==Rend&&s.bulk.target>=0){const auto&e=s.enemies[s.bulk.target];int count=(e.strength<0)+(e.weak>0)+(e.vulnerable>0)+(e.poison>0)+(s.bulk.enemy_beetle[s.bulk.target]>0)+(s.bulk.enemy_demise[s.bulk.target]>0);hit+=count*power;}
    enqueue(s,EffectKind::Damage,hit,s.bulk.target);
   }
  }
  switch(c.kind){
   case Alchemize:op(BulkOp::GrantPotion);break;
   case Anointed:op(BulkOp::MoveRare);break;
   case Automation:pw(BulkPower::Automation,1);break;
   case BeatDown:for(int i=0;i<draw;i++)op(BulkOp::AutoplayRandomDiscard,1);break;
   case Bolas:case ThrummingHatchet:op(BulkOp::ReturnNextTurn,s.playing);break;
   case Calamity:pw(BulkPower::Calamity,1);break;
   case Catastrophe:for(int i=0;i<draw;i++)op(BulkOp::AutoplayRandomDraw);break;
   case DarkShackles:op(BulkOp::TemporaryEnemyStrength,s.bulk.target,power*(s.bulk.debuff_double?2:1));break;
   case Discovery:op(BulkOp::Offer,0,1);break;
   case DramaticEntrance:case GoldAxe:case MindBlast:case Rend:case UltimateDefend:case UltimateStrike:case Volley:break;
   case Entropy:pw(BulkPower::Entropy,1);break;
   case Equilibrium:case Salvo:pw(BulkPower::RetainHand,1);break;
   case EternalArmor:pw(BulkPower::Plating,power);break;
   case Fasten:pw(BulkPower::Fasten,power);break;
   case Fisticuffs:op(BulkOp::DamageAsBlock);break;
   case HiddenGem:op(BulkOp::HiddenReplay,power);break;
   case JackOfAllTrades:for(int i=0;i<draw;i++)op(BulkOp::RandomGenerated,4);break;
   case Jackpot:for(int i=0;i<draw;i++)op(BulkOp::RandomGenerated,6,c.upgraded?2:0);break;
   case Mayhem:pw(BulkPower::Mayhem,1);break;
   case Nostalgia:pw(BulkPower::Nostalgia,1);break;
   case Omnislice:op(BulkOp::CopyDamageOthers,s.bulk.target);break;
   case Panache:pw(BulkPower::Panache,power);break;
   case PrepTime:pw(BulkPower::PrepTime,power);break;
   case Production:enqueue(s,EffectKind::Energy,power);break;
   case Prolong:op(BulkOp::ProlongBlock);break;
   case Prowess:op(BulkOp::Strength,power);op(BulkOp::Dexterity,power);break;
   case Restlessness:op(BulkOp::Restless,draw,power);break;
   case RollingBoulder:pw(BulkPower::RollingBoulder,power);s.bulk.boulder_increment=(int16_t)min(9999,s.bulk.boulder_increment+5);break;
   case SecretTechnique:op(BulkOp::ChooseDraw,2);break;
   case SecretWeapon:op(BulkOp::ChooseDraw,1);break;
   case SeekerStrike:op(BulkOp::ChooseDraw,0,draw);break;
   case Splash:op(BulkOp::Offer,5,c.upgraded?3:1);break;
   case Stratagem:pw(BulkPower::Stratagem,1);break;
   case TheBomb:op(BulkOp::Bomb,power,3);break;
   case TheGambit:pw(BulkPower::Gambit,1);break;
   case ThinkingAhead:op(BulkOp::Draw,draw);op(BulkOp::ChooseHandTop);break;
   default:fail(s);return;
  }
  enqueue(s,EffectKind::Finish);
 }
 if(s.phase==Phase::Error)return;
 // Newly authored commands run before the saved caller continuation.
 int added=s.effect_count-n;
 if(n&&added){Effect tmp[MaxEffects];for(int i=0;i<added;i++)tmp[i]=s.effects[n+i];for(int i=n-1;i>=0;i--)s.effects[i+added]=s.effects[i];for(int i=0;i<added;i++)s.effects[i]=tmp[i];}
}
static bool bulk_start_instance(State&s,int id,int target,bool automatic){
 if(id<0||id>=s.card_count||!alive(s)||s.hp<=0)return false;
 Card card=s.cards[id];if(card.kind>=CardKinds)return false;
 const auto&d=definitions[card.kind];
 if(d.targeted&&(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)){
  if(!automatic)return false;int choices[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)choices[n++]=i;
  if(!n)return false;target=choices[random_below(s.rng_cards,n)];
 }
 int priced=cost(s,id),spent=automatic?0:priced<0?s.energy:priced;
 if(spent>s.energy)return false;
 Zone zone=s.zones[id];Pile*source=zone==Zone::Hand?&s.hand:zone==Zone::Draw?&s.draw:zone==Zone::Discard?&s.discard:nullptr;
 if(!source)return false;
 if(s.bulk.depth==MaxAutoDepth){fail(s);return false;}
 if(automatic){
  s.bulk.frames[s.bulk.depth++]={s.playing,s.bulk.spent,s.bulk.x_value,s.attack_vigor,s.bulk.target,s.bulk.last_damage,s.bulk.repeat_remaining,s.double_attack,s.bulk.triple_attack,s.bulk.debuff_double,s.bulk.nostalgia_return};
 }
 int idx=-1;for(unsigned i=0;i<source->count;i++)if(source->ids[i]==id){idx=i;break;}if(idx<0){if(automatic)--s.bulk.depth;return false;}
 int x=cost(card)<0?s.energy:spent;if((s.relics&ChemicalX)&&cost(card)<0)x+=2;
 remove(*source,idx);s.zones[id]=Zone::Limbo;s.playing=(int16_t)id;s.energy-=(int16_t)spent;
 s.bulk.spent=(int16_t)spent;s.bulk.x_value=(int16_t)min(9999,x);s.bulk.target=(int16_t)target;
 s.attack_vigor=0;s.double_attack=0;s.bulk.triple_attack=0;s.bulk.debuff_double=0;
 s.bulk.repeat_remaining=s.bulk.replay[id];
 if(bulk_power(s,BulkPower::Duplicator)){++s.bulk.repeat_remaining;bulk_power_add(s,BulkPower::Duplicator,-1);}
 s.bulk.nostalgia_return=d.type!=CardType::Power&&s.bulk.nonpower_started<bulk_power(s,BulkPower::Nostalgia);
 if(d.type!=CardType::Power)s.bulk.nonpower_started=(uint16_t)min(9999,s.bulk.nonpower_started+1);
 bool debuff=card.kind==Bash||card.kind==Thunderclap||card.kind==Uppercut||card.kind==Neutralize||card.kind==DeadlyPoison||card.kind==PoisonedStab||card.kind==LegSweep||card.kind==SuckerPunch||card.kind==BouncingFlask||card.kind==Shockwave||card.kind==DarkShackles;
 unsigned lamp=RelicOrdinal(UnsettlingLamp);
 if(debuff&&(s.relics&UnsettlingLamp)&&!s.bulk.combat_flags[lamp]){s.bulk.combat_flags[lamp]=1;s.bulk.debuff_double=1;bulk_flash(s,UnsettlingLamp);}
 bulk_queue_current(s);return true;
}
static void bulk_draw_one(State&s,int remaining){
 s.bulk.last_drawn=-1;if(remaining<=0||s.no_draw||s.hand.count>=MaxHand)return;
 if(!s.draw.count){
  if(!s.discard.count)return;
  // Shuffle hooks (including a choice) resolve before the pending draw.
  bulk_enqueue(s,BulkOp::DrawOne,remaining,0,true);shuffle(s);return;
 }
 if(remaining>1)bulk_enqueue(s,BulkOp::DrawOne,remaining-1,0,true);
 int id=remove(s.draw,s.draw.count-1);put(s,s.hand,id,Zone::Hand);s.bulk.last_drawn=(int16_t)id;record(s,LogKind::Draw,1);bulk_after_draw(s,id);
}
void bulk_after_draw(State&s,int){
 s.bulk.cards_drawn=(uint16_t)min(9999,s.bulk.cards_drawn+1);
 for(int i=MaxPowerInstances-1;i>=0;i--)if(s.bulk.automation_instances[i]&&++s.bulk.automation_counts[i]>=10){
  s.bulk.automation_counts[i]=0;enqueue(s,EffectKind::Energy,s.bulk.automation_instances[i],0,true);
 }
}
void bulk_after_shuffle(State&s){
 if(s.bulk.setup)return;
 if(s.relics&TheAbacus){bulk_gain_block(s,6);bulk_flash(s,TheAbacus);}
 if(bulk_power(s,BulkPower::Stratagem))bulk_enqueue(s,BulkOp::ChooseDraw,0,-bulk_power(s,BulkPower::Stratagem),true);
}
void bulk_after_exhaust(State&s,int id){
 if(s.relics&CharonsAshes){bulk_enqueue(s,BulkOp::UnpoweredAll,3,0,true);bulk_flash(s,CharonsAshes);}
 if(s.relics&JossPaper){auto&n=s.relic_counters[RelicOrdinal(JossPaper)];if(++n==5){n=0;bulk_enqueue(s,BulkOp::Draw,1,0,true);bulk_flash(s,JossPaper);}}
 unsigned index=RelicOrdinal(BurningSticks);
 if((s.relics&BurningSticks)&&definitions[s.cards[id].kind].type==CardType::Skill&&!s.bulk.combat_flags[index]){
  s.bulk.combat_flags[index]=1;int copy=make_card(s,s.cards[id]);if(copy>=0)put(s,s.hand.count<MaxHand?s.hand:s.discard,copy,s.hand.count<MaxHand?Zone::Hand:Zone::Discard);bulk_flash(s,BurningSticks);
 }
}
void bulk_after_discard(State&s,int){
 if(s.bulk.enemy_turn)return;
 if(s.relics&ToughBandages){bulk_gain_block(s,3);bulk_flash(s,ToughBandages);}
 if(s.relics&Tingsha){int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[n++]=i;if(n)damage_unpowered(s,ids[random_below(s.rng_cards,n)],3);bulk_flash(s,Tingsha);}
}
void bulk_enemy_died(State&s,int target){
 if(target<0||target>=s.enemy_count||s.enemies[target].hp||s.bulk.dead_mask&(1u<<target))return;
 s.bulk.dead_mask|=(uint8_t)(1u<<target);
 if(s.relics&GremlinHorn){s.energy=(int16_t)min(999,s.energy+1);bulk_enqueue(s,BulkOp::Draw,1,0,true);bulk_flash(s,GremlinHorn);}
}
void bulk_resolve(State&s){resolve(s);if(!resolve_active&&s.phase==Phase::Combat&&s.playing<0&&!s.effect_count&&!s.bulk.depth)finish_combat_if_won(s);}
