static void death_cleanup(State&s){
 enemy_player_died(s);
 ironclad_combat_end(s);silent_combat_end(s);defect_combat_end(s);regent_combat_end(s,false);necro_combat_end(s);
 for(unsigned id=0;id<s.card_count;id++)if(s.zones[id]==Zone::Limbo){const bool dupe=s.world_relics.card_is_dupe[id];put(s,dupe?s.powers:s.discard,id,dupe?Zone::Power:Zone::Discard);}
 s.playing=-1;s.effect_count=0;s.expansion.depth=0;s.expansion.potion_active=0;s.expansion.enemy_side=0;
 s.discard_required=s.exhaust_limit=0;clear_extended_choice(s);relic_attack_end(s);s.expansion.triple=0;s.phase=Phase::Dead;
 event_card_combat_end(s,false);if(s.world.active){s.world.finished=1;original_event_finish_run(s);}
}
static bool revive(State&s){
 if(s.hp>0)return true;
 // Fairy is an ordinary ShouldDie listener; Lizard Tail is ShouldDieLate.
 // OnUseWrapper consumes the bottle and awaits healing before potion listeners.
 for(unsigned slot=0;slot<potion_slot_count(s);++slot)if(s.potions[slot]==Potion::FairyInABottle){
  s.potions[slot]=Potion::Empty;record(s,LogKind::Potion,int(Potion::FairyInABottle));
  before_potion_use(s,Potion::FairyInABottle,-1);
  const bool saved=combat_phase(s.phase)&&world_shared_active(s);
  if(saved)world_stage(s,WorldStage::AutoPotionUsed,int(Potion::FairyInABottle));
  heal(s,max(1,s.max_hp*3/10));
  if(!saved)world_relic_hook(s,WorldRelicHook::AfterPotionUsed,int(Potion::FairyInABottle),-1);
  return true;
 }
 return wr_revive(s);
}
void player_kill(State&s,bool force){
 if(s.phase==Phase::Dead||s.phase==Phase::Error)return;
 const int old=s.hp;s.hp=0;
 if(combat_phase(s.phase)&&world_shared_active(s)){
  world_stage(s,WorldStage::PlayerDeath,force?1:0);
  if(old)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,0,-1);
 }else{
  if(old)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,0,-1);
  if(force||!revive(s))death_cleanup(s);
 }
}
void player_lose_max_hp(State&s,int amount){
 if(amount<=0||s.phase==Phase::Dead||s.phase==Phase::Error)return;
 // Damage uses the raw new maximum, even below one; SetMaxHp clamps only
 // after the damage callbacks and any death prevention have completed.
 const int raw=s.max_hp-amount,target=max(1,raw),loss=max(0,s.hp-raw);
 const bool saved=combat_phase(s.phase)&&world_shared_active(s);
 if(saved)world_stage(s,WorldStage::SetMaxHp,target);
 if(loss)player_hp_loss(s,loss,false,false,-1);
 if(!saved){s.max_hp=int16_t(target);s.hp=int16_t(min(s.hp,s.max_hp));}
}
int player_hp_loss(State&s,int amount,bool attack,bool blockable,int card_source,int dealer){
 if(s.hp<=0)return 0;auto&x=s.expansion;amount=max(0,amount);
 const bool in_combat=combat_phase(s.phase);
 if(dealer<0||dealer>=s.enemy_count)dealer=-1;
 if(card_source==-2)card_source=attack?-1:s.playing;
 if(card_source<0||card_source>=s.card_count)card_source=-1;
 if(attack)amount=amount*wr_incoming_damage_percent(s,dealer,true)/100;
 if(in_combat&&power_value(s,XPower::Intangible)){if(amount>1)notify_power_flash(s,-1,"INTANGIBLE_POWER");amount=min(1,amount);}
 int absorbed=blockable?min(s.block,amount):0;s.block-=(int16_t)absorbed;int unblocked=amount-absorbed,loss=unblocked;
 const bool redirected=attack&&x.osty_hp;const int pet_actual=redirected?min(loss,x.osty_hp):-1;
 if(redirected){int old_pet=x.osty_hp,pet=min(loss,x.osty_hp);x.osty_hp-=(int16_t)pet;loss-=pet;
  notify_damage_result(s,-2,0,unblocked,old_pet,false,false,amount==0);necro_osty_hp_lost(s,old_pet);}
 loss=wr_player_hp_loss(s,loss);
 if(loss&&in_combat&&power_value(s,XPower::Buffer)){gain_power(s,XPower::Buffer,-1);loss=0;}
 const int old=s.hp;s.hp=(int16_t)max(0,s.hp-loss);const int actual=old-s.hp;
 if(in_combat)notify_damage_result(s,-1,absorbed,loss,old,blockable&&(absorbed>0||s.block>0)&&(redirected?unblocked:loss)==0,
  absorbed>0&&s.block==0,amount==0&&!redirected,attack||card_source<0,pet_actual);
 record(s,LogKind::LoseHp,actual);
 const int flags=(attack?1:0)|(blockable?2:0)|(in_combat?4:0)|(!s.hp?8:0);
 if(in_combat&&world_shared_active(s)){
  // AfterBlockBroken -> HPChanged -> DamageGiven -> DamageReceived / Kill.
  // Capture WasTargetKilled but recheck HP after Given (Paper Cuts can itself
  // cause a nested damage/death-prevention sequence). None uses healed HP as
  // the amount of this DamageResult.
  queue_extension(s,XOp::SharedPlayerDamageReceived,actual,(card_source+1)*16+flags,true);
  if(actual&&attack&&dealer>=0)queue_extension(s,XOp::SharedPlayerDamageGiven,actual,dealer,true);
  if(actual)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,s.hp,-1);
  if(absorbed&&!s.block)world_relic_hook(s,WorldRelicHook::AfterBlockBroken,-1,dealer);
  return actual;
 }
 // Historical routes remain synchronous. Lethal damage does not dispatch
 // AfterDamageReceived merely because a later Fairy heals the player.
 if(actual)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,s.hp,-1);
 if(!s.hp){if(!revive(s))death_cleanup(s);return actual;}
 if(in_combat){ironclad_hp_lost(s,actual,card_source);relic_after_hp_loss(s,actual);}
 if(actual)world_relic_hook(s,WorldRelicHook::AfterHpLost,actual,flags&7,card_source);
 if(attack&&actual&&power_value(s,XPower::Gambit)){x.powers[int(XPower::Gambit)]=0;player_kill(s);}
 return actual;
}
int enemy_damage(const State&s,int i){
 if(i<0||i>=s.enemy_count)return 0;const auto&e=s.enemies[i];if(e.move==2)return 0;
 int n=max(0,(e.move==0?12:6)+e.strength),mul=(e.weak?necro_weak_multiplier(s,i,wr_enemy_weak_percent(s,75)):100)*(s.expansion.beetle[i]?70:100);
 return ironclad_enemy_damage(s,i,n*mul/10000);
}
static void autoplay_from(State&s,Pile&p,int filter){
 if(!alive(s))return;
 // Random selection does not shuffle an empty source pile.
 uint16_t ids[MaxCards];unsigned n=0;
 for(unsigned i=0;i<p.count;i++)if(!filter||(int)card_type(s,s.cards[p.ids[i]],p.ids[i])==filter-1)ids[n++]=p.ids[i];
 if(n)start_instance(s,ids[random_below(s.rng_cards,n)],-1,true);
}
static void autoplay_top_batch(State&s,int remaining,int progress){
 if(!alive(s))return;
 if(!remaining)remaining=1;
 unsigned reserved=progress&1023;bool shuffled=(progress&1024)!=0;
 if(remaining<1||remaining>MaxCards||progress<0||(progress&~2047)||reserved+remaining>MaxCards||reserved>s.effect_count){fail(s);return;}
 for(unsigned i=0;i<reserved;i++){
  const auto&e=s.effects[i];
  if(e.kind!=EffectKind::Extended||e.target!=(int)XOp::AutoInstance||e.value<0||e.value>=s.card_count ){fail(s);return;}
 }
 // Reserve the whole possible batch and the one suspended shuffle boundary
 // before moving any card out of its pile. The shuffle hook may add a choice.
 unsigned count=(unsigned)min(remaining,s.draw.count+s.discard.count);
 unsigned shuffle_space=count>s.draw.count?1+(power_value(s,XPower::Stratagem)>0):0;
 if(s.effect_count+count+shuffle_space>MaxEffects){fail(s);return;}
 while(remaining){
  if(!s.draw.count){
   if(!shuffled&&s.discard.count){
    queue_extension(s,XOp::AutoTop,remaining,(int)reserved|1024,true);shuffle(s);
   }
   return;
  }
  int id=s.draw.ids[s.draw.count-1];
  queue_extension(s,XOp::AutoInstance,id,0,true);
  if(s.phase==Phase::Error)return;
  Effect next=s.effects[0];for(unsigned i=0;i<reserved;i++)s.effects[i]=s.effects[i+1];s.effects[reserved]=next;
  remove(s.draw,s.draw.count-1);s.zones[id]=Zone::Limbo;
  ++reserved;--remaining;shuffled=false;
 }
}
static void next_enemy(State&s,int i){
 if(!alive(s)){victory(s);return;}
 if(i>=s.enemy_count){queue_extension(s,XOp::EnemyDone,0,0,true);return;}
 queue_extension(s,XOp::EnemyAct,i+1,0,true);
 if(s.enemy_world.active){enemy_queue_turn(s,i);return;}
 auto&e=s.enemies[i];if(!e.hp)return;
 e.block=0;int hit=enemy_damage(s,i);bool attack=e.move!=2;record(s,LogKind::Enemy,i,e.move);
 if(attack&&s.thorns){notify_power_flash(s,-1,"THORNS_POWER");damage_unpowered(s,i,s.thorns);}
 int blocked=min(s.block,hit);player_hp_loss(s,hit,attack,true,-1,i);if(s.phase==Phase::Dead)return;
 if(attack)regent_after_blocked_attack(s,i,blocked);
 if(attack&&s.flame_barrier)damage_unpowered(s,i,s.flame_barrier);
 if(e.hp){
  if(e.move==1)e.block=5;
  if(e.move==2){int old=e.strength;e.strength=(int16_t)min(9999,e.strength+2);if(e.strength!=old){necro_note_debuff(s,i,NDebuff::Strength);necro_prune_debuffs(s);}}
 }
 if(e.weak)--e.weak;if(e.vulnerable)--e.vulnerable;if(s.expansion.beetle[i])--s.expansion.beetle[i];
 e.move=(uint8_t)((e.move+1)%3);
}
static void forge_blade(State&s,int n){
 bool found=false;auto&x=s.expansion;
 for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==SovereignBlade&&s.zones[id]!=Zone::Exhaust){
  x.blade_damage[id]=(int16_t)min(9999,max(10,x.blade_damage[id])+n);found=true;
 }
 if(!found){int id=create_to_hand(s,{SovereignBlade,0});if(id>=0)x.blade_damage[id]=(int16_t)min(9999,10+n);}
}
static void extension_effect(State&s,Effect e){
 auto&x=s.expansion;auto op=(XOp)e.target;int a=e.value,b=e.aux;
 switch(op){
 case XOp::Noop:break;
 case XOp::WorldStage:world_shared_effect(s,(WorldStage)a,b);break;
 case XOp::WRelicHook:world_relic_effect(s,a);break;
 case XOp::SharedPlayerDamageGiven:enemy_player_damage_given(s,b,a);break;
 case XOp::SharedPlayerDamageReceived:{
  const int flags=b&15,source=b/16-1;
  if((flags&8)&&!s.hp){player_kill(s);break;}
  queue_extension(s,XOp::SharedPlayerDamageRelics,a,b,true);
  if(a&&(flags&1)&&power_value(s,XPower::Gambit))world_stage(s,WorldStage::GambitKill);
  ironclad_hp_lost(s,a,source);break;}
 case XOp::SharedPlayerDamageRelics:
  relic_after_hp_loss(s,a);if(a)world_relic_hook(s,WorldRelicHook::AfterHpLost,a,b&7,b/16-1);break;
 case XOp::SharedAfterCardBody:
  queue_extension(s,XOp::SharedAfflictionAfterBody,a,0,true);event_card_after_body(s,a);break;
 case XOp::SharedAfflictionAfterBody:
  enqueue(s,EffectKind::Finish,0,0,true);enemy_card_after_body(s,a);break;
 case XOp::SharedCheckWeakAfterHit:
  if(enemy_allow_target(s,a)&&(enemy_intent(s,a).flags&EnemyIntentAttack))apply_weak(s,a,b);break;
 case XOp::SharedAttackStart:{
  unsigned f=x.depth;x.attack_observed[f]=1;x.attack_killed[f]=x.attack_fatal[f]=0;
  for(int t=0;t<s.enemy_count;++t)if(enemy_allow_target(s,t)&&enemy_death_is_fatal(s,t))x.attack_fatal[f]|=uint8_t(1U<<t);
  break;}
 case XOp::SharedAttackEnd:
  queue_extension(s,XOp::SharedAttackReward,a,0,true);if(a>=0&&a<s.card_count)world_relic_hook(s,WorldRelicHook::AfterAttack,necro_osty_attack(s.cards[a])?-2:-1,s.expansion.target,a);enemy_after_player_attack(s);break;
 case XOp::SharedAttackReward:{
  if(a<0||a>=s.card_count)break;unsigned f=x.depth;bool killed=x.attack_killed[f]!=0,fatal=(x.attack_killed[f]&x.attack_fatal[f])!=0;
  Card c=s.cards[a];const auto&d=definitions[c.kind];int p=d.power+(c.upgraded?d.power_up:0);
  if(fatal&&c.kind==HandOfGreed)gain_gold(s,p);
  else if(fatal&&c.kind==Feed)increase_max_hp(s,p);
  else if(fatal&&c.kind==TheHunt)silent_hunt_reward(s);
  else if(killed&&c.kind==DefectSunder)enqueue(s,EffectKind::Energy,3,0,true);
  x.attack_observed[f]=2;break;}

 case XOp::AllCharactersBeetle:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&!silent_blocks_debuff(s,a)){int n=min(9999-x.beetle[a],b);x.beetle[a]+=(int16_t)n;necro_note_debuff(s,a,NDebuff::Beetle);necro_debuff_applied(s,a,n);}break;
 case XOp::Power:{
  if(a<0||a>=XPowerCount){fail(s);break;}
  int16_t*instances=a==(int)XPower::Automation?x.automation:a==(int)XPower::Panache?x.panache:a==(int)XPower::RollingBoulder?x.boulder:nullptr;
  if(instances){int i=0;while(i<MaxPowerInstances&&instances[i])++i;if(i==MaxPowerInstances){fail(s);break;}instances[i]=(int16_t)b;if(a==(int)XPower::Panache){x.panache_skip[i]=1;after_play_register(s,(unsigned)PostPlayActor::PanacheStart+i);}}
  if(a==(int)XPower::Gambit)x.powers[a]=1;else gain_power(s,(XPower)a,b);break;}
 case XOp::AllDamage:for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,a);break;
 case XOp::StrengthLoss:
  // Legacy continuations did not store the temporary source separately.
  necro_apply_temp_strength(s,a,b,(unsigned)NTempStrength::DarkShackles);break;
 case XOp::AllRare:{
  uint16_t ids[MaxCards];unsigned n=0;
  for(unsigned i=0;i<s.draw.count;i++)if(card_rarity(s.cards[s.draw.ids[i]].kind)==4)ids[n++]=s.draw.ids[i];
  while(n&&s.hand.count<MaxHand){unsigned j=random_below(s.rng_cards,n);int id=ids[j];ids[j]=ids[--n];move_instance(s,id,s.draw,s.hand,Zone::Hand);}
  break;}
 case XOp::Offer:offer_cards(s,a,b);break;
 case XOp::Generate:generate_card(s,a,b);break;
 case XOp::ChooseDraw:choose_draw(s,a,b>0?b:0,b<0?-b:1);break;
 case XOp::ChooseDiscard:set_extended_choice(s,XChoice::Discard,s.discard,1,1,b);break;
 case XOp::ChooseTop:set_extended_choice(s,XChoice::Top,s.hand,1,1);break;
 case XOp::DiscardRedraw:set_extended_choice(s,XChoice::DiscardRedraw,s.hand,0,s.hand.count);break;
 case XOp::ExhaustAny:set_extended_choice(s,XChoice::Exhaust,s.hand,0,s.hand.count);break;
 case XOp::Free:{
  Pile eligible{};for(unsigned i=0;i<s.hand.count;i++)if(card_has_reducible_cost(s,s.hand.ids[i]))eligible.ids[eligible.count++]=s.hand.ids[i];
  set_extended_choice(s,XChoice::Free,eligible,1,1);break;}
 case XOp::Transform:set_extended_choice(s,XChoice::Transform,s.hand,a,a);break;
 case XOp::AutoDraw:autoplay_from(s,s.draw,a);break;
 case XOp::AutoDiscard:autoplay_from(s,s.discard,a);break;
 case XOp::AutoTop:autoplay_top_batch(s,a,b);break;
 case XOp::AutoInstance:
  start_instance(s,a,-1,true);
  break;
 case XOp::Draw:if(wr_can_draw(s,b==1))queue_extension(s,XOp::DrawOne,a,b,true);break;
 case XOp::DrawOne:draw_one(s,a,b==1);break;
 case XOp::EscapeBlock:
  if(x.last_drawn>=0&&x.last_drawn<s.card_count&&card_type(s,s.cards[x.last_drawn],x.last_drawn)==CardType::Skill)gain_card_block(s,a);break;
 case XOp::EnchantReplay:{
  uint16_t ids[MaxCards];unsigned n=0;for(unsigned i=0;i<s.draw.count;i++)if(!x.replay[s.draw.ids[i]])ids[n++]=s.draw.ids[i];
  if(n)x.replay[ids[random_below(s.rng_cards,n)]]=(uint8_t)min(255,a);break;}
 case XOp::Create:for(int i=0;i<a;i++){Card c=card_from_token(b);if(create_to_hand(s,c)<0)break;}break;
 case XOp::CopyDamage:{int amount=x.last_damage;for(int i=0;i<s.enemy_count;i++)if(i!=a)damage_unpowered(s,i,amount);break;}
 case XOp::DamageBlock:if(s.playing>=0)gain_card_block(s,powered_block(s,s.cards[s.playing],x.last_damage));break;
 case XOp::ReturnNext:if(a>=0&&a<s.card_count)x.return_turn[a]=(uint16_t)(s.turn+1);break;
 case XOp::Prolong:s.block_next_turn=(int16_t)min(9999,s.block_next_turn+s.block);break;
 case XOp::Restless:
  if(!s.hand.count){enqueue(s,EffectKind::Energy,b,0,true);queue_extension(s,XOp::Draw,a,0,true);}break;
 case XOp::Bomb:{int i=0;while(i<MaxBombs&&x.bomb_turns[i])++i;if(i==MaxBombs){fail(s);break;}x.bomb_damage[i]=(int16_t)a;x.bomb_turns[i]=(uint8_t)b;break;}
 case XOp::ShuffleAll:
  while(s.hand.count){int id=remove(s.hand,s.hand.count-1);put(s,s.discard,id,Zone::Discard);}shuffle(s);break;
 case XOp::RandomCosts:
  for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(cost(s.cards[id])>=0&&cost(s,id)>=0){x.turn_cost[id]=(uint8_t)(1+random_below(s.rng_cards,4));temporary_cost_set(s,id);}}break;
 case XOp::StrikeReplay:for(unsigned id=0;id<s.card_count;id++)if(card_strike_tag(s.cards[id].kind)&&x.replay[id]<255)++x.replay[id];break;
 case XOp::PotionDone:x.potion_active=0;world_relic_hook(s,WorldRelicHook::AfterPotionUsed,a,b);break;
 case XOp::Demise:if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&!silent_blocks_debuff(s,a)){int n=min(9999-x.demise[a],b);x.demise[a]+=(int16_t)n;necro_note_debuff(s,a,NDebuff::Demise);necro_debuff_applied(s,a,n);}break;
 case XOp::ExhaustHand:for(unsigned i=s.hand.count;i>0;i--)enqueue(s,EffectKind::ExhaustCard,s.hand.ids[i-1],0,true);break;
 case XOp::Strength:world_relic_gain_strength(s,a);break;
 case XOp::Dexterity:world_relic_gain_dexterity(s,a);break;
 case XOp::AfterPlay:after_play(s,a);break;
 case XOp::RandomHits:{
  if(b<=0||!alive(s))break;if(b>1)queue_extension(s,XOp::RandomHits,a,b-1,true);
  int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[n++]=i;
  if(n)enqueue(s,EffectKind::Damage,a,ids[random_below(s.rng_cards,n)],true);break;}
 case XOp::Potion:procure_potion(s,random_potion(s,true));break;
 case XOp::PlayerDamage:player_hp_loss(s,a,false,true);break;
 case XOp::Summon:necro_summon(s,a);break;
 case XOp::Forge:forge_blade(s,a);break;
 case XOp::ChannelDark:defect_channel_dark(s,a);break;
 case XOp::OrbsEnd:defect_orbs_end(s);break;
 case XOp::TurnEnd:
  if(world_shared_active(s)){world_stage(s,WorldStage::EndVeryEarly);break;}
  event_card_before_hand_end(s);
  queue_extension(s,XOp::AllCharactersFlush,0,0,true);
  relic_hook(s,RelicHook::TurnEnd);
  if(power_value(s,XPower::Plating)){notify_power_flash(s,-1,"PLATING_POWER");gain_unpowered_block(s,power_value(s,XPower::Plating));}
  if(power_value(s,XPower::Regen)){heal(s,power_value(s,XPower::Regen));gain_power(s,XPower::Regen,-1);}
  if(power_value(s,XPower::Ritual)){notify_power_flash(s,-1,"RITUAL_POWER");s.strength=(int16_t)min(9999,s.strength+power_value(s,XPower::Ritual));}
  for(int i=MaxBombs-1;i>=0;i--)if(x.bomb_turns[i]&&!--x.bomb_turns[i]){queue_extension(s,XOp::AllDamage,x.bomb_damage[i],0,true);x.bomb_damage[i]=0;}
  queue_extension(s,XOp::OrbsEnd,0,0,true);defect_turn_end(s);regent_turn_end(s);ironclad_turn_end(s);burn_hand(s);break;
 case XOp::EndDiscard:
  if(!alive(s)){victory(s);break;}
  for(unsigned i=0;i<s.hand.count;){int id=s.hand.ids[i];if(card_retains(s,s.cards[id],id)||silent_retains(s,id)||power_value(s,XPower::Retain)){++i;continue;}remove(s.hand,i);put(s,s.discard,id,Zone::Discard);}
  if(power_value(s,XPower::Retain))gain_power(s,XPower::Retain,-1);
  queue_extension(s,XOp::AllCharactersSideEnd,0,0,true);break;
 case XOp::EnemyStart:
  ironclad_player_side_end(s);silent_player_side_end(s);defect_player_side_end(s);regent_player_side_end(s);necro_player_side_end(s);event_card_player_side_end(s);
  if(s.phase==Phase::Dead)break;
  for(auto&v:x.turn_cost)v=0;
  for(auto&v:x.star_turn_free)v=0;
  s.strength=(int16_t)max(-9999,s.strength-s.temporary_strength);s.temporary_strength=0;
  s.dexterity=(int16_t)max(-9999,s.dexterity-s.temporary_dexterity);s.temporary_dexterity=0;
  s.no_draw=0;x.powers[(int)XPower::Duplicator]=0;x.enemy_side=1;
  if(x.skip_enemy){--x.skip_enemy;queue_extension(s,XOp::TurnStart,0,0,true);break;}
  silent_enemy_poison(s);
  if(!alive(s)){victory(s);break;}queue_extension(s,XOp::EnemyAct,0,0,true);break;
 case XOp::EnemyAct:next_enemy(s,a);break;
 case XOp::EnemyDone:
  if(world_shared_active(s)){world_stage(s,WorldStage::EnemyBeforeEndVeryEarly);break;}
  necro_enemy_side_end(s);
  for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp&&x.demise[t]){notify_power_flash(s,t,"DEMISE_POWER");int old=s.enemies[t].hp,n=min(old,x.demise[t]);s.enemies[t].hp-=n;
   notify_damage_result(s,t,0,x.demise[t],old,false,false,false);record(s,LogKind::LoseHp,n,t+1);}
  ironclad_enemy_side_end(s);regent_enemy_side_end(s);
  s.flame_barrier=0;if(s.no_block)--s.no_block;
  if(power_value(s,XPower::Intangible))gain_power(s,XPower::Intangible,-1);
  for(int i=0;i<s.enemy_count;i++){
   s.enemies[i].strength=(int16_t)min(9999,s.enemies[i].strength+x.restore_strength[i]);x.restore_strength[i]=0;
   necro_clear_temp_strength(s,i);
  }
  if(!alive(s)){victory(s);break;}queue_extension(s,XOp::TurnStart,0,0,true);break;
 case XOp::TurnStart:
  if(world_shared_active(s)){world_stage(s,WorldStage::PlayerStart);break;}
  if(s.turn>=20000){fail(s);break;}
  ++s.turn;x.enemy_side=0;
  {bool keep=ironclad_retains_block(s);if(!keep)keep=silent_keeps_block(s);
   s.block=(int16_t)min(9999,(keep?s.block:0)+s.block_next_turn);s.block_next_turn=0;}
  event_card_after_block_cleared(s);
  s.energy=(int16_t)min(999,3+ironclad_max_energy(s)+necro_max_energy(s));silent_turn_start(s);defect_turn_start(s);regent_turn_start(s);necro_turn_start(s);
  x.nonpower_started=0;x.returning=0;for(auto&v:x.turn_cost)v=0;for(auto&v:x.star_turn_free)v=0;for(auto&v:x.panache_count)v=0;
  relic_reset_turn_counters(s);if(s.block)record(s,LogKind::Block,s.block);
  if(power_value(s,XPower::Plating))gain_power(s,XPower::Plating,-1);
  if(s.demon_form){notify_power_flash(s,-1,"DEMON_FORM_POWER");s.strength=(int16_t)min(9999,s.strength+s.demon_form);record(s,LogKind::Strength,s.demon_form);}
  s.vigor=(int16_t)min(9999,s.vigor+power_value(s,XPower::PrepTime));
  relic_hook(s,RelicHook::TurnStart);
  for(unsigned id=0;id<s.card_count;id++)if(x.return_turn[id]&&x.return_turn[id]<=s.turn){
   x.return_turn[id]=0;
   if(s.zones[id]==Zone::Draw){if(s.hand.count<MaxHand)move_instance(s,id,s.draw,s.hand,Zone::Hand);else move_instance(s,id,s.draw,s.discard,Zone::Discard);}
   else if(s.zones[id]==Zone::Discard&&s.hand.count<MaxHand)move_instance(s,id,s.discard,s.hand,Zone::Hand);
   else if(s.zones[id]==Zone::Exhaust){bool room=s.hand.count<MaxHand;move_instance(s,id,s.exhaust,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard);}
  }
  queue_extension(s,XOp::AllCharactersDrawBefore,0,0,true);
  defect_after_energy_reset(s);regent_after_energy_reset(s);ironclad_turn_start(s);
  break;
 case XOp::TurnDraw:{
  queue_extension(s,XOp::TurnReady,0,0,true);int n=5+s.draw_next_turn+silent_hand_draw_count(s)+defect_hand_draw_count(s)+necro_hand_draw_count(s)+regent_hand_draw_bonus(s);s.draw_next_turn=0;
  if(power_value(s,XPower::Clarity)){++n;gain_power(s,XPower::Clarity,-1);}
  if(power_value(s,XPower::Radiance)){s.energy=(int16_t)min(999,s.energy+1);gain_power(s,XPower::Radiance,-1);}
  queue_extension(s,XOp::Draw,n,0,true);break;}
 case XOp::TurnReady:{
  // Auto-pre-play begins after the ordinary hand draw and turn-start powers.
  if(power_value(s,XPower::Mayhem))queue_extension(s,XOp::AutoTop,min(MaxCards,power_value(s,XPower::Mayhem)),0,true);
  regent_auto_preplay(s);event_card_auto_preplay(s);queue_extension(s,XOp::TurnRelics,0,0,true);record(s,LogKind::Turn,s.turn);
  unsigned count=0;while(count<MaxPowerInstances&&x.boulder[count])++count;
  unsigned prefix=power_value(s,XPower::Entropy)?x.entropy_boulder_prefix:count;
  if(prefix>count){fail(s);break;}
  for(unsigned i=count;i>prefix;i--)queue_extension(s,XOp::BoulderTick,(int)i-1,0,true);
  if(power_value(s,XPower::Entropy))queue_extension(s,XOp::Transform,power_value(s,XPower::Entropy),0,true);
  for(unsigned i=prefix;i>0;i--)queue_extension(s,XOp::BoulderTick,(int)i-1,0,true);
  ironclad_after_hand_draw(s);silent_turn_ready(s);defect_turn_ready(s);regent_after_hand_draw(s);necro_turn_ready(s);
  break;}
 case XOp::BoulderTick:{
  if(a<0||a>=MaxPowerInstances||!x.boulder[a]){fail(s);break;}
  for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,x.boulder[a]);
  x.boulder[a]=(int16_t)min(9999,x.boulder[a]+5);
  int total=0;for(auto amount:x.boulder)total+=amount;
  x.powers[(int)XPower::RollingBoulder]=(int16_t)min(9999,total);break;}
 case XOp::TurnRelics:relic_hook(s,RelicHook::AfterHandDraw);break;
 case XOp::PostPlayListener:post_play_listener(s,(unsigned)a);break;
 case XOp::AllCharactersBefore:before_current(s);break;
 case XOp::AllCharactersBody:queue_card_body(s);break;
 case XOp::AllCharactersLate:all_characters_late(s);break;
 case XOp::AllCharactersFlush:queue_extension(s,XOp::EndDiscard,0,0,true);event_card_hand_end(s);exhaust_ethereal_hand(s);break;
 case XOp::AllCharactersSideEnd:queue_extension(s,XOp::EnemyStart,0,0,true);defect_after_turn_end(s);break;
 case XOp::AllCharactersDrawBefore:
  queue_extension(s,XOp::TurnDraw,0,0,true);silent_before_hand_draw(s);defect_before_hand_draw(s);regent_before_hand_draw(s);necro_before_hand_draw(s);event_card_before_hand_draw(s);break;
 default:if(!event_card_effect(s,e)&&!enemy_effect(s,e)&&!ironclad_effect(s,e)&&!silent_effect(s,e)&&!defect_effect(s,e)&&!regent_effect(s,e)&&!necro_effect(s,e))fail(s);break;
 }
}
