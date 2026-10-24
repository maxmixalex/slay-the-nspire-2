// The original turn manager awaits each hook collection. These small saved
// operations provide the same suspension boundary without native coroutines.
static bool world_shared_active(const State&s){return s.world.active||s.enemy_world.active||s.world_relics.active;}
static void world_stage(State&s,WorldStage op,int value,bool first){queue_extension(s,XOp::WorldStage,int(op),value,first);}
bool world_effect_clears_combat_powers(Effect e){return e.kind==EffectKind::Extended&&XOp(e.target)==XOp::WorldStage&&e.value==int(WorldStage::VictoryPowerClear);}
static bool world_victory_pending(const State&s){
 for(unsigned i=0;i<s.effect_count;i++){const auto&e=s.effects[i];if(e.kind==EffectKind::Extended&&e.target==int(XOp::WorldStage)&&
  ((e.value>=int(WorldStage::VictoryEarly)&&e.value<=int(WorldStage::VictoryCleanup))||e.value==int(WorldStage::VictoryPlayerEnd)||e.value==int(WorldStage::VictoryPowerClear)))return true;}return false;
}
static void world_clear_player_powers(State&s){
 power_order_clear_actor(s,0);
 // Player.AfterCombatEnd removes powers without invoking their removal hooks.
 // Combat card instances remain owned until the final cleanup, so a saved
 // callback never leaves a Limbo reservation or a permanent-deck binding lost.
 s.block=s.strength=s.dexterity=s.pain=s.embrace=0;s.power_count=s.double_attack=s.no_draw=0;
 for(auto&v:s.power_order)v=0;
 s.temporary_strength=s.temporary_dexterity=s.vigor=s.attack_vigor=s.thorns=0;
 s.noxious_fumes=s.accuracy=s.no_block=s.demon_form=s.infinite_blades=0;
 s.block_next_turn=s.draw_next_turn=s.flame_barrier=0;
 auto clear=[](auto&v){for(auto&n:v)n={};};
 auto powers=[&](auto&v){clear(v.powers);clear(v.power_order);v.power_count=0;};
 powers(s.ironclad);powers(s.silent);powers(s.defect);powers(s.regent);powers(s.necro);powers(s.event_cards);
 s.ironclad.setup_strength=0;s.defect.temporary_focus=0;s.regent.pale_activated=0;s.necro.player_doom=0;s.necro.doom_applied_turn=0;
 clear(s.silent.nightmare_active);clear(s.silent.nightmares);
 clear(s.regent.orbit);clear(s.regent.orbit_remainder);clear(s.regent.monologue);clear(s.regent.monologue_applied);
 for(auto&row:s.regent.monologue_capture)clear(row);
 clear(s.event_cards.toric_block);clear(s.event_cards.toric_turns);
 auto&x=s.expansion;clear(x.powers);clear(x.after_play_order);clear(x.result_order);clear(x.automation);clear(x.automation_count);
 clear(x.panache);clear(x.panache_count);clear(x.panache_skip);clear(x.boulder);x.entropy_boulder_prefix=0;
 auto&e=s.enemy_world;clear(e.player_power);clear(e.player_aux);clear(e.player_power_order);clear(e.player_duration_skip);clear(e.player_source);e.player_power_count=0;
}
static void world_card_entered(State&s,int id){if(combat_phase(s.phase)&&s.turn&&s.world_relics.active)world_relic_hook(s,WorldRelicHook::AfterCardEnteredCombat,id);}
static bool shared_spend_card_energy(State&s,int amount){
 if(s.phase!=Phase::Combat||s.hp<=0||amount<0||amount>s.energy)return false;
 s.energy-=int16_t(amount);s.defect.energy_spent_turn=uint16_t(min(9999,int(s.defect.energy_spent_turn)+amount));
 regent_energy_spent(s,amount);return true;
}
static bool shared_spend_card_stars(State&s,int amount){
 if(s.phase!=Phase::Combat||s.hp<=0||amount<0||amount>s.expansion.stars)return false;
 s.expansion.stars-=int16_t(amount);
 if(amount)world_stage(s,WorldStage::StarsSpent,amount);
 regent_stars_spent(s,amount);return true;
}
static void world_shared_effect(State&s,WorldStage op,int value){
 auto&x=s.expansion;
 auto next=[&](WorldStage to,int n=0){world_stage(s,to,n);};
 auto hook=[&](WorldRelicHook which,int a=0,int b=0,int c=0){world_relic_hook(s,which,a,b,c);};
 switch(op){
 case WorldStage::EnterRoom:
  // Finish queued creature construction callbacks after room entry and before
  // combat start. The initial roster is not an AddToCombat summon.
  world_stage(s,WorldStage::CombatStart,0,false);hook(WorldRelicHook::AfterRoomEntered,s.world.room_type);
  if(has_run_modifier(s,RunModifier::Terminal)){
   if(!s.world.event_combat_parent&&s.max_hp>1)player_lose_max_hp(s,1);
   gain_power(s,XPower::Plating,5);
  }
  if(has_run_modifier(s,RunModifier::Murderous)){
   world_relic_gain_strength(s,3);
   for(unsigned i=0;i<s.enemy_count;++i)if(s.enemies[i].hp>0)enemy_apply_power(s,int(i),EnemyPower::Strength,3);
  }
  break;
 case WorldStage::CombatStart:next(WorldStage::CombatStartLate);hook(WorldRelicHook::BeforeCombatStart);break;
 case WorldStage::CombatStartLate:next(WorldStage::PlayerStart,1);hook(WorldRelicHook::BeforeCombatStartLate);break;
 case WorldStage::PlayerStart:
  if(value!=1){if(s.turn>=20000){fail(s);break;}++s.turn;}
  x.enemy_side=0;x.nonpower_started=0;x.returning=0;
  for(auto&n:x.turn_cost)n=0;for(auto&n:x.star_turn_free)n=0;for(auto&n:x.panache_count)n=0;
  next(WorldStage::PlayerEnemyStart,value==2?2:0);hook(WorldRelicHook::BeforeSideTurnStart,0);break;
 case WorldStage::PlayerEnemyStart:
  next(WorldStage::PlayerClearBlock);if(s.enemy_world.active)queue_extension(s,XOp::EnemySideHook,value==2?2:1,0,true);break;
 case WorldStage::PlayerClearBlock:{
  if(s.turn>1){bool keep=ironclad_retains_block(s);if(!keep)keep=silent_keeps_block(s);if(!keep)keep=wr_retain_block(s);if(!keep)s.block=0;}
  next(WorldStage::PlayerBlockRelics);
  if(s.block_next_turn){int n=s.block_next_turn;s.block_next_turn=0;gain_unpowered_block(s,n);}
  wr_power_after_block_clear(s);event_card_after_block_cleared(s);break;}
 case WorldStage::PlayerBlockRelics:next(WorldStage::PlayerBlockCards);hook(WorldRelicHook::AfterBlockCleared,-1);break;
 case WorldStage::PlayerBlockCards:next(WorldStage::PlayerEnergy);break;
 case WorldStage::PlayerEnergy:{
  const int maximum=wr_max_energy(s,3+ironclad_max_energy(s)+necro_max_energy(s));
  s.energy=(int16_t)min(999,(wr_reset_energy(s)?0:s.energy)+maximum);
  silent_turn_start(s);defect_turn_start(s);regent_turn_start(s);necro_turn_start(s);
  s.attacked_this_turn=0;relic_reset_turn_counters(s);
  if(s.turn>1&&power_value(s,XPower::Plating))gain_power(s,XPower::Plating,-1);
  if(s.demon_form){notify_power_flash(s,-1,"DEMON_FORM_POWER");world_relic_gain_strength(s,s.demon_form);}
  s.vigor=(int16_t)min(9999,s.vigor+power_value(s,XPower::PrepTime));power_order_set(s,0,2,s.vigor!=0);
  for(unsigned id=0;id<s.card_count;id++)if(x.return_turn[id]&&x.return_turn[id]<=s.turn){
   x.return_turn[id]=0;
   if(s.zones[id]==Zone::Draw){bool hand=s.hand.count<MaxHand;move_instance(s,id,s.draw,hand?s.hand:s.discard,hand?Zone::Hand:Zone::Discard);}
   else if(s.zones[id]==Zone::Discard&&s.hand.count<MaxHand)move_instance(s,id,s.discard,s.hand,Zone::Hand);
   else if(s.zones[id]==Zone::Exhaust){bool hand=s.hand.count<MaxHand;move_instance(s,id,s.exhaust,hand?s.hand:s.discard,hand?Zone::Hand:Zone::Discard);}
  }
  next(WorldStage::PlayerEnergyRelics);defect_after_energy_reset(s);regent_after_energy_reset(s);ironclad_turn_start(s);break;}
 case WorldStage::PlayerEnergyRelics:next(WorldStage::PlayerEnergyLate);hook(WorldRelicHook::AfterEnergyReset);break;
 case WorldStage::PlayerEnergyLate:next(WorldStage::PlayerBeforeDraw);hook(WorldRelicHook::AfterEnergyResetLate);break;
 case WorldStage::PlayerBeforeDraw:
  next(WorldStage::PlayerBeforeDrawRelics);silent_before_hand_draw(s);defect_before_hand_draw(s);regent_before_hand_draw(s);necro_before_hand_draw(s);event_card_before_hand_draw(s);break;
 case WorldStage::PlayerBeforeDrawRelics:next(WorldStage::PlayerBeforeDrawCards);hook(WorldRelicHook::BeforeHandDraw);break;
 case WorldStage::PlayerBeforeDrawCards:next(WorldStage::PlayerDraw);break;
 case WorldStage::PlayerDraw:{
  int n=5+s.draw_next_turn+silent_hand_draw_count(s)+defect_hand_draw_count(s)+necro_hand_draw_count(s)+regent_hand_draw_bonus(s);s.draw_next_turn=0;
  if(power_value(s,XPower::Clarity)){++n;gain_power(s,XPower::Clarity,-1);}
  if(power_value(s,XPower::Radiance)){s.energy=(int16_t)min(999,s.energy+1);gain_power(s,XPower::Radiance,-1);}
  n=wr_hand_draw(s,n,true);if(s.turn==1)n=max(n,prioritize_innate(s));n=min(MaxHand,n);
  next(WorldStage::PlayerAfterDraw);queue_extension(s,XOp::Draw,n,1,true);break;}
 case WorldStage::PlayerAfterDraw:{
  next(WorldStage::PlayerAfterDrawRelics);
  unsigned count=0;while(count<MaxPowerInstances&&x.boulder[count])++count;
  unsigned prefix=power_value(s,XPower::Entropy)?x.entropy_boulder_prefix:count;if(prefix>count){fail(s);break;}
  for(unsigned i=count;i>prefix;--i)queue_extension(s,XOp::BoulderTick,int(i)-1,0,true);
  if(power_value(s,XPower::Entropy))queue_extension(s,XOp::Transform,power_value(s,XPower::Entropy),0,true);
  for(unsigned i=prefix;i>0;--i)queue_extension(s,XOp::BoulderTick,int(i)-1,0,true);
  ironclad_after_hand_draw(s);silent_turn_ready(s);defect_turn_ready(s);regent_after_hand_draw(s);necro_turn_ready(s);break;}
 case WorldStage::PlayerAfterDrawRelics:next(WorldStage::PlayerAfterDrawLate);hook(WorldRelicHook::AfterHandDraw);break;
 case WorldStage::PlayerAfterDrawLate:next(WorldStage::PlayerAfterSide);hook(WorldRelicHook::AfterHandDrawLate);break;
 case WorldStage::PlayerAfterSide:next(WorldStage::PlayerAuto);hook(WorldRelicHook::AfterSideTurnStart,0);break;
 case WorldStage::PlayerAuto:
  next(WorldStage::PlayerAutoRelics);if(power_value(s,XPower::Mayhem))queue_extension(s,XOp::AutoTop,min(MaxCards,power_value(s,XPower::Mayhem)),0,true);regent_auto_preplay(s);
  if(!s.hand.count)hook(WorldRelicHook::AfterHandEmptied);break;
 case WorldStage::PlayerAutoRelics:next(WorldStage::PlayerAutoCards);hook(WorldRelicHook::AfterAutoPrePlay);break;
 case WorldStage::PlayerAutoCards:next(WorldStage::PlayerAutoLate);event_card_auto_preplay(s);break;
 case WorldStage::PlayerAutoLate:next(WorldStage::PlayerReady);hook(WorldRelicHook::AfterAutoPrePlayLate);break;
 case WorldStage::PlayerReady:record(s,LogKind::Turn,s.turn);if(!alive(s))victory(s);break;
 case WorldStage::EndVeryEarly:next(WorldStage::EndEarly);hook(WorldRelicHook::BeforeSideTurnEndVeryEarly,0);break;
 case WorldStage::EndEarly:next(WorldStage::EndPowers);hook(WorldRelicHook::BeforeSideTurnEndEarly,0);break;
 case WorldStage::EndPowers:
  next(WorldStage::EndRelics);
  if(power_value(s,XPower::Plating)){notify_power_flash(s,-1,"PLATING_POWER");gain_unpowered_block(s,power_value(s,XPower::Plating));}
  if(power_value(s,XPower::Regen)){heal(s,power_value(s,XPower::Regen));gain_power(s,XPower::Regen,-1);}
  if(power_value(s,XPower::Ritual)){notify_power_flash(s,-1,"RITUAL_POWER");world_relic_gain_strength(s,power_value(s,XPower::Ritual));}
  for(int i=MaxBombs-1;i>=0;--i)if(x.bomb_turns[i]&&!--x.bomb_turns[i]){queue_extension(s,XOp::AllDamage,x.bomb_damage[i],0,true);x.bomb_damage[i]=0;}
  defect_turn_end(s);regent_turn_end(s);ironclad_turn_end(s);break;
 case WorldStage::EndRelics:next(WorldStage::EndOrbs);hook(WorldRelicHook::BeforeSideTurnEnd,0);break;
 case WorldStage::EndOrbs:
  event_card_before_hand_end(s);next(WorldStage::EndHand);exhaust_ethereal_hand(s);defect_orbs_end(s);break;
 case WorldStage::EndHand:next(WorldStage::EndFlush);event_card_hand_end(s);burn_hand(s);break;
 case WorldStage::EndFlush:
  if(!alive(s)){victory(s);break;}
  next(WorldStage::EndAfterFlush);
  if(wr_flush(s))for(unsigned i=0;i<s.hand.count;){int id=s.hand.ids[i];if(card_retains(s,s.cards[id],id)||silent_retains(s,id)||power_value(s,XPower::Retain)){++i;continue;}remove(s.hand,int(i));put(s,s.discard,id,Zone::Discard);}
  if(power_value(s,XPower::Retain))gain_power(s,XPower::Retain,-1);break;
 case WorldStage::EndAfterFlush:next(WorldStage::EndSidePowers);hook(WorldRelicHook::AfterFlush);break;
 case WorldStage::EndSidePowers:
  next(WorldStage::EndSideRelics);defect_after_turn_end(s);wr_power_side_end(s,0);
  ironclad_player_side_end(s);silent_player_side_end(s);defect_player_side_end(s);regent_player_side_end(s);necro_player_side_end(s);event_card_player_side_end(s);break;
 case WorldStage::EndSideRelics:next(WorldStage::EndSideEnemies);hook(WorldRelicHook::AfterSideTurnEnd,0);break;
 case WorldStage::EndSideEnemies:next(WorldStage::SwitchEnemy);if(s.enemy_world.active)queue_extension(s,XOp::EnemySideHook,1,1,true);break;
 case WorldStage::SwitchEnemy:
  if(s.phase!=Phase::Combat)break;
  for(auto&n:x.turn_cost)n=0;for(auto&n:x.star_turn_free)n=0;
  world_relic_gain_strength(s,-s.temporary_strength);s.temporary_strength=0;world_relic_gain_dexterity(s,-s.temporary_dexterity);s.temporary_dexterity=0;
  s.no_draw=0;x.powers[(int)XPower::Duplicator]=0;
  if(x.skip_enemy){--x.skip_enemy;next(WorldStage::PlayerStart,2);break;}
  if(wr_extra_turn(s,true)){next(WorldStage::PlayerStart,2);break;}
  x.enemy_side=1;next(WorldStage::EnemyBeforeStart);break;
 case WorldStage::EnemyBeforeStart:next(WorldStage::EnemyAfterStart);hook(WorldRelicHook::BeforeSideTurnStart,1);break;
 case WorldStage::EnemyAfterStart:next(WorldStage::EnemyActBegin);if(s.enemy_world.active)queue_extension(s,XOp::EnemySideHook,0,0,true);silent_enemy_poison(s);break;
 case WorldStage::EnemyActBegin:
  if(!alive(s)){victory(s);break;}queue_extension(s,XOp::EnemyAct,0,0,true);hook(WorldRelicHook::AfterSideTurnStart,1);break;
 case WorldStage::EnemyBeforeEndVeryEarly:next(WorldStage::EnemyBeforeEndEarly);hook(WorldRelicHook::BeforeSideTurnEndVeryEarly,1);break;
 case WorldStage::EnemyBeforeEndEarly:next(WorldStage::EnemyBeforeEnd);hook(WorldRelicHook::BeforeSideTurnEndEarly,1);break;
 case WorldStage::EnemyBeforeEnd:next(WorldStage::EnemyEndPowers);hook(WorldRelicHook::BeforeSideTurnEnd,1);break;
 case WorldStage::EnemyEndPowers:{
  next(WorldStage::EnemyEndRelics);
  int doom_deaths=0;for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp>0&&x.doom[t]>=s.enemies[t].hp&&x.doom[t]&&enemy_death_is_fatal(s,t))++doom_deaths;
  necro_enemy_side_end(s);if(doom_deaths)hook(WorldRelicHook::AfterDiedToDoom,doom_deaths);
  for(int t=0;t<s.enemy_count;t++)if(s.enemies[t].hp&&x.demise[t]){notify_power_flash(s,t,"DEMISE_POWER");int old=s.enemies[t].hp,n=min(old,x.demise[t]);s.enemies[t].hp-=(int16_t)n;
   notify_damage_result(s,t,0,x.demise[t],old,false,false,false);record(s,LogKind::LoseHp,n,t+1);}
  ironclad_enemy_side_end(s);regent_enemy_side_end(s);wr_power_side_end(s,1);
  s.flame_barrier=0;if(s.no_block)--s.no_block;if(power_value(s,XPower::Intangible))gain_power(s,XPower::Intangible,-1);
  for(int i=0;i<s.enemy_count;i++){s.enemies[i].strength=(int16_t)min(9999,s.enemies[i].strength+x.restore_strength[i]);x.restore_strength[i]=0;necro_clear_temp_strength(s,i);}break;}
 case WorldStage::EnemyEndRelics:next(WorldStage::EnemyAfterEnd);hook(WorldRelicHook::AfterSideTurnEnd,1);break;
 case WorldStage::EnemyAfterEnd:next(WorldStage::NextPlayer);if(s.enemy_world.active)queue_extension(s,XOp::EnemySideHook,0,1,true);break;
 case WorldStage::NextPlayer:if(!alive(s))victory(s);else next(WorldStage::PlayerStart);break;
 case WorldStage::VictoryEarly:next(WorldStage::VictoryNormal);hook(WorldRelicHook::AfterCombatVictoryEarly);break;
 case WorldStage::VictoryNormal:next(WorldStage::VictoryCleanup);hook(WorldRelicHook::AfterCombatVictory);break;
 case WorldStage::VictoryEnd:next(WorldStage::VictoryPowerClear);hook(WorldRelicHook::AfterCombatEnd);break;
 case WorldStage::VictoryPlayerEnd:
  // RunState dispatches permanent deck cards, then player powers, before
  // the creature's relic listeners. All still see the pre-clear power state.
  next(WorldStage::VictoryEnd);
  ironclad_combat_end(s);silent_combat_end(s);defect_combat_end(s);regent_combat_end(s,true);necro_combat_end(s);event_card_combat_end(s,true);break;
 case WorldStage::VictoryPowerClear:world_clear_player_powers(s);next(WorldStage::VictoryEarly);break;
 case WorldStage::VictoryCleanup:victory_cleanup(s);break;
 case WorldStage::BeforeCardRelics:
  next(WorldStage::BeforeCardCards);if(s.playing>=0)hook(WorldRelicHook::BeforeCardPlayed,s.playing,s.defect.spent[x.depth],s.regent.stars_spent[x.depth]|(s.regent.automatic[x.depth]?0x10000:0));break;
 case WorldStage::BeforeCardCards:before_current_cards(s);break;
 case WorldStage::DrawRelics:next(WorldStage::DrawCards,value);hook(WorldRelicHook::AfterCardDrawn,value);break;
 case WorldStage::DrawCards:next(WorldStage::DrawEnemies,value);event_card_after_draw(s,value);break;
 case WorldStage::DrawEnemies:enemy_card_drawn(s,value);break;
 case WorldStage::StarsSpent:hook(WorldRelicHook::AfterStarsSpent,value);break;
 case WorldStage::HandEmptied:if(!s.hand.count&&s.playing<0&&!x.depth&&!x.potion_active&&!x.enemy_side)hook(WorldRelicHook::AfterHandEmptied);break;
 case WorldStage::OrbPassive:defect_orb_passive(s,unsigned(value)%MaxOrbs,value/MaxOrbs-1);break;
 case WorldStage::PlayerDeath:if(value||!revive(s))death_cleanup(s);break;
 case WorldStage::AutoPotionUsed:hook(WorldRelicHook::AfterPotionUsed,value,-1);break;
 case WorldStage::SetMaxHp:s.max_hp=int16_t(value);s.hp=int16_t(min(s.hp,s.max_hp));break;
 case WorldStage::GambitKill:
  if(power_value(s,XPower::Gambit)){x.powers[int(XPower::Gambit)]=0;player_kill(s);}break;
 default:fail(s);break;
 }
}
