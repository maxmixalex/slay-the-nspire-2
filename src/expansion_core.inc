// A serialized continuation queue allows potions, automatic card plays and
// ordinary card plays to suspend in the same instance-based selection UI.
static void queue_current(State&);
static void extension_effect(State&,Effect);
static void finish_extended_card(State&);
static void draw_hook(State&);
static void shuffle_hook(State&);
static void clear_extended_choice(State&);
static void death_cleanup(State&);
static void queue_legacy(State&,Card,int,int);
int power_value(const State&s,XPower p){return s.expansion.powers[(unsigned)p];}
void gain_power(State&s,XPower p,int n){
 auto&x=s.expansion;auto&v=x.powers[(unsigned)p];
 if(p==XPower::Entropy&&!v&&n>0){x.entropy_boulder_prefix=0;for(auto b:x.boulder)if(b)++x.entropy_boulder_prefix;}
 int old=v;v=(int16_t)max(p==XPower::Focus?-9999:0,min(9999,v+n));if(p==XPower::Entropy&&!v)x.entropy_boulder_prefix=0;
 if(!power_instanced(18+unsigned(p)))power_order_set(s,0,18+unsigned(p),v!=0);
 if(p==XPower::Calamity){
  if(!old&&v)after_play_register(s,(unsigned)PostPlayActor::Calamity);
  else if(old&&!v)after_play_unregister(s,(unsigned)PostPlayActor::Calamity);
 }
 if(p==XPower::Nostalgia){
  if(!old&&v)card_result_register(s,CardResultActor::Nostalgia);
  else if(old&&!v)card_result_unregister(s,CardResultActor::Nostalgia);
 }
}
void queue_extension(State&s,XOp op,int a,int b,bool first){
 if(a< -9999||a>9999||b< -9999||b>9999){fail(s);return;}
 if(enqueue(s,EffectKind::Extended,a,(int)op,first))s.effects[first?0:s.effect_count-1].aux=(int16_t)b;
}
int cost(const State&s,int id){
 if(id<0||id>=s.card_count)return 99;
 return card_preview_cost(s,s.cards[id],id);
}
static int local_energy_cost(const State&s,Card c,int id){
 int base=event_card_base_cost(s,c,id,cost(c));if(base<0||id<0||id>=s.card_count||s.cards[id].kind!=c.kind)return base;
 const auto&x=s.expansion;const auto&i=s.ironclad;const auto&v=s.silent;
 const auto&d=s.defect;const auto&r=s.regent;const auto&n=s.necro;
 int stomp=0,silent=0,defect=0,kick=0,necro=0;
 // LocalCostModifier applies its entire ordered Add/Set list before any
 // clamp or global hook. Each latest Set captures the preceding Add totals;
 // independent expiry boundaries retain the older Set underneath it.
 if(x.turn_cost[id]){
  base=x.turn_cost[id]-1;stomp=i.stomp_turn_origin[id];silent=v.turn_origin[id];
  defect=d.turn_origin[id];kick=r.kick_turn_origin[id];necro=n.turn_origin[id];
 }else if(n.flatten_zero[id]){
  // Flatten has a true SetThisTurn, not SetThisTurnOrUntilPlayed. Only
  // Necrobinder's Transfigure can add a local cost to a Flatten instance.
  base=0;necro=n.flatten_origin[id];
 }else if(v.bullet_free[id]){
  // The historical v13 writer stored BulletTime separately. New plays use
  // the shared until-play Set; keep old saved instances readable here.
  base=0;silent=v.bullet_origin[id];
 }else if(x.combat_cost[id]){
  base=x.combat_cost[id]-1;stomp=i.stomp_combat_origin[id];silent=v.combat_origin[id];
  defect=d.combat_origin[id];kick=r.kick_combat_origin[id];necro=n.combat_origin[id];
 }
 if(c.kind==Stomp)base-=int(i.stomp_discount[id])-stomp;
 base-=int(v.discount[id])-silent;
 base+=int(d.cost_delta[id])-defect;
 if(c.kind==RegentKinglyKick)base-=int(r.kick_discount[id])-kick;
 base+=int(n.cost_delta[id])-necro;
 return base;
}
static int global_energy_cost(const State&s,Card c,int id,int base,bool active){
 const auto&i=s.ironclad;const auto&v=s.silent;const auto&d=s.defect;const auto&r=s.regent;const auto&n=s.necro;
 // ModifyEnergyCostInCombat skips both global passes for a negative local
 // subtotal. In particular, BorrowedTime cannot revive a negative subtotal.
 if(base>=0){
  base+=n.powers[(int)NPower::BorrowedTime];
  base=wr_energy_cost(s,c,id,base,false);
  base=event_card_cost(s,c,id,base);base=enemy_card_energy_cost(s,id,base);
  const CardType type=card_type(s,c);
  const bool ethereal=(id>=0&&id<s.card_count&&s.cards[id].kind==c.kind&&n.ethereal[id])||
      ((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&8);
  if((type==CardType::Skill&&i.powers[(int)IPower::Corruption])||
     (active&&((type==CardType::Attack&&i.powers[(int)IPower::FreeAttack])||
               (type==CardType::Skill&&v.powers[(int)SPower::FreeSkill])||
               (type==CardType::Power&&d.powers[(int)DPower::FreePower])||
               (n.powers[(int)NPower::Veilpiercer]&&ethereal)||
               r.void_played_turn<r.powers[(int)RPower::VoidForm])))base=0;
  base=wr_energy_cost(s,c,id,base,true);
 }
 return max(0,min(9999,base));
}
int card_preview_cost(const State&s,Card c,int id){
 const int base=event_card_base_cost(s,c,id,cost(c));if(base<0||id<0||id>=s.card_count||s.cards[id].kind!=c.kind)return base;
 return global_energy_cost(s,c,id,local_energy_cost(s,c,id),s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo);
}
int card_free_preview_cost(const State&s,Card c,int id){
 const int base=cost(c);return base<0?base:global_energy_cost(s,c,id,0,true);
}
static bool card_has_reducible_cost(const State&s,int id){
 if(id<0||id>=s.card_count)return false;const Card c=s.cards[id];
 const int stars=(s.expansion.star_turn_free[id]||s.expansion.star_combat_free[id])?0:card_star_cost(s,c);
 return local_energy_cost(s,c,id)>0||cost(s,id)>0||stars>0||card_star_cost(s,c,id)>0;
}
static void temporary_cost_set(State&s,int id){
 ironclad_temporary_cost_set(s,id);silent_temporary_cost_set(s,id);defect_temporary_cost_set(s,id);regent_temporary_cost_set(s,id);necro_temporary_cost_set(s,id);
}
static void combat_cost_set(State&s,int id){
 ironclad_combat_cost_set(s,id);silent_combat_cost_set(s,id);defect_combat_cost_set(s,id);regent_combat_cost_set(s,id);necro_combat_cost_set(s,id);
}
static void set_free_this_turn(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&x=s.expansion;
 if(cost(s.cards[id])>=0){x.turn_cost[id]=1;temporary_cost_set(s,id);}
 x.star_turn_free[id]=1;
 s.silent.bullet_free[id]=0;s.silent.bullet_origin[id]=0;
}
static void set_free_this_combat(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&x=s.expansion;
 if(cost(s.cards[id])>=0){x.combat_cost[id]=1;x.turn_cost[id]=0;combat_cost_set(s,id);}
 x.star_combat_free[id]=1;x.star_turn_free[id]=0;
}
static void flatten_cost_set(State&s,int id){
 if(id<0||id>=s.card_count||s.cards[id].kind!=NecroFlatten)return;
 s.expansion.turn_cost[id]=0;
 s.silent.bullet_free[id]=0;s.silent.bullet_origin[id]=0;
 s.necro.flatten_zero[id]=1;s.necro.flatten_origin[id]=s.necro.cost_delta[id];
}
static void copy_card_modifiers(State&s,int to,int from){
 auto&x=s.expansion;x.turn_cost[to]=x.turn_cost[from];x.combat_cost[to]=x.combat_cost[from];
 x.star_turn_free[to]=x.star_turn_free[from];x.star_combat_free[to]=x.star_combat_free[from];
 x.replay[to]=x.replay[from];x.blade_damage[to]=x.blade_damage[from];
 ironclad_card_copied(s,to,from);silent_card_copied(s,to,from);defect_card_copied(s,to,from);regent_card_copied(s,to,from);necro_card_copied(s,to,from);event_card_copied(s,to,from);enemy_card_copied(s,to,from);
 // CreateClone preserves this flag. Only explicit CreateDupe callers set it.
 s.world_relics.card_is_dupe[to]=s.world_relics.card_is_dupe[from];
}
static void reset_card_modifiers(State&s,int id){
 auto&x=s.expansion;x.turn_cost[id]=x.combat_cost[id]=x.replay[id]=0;
 x.star_turn_free[id]=x.star_combat_free[id]=0;
 x.return_turn[id]=0;x.blade_damage[id]=0;
 ironclad_card_reset(s,id);silent_card_reset(s,id);defect_card_reset(s,id);regent_card_reset(s,id);necro_card_reset(s,id);event_card_reset(s,id);enemy_card_reset(s,id);
 s.world_relics.card_is_dupe[id]=0;
}
bool card_exhausts(Card c){if(c.kind>=CardKinds)return false;const auto&d=definitions[c.kind];return d.exhausts||((c.upgraded?d.upgraded_keywords:d.keywords)&1);}
bool card_can_generate(unsigned k){return k<CardKinds&&card_rarity(k)>=2&&card_rarity(k)<=4&&!card_generated(k)&&k!=Alchemize&&k!=HandOfGreed&&k!=HiddenGem&&k!=Feed&&k!=NotYet&&k!=Nightmare&&k!=TheHunt&&k!=NecroEidolon&&k!=NecroTransfigure;}
void gain_unpowered_block(State&s,int n){
 n=max(0,min(silent_modify_block(s,n),9999-s.block));s.block+=(int16_t)n;
 record(s,LogKind::Block,n);if(n)world_relic_hook(s,WorldRelicHook::AfterBlock,n,-1,-1);ironclad_after_block(s,n,false);
}
static void gain_card_block(State&s,int n){
 Card c=s.playing>=0?s.cards[s.playing]:Card{Defend,0};
 n=ironclad_block(s,c,n);n=n*wr_block_percent(s,c,s.playing)/100;n=max(0,min(enemy_player_block_multiplier(s,silent_modify_block(s,n)),9999-s.block));
 s.block+=(int16_t)n;record(s,LogKind::Block,n);if(n)world_relic_hook(s,WorldRelicHook::AfterBlock,n,-1,s.playing);ironclad_after_block(s,n,true);
}
static bool enqueue_card_block(State&s,int n,bool first){
 if(!enqueue(s,EffectKind::Block,n,0,first))return false;
 s.effects[first?0:s.effect_count-1].aux=1;return true;
}
static int powered_block(const State&s,Card c,int n,int instance){
 if(instance<0)instance=s.playing;n+=event_enchantment_block_add(s,c,instance);
 if(s.no_block||n<=0)return 0;
 return max(0,min(9999,n+s.dexterity+((c.kind==Defend||c.kind==DefendSilent||c.kind==UltimateDefend||c.kind==DefectDefend||c.kind==RegentDefend||c.kind==NecroDefendNecrobinder)?power_value(s,XPower::Fasten):0)));
}
int card_block(const State&s,Card c,int instance){
 if(c.kind>=CardKinds)return 0;const auto&d=definitions[c.kind];
 int base=defect_base_block(s,c,instance,d.block+(c.upgraded?d.block_up:0));
 base=necro_base_block(s,c,instance,base);base=event_card_base_block(s,c,instance,base);if(c.kind==SovereignBlade)base=regent_blade_block(s);
 int n=powered_block(s,c,base,instance);
 return max(0,min(9999,enemy_player_block_multiplier(s,silent_modify_block(s,ironclad_block(s,c,n,instance)*wr_block_percent(s,c,instance)/100))));
}
static bool matches_generated(const State&s,unsigned k,int filter){
 if(!card_can_generate(k))return false;
 bool own=card_in_pool(k,s.character),neutral=card_colorless(k);
 if(filter==4)return neutral;
 if(filter==5)return !neutral&&!own&&definitions[k].type==CardType::Attack;
 if(!own)return false;
 if(filter==6)return cost({(CardKind)k,0})==0;
 return !filter||(int)definitions[k].type==filter-1;
}
static bool matches_transform(unsigned original,unsigned candidate){
 if(original==candidate||!card_can_generate(candidate))return false;
 CardColor pool=card_color(original);
 if(card_generated(original))pool=CardColor::Colorless;
 return card_color(candidate)==pool;
}
static int create_to_hand(State&s,Card c,bool notify=true){
 int id=make_card(s,c);if(id<0)return -1;
 if(c.kind==SovereignBlade)s.expansion.blade_damage[id]=10;
 bool room=s.hand.count<MaxHand;put(s,room?s.hand:s.discard,id,room?Zone::Hand:Zone::Discard);
 record(s,LogKind::Create,1,c.kind);if(notify)notify_card_generated(s,id);return id;
}
static void generate_card(State&s,int filter,int flags){
 CardKind pool[CardKinds];unsigned n=0;
 for(unsigned k=0;k<CardKinds;k++)if(matches_generated(s,k,filter))pool[n++]=(CardKind)k;
 // The high bits request a single distinct batch; ordinary independent
 // generation keeps a zero batch field and creates one card.
 unsigned count=flags>>2;if(!count)count=1;
 while(count--&&n){
  unsigned j=random_below(s.rng_cards,n);CardKind kind=pool[j];pool[j]=pool[--n];
  int id=create_to_hand(s,{kind,(uint8_t)((flags&2)!=0)});if(id<0)return;
  if(flags&1)set_free_this_turn(s,id);
 }
}
static void clear_extended_choice(State&s){auto&x=s.expansion;x.choice=XChoice::None;x.choices.count=0;x.choice_min=x.choice_max=x.choice_free=0;}
static void set_extended_choice(State&s,XChoice kind,const Pile&pool,int lo,int hi,int flags=0){
 if(!alive(s)||!pool.count)return;
 // These fixed-count pile commands can request more cards than fit in Hand.
 // The source shortcut precedes UI capacity clamps and returns every card;
 // CardPileCmd.Add then sends the overflow to Discard in source pile order.
 if((kind==XChoice::Draw||kind==XChoice::RForegone)&&lo==hi&&pool.count>MaxHand&&int(pool.count)<=lo){
  apply_complete_draw_choice(s,pool,(flags&1)!=0);return;
 }
 hi=min(hi,min(MaxHand,(int)pool.count));lo=min(lo,hi);if(hi<=0)return;
 auto&x=s.expansion;x.choices=pool;x.choice=kind;x.choice_min=(uint8_t)lo;x.choice_max=(uint8_t)hi;x.choice_free=flags&1;
 if(!s.effect_count)queue_extension(s,XOp::Noop);
 s.phase=Phase::ExtendedChoice;
}
Card choice_card(const State&s,int token){
 if(s.phase==Phase::ExtendedChoice&&(s.expansion.choice==XChoice::Offer||s.expansion.choice==XChoice::EnemyKnowledge||s.expansion.choice==XChoice::EventAbundance)&&token>=MaxCards&&token<MaxCards+3)return s.expansion.offers[token-MaxCards];
 return token>=0&&token<s.card_count?s.cards[token]:Card{Strike,0};
}
static bool move_instance(State&s,int id,Pile&from,Pile&to,Zone z){
 for(unsigned i=0;i<from.count;i++)if(from.ids[i]==id){remove(from,i);return put(s,to,id,z);}return false;
}
static bool apply_extended_choice(State&s,const uint16_t*ids,unsigned n){
 auto&x=s.expansion;
 if(event_card_choice(x.choice))return apply_event_card_choice(s,ids,n);
 if(ironclad_choice(x.choice))return apply_ironclad_choice(s,ids,n);
 if(x.choice==XChoice::SlySkill||x.choice==XChoice::NightmareCard)return apply_silent_choice(s,ids,n);
 if(regent_choice(x.choice))return apply_regent_choice(s,ids,n);
 if(x.choice>=XChoice::NCleanse&&x.choice<=XChoice::NTransfigure)return apply_necro_choice(s,ids,n);
 if(s.phase!=Phase::ExtendedChoice||x.choice==XChoice::None||n>MaxHand||n<(x.choice==XChoice::Offer?0:x.choice_min)||n>x.choice_max||(n&&!ids))return false;
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
  if(kind==XChoice::Offer){id=create_to_hand(s,x.offers[id-MaxCards]);if(id<0)break;if(free)set_free_this_turn(s,id);}
  else if(kind==XChoice::Draw||kind==XChoice::Discard){
   bool room=s.hand.count<MaxHand;
   if(move_instance(s,id,kind==XChoice::Draw?s.draw:s.discard,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard)&&free)set_free_this_turn(s,id);
  }else if(kind==XChoice::Top)move_instance(s,id,s.hand,s.draw,Zone::Draw);
  else if(kind==XChoice::Free)set_free_this_combat(s,id);
  else if(kind==XChoice::DiscardRedraw){if(move_instance(s,id,s.hand,s.discard,Zone::Discard))record(s,LogKind::Discard,s.cards[id].kind);}
  else if(kind==XChoice::Transform){
   CardKind pool[CardKinds];unsigned count=0;for(unsigned k=0;k<CardKinds;k++)if(matches_transform(s.cards[id].kind,k))pool[count++]=(CardKind)k;
   if(count){s.cards[id]={pool[random_below(s.rng_cards,count)],0};reset_card_modifiers(s,id);notify_card_generated(s,id);}
  }
 }
 if(kind==XChoice::Exhaust)for(unsigned i=n;i>0;i--)enqueue(s,EffectKind::ExhaustCard,selected[i-1],0,true);
 if(kind==XChoice::DiscardRedraw){
  for(unsigned i=n;i>0;i--)silent_card_discarded(s,selected[i-1]);
  queue_extension(s,XOp::Draw,(int)n,0,true);
 }
 return true;
}
bool choose_extended(State&s,const uint16_t*ids,unsigned n){
 if(s.expansion.choice==XChoice::EnemyKnowledge)return n==1&&ids&&enemy_choice(s,ids[0]);
 if(!apply_extended_choice(s,ids,n))return false;
 resolve_pending(s);++s.revision;return true;
}

static void offer_cards(State&s,int filter,int flags){
 CardKind pool[CardKinds];unsigned n=0;for(unsigned k=0;k<CardKinds;k++)if(matches_generated(s,k,filter))pool[n++]=(CardKind)k;
 Pile choices{};
 for(unsigned i=0;i<3&&n;i++){unsigned j=random_below(s.rng_cards,n);s.expansion.offers[i]={pool[j],(uint8_t)((flags&2)!=0)};pool[j]=pool[--n];choices.ids[choices.count++]=(uint16_t)(MaxCards+i);}
 set_extended_choice(s,XChoice::Offer,choices,0,1,flags);
}
static void choose_draw(State&s,int filter,int sample=0,int count=1){
 Pile eligible{};
 for(unsigned i=0;i<s.draw.count;i++){int id=s.draw.ids[i];if(!filter||(int)card_type(s,s.cards[id],id)==filter-1)eligible.ids[eligible.count++]=(uint16_t)id;}
 if(sample&&eligible.count>(unsigned)sample){
  for(unsigned i=0;i<(unsigned)sample;i++){unsigned j=i+random_below(s.rng_cards,eligible.count-i);auto t=eligible.ids[i];eligible.ids[i]=eligible.ids[j];eligible.ids[j]=t;}
  eligible.count=(uint16_t)sample;
 }
 set_extended_choice(s,XChoice::Draw,eligible,count,count);
}
static void restore_frame(State&s){
 auto&x=s.expansion;
 if(!x.depth){s.playing=-1;x.remaining=0;x.triple=x.returning=0;return;}
 XFrame f=x.frames[--x.depth];x.frames[x.depth]=XFrame{};
 s.playing=f.playing;x.target=f.target;x.x=f.x;x.last_damage=f.last_damage;s.attack_vigor=f.vigor;x.remaining=f.remaining;s.double_attack=f.double_attack;x.triple=f.triple;x.returning=f.returning;
}
static void finish_extended_card(State&s){
 if(s.playing<0)return;auto&x=s.expansion;Card card=s.cards[s.playing];auto type=card_type(s,card);
 x.cards_finished=(uint16_t)min(9999,x.cards_finished+1);
 queue_extension(s,XOp::AfterPlay,s.playing,0,true);
 queue_extension(s,XOp::AllCharactersLate,0,0,true);
 if(type==CardType::Attack)s.attacked_this_turn=1;
 ironclad_after_card(s,card);silent_after_play(s,s.playing);defect_after_play(s,s.playing);regent_after_card(s,card);necro_after_play(s,s.playing);
 queue_post_play_listeners(s);
}
static void after_play(State&s,int id){
 if(id!=s.playing||id<0||id>=s.card_count){fail(s);return;}auto&x=s.expansion;
 if(x.remaining&&alive(s)){--x.remaining;relic_attack_end(s);x.triple=0;queue_current(s);return;}
 // Result-pile moves only own a card that is still in the play pile.
 // Nested Flak/Make It So can already have exhausted or recalled that instance.
 const bool captured=x.result_location[x.depth]!=0;
 const uint8_t result=captured?x.result_location[x.depth]:card_result_legacy(s,id);
 // Historical pending plays consumed this flag at completion. New plays
 // consume it at base capture and retain flags subsequently applied by a body.
 if(!captured)s.ironclad.forced_exhaust[id]=0;
 bool ex=(result&7)==ResultExhaust;
 if(s.zones[id]==Zone::Limbo){
  // PileType.None is stored with spent Power instances. All other results
  // retain the one location captured before the physical play's first body.
  if(!ex)card_result_put(s,id,result);
 }else ex=false;
 event_card_after_return_to_draw(s,id);
 x.turn_cost[id]=0;x.star_turn_free[id]=0;
 // A true whole-turn Flatten Set remains in its separate layer; ending the
 // physical play removes all until-play Sets above it, including BulletTime.
 s.silent.bullet_free[id]=0;s.silent.bullet_origin[id]=0;
 ironclad_after_play_group(s,id);defect_after_play_group(s,id);regent_after_play_group(s,id);necro_after_play_group(s,id);
 relic_attack_end(s);x.attack_observed[x.depth]=x.attack_killed[x.depth]=x.attack_fatal[x.depth]=0;restore_frame(s);
 if(ex)exhaust_id(s,id);
 if(!alive(s)){victory(s);return;}
 if(s.playing<0&&!x.depth)world_stage(s,WorldStage::HandEmptied);
 if(s.regent.end_turn_requested&&s.playing<0&&!x.depth){s.regent.end_turn_requested=0;queue_extension(s,XOp::TurnEnd,0,0,true);}
}

static void queue_current(State&s){queue_extension(s,XOp::AllCharactersBefore,0,0,true);}
static void prepend_appended_effects(State&s,int old){
 // Rotate [caller continuation][new leaf] to [new leaf][caller continuation].
 // Three reversals preserve each block's exact order with one Effect of
 // scratch, including a full queue. No MaxEffects array is put on the stack.
 if(old<=0||old>=s.effect_count)return;
 const auto reverse=[&](int first,int last){while(first<last){const Effect value=s.effects[first];s.effects[first++]=s.effects[last];s.effects[last--]=value;}};
 reverse(0,old-1);reverse(old,s.effect_count-1);reverse(0,s.effect_count-1);
}
static void before_current(State&s){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 const int old=s.effect_count;Card c=s.cards[s.playing];
 // Necrobinder reads the effective cost before one-shot free-power/skill
 // listeners consume their layers. Captures happen once per replay.
 necro_before_play(s,s.playing);ironclad_before_card(s,c);silent_before_play(s,s.playing);
 defect_before_play(s,s.playing);regent_before_card(s,c);
 if(s.world_relics.active)world_stage(s,WorldStage::BeforeCardRelics,0,false);else{event_card_begin_play(s,s.playing);enemy_before_card_play(s,s.playing,s.necro.play_index[s.expansion.depth]>1);queue_extension(s,XOp::AllCharactersBody);}
 prepend_appended_effects(s,old);
}
static void before_current_cards(State&s){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 const int old=s.effect_count;event_card_begin_play(s,s.playing);enemy_before_card_play(s,s.playing,s.necro.play_index[s.expansion.depth]>1);queue_extension(s,XOp::AllCharactersBody);
 prepend_appended_effects(s,old);
}
// Each pinned card body contains at most one AttackCommand. Multi-hit and
// all-target instructions may expand dynamically but share this boundary.
static bool card_attack_command(const Effect&e){
 if(e.kind==EffectKind::Damage||e.kind==EffectKind::GreedDamage||e.kind==EffectKind::RandomDamage||e.kind==EffectKind::WhirlwindHits)return true;
 if(e.kind!=EffectKind::Extended)return false;
 switch(XOp(e.target)){
 case XOp::RandomHits:case XOp::IHits:case XOp::IAllHits:case XOp::IFeed:
 case XOp::SHits:case XOp::SEchoSlash:case XOp::DHits:case XOp::DFlak:
 case XOp::RHits:case XOp::RAllHits:case XOp::NRepeatedHit:return true;
 default:return false;
 }
}
static void queue_card_body(State&s){
 if(s.playing<0||s.playing>=s.card_count){fail(s);return;}
 auto&x=s.expansion;Card c=s.cards[s.playing];const auto&d=definitions[c.kind];int old=s.effect_count;
 x.last_damage=0;x.attack_observed[x.depth]=x.attack_killed[x.depth]=x.attack_fatal[x.depth]=0;
 const auto history_type=card_type(s,c,s.playing);
 if(history_type==CardType::Attack||history_type==CardType::Skill)x.nonpower_started=(uint16_t)min(9999,x.nonpower_started+1);
 if(history_type==CardType::Attack&&!s.defect.spent[x.depth])s.defect.zero_attacks_started=(uint16_t)min(9999,s.defect.zero_attacks_started+1);
 if(queue_event_card(s,c,x.target,x.x)||queue_enemy_token(s,c,x.target,x.x)||queue_ironclad(s,c,x.target,x.x)||queue_silent(s,c,x.target,x.x)||queue_defect(s,c,x.target,x.x)||queue_regent(s,c,x.target,x.x)||queue_necro(s,c,x.target,x.x)||queue_common_token(s,c,x.target)){}
 else if(c.kind<Alchemize)queue_legacy(s,c,x.target,x.x);
 else{
  record(s,LogKind::Play,c.kind,c.upgraded);
  if(d.type==CardType::Attack)relic_attack_begin(s);
  int hit=card_base_damage(s,c,x.target,s.playing),block=d.block+(c.upgraded?d.block_up:0),power=d.power+(c.upgraded?d.power_up:0),draw=d.draw+(c.upgraded?d.draw_up:0);
  auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};auto pw=[&](XPower p,int n){op(XOp::Power,(int)p,n);};
  if(block)enqueue_card_block(s,powered_block(s,c,block));
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
  case BeatDown:{
   // Capture distinct discard instances before the first attack is played.
   // Returned attacks must not enter the selection again within this play.
   uint16_t ids[MaxCards];unsigned n=0;
   for(unsigned i=0;i<s.discard.count;i++)if(card_type(s,s.cards[s.discard.ids[i]],s.discard.ids[i])==CardType::Attack)ids[n++]=s.discard.ids[i];
   for(unsigned i=0;i<n&&i<(unsigned)draw;i++){
    unsigned j=i+random_below(s.rng_cards,n-i);uint16_t id=ids[j];ids[j]=ids[i];ids[i]=id;op(XOp::AutoInstance,id);
   }
   break;}
  case Bolas:case ThrummingHatchet:op(XOp::ReturnNext,s.playing);break;
  case Calamity:pw(XPower::Calamity,1);break;
  case Catastrophe:for(int i=0;i<draw;i++)op(XOp::AutoDraw);break;
  case DarkShackles:op(XOp::NTempStrength,(int)NTempStrength::DarkShackles*MaxEnemies+x.target,power);break;
  case Discovery:op(XOp::Offer,0,1);break;
  case Entropy:pw(XPower::Entropy,draw);break;
  case Equilibrium:case Salvo:pw(XPower::Retain,1);break;
  case EternalArmor:pw(XPower::Plating,power);break;
  case Fasten:pw(XPower::Fasten,power);break;
  case Fisticuffs:op(XOp::DamageBlock);break;
  case HiddenGem:op(XOp::EnchantReplay,power);break;
  case JackOfAllTrades:{
   CardKind pool[CardKinds];unsigned n=0;
   for(unsigned k=0;k<CardKinds;k++)if(k!=JackOfAllTrades&&matches_generated(s,k,4))pool[n++]=(CardKind)k;
   for(int i=0;i<draw&&n;i++){unsigned j=random_below(s.rng_cards,n);op(XOp::Create,1,pool[j]);pool[j]=pool[--n];}
   break;}
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
 // Capture Fatal once before the complete attack, and retain WasTargetKilled
 // independently of any revival executed by an intervening death listener.
 int first_attack=-1,last_attack=-1;
 for(int at=old;at<s.effect_count;++at)if(card_attack_command(s.effects[at])){if(first_attack<0)first_attack=at;last_attack=at;}
 if(first_attack>=0){
  if(s.effect_count+2>MaxEffects){fail(s);return;}
  for(int at=s.effect_count;at>last_attack+1;--at)s.effects[at]=s.effects[at-1];
  s.effects[last_attack+1]={EffectKind::Extended,s.playing,(int16_t)XOp::SharedAttackEnd,0};++s.effect_count;
  for(int at=s.effect_count;at>first_attack;--at)s.effects[at]=s.effects[at-1];
  s.effects[first_attack]={EffectKind::Extended,s.playing,(int16_t)XOp::SharedAttackStart,0};++s.effect_count;
 }
 // Every physical body finishes enchantment and affliction OnPlay before
 // history and AfterCardPlayed. Both boundaries can suspend independently.
 for(int at=old;at<s.effect_count;++at)if(s.effects[at].kind==EffectKind::Finish)s.effects[at]={EffectKind::Extended,s.playing,(int16_t)XOp::SharedAfterCardBody,0};
 // Place the new leaf's commands ahead of the saved caller continuation.
 prepend_appended_effects(s,old);
}
static bool start_instance(State&s,int id,int target,bool automatic,int captured_x,int captured_star_x){
 if(id<0||id>=s.card_count||!alive(s)||s.hp<=0)return false;
 Card c=s.cards[id];if(c.kind>=CardKinds)return false;const auto&d=definitions[c.kind];auto&x=s.expansion;
 if(!wr_can_play(s,automatic)||!enemy_card_can_play(s,id,automatic)||!event_card_play_allowed(s,id,automatic))return false;
 if(!automatic&&(cost(c)<-1||!silent_can_play(s,c)||!necro_can_play(s,c)))return false;
 if(card_targets_enemy(s,c,id)&&!enemy_allow_target(s,target)){
  if(!automatic)return false;int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(enemy_allow_target(s,i))ids[n++]=i;
  if(!n)return false;target=ids[random_below(s.rng_cards,n)];
 }
 int price=cost(s,id),resource_energy=price<0?s.energy:price;
 int star_price=card_star_cost(s,c,id),resource_stars=star_price<0?x.stars:star_price;
 int spent=automatic?0:resource_energy,stars_spent=automatic?0:resource_stars;
 if(spent>s.energy||stars_spent>x.stars)return false;
 Zone z=s.zones[id];Pile*source=z==Zone::Hand?&s.hand:z==Zone::Draw?&s.draw:z==Zone::Discard?&s.discard:(automatic&&z==Zone::Exhaust)?&s.exhaust:nullptr;
 bool reserved=automatic&&z==Zone::Limbo;
 if(automatic){if(s.playing==id)return false;for(unsigned i=0;i<x.depth;i++)if(x.frames[i].playing==id)return false;}
 if(!source&&!reserved)return false;int at=-1;
 if(source){for(unsigned i=0;i<source->count;i++)if(source->ids[i]==id){at=i;break;}if(at<0)return false;}
 if(cost(c)<-1){
  // Original AutoPlay moves an Unplayable card through the play pile to its
  // result pile without starting a CardPlay or consuming temporary prices.
  // This also applies to ordinary draw/discard sources, not just reserved
  // AutoTop cards. An already active ancestor is rejected above.
  if(source)remove(*source,at);s.zones[id]=Zone::Limbo;
  if(s.world_relics.card_is_dupe[id])put(s,s.powers,id,Zone::Power);
  else if(card_exhausts(s,c,id)||s.ironclad.forced_exhaust[id])exhaust_id(s,id);
  else put(s,s.discard,id,Zone::Discard);
  return true;
 }
 if(automatic&&x.depth==MaxAutoFrames){fail(s);return false;}
 if(automatic)x.frames[x.depth++]={s.playing,x.target,x.x,x.last_damage,s.attack_vigor,x.remaining,s.double_attack,x.triple,x.returning};
 // Paid AutoPlay(skipXCapture:true) retains the card's previous X captures,
 // while ResourceInfo.EnergyValue still reads its current post-payment cost.
 int energy=automatic&&captured_x>=0&&cost(c)==-1?captured_x:resource_energy;
 if(source)remove(*source,at);s.zones[id]=Zone::Limbo;s.playing=(int16_t)id;s.energy-=(int16_t)spent;x.stars-=(int16_t)stars_spent;
 x.x=(int16_t)(cost(c)==-1?wr_x_value(s,energy):energy);x.target=(int16_t)target;x.last_drawn=-1;x.remaining=x.replay[id];
 s.attack_vigor=0;s.double_attack=0;x.triple=0;
 card_result_capture(s,id,resource_energy);
 if(power_value(s,XPower::Duplicator)){++x.remaining;gain_power(s,XPower::Duplicator,-1);}
 x.remaining=(uint16_t)min(9999,x.remaining+ironclad_replays(s,c)+silent_extra_replays(s,c)+defect_extra_replays(s,c)+regent_replays(s,c)+event_card_replays(s,id));
 x.remaining=(uint16_t)min(9999,wr_play_count(s,id,x.remaining+1,true)-1);
 defect_play_start(s,id,resource_energy,automatic,spent);necro_start_instance(s,id,resource_energy);
 queue_current(s);if(stars_spent)world_stage(s,WorldStage::StarsSpent,stars_spent);regent_begin_play(s,id,automatic,spent,stars_spent,captured_star_x);return true;
}
bool card_has_nonresource_play_block(const State&s,int id){
 if(id<0||id>=s.card_count||s.cards[id].kind>=CardKinds)return false;
 const Card c=s.cards[id];
 // NCard's overlay follows CardModel.CanPlay reasons. A busy action queue
 // and UI phase/input locks are not card rules and must not add crosses.
 return !wr_can_play(s,false)||cost(c)<-1||!silent_can_play(s,c)||!necro_can_play(s,c)||
        !enemy_card_can_play(s,id,false)||!event_card_play_allowed(s,id,false);
}
bool can_play_card(const State&s,int id){
 if(s.phase!=Phase::Combat||s.effect_count||s.playing>=0||id<0||id>=s.card_count||s.zones[id]!=Zone::Hand)return false;
 Card c=s.cards[id];return c.kind<CardKinds&&!card_has_nonresource_play_block(s,id)&&cost(s,id)<=s.energy&&card_star_cost(s,c,id)<=s.expansion.stars;
}
bool card_targets_enemy(const State&s,Card c,int instance,int deck_index){
 if(c.kind>=CardKinds)return false;bool result=regent_targets_enemy(s,c,silent_card_targets_enemy(s,c));
 event_card_targeted(s,c,instance,deck_index,result);return result;
}
static void draw_hook(State&s){
 auto&x=s.expansion;
 for(int i=MaxPowerInstances-1;i>=0;i--)if(x.automation[i]&&++x.automation_count[i]==10){x.automation_count[i]=0;enqueue(s,EffectKind::Energy,x.automation[i],0,true);}
}
static void shuffle_hook(State&s){
 int n=power_value(s,XPower::Stratagem);if(n)queue_extension(s,XOp::ChooseDraw,0,-n,true);
}
static void draw_one(State&s,int n,bool hand_draw=false){
 s.expansion.last_drawn=-1;
 if(n<=0||s.no_draw||s.hand.count>=MaxHand)return;
 if(!s.draw.count){if(!s.discard.count)return;queue_extension(s,XOp::DrawOne,n,hand_draw?1:0,true);shuffle(s);return;}
 if(n>1)queue_extension(s,XOp::DrawOne,n-1,hand_draw?1:0,true);
 int id=remove(s.draw,s.draw.count-1);put(s,s.hand,id,Zone::Hand);s.expansion.last_drawn=(int16_t)id;record(s,LogKind::Draw,1);draw_hook(s);
 world_stage(s,WorldStage::DrawRelics,id);
 ironclad_after_draw(s,id);silent_card_drawn(s,id);defect_card_drawn(s,id);regent_after_draw(s,id);necro_card_drawn(s,id);
 if(s.cards[id].kind==Void){s.energy=(int16_t)max(0,s.energy-1);record(s,LogKind::Energy,-1);}
}
void resolve_pending(State&s){resolve(s);if(!resolving&&s.phase==Phase::Combat&&s.playing<0&&!s.effect_count&&!s.expansion.depth){finish_combat_if_won(s);if(s.effect_count)resolve(s);}}
bool lab_potion(State&s,Potion p){
 if(!s.practice||s.phase!=Phase::Combat||s.effect_count||s.playing>=0||!potion_definition(p))return false;
 if(!procure_potion(s,p))return false;++s.revision;return true;
}
bool lab_card(State&s,Card c){
 if(!s.practice||s.phase!=Phase::Combat||s.effect_count||s.playing>=0||c.kind>=CardKinds||c.upgraded>1||(c.upgraded&&!card_upgradeable({c.kind,0}))||s.card_count>=MaxCards)return false;
 create_to_hand(s,c);resolve_pending(s);++s.revision;return true;
}
