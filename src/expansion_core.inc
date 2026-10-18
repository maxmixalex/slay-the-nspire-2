// A serialized continuation queue allows potions, automatic card plays and
// ordinary card plays to suspend in the same instance-based selection UI.
static bool start_instance(State&,int,int,bool);
static void queue_current(State&);
static void extension_effect(State&,Effect);
static void finish_extended_card(State&);
static void draw_hook(State&);
static void shuffle_hook(State&);
static void clear_extended_choice(State&);
static void death_cleanup(State&);
static void queue_legacy(State&,Card,int,int);
int power_value(const State&s,XPower p){return s.expansion.powers[(unsigned)p];}
void gain_power(State&s,XPower p,int n){auto&v=s.expansion.powers[(unsigned)p];v=(int16_t)max(0,min(9999,v+n));}
void queue_extension(State&s,XOp op,int a,int b,bool first){
 if(a< -9999||a>9999||b< -9999||b>9999){fail(s);return;}
 if(enqueue(s,EffectKind::Extended,a,(int)op,first))s.effects[first?0:s.effect_count-1].aux=(int16_t)b;
}
int cost(const State&s,int id){
 if(id<0||id>=s.card_count)return 99;
 if(s.expansion.turn_cost[id])return s.expansion.turn_cost[id]-1;
 if(s.expansion.combat_cost[id])return s.expansion.combat_cost[id]-1;
 return cost(s.cards[id]);
}
bool card_exhausts(Card c){if(c.kind>=CardKinds)return false;const auto&d=definitions[c.kind];return d.exhausts||((c.upgraded?d.upgraded_keywords:d.keywords)&1);}
bool card_can_generate(unsigned k){return k<CardKinds&&!card_generated(k)&&k!=Alchemize;}
static void gain_unpowered_block(State&s,int n){n=max(0,min(n,9999-s.block));s.block+=(int16_t)n;record(s,LogKind::Block,n);}
static int powered_block(const State&s,Card c,int n){
 if(s.no_block||n<=0)return 0;
 return max(0,min(9999,n+s.dexterity+((c.kind==Defend||c.kind==DefendSilent||c.kind==UltimateDefend)?power_value(s,XPower::Fasten):0)));
}
static bool matches_generated(const State&s,unsigned k,int filter){
 if(!card_can_generate(k))return false;
 bool own=card_in_pool(k,s.character),neutral=card_colorless(k);
 if(filter==4)return neutral;
 if(filter==5)return !neutral&&!own&&definitions[k].type==CardType::Attack;
 if(!(own||neutral))return false;
 if(filter==6)return cost({(uint8_t)k,0})==0;
 return !filter||(int)definitions[k].type==filter-1;
}
static int create_to_hand(State&s,Card c){
 int id=make_card(s,c);if(id<0)return -1;
 if(c.kind==SovereignBlade)s.expansion.blade_damage[id]=10;
 bool room=s.hand.count<MaxHand;put(s,room?s.hand:s.discard,id,room?Zone::Hand:Zone::Discard);
 record(s,LogKind::Create,1,c.kind);return id;
}
static void generate_card(State&s,int filter,int flags){
 uint8_t pool[CardKinds];unsigned n=0;
 for(unsigned k=0;k<CardKinds;k++)if(matches_generated(s,k,filter))pool[n++]=(uint8_t)k;
 if(!n)return;
 int id=create_to_hand(s,{pool[random_below(s.rng_cards,n)],(uint8_t)((flags&2)!=0)});
 if(id>=0&&(flags&1))s.expansion.turn_cost[id]=1;
}
static void clear_extended_choice(State&s){auto&x=s.expansion;x.choice=XChoice::None;x.choices.count=0;x.choice_min=x.choice_max=x.choice_free=0;}
static void set_extended_choice(State&s,XChoice kind,const Pile&pool,int lo,int hi,int flags=0){
 if(!alive(s)||!pool.count)return;
 hi=min(hi,min(MaxHand,(int)pool.count));lo=min(lo,hi);if(hi<=0)return;
 auto&x=s.expansion;x.choices=pool;x.choice=kind;x.choice_min=(uint8_t)lo;x.choice_max=(uint8_t)hi;x.choice_free=flags&1;
 if(!s.effect_count)queue_extension(s,XOp::Noop);
 s.phase=Phase::ExtendedChoice;
}
Card choice_card(const State&s,int token){
 if(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Offer&&token>=MaxCards&&token<MaxCards+3)return s.expansion.offers[token-MaxCards];
 return token>=0&&token<s.card_count?s.cards[token]:Card{Strike,0};
}
static bool move_instance(State&s,int id,Pile&from,Pile&to,Zone z){
 for(unsigned i=0;i<from.count;i++)if(from.ids[i]==id){remove(from,i);return put(s,to,id,z);}return false;
}
bool choose_extended(State&s,const uint16_t*ids,unsigned n){
 auto&x=s.expansion;
 if(s.phase!=Phase::ExtendedChoice||x.choice==XChoice::None||n>MaxHand||n<x.choice_min||n>x.choice_max||(n&&!ids))return false;
 // Refuse a continuation that cannot be queued before changing the choice or
 // any pile. Optional exhaust uses the same atomic boundary as legacy Purity.
 unsigned queued=x.choice==XChoice::Exhaust?n:x.choice==XChoice::DiscardRedraw?1:0;
 if(s.effect_count+queued>MaxEffects)return false;
 uint16_t selected[MaxHand];
 for(unsigned i=0;i<n;i++){
  bool found=false;for(unsigned j=0;j<x.choices.count;j++)if(x.choices.ids[j]==ids[i])found=true;
  if(!found)return false;for(unsigned j=0;j<i;j++)if(ids[j]==ids[i])return false;selected[i]=ids[i];
 }
 XChoice kind=x.choice;bool free=x.choice_free;clear_extended_choice(s);s.phase=Phase::Combat;
 for(unsigned i=0;i<n;i++){
  int id=selected[i];
  if(kind==XChoice::Offer){id=create_to_hand(s,x.offers[id-MaxCards]);if(id<0)break;if(free)x.turn_cost[id]=1;}
  else if(kind==XChoice::Draw||kind==XChoice::Discard){
   if(s.hand.count>=MaxHand)break;
   if(move_instance(s,id,kind==XChoice::Draw?s.draw:s.discard,s.hand,Zone::Hand)&&free)x.turn_cost[id]=1;
  }else if(kind==XChoice::Top)move_instance(s,id,s.hand,s.draw,Zone::Draw);
  else if(kind==XChoice::Free){x.combat_cost[id]=1;x.turn_cost[id]=0;}
  else if(kind==XChoice::DiscardRedraw){if(move_instance(s,id,s.hand,s.discard,Zone::Discard))record(s,LogKind::Discard,s.cards[id].kind);}
  else if(kind==XChoice::Transform){
   uint8_t pool[CardKinds];unsigned count=0;for(unsigned k=0;k<CardKinds;k++)if(k!=s.cards[id].kind&&matches_generated(s,k,0))pool[count++]=(uint8_t)k;
   if(count){s.cards[id]={pool[random_below(s.rng_cards,count)],0};x.turn_cost[id]=x.combat_cost[id]=x.replay[id]=0;x.return_turn[id]=0;x.blade_damage[id]=0;}
  }
 }
 if(kind==XChoice::Exhaust)for(unsigned i=n;i>0;i--)enqueue(s,EffectKind::ExhaustCard,selected[i-1],0,true);
 if(kind==XChoice::DiscardRedraw)queue_extension(s,XOp::Draw,(int)n,0,true);
 resolve_pending(s);++s.revision;return true;
}
static void offer_cards(State&s,int filter,int flags){
 uint8_t pool[CardKinds];unsigned n=0;for(unsigned k=0;k<CardKinds;k++)if(matches_generated(s,k,filter))pool[n++]=(uint8_t)k;
 Pile choices{};
 for(unsigned i=0;i<3&&n;i++){unsigned j=random_below(s.rng_cards,n);s.expansion.offers[i]={pool[j],(uint8_t)((flags&2)!=0)};pool[j]=pool[--n];choices.ids[choices.count++]=(uint16_t)(MaxCards+i);}
 set_extended_choice(s,XChoice::Offer,choices,1,1,flags);
}
static void choose_draw(State&s,int filter,int sample=0,int count=1){
 if(s.hand.count>=MaxHand)return;
 Pile eligible{};
 for(unsigned i=0;i<s.draw.count;i++){int id=s.draw.ids[i];if(!filter||(int)definitions[s.cards[id].kind].type==filter-1)eligible.ids[eligible.count++]=(uint16_t)id;}
 if(sample&&eligible.count>(unsigned)sample){
  for(unsigned i=0;i<(unsigned)sample;i++){unsigned j=i+random_below(s.rng_cards,eligible.count-i);auto t=eligible.ids[i];eligible.ids[i]=eligible.ids[j];eligible.ids[j]=t;}
  eligible.count=(uint16_t)sample;
 }
 set_extended_choice(s,XChoice::Draw,eligible,count,min(count,MaxHand-s.hand.count));
}
static void restore_frame(State&s){
 auto&x=s.expansion;
 if(!x.depth){s.playing=-1;x.remaining=0;x.triple=x.returning=0;return;}
 XFrame f=x.frames[--x.depth];x.frames[x.depth]=XFrame{};
 s.playing=f.playing;x.target=f.target;x.x=f.x;x.last_damage=f.last_damage;s.attack_vigor=f.vigor;x.remaining=f.remaining;s.double_attack=f.double_attack;x.triple=f.triple;x.returning=f.returning;
}
static void finish_extended_card(State&s){
 if(s.playing<0)return;auto&x=s.expansion;Card card=s.cards[s.playing];auto type=definitions[card.kind].type;
 x.cards_finished=(uint16_t)min(9999,x.cards_finished+1);
 queue_extension(s,XOp::AfterPlay,s.playing,0,true);
 relic_card_played(s,card);if(type==CardType::Attack)s.attacked_this_turn=1;
 if(type==CardType::Attack)for(int i=0;i<power_value(s,XPower::Calamity);i++)queue_extension(s,XOp::Generate,1,0,true);
 for(int i=MaxPowerInstances-1;i>=0;i--)if(x.panache[i]){
  if(x.panache_skip[i]){x.panache_skip[i]=0;continue;}
  if(++x.panache_count[i]==5){x.panache_count[i]=0;queue_extension(s,XOp::AllDamage,x.panache[i],0,true);}
 }
}
static void after_play(State&s,int id){
 if(id!=s.playing||id<0||id>=s.card_count){fail(s);return;}auto&x=s.expansion;
 if(x.remaining&&alive(s)){--x.remaining;relic_attack_end(s);x.triple=0;queue_current(s);return;}
 Card card=s.cards[id];auto type=definitions[card.kind].type;bool ex=card_exhausts(card)&&type!=CardType::Power;
 if(type==CardType::Power)put(s,s.powers,id,Zone::Power);
 else if(!ex)put(s,x.returning?s.draw:s.discard,id,x.returning?Zone::Draw:Zone::Discard);
 relic_attack_end(s);restore_frame(s);
 if(ex)exhaust_id(s,id);
 if(!alive(s))victory(s);
}
static void queue_current(State&s){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 auto&x=s.expansion;Card c=s.cards[s.playing];const auto&d=definitions[c.kind];int old=s.effect_count;
 x.last_damage=0;
 if(d.type!=CardType::Power)x.nonpower_started=(uint16_t)min(9999,x.nonpower_started+1);
 if(c.kind<Alchemize)queue_legacy(s,c,x.target,x.x);
 else{
  record(s,LogKind::Play,c.kind,c.upgraded);
  if(d.type==CardType::Attack)relic_attack_begin(s);
  int hit=card_base_damage(s,c),block=d.block+(c.upgraded?d.block_up:0),power=d.power+(c.upgraded?d.power_up:0),draw=d.draw+(c.upgraded?d.draw_up:0);
  auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};auto pw=[&](XPower p,int n){op(XOp::Power,(int)p,n);};
  if(block)enqueue(s,EffectKind::Block,powered_block(s,c,block));
  if(d.type==CardType::Attack){
   if(c.kind==DramaticEntrance)for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
   else if(c.kind==Volley)op(XOp::RandomHits,hit,x.x);
   else{
    if(c.kind==Rend&&x.target>=0){auto&e=s.enemies[x.target];int debuffs=(e.strength<0)+(e.weak>0)+(e.vulnerable>0)+(e.poison>0)+(x.beetle[x.target]>0)+(x.demise[x.target]>0)+(x.doom[x.target]>0);hit+=debuffs*power;}
    enqueue(s,EffectKind::Damage,hit,x.target);
   }
  }
  switch(c.kind){
  case Alchemize:op(XOp::Potion);break;
  case Anointed:op(XOp::AllRare);break;
  case Automation:pw(XPower::Automation,1);break;
  case BeatDown:for(int i=0;i<draw;i++)op(XOp::AutoDiscard,1);break;
  case Bolas:case ThrummingHatchet:op(XOp::ReturnNext,s.playing);break;
  case Calamity:pw(XPower::Calamity,1);break;
  case Catastrophe:for(int i=0;i<draw;i++)op(XOp::AutoDraw);break;
  case DarkShackles:op(XOp::StrengthLoss,x.target,power);break;
  case Discovery:op(XOp::Offer,0,1);break;
  case Entropy:pw(XPower::Entropy,draw);break;
  case Equilibrium:case Salvo:pw(XPower::Retain,1);break;
  case EternalArmor:pw(XPower::Plating,power);break;
  case Fasten:pw(XPower::Fasten,power);break;
  case Fisticuffs:op(XOp::DamageBlock);break;
  case HiddenGem:op(XOp::EnchantReplay,power);break;
  case JackOfAllTrades:for(int i=0;i<draw;i++)op(XOp::Generate,4);break;
  case Jackpot:for(int i=0;i<draw;i++)op(XOp::Generate,6,c.upgraded?2:0);break;
  case Mayhem:pw(XPower::Mayhem,1);break;
  case Nostalgia:pw(XPower::Nostalgia,1);break;
  case Omnislice:op(XOp::CopyDamage,x.target);break;
  case Panache:pw(XPower::Panache,power);break;
  case PrepTime:pw(XPower::PrepTime,power);break;
  case Production:enqueue(s,EffectKind::Energy,power);break;
  case Prolong:op(XOp::Prolong);break;
  case Prowess:op(XOp::Strength,power);op(XOp::Dexterity,power);break;
  case Restlessness:op(XOp::Restless,draw,power);break;
  case RollingBoulder:pw(XPower::RollingBoulder,power);break;
  case SecretTechnique:op(XOp::ChooseDraw,2);break;
  case SecretWeapon:op(XOp::ChooseDraw,1);break;
  case SeekerStrike:op(XOp::ChooseDraw,0,draw);break;
  case Splash:op(XOp::Offer,5,c.upgraded?3:1);break;
  case Stratagem:pw(XPower::Stratagem,1);break;
  case TheBomb:op(XOp::Bomb,power,3);break;
  case TheGambit:pw(XPower::Gambit,1);break;
  case ThinkingAhead:op(XOp::Draw,draw);op(XOp::ChooseTop);break;
  case Soul:op(XOp::Draw,draw);break;
  case DramaticEntrance:case GoldAxe:case MindBlast:case Rend:case UltimateDefend:case UltimateStrike:case Volley:case SovereignBlade:break;
  default:fail(s);return;
  }
  enqueue(s,EffectKind::Finish);
 }
 if(s.phase==Phase::Error)return;
 // Place the new leaf's commands ahead of the saved caller continuation.
 int added=s.effect_count-old;
 if(old&&added){Effect temp[MaxEffects];for(int i=0;i<added;i++)temp[i]=s.effects[old+i];for(int i=old-1;i>=0;i--)s.effects[i+added]=s.effects[i];for(int i=0;i<added;i++)s.effects[i]=temp[i];}
}
static bool start_instance(State&s,int id,int target,bool automatic){
 if(id<0||id>=s.card_count||!alive(s)||s.hp<=0)return false;
 Card c=s.cards[id];if(c.kind>=CardKinds)return false;const auto&d=definitions[c.kind];auto&x=s.expansion;
 if(d.targeted&&(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)){
  if(!automatic)return false;int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[n++]=i;
  if(!n)return false;target=ids[random_below(s.rng_cards,n)];
 }
 int price=cost(s,id),spent=automatic?0:price<0?s.energy:price;
 if(spent>s.energy)return false;
 Zone z=s.zones[id];Pile*source=z==Zone::Hand?&s.hand:z==Zone::Draw?&s.draw:z==Zone::Discard?&s.discard:nullptr;
 if(!source)return false;int at=-1;for(unsigned i=0;i<source->count;i++)if(source->ids[i]==id){at=i;break;}if(at<0)return false;
 if(automatic&&x.depth==MaxAutoFrames){fail(s);return false;}
 if(automatic)x.frames[x.depth++]={s.playing,x.target,x.x,x.last_damage,s.attack_vigor,x.remaining,s.double_attack,x.triple,x.returning};
 int energy=cost(c)<0?s.energy:spent;
 remove(*source,at);s.zones[id]=Zone::Limbo;s.playing=(int16_t)id;s.energy-=(int16_t)spent;
 x.x=(int16_t)energy;x.target=(int16_t)target;x.last_drawn=-1;x.remaining=x.replay[id];
 s.attack_vigor=0;s.double_attack=0;x.triple=0;
 x.returning=d.type!=CardType::Power&&x.nonpower_started<power_value(s,XPower::Nostalgia);
 if(power_value(s,XPower::Duplicator)){++x.remaining;gain_power(s,XPower::Duplicator,-1);}
 queue_current(s);return true;
}
static void draw_hook(State&s){
 auto&x=s.expansion;
 for(int i=MaxPowerInstances-1;i>=0;i--)if(x.automation[i]&&++x.automation_count[i]==10){x.automation_count[i]=0;enqueue(s,EffectKind::Energy,x.automation[i],0,true);}
}
static void shuffle_hook(State&s){
 int n=power_value(s,XPower::Stratagem);if(n)queue_extension(s,XOp::ChooseDraw,0,-n,true);
}
static void draw_one(State&s,int n){
 s.expansion.last_drawn=-1;
 if(n<=0||s.no_draw||s.hand.count>=MaxHand)return;
 if(!s.draw.count){if(!s.discard.count)return;queue_extension(s,XOp::DrawOne,n,0,true);shuffle(s);return;}
 if(n>1)queue_extension(s,XOp::DrawOne,n-1,0,true);
 int id=remove(s.draw,s.draw.count-1);put(s,s.hand,id,Zone::Hand);s.expansion.last_drawn=(int16_t)id;record(s,LogKind::Draw,1);draw_hook(s);
}
void resolve_pending(State&s){resolve(s);if(!resolving&&s.phase==Phase::Combat&&s.playing<0&&!s.effect_count&&!s.expansion.depth)finish_combat_if_won(s);}
bool lab_potion(State&s,Potion p){
 if(!s.practice||s.phase!=Phase::Combat||s.effect_count||s.playing>=0||!potion_definition(p))return false;
 if(!procure_potion(s,p))return false;++s.revision;return true;
}
bool lab_card(State&s,Card c){
 if(!s.practice||s.phase!=Phase::Combat||s.effect_count||s.playing>=0||c.kind>=CardKinds||c.upgraded>1||s.card_count>=MaxCards)return false;
 create_to_hand(s,c);++s.revision;return true;
}
