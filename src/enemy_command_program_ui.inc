// Source awaits are advanced by real command notifications. Skeleton
// durations only advance skeletons; they cannot invent a gameplay wait.
static void ui_enemy_program_wait(App&a,unsigned milliseconds){
 if(ui_combat_ending(a.game))return;
 auto&f=a.combat_feedback;auto&p=f.commands;
 if(p.active)f.effect_at=p.wait(f.effect_at,milliseconds);
 else f.effect_at+=(milliseconds+32u)/33u;
 f.end_at=ui_feedback_later(f.end_at,f.effect_at);
 a.enemy_visual_clock=ui_feedback_later(a.enemy_visual_clock,f.effect_at);
}
static bool ui_enemy_program_condition(const App&a,const enemy_command_program::Step&step){
 using namespace enemy_command_program;const auto&f=a.combat_feedback;
 if(f.move_slot<0||f.move_slot>=a.game.enemy_count)return false;
 const auto&m=a.game.enemy_world.models[f.move_slot];
 if((step.flags&Alive)&&a.game.enemies[f.move_slot].hp<=0)return false;
 if((step.flags&FirstSpeech)&&!f.commands.first_speech)return false;
 if((step.flags&RespawnOne)&&m.data[0]!=1)return false;
 if((step.flags&RespawnTwo)&&m.data[0]!=2)return false;
 if((step.flags&OffBalance)&&!m.data[0])return false;
 if(step.flags&HasSlot){
  bool open=false;
  for(unsigned name=0;name<a.game.enemy_world.slot_limit&&!open;++name){
   bool occupied=false;for(int t=0;t<a.game.enemy_count;++t){const auto&other=a.game.enemy_world.models[t];
    if((other.flags&EnemyPresent)&&!(other.flags&EnemyRemoved)&&other.slot==name){occupied=true;break;}}
   open=!occupied;
  }
  if(!open)return false;
 }
 return true;
}
static bool ui_enemy_program_control(unsigned kind){
 using namespace enemy_command_program;
 return kind==Wait||kind==Trigger||kind==Direct||kind==Segments||kind==Scale;
}
static void ui_enemy_program_control(App&a,const enemy_command_program::Step&step){
 using namespace enemy_command_program;auto&f=a.combat_feedback;auto&p=f.commands;
 if(!ui_enemy_program_condition(a,step))return;
 p.align(f.effect_at);const unsigned slot=unsigned(f.move_slot);
 const auto kind=a.game.enemy_world.models[slot].kind;
 const unsigned wait=a.fast_mode?step.fast_ms:step.normal_ms;
 const uint32_t at=f.effect_at;
 if(step.kind==Wait){ui_enemy_program_wait(a,wait);return;}
 if(step.kind==Segments){
  for(int i=0;i<a.game.enemy_count;++i){const auto model=a.game.enemy_world.models[i].kind;
   if(model<EnemyId::DecimillipedeSegmentBack||model>EnemyId::DecimillipedeSegmentMiddle)continue;
   const auto facts=ui_enemy_facts(a.game,i);a.enemy_trigger(i,a.enemy_presentation[i].dead?"dead_loop":"idle_loop",f.move,true,&facts,at);
  }
  ui_enemy_program_wait(a,wait);return;
 }
 auto facts=ui_enemy_facts(a.game,int(slot));
 if(step.kind==Scale){a.enemy_trigger(int(slot),"DefaultScaleChanged",f.move,false,&facts,at);return;}
 const char*action=triggers[step.arg];
 if(step.kind==Trigger){
  // A source Uncurl await exists only when the animator was curled.
  int projected=p.state;const auto*old=world_enemy_state(unsigned(kind),projected);
  if(old&&!old->loop&&old->next>=0)projected=old->next;
  const int state=world_enemy_transition(unsigned(kind),projected,action,ui_enemy_fact,&facts);
  if((step.flags&Uncurl)&&state<0)return;
  if(state>=0)p.state=int16_t(state);
  if(kind==EnemyId::TestSubject&&f.move==0&&ui_visual_equal(action,"RespawnTrigger"))facts.default_scale=0;
 }
 a.enemy_trigger(int(slot),action,f.move,step.kind==Direct,&facts,at);
 unsigned scream=0;
 if((kind==EnemyId::DevotedSculptor&&f.move==0)||(kind==EnemyId::CeremonialBeast&&f.move==3))scream=300;
 else if(kind==EnemyId::TerrorEel&&f.move==3)scream=700;
 else if(kind==EnemyId::TheInsatiable&&f.move==0)scream=500;
 if(scream&&step.kind==Trigger)a.screen_effect_at(at+(scream+32u)/33u,ScreenShakePresentation::Punch,ScreenShakePresentation::Medium,800);
 ui_enemy_program_wait(a,wait);
}
static const enemy_command_program::Step*ui_enemy_program_command(App&a,unsigned kind,unsigned arg){
 using namespace enemy_command_program;auto&f=a.combat_feedback;auto&p=f.commands;if(!p.active)return nullptr;
 p.align(f.effect_at);unsigned found=p.end;
 for(unsigned i=p.cursor;i<p.end;++i){const auto&step=steps[i];if(!ui_enemy_program_condition(a,step)||step.kind!=kind)continue;
  if(kind==Power||kind==Remove||kind==Spawn||kind==Cards){if(step.arg!=65535u&&step.arg!=arg)continue;}
  found=i;break;
 }
 // Repeated foreach commands and nested hooks still await their own real
 // post-command delay; they must not consume a later unrelated marker.
 if(found==p.end)return nullptr;
 while(p.cursor<found){const auto&step=steps[p.cursor++];if(ui_enemy_program_control(step.kind))ui_enemy_program_control(a,step);}
 ++p.cursor;return &steps[found];
}
static void ui_enemy_program_finish(App&a){
 auto&f=a.combat_feedback;auto&p=f.commands;if(!p.active)return;
 if(ui_combat_ending(a.game)){p.cursor=p.end;p.active=p.card_batch=false;return;}
 p.align(f.effect_at);
 while(p.cursor<p.end){const auto&step=enemy_command_program::steps[p.cursor++];if(ui_enemy_program_control(step.kind))ui_enemy_program_control(a,step);}
 f.move_end=ui_feedback_later(f.move_end,f.effect_at);p.active=false;p.card_batch=false;
}
static void ui_enemy_command_capture(App&a){
 auto&f=a.combat_feedback;const auto&s=a.game;
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)f.append(actor,ui_health_values(s,int(actor)),ui_feedback_identity(s,actor),f.effect_at);
 ui_capture_combat_status(a,f.effect_at);
 f.hand.append(s.hand,f.effect_at);
}
void App::observe_enemy_command(const State&s,const CoreEnemyCommand&command,bool before){
 if(&s!=&game)return;
 if(command.kind==CoreEnemyCommand::Pause){
  if(before||ui_combat_ending(s))return;
  const unsigned wait=fast_mode?command.model:unsigned(command.amount);
  if(ui_feedback_bound(*this)){
   // A damage hook can await TriggerAnim before issuing another observed
   // command. Preserve the already-applied HP/block at this boundary;
   // otherwise its next RemovePower would incorrectly delay the hit.
   ui_enemy_command_capture(*this);ui_enemy_program_wait(*this,wait);
  }
  else if(s.enemy_world.active){
   // Entry relic hooks can awaken an enemy before a player action binds
   // CombatFeedback. Retain that explicit source await on the enemy clock.
   // This is TriggerAnim's requested wait, never the skeleton's duration.
   enemy_visual_clock=ui_feedback_later(enemy_visual_clock,animation)+(wait+32u)/33u;
  }
  return;
 }
 if(!ui_feedback_bound(*this))return;
 auto&f=combat_feedback;const auto operation=XOp(f.commands.operation);
 if(before){
  // Preserve any outer mutation before an inner command awaits a prefix.
  ui_enemy_command_capture(*this);
  unsigned kind=enemy_command_program::Power;bool primary=false;
  switch(command.kind){
  case CoreEnemyCommand::Power:kind=enemy_command_program::Power;primary=operation==XOp::EnemyApplyPower||
    (operation==XOp::EnemyActionSpecial&&command.source==f.move_slot);break;
  case CoreEnemyCommand::RemovePower:kind=enemy_command_program::Remove;primary=operation==XOp::EnemyRemovePower||
    (operation==XOp::EnemyActionSpecial&&f.move_slot>=0&&s.enemy_world.models[f.move_slot].kind==EnemyId::ToughEgg);break;
  case CoreEnemyCommand::Block:kind=enemy_command_program::Block;primary=operation==XOp::EnemyGainBlock;break;
  case CoreEnemyCommand::Heal:kind=enemy_command_program::Heal;primary=operation==XOp::EnemyHeal||operation==XOp::EnemyActionSpecial;break;
  case CoreEnemyCommand::Spawn:kind=enemy_command_program::Spawn;primary=operation==XOp::EnemySpawn||operation==XOp::EnemyActionSpecial;break;
  case CoreEnemyCommand::SetHp:kind=enemy_command_program::SetHp;primary=operation==XOp::EnemyActionSpecial;break;
  case CoreEnemyCommand::Steal:kind=enemy_command_program::Steal;primary=operation==XOp::EnemyActionSpecial;break;
  case CoreEnemyCommand::Pause:break;
  }
  if(primary)ui_enemy_program_command(*this,kind,command.model);
  return;
 }
 ui_enemy_command_capture(*this);
 unsigned delay=0;
 switch(command.kind){
 case CoreEnemyCommand::Power:delay=fast_mode?100u:250u;break;
 case CoreEnemyCommand::RemovePower:delay=fast_mode?200u:400u;break;
 case CoreEnemyCommand::Block:if(command.amount>0)delay=command.model?(fast_mode?0u:30u):(fast_mode?100u:250u);break;
 case CoreEnemyCommand::Heal:delay=fast_mode?100u:250u;break;
 default:break;
 }
 ui_enemy_program_wait(*this,delay);
}
