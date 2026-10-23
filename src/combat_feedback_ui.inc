// EndTurn still resolves the authoritative State synchronously. This small
// view records settled effect values and releases them on the animation
// clock; it never copies State, invokes a command, or advances an RNG stream.
static HealthBarValues ui_health_values(const State&s,int actor){
 HealthBarValues v;
 if(actor==0){v.hp=s.hp;v.max_hp=s.max_hp;v.block=s.block;v.doom=s.necro.player_doom;}
 else if(actor==MaxEnemies+1){v.hp=s.expansion.osty_hp;v.max_hp=s.expansion.osty_max_hp;v.tracked_block=s.block;}
 else if(actor==int(CombatFeedback::Energy)){v.hp=s.energy;v.max_hp=max_energy(s);}
 else if(actor>0&&actor<=s.enemy_count){const int slot=actor-1;const auto&e=s.enemies[slot];
  v.hp=e.hp;v.max_hp=e.max_hp;v.block=e.block;v.doom=s.expansion.doom[slot];
  v.poison_damage=health_bar::poison_next_damage(e.poison,s.silent.powers[unsigned(SPower::Accelerant)]);
  v.infinite=enemy_infinite_hp(s,slot);
 }return v;
}
static uint32_t ui_feedback_identity(const State&s,unsigned actor){
 if(actor>0&&actor<=MaxEnemies){if(actor>unsigned(s.enemy_count))return 0;const auto&m=s.enemy_world.models[actor-1];return (uint32_t(m.combat_id)<<16)|(1+unsigned(m.kind));}
 return actor==0?1+unsigned(s.character):1;
}
static bool ui_feedback_bound(const App&a){const auto&f=a.combat_feedback;const auto&s=a.game;
 return f.active&&s.enemy_world.active&&f.seed==s.seed&&f.floor==s.floor&&f.combat==s.world.combat_count&&f.encounter==int(s.enemy_world.encounter);
}
static uint32_t ui_feedback_later(uint32_t a,uint32_t b){return int32_t(a-b)>0?a:b;}
HealthBarValues App::combat_health(int actor)const{
 if(actor<0||actor>=int(CombatFeedback::Actors))return {};
 if(!ui_feedback_bound(*this))return ui_health_values(game,actor);
 // Identity travels with each sample. A reused slot keeps the old actor's
 // displayed values until its replacement cue, then switches atomically.
 return combat_feedback.shown[actor];
}
int App::combat_energy(bool maximum)const{const auto v=combat_health(CombatFeedback::Energy);return maximum?v.max_hp:v.hp;}
bool App::combat_actions_locked()const{
 return busy>0||(ui_feedback_bound(*this)&&(!combat_feedback.finished||int32_t(combat_feedback.end_at-animation)>0))||int32_t(combat_linger_until-animation)>0;
}
bool App::combat_choice_ready()const{
 if(!choice_phase(game.phase))return true;
 if(int32_t(combat_linger_until-animation)>0)return false;
 const auto&f=combat_feedback;
 return !ui_feedback_bound(*this)||!f.choice_pending||int32_t(animation-f.choice_at)>=0;
}
void App::continue_combat_choice(){
 if(!ui_feedback_bound(*this)||!combat_feedback.choice_pending||!combat_choice_ready())return;
 auto&f=combat_feedback;
 // Original selection commands await the player's answer. Already playing
 // animation tails keep running, but the unexecuted command continuation
 // starts from that answer, not from its old, now elapsed presentation time.
 const uint32_t waited=int32_t(animation-f.choice_at)>0?animation-f.choice_at:0;
 f.effect_at=ui_feedback_later(f.effect_at,f.choice_at)+waited;
 f.end_at=ui_feedback_later(f.end_at,f.choice_at)+waited;
 enemy_visual_clock=ui_feedback_later(enemy_visual_clock,f.choice_at)+waited;
 if(f.move_slot>=0){f.move_at+=waited;f.move_end=ui_feedback_later(f.move_end,f.choice_at)+waited;}
 // Keep the barrier armed until an observed effect leaves this choice.
 // A rejected core selection is still answerable; another attempt only
 // accounts for time since this attempt, never the entire wait twice.
 f.choice_at=animation;
}
EnemyIntent App::combat_intent(int slot)const{
 if(slot<0||slot>=game.enemy_count)return {};
 if(!ui_feedback_bound(*this))return enemy_intent(game,slot);
 const auto&f=combat_feedback;
 if(combat_health(slot+1).hp<=0||((f.hidden&(1u<<slot))&&int32_t(animation-f.hide_at[slot])>=0))return {};
 return f.intents[slot];
}
void App::begin_combat_feedback(){
 combat_feedback.reset();if(!game.enemy_world.active||game.phase!=Phase::Combat)return;
 sync_enemy_feedback();auto&f=combat_feedback;f.active=true;f.seed=game.seed;f.floor=game.floor;f.combat=game.world.combat_count;f.encounter=int(game.enemy_world.encounter);
 f.effect_at=f.move_at=f.move_end=f.end_at=animation;
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor){f.shown[actor]=f.observed[actor]=ui_health_values(game,int(actor));f.identity[actor]=f.observed_identity[actor]=ui_feedback_identity(game,actor);}
 for(int i=0;i<MaxEnemies;++i){f.intents[i]=i<game.enemy_count?enemy_intent(game,i):EnemyIntent{};f.hide_at[i]=animation;}
}
void App::combat_feedback_move(int slot,int move,uint32_t start,uint32_t action,uint32_t end){
 if(!ui_feedback_bound(*this)||slot<0||slot>=game.enemy_count)return;auto&f=combat_feedback;
 f.move_slot=slot;f.move=move;f.move_at=action;f.move_end=end;
 f.effect_at=ui_feedback_later(f.effect_at,action);f.end_at=ui_feedback_later(f.end_at,end);
 f.hidden|=uint8_t(1u<<slot);f.hide_at[slot]=start;
}
void App::observe_combat_effect(const State&s,Effect effect){
 if(&s!=&game||!ui_feedback_bound(*this))return;auto&f=combat_feedback;
 const XOp op=effect.kind==EffectKind::Extended?XOp(effect.target):XOp::Count;
 const EnemyId kind=f.move_slot>=0&&f.move_slot<s.enemy_count?s.enemy_world.models[f.move_slot].kind:EnemyId::Count;
 const bool nibbit=kind==EnemyId::Nibbit,louse=kind==EnemyId::LouseProgenitor;
 const auto wait=[&](unsigned normal,unsigned fast){return TurnBannerPresentation::wait_ticks(normal,fast,fast_mode);};
 uint32_t after=0;
 if(op==XOp::EnemyMoveFinish){f.effect_at=ui_feedback_later(f.effect_at,ui_feedback_later(f.move_end,enemy_visual_clock));f.move_slot=-1;}
 else if(op==XOp::EnemyHit&&effect.aux%8==f.move_slot&&(nibbit||louse)){
  // WithAttackerAnim delay is halved in Fast. Damage's post-hit wait belongs
  // to the command tail, not to the visible HP/block impact.
  const unsigned delay=nibbit?150:200;
  f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(delay,delay/2));
 }else if(op==XOp::EnemyAttackEnd&&effect.value==f.move_slot&&(nibbit||louse)){
  f.effect_at=ui_feedback_later(f.effect_at,f.move_at+(nibbit?wait(350,175):wait(400,200)));
 }else if(op==XOp::EnemyGainBlock&&effect.aux%8==f.move_slot){
  if(nibbit&&f.move==1){f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(350,175));after=f.move_at+wait(600,275);}
  else if(louse&&f.move==2){f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(250,125));after=f.move_at+wait(500,225);}
 }else if(op==XOp::EnemyApplyPower&&effect.aux/720-1==f.move_slot){
  if(nibbit&&f.move==2){f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(600,250));after=f.move_at+wait(850,350);}
  else if(louse&&f.move==2){f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(500,225));after=f.move_at+wait(750,325);}
  else if(louse&&f.move==0){f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(400,200));after=f.move_at+wait(650,300);}
 }else if(op==XOp::EnemyKnowledgeChoice&&effect.value==f.move_slot&&kind==EnemyId::KnowledgeDemon){
  // CurseOfKnowledgeMove (0x0600a324) awaits TriggerAnim(...,1.0)
  // before ChooseCurse. TriggerAnim (0x0600b31b) selects 1.0s Normal
  // or min(wait*.5,.25)=.25s Fast. Other Knowledge move waits retain
  // their existing fallback; this is the audited selection boundary only.
  f.effect_at=ui_feedback_later(f.effect_at,f.move_at+wait(1000,250));
 }
 // Unreviewed models use their existing move-start cue. This deliberately
 // makes no claim about uninspected attack delays or multi-hit intervals.
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)f.append(actor,ui_health_values(s,int(actor)),ui_feedback_identity(s,actor),f.effect_at);
 // Round cumulative command time once, rather than adding rounded waits.
 // Otherwise Fast's 175ms attack +100ms block drifts one 33ms tick later.
 if(after)f.effect_at=ui_feedback_later(f.effect_at,after);
 f.end_at=ui_feedback_later(f.end_at,f.effect_at);
 // A choice is a barrier in this same command stream, not a request to
 // display the synchronously advanced phase before its preceding effects.
 // Re-observing also handles automatic choices which never reach the UI.
 f.choice_pending=choice_phase(s.phase);
 if(f.choice_pending){f.choice_at=ui_feedback_later(f.effect_at,animation);f.finished=false;}
}
void App::finish_combat_feedback(){
 if(!combat_feedback.active)return;
 if(!ui_feedback_bound(*this)||game.phase==Phase::Error){combat_feedback.reset();return;}
 auto&f=combat_feedback;
 // A saved choice may suspend a hook in the middle of the same side. Keep
 // observing its continuation when the player completes that choice.
 if(choice_phase(game.phase)){
  if(!f.choice_pending){f.choice_pending=true;f.choice_at=ui_feedback_later(f.effect_at,animation);}
  return;
 }
 f.choice_pending=false;
 if(game.effect_count||game.playing>=0)return;
 f.end_at=ui_feedback_later(f.end_at,enemy_visual_clock);f.effect_at=f.end_at;
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)f.append(actor,ui_health_values(game,int(actor)),ui_feedback_identity(game,actor),f.end_at);
 f.finished=true;
 if(!combat_phase(game.phase))combat_linger_until=ui_feedback_later(combat_linger_until,f.end_at+1);
}
static void ui_feedback_apply(App&a,unsigned actor,const HealthBarValues&value,uint32_t identity,uint32_t at){
 auto&f=a.combat_feedback;const auto old=f.shown[actor];const bool same=f.identity[actor]==identity;
 f.shown[actor]=value;f.identity[actor]=identity;
 // An intent belongs to the retained creature, not its reusable array slot.
 // At the replacement cue, stop showing that creature's old intent. The
 // ordinary trace endpoint still reveals the new occupant's canonical one.
 if(!same&&actor>0&&actor<=MaxEnemies)f.intents[actor-1]={};
 if(same&&old.hp>value.hp&&actor<=MaxEnemies){
  a.last_hit[actor]=old.hp-value.hp;a.hit_start[actor]=int(at);
  if(actor==0)a.motion[0]={AnimHeroHurt,at};
  else if(value.hp>0&&int32_t(a.animation-at)<24)a.enemy_trigger(int(actor)-1,"Hit");
 }
}
void App::sync_combat_feedback(){
 auto&f=combat_feedback;if(!f.active)return;
 if(title||!ui_feedback_bound(*this)||game.phase==Phase::Error){f.reset();return;}
 while(f.head<f.count&&int32_t(animation-f.cues[f.head].at)>=0){const auto cue=f.cues[f.head++];ui_feedback_apply(*this,cue.actor,cue.value,cue.identity,cue.at);}
 if(f.head==f.count)f.head=f.count=0;
 if(f.finished&&int32_t(animation-f.end_at)>=0){
  // Overflow only drops intermediate presentation entries. At the scheduled
  // side boundary every displayed value converges to the captured result.
  for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)ui_feedback_apply(*this,actor,f.observed[actor],f.observed_identity[actor],f.end_at);
  f.reset();
 }
}
