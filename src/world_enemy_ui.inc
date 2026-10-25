// The source simulation reports actual move starts, before their animator
// predicates change. These snapshots are presentation-only and never saved.
static bool ui_visual_equal(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static EnemyVisualFacts ui_enemy_facts(const State&s,int slot){EnemyVisualFacts f;
 const unsigned n=world_enemy_fact_count();for(unsigned i=0;i<n&&i<MaxEnemyVisualFacts;i++)if(enemy_visual_fact(s,slot,world_enemy_fact_name(i),f.values[i]))f.known|=uint64_t(1)<<i;return f;
}
static bool ui_enemy_fact(void* context,const char*key,int&value){const auto&f=*static_cast<const EnemyVisualFacts*>(context);
 for(unsigned i=0;i<world_enemy_fact_count()&&i<MaxEnemyVisualFacts;i++)if(ui_visual_equal(key,world_enemy_fact_name(i))){if(!(f.known&(uint64_t(1)<<i)))return false;value=f.values[i];return true;}return false;
}
static unsigned ui_enemy_skin(const State&s,int slot,unsigned model){
 const auto*v=world_enemy_visual(model);if(!v||!v->skin_count)return 0;
 // Appearance has its own stable presentation hash, never a gameplay draw.
 // Named original slots constrain the applicable skins before weighting.
 uint32_t hash=uint32_t(s.enemy_world.models[slot].combat_id)*2654435761u^s.seed^uint32_t(model)*2246822519u;unsigned total=0;
 for(unsigned i=0;i<v->skin_count;i++){const auto*k=world_enemy_skin(model,i);if(k&&(!k->slot_mask||(k->slot_mask&(1u<<s.enemy_world.models[slot].slot))))total+=k->weight;}
 if(!total)return 0;unsigned choice=hash%total;
 for(unsigned i=0;i<v->skin_count;i++){const auto*k=world_enemy_skin(model,i);if(!k|| (k->slot_mask&&!(k->slot_mask&(1u<<s.enemy_world.models[slot].slot))))continue;if(choice<k->weight)return i;choice-=k->weight;}return 0;
}
static uint32_t ui_enemy_ticks(const App&a,int clip){
 const auto*row=clip>=0?world_enemy_clip_directory(unsigned(clip)):nullptr;auto c=a.content.clip(clip);
 unsigned ms=row?row->duration_ms:uint32_t(c.count)*c.frame_ms;return (ms+32)/33? (ms+32)/33:1;
}
static uint32_t ui_enemy_move_wait(const App&a,EnemyId kind,int move){
 const auto audited=enemy_command_timing(kind,move,a.fast_mode);
 if(audited.effect!=EnemyCommandTiming::None)return (audited.total_ms+32u)/33u;
 // Audited command waits, not skeleton durations. AttackCommand waits for
 // its anim delay and Damage; Block/Power add their own waits. Keep this
 // deliberately scoped to the inspected solo moves. The shared table above
 // adds Leaf Slimes and Damp Cultist; other models retain their clip fallback.
 unsigned normal=0,fast=0;
 if(kind==EnemyId::Nibbit){
  switch(move){case 0:normal=350;fast=175;break;case 1:normal=600;fast=275;break;case 2:normal=850;fast=350;break;}
 }else if(kind==EnemyId::LouseProgenitor){
  switch(move){case 0:normal=650;fast=300;break;case 1:normal=400;fast=200;break;case 2:normal=750;fast=325;break;}
 }
 return normal?TurnBannerPresentation::wait_ticks(normal,fast,a.fast_mode):0;
}
static bool ui_enemy_death_fades(const State&s,int slot){
 const auto&m=s.enemy_world.models[slot];
 switch(m.kind){
  case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:
  case EnemyId::SoulNexus:case EnemyId::CeremonialBeast:case EnemyId::Rocket:case EnemyId::Fabricator:
  case EnemyId::Crusher:case EnemyId::LivingFog:case EnemyId::PhantasmalGardener:case EnemyId::GasBomb:return false;
  case EnemyId::WaterfallGiant:return m.data[2]==0;
  default:return true;
 }
}
static void ui_enemy_removal(App&a,int slot,bool restored=false){
 auto&m=a.enemy_presentation[slot];const auto&source=a.game.enemy_world.models[slot];
 // Downed/reviving actors remain in the battle. Removed actors finish their
 // death track, wait the original half second, then leave the scene.
 if(!(source.flags&EnemyRemoved)||a.combat_health(slot+1).hp>0){m.removing=false;return;}
 if(m.removing)return;m.removing=true;
 if(restored){m.fade_start=m.hide_at=a.animation;return;}
 uint32_t wait=m.clip>=0?uint32_t(int32_t(m.until-a.animation)>0?m.until-a.animation:0)+16u:0u;
 if(wait>607u)wait=607u; // AnimDie caps its animation wait at twenty seconds.
 if(m.clip<0&&(source.kind==EnemyId::Rocket||source.kind==EnemyId::Crusher))wait=76u;
 m.fade_start=a.animation+wait;
 // Original death dissolve is capped at 2.5 seconds. This bounded opacity
 // adapter removes the retained corpse without a second skeleton or shader.
 m.hide_at=m.fade_start+(ui_enemy_death_fades(a.game,slot)?76u:0u);
}
unsigned App::enemy_opacity(int slot)const{
 if(slot<0||slot>=game.enemy_count)return 0;
 if(ui_feedback_bound(*this)&&combat_feedback.identity[slot+1]!=ui_feedback_identity(game,unsigned(slot+1))&&combat_feedback.shown[slot+1].hp<=0)return 0;
 const auto&m=enemy_presentation[slot];if(!m.removing)return 255;
 if(int32_t(animation-m.hide_at)>=0)return 0;
 if(int32_t(animation-m.fade_start)<=0)return 255;
 const uint32_t duration=m.hide_at-m.fade_start;
 return duration?unsigned(((m.hide_at-animation)*255u)/duration):0u;
}
static bool ui_enemy_set_clip(App&a,int slot,int clip,const EnemyVisualFacts&facts,uint32_t start,bool loop){
 if(clip<0)return false;auto&m=a.enemy_presentation[slot];m.clip=clip;m.facts=facts;m.loop=loop;m.start=start;m.until=start+ui_enemy_ticks(a,clip);return true;
}
static bool ui_enemy_set_state(App&a,int slot,int state,const EnemyVisualFacts&facts,uint32_t start){
 auto&m=a.enemy_presentation[slot];const auto*node=world_enemy_state(m.model,state);if(!node)return false;
 int clip=world_enemy_resolve_clip(m.model,node->clip,m.skin,m.move,ui_enemy_fact,const_cast<EnemyVisualFacts*>(&facts));if(clip<0)return false;
 if(!ui_enemy_set_clip(a,slot,clip,facts,start,node->loop!=0))return false;m.state=state;m.next_clip=nullptr;m.direct=false;return true;
}
static void ui_enemy_advance(App&a,int slot){
 auto&m=a.enemy_presentation[slot];if(!m.identity)return;
 for(unsigned pass=0;pass<MaxEnemyVisualCommands+2;pass++){
  bool finished=m.clip<0||(!m.loop&&int32_t(a.animation-m.until)>=0);
  if(finished&&m.count&&!m.direct&&int32_t(a.animation-m.queue[0].earliest)>=0&&int32_t(m.queue[0].earliest-m.until)>=0){
   // CreatureAnimator.OnAnimationCompleted changes logical state before a
   // later trigger. In particular Louse's Uncurl -> Idle must settle before
   // its queued Attack (which is intentionally not an any-state trigger).
   const auto*state=world_enemy_state(m.model,m.state);
   if(state&&state->next>=0&&ui_enemy_set_state(a,slot,state->next,m.facts,m.until))continue;
  }
  if(m.count&&int32_t(a.animation-m.queue[0].earliest)>=0){
   EnemyVisualCommand command=m.queue[0];for(unsigned i=1;i<m.count;i++)m.queue[i-1]=m.queue[i];--m.count;m.move=command.move;
   int state=command.direct?-1:world_enemy_transition(m.model,m.state,command.action,ui_enemy_fact,&command.facts);
   // Original animator triggers interrupt an earlier hurt/spawn track. The
   // command's cue owns the clock, including when a render catches up later.
   if(state>=0&&ui_enemy_set_state(a,slot,state,command.facts,command.earliest))continue;
   if(command.direct){int clip=world_enemy_resolve_clip(m.model,command.action,m.skin,command.move,ui_enemy_fact,&command.facts);if(ui_enemy_set_clip(a,slot,clip,command.facts,command.earliest,false)){const auto*move=world_enemy_move(m.model,command.move);m.next_clip=move?move->next_clip:nullptr;m.direct=true;continue;}}
   continue; // Unregistered original trigger holds the current animation.
  }
  if(finished){
   if(m.next_clip){auto facts=ui_enemy_facts(a.game,slot);int clip=world_enemy_resolve_clip(m.model,m.next_clip,m.skin,-1,ui_enemy_fact,&facts);m.next_clip=nullptr;m.direct=false;m.move=-1;if(ui_enemy_set_clip(a,slot,clip,facts,m.until,true))continue;}
   if(m.direct)m.move=-1;
   const auto*state=world_enemy_state(m.model,m.state);if(state&&state->next>=0){if(ui_enemy_set_state(a,slot,state->next,m.facts,m.until))continue;}
   // Direct tracks keep the animator's logical state. Restore its settled
   // loop even when the state id is unchanged; otherwise a strike overlay
   // freezes the actor at its last frame. Detached segments retain dead_loop.
   if(m.direct){auto facts=ui_enemy_facts(a.game,slot);int next=m.dead?m.state:world_enemy_resume(m.model,ui_enemy_fact,&facts);m.move=-1;m.direct=false;
    const auto*resume=world_enemy_state(m.model,next);if(resume&&resume->loop&&ui_enemy_set_state(a,slot,next,facts,a.animation))continue;
   }
   // Terminal death states hold their last frame. Living actors may have a
   // changed saved idle predicate, e.g. egg hatch or a resurrected stage.
   if(!m.dead){auto facts=ui_enemy_facts(a.game,slot);int next=world_enemy_resume(m.model,ui_enemy_fact,&facts);m.move=-1;if(next>=0&&next!=m.state&&ui_enemy_set_state(a,slot,next,facts,a.animation))continue;}
  }
  if(m.loop&&!m.count&&!m.dead&&m.model!=unsigned(EnemyId::LouseProgenitor)){auto facts=ui_enemy_facts(a.game,slot);int resume=world_enemy_resume(m.model,ui_enemy_fact,&facts);const auto*state=world_enemy_state(m.model,resume);
   // Only settled loops are refreshed. Initial spawn clips must not replay
   // whenever a saved phase predicate or shared arm changes.
   // Louse has no resume predicate: its curled_loop is an animator state,
   // and must persist until the explicit Uncurl trigger (not initial Idle).
   if(state&&state->loop){int clip=world_enemy_resolve_clip(m.model,state->clip,m.skin,-1,ui_enemy_fact,&facts);if(clip>=0&&clip!=m.clip){m.move=-1;ui_enemy_set_state(a,slot,resume,facts,a.animation);}}
  }return;
 }
}
void App::enemy_trigger(int slot,const char*trigger,int move,bool direct,const EnemyVisualFacts*facts,uint32_t earliest){
 if(slot<0||slot>=game.enemy_count||!game.enemy_world.active)return;auto&m=enemy_presentation[slot];if(!m.identity)return;
 const bool terminal=ui_visual_equal(trigger,"Dead")||ui_visual_equal(trigger,"Revive")||ui_visual_equal(trigger,"DeadTrigger");
 if(!direct&&(terminal||ui_visual_equal(trigger,"Hit"))&&(!earliest||int32_t(animation-earliest)>=0)){auto snapshot=facts?*facts:ui_enemy_facts(game,slot);
  int state=world_enemy_transition(m.model,m.state,trigger,ui_enemy_fact,&snapshot);if(state>=0){if(terminal)m.count=0;m.move=move;ui_enemy_set_state(*this,slot,state,snapshot,animation);return;}
  if(terminal)return;
  // Repeated simultaneous damage notifications need no duplicate hurt queue.
  for(unsigned i=0;i<m.count;i++)if(ui_visual_equal(m.queue[i].action,trigger))return;
 }
 if(m.count>=MaxEnemyVisualCommands){m.overflow=true;return;}
 const uint32_t when=earliest?earliest:animation;unsigned at=m.count;
 // A move can already have a later AttackEnd/cast queued when its second
 // AttackCommand hit is observed. Insert by clock, preserving same-clock
 // source order, within the existing fixed-capacity command array.
 while(at&&int32_t(when-m.queue[at-1].earliest)<0){m.queue[at]=m.queue[at-1];--at;}
 m.queue[at]={trigger,facts?*facts:ui_enemy_facts(game,slot),int16_t(move),uint8_t(direct),when};++m.count;ui_enemy_advance(*this,slot);
}
CreaturePose App::combat_enemy_pose(int slot)const{
 if(game.enemy_world.active&&slot>=0&&slot<game.enemy_count&&enemy_presentation[slot].identity&&
    enemy_visual_seed==game.seed&&enemy_visual_floor==game.floor&&enemy_visual_combat==game.world.combat_count&&enemy_visual_encounter==int(game.enemy_world.encounter))
  return enemy_presentation[slot].pose;
 const bool bound=creature_layout.right&&enemy_visual_seed==game.seed&&enemy_visual_floor==game.floor&&
  enemy_visual_combat==game.world.combat_count&&enemy_visual_encounter==int(game.enemy_world.encounter);
 return enemy_pose(game,slot,bound?creature_layout:combat_layout_for(game));
}
CreaturePose App::combat_player_pose(bool pet)const{
 if(creature_layout.right&&game.enemy_world.active&&enemy_visual_seed==game.seed&&enemy_visual_floor==game.floor&&
    enemy_visual_combat==game.world.combat_count&&enemy_visual_encounter==int(game.enemy_world.encounter)){
  if(!pet)return player_presentation[0];
  const auto camera=encounter_camera(game.enemy_world.encounter);auto point=player_formation_point(game,camera.zoom,true);
  const auto&motion=osty_presentation;const uint32_t now=animation*33u;point.x=motion.x(now);
  const auto registration=player_sprite_registration(game.character,true);
  return project_creature_pose(camera,creature_layout,point,player_anchors(game.character,true),false,
   registration.units,motion.scale(now),motion.bounds_scale,registration.x,registration.y);
 }
 return player_pose(game,combat_layout_for(game),pet);
}
void App::sync_enemy_feedback(){
 sync_combat_feedback();
 const bool pending_reward=combat_status.reward_scheduled&&combat_status.visual_seed==game.seed&&combat_status.visual_floor==game.floor&&
  combat_status.visual_combat==game.world.combat_count&&combat_status.visual_encounter==int(game.enemy_world.encounter)&&int32_t(combat_status.reward_at-animation)>0;
 // A completed encounter remains in State for saves/rewards. Re-entering an
 // event/rest/shop must not reconstruct those old corpses as a new battle.
 if(!game.enemy_world.active||(game.world.active&&!combat_phase(game.phase)&&!ui_feedback_bound(*this)&&!world_room_is_combat(game)&&!pending_reward)){
  for(auto&m:enemy_presentation)m=EnemyPresentation{};creature_layout={};enemy_visual_clock=combat_linger_until=animation;enemy_visual_encounter=-1;return;}
 if(enemy_visual_seed!=game.seed||enemy_visual_floor!=game.floor||enemy_visual_combat!=game.world.combat_count||enemy_visual_encounter!=int(game.enemy_world.encounter)){
  for(auto&m:enemy_presentation)m=EnemyPresentation{};enemy_visual_clock=combat_linger_until=animation;
  combat_status.reward_at=0;combat_status.reward_scheduled=combat_status.had_live_scene=false;
  enemy_visual_seed=game.seed;enemy_visual_floor=game.floor;enemy_visual_combat=game.world.combat_count;enemy_visual_encounter=int(game.enemy_world.encounter);
  creature_layout={};
 }
 if(!creature_layout.right){creature_layout=combat_layout_for(game);player_presentation[0]=player_pose(game,creature_layout);player_presentation[1]=player_pose(game,creature_layout,true);orb_presentation={};
  const auto camera=encounter_camera(game.enemy_world.encounter);const auto pet=combat_health(MaxEnemies+1);
  osty_presentation.bind(animation*33u,pet.max_hp,pet.hp>0,player_formation_point(game,camera.zoom,true).x);
 }
 if(game.character==Character::Necrobinder||game.expansion.osty_max_hp>0){
  const auto pet=combat_health(MaxEnemies+1);const auto camera=encounter_camera(game.enemy_world.encounter);
  const auto bounds=player_anchors(game.character);const int hp=health_bar::bounded(pet.max_hp,0,150);
  const int x=player_formation_point(game,camera.zoom).x+((bounds.right-bounds.left)/2+150)*camera.zoom+100*camera.zoom*hp/150;
  uint32_t cue=animation*33u;
  if(pet.hp<=0&&osty_presentation.alive){
   // Core observers can synchronize this health cue before the ordinary
   // character render chooses its Dead track. The same visible death edge
   // still owns the full animation-plus500ms wait in AnimDie.
   const auto clip=content.clip(AnimOstyDie);const uint32_t began=osty_motion.clip==AnimOstyDie?osty_motion.start*33u:animation*33u;
   const uint32_t end=began+uint32_t(clip.count)*clip.frame_ms+500u;
   if(int32_t(end-cue)>0)cue=end;
  }
  osty_presentation.observe(animation*33u,pet.max_hp,pet.hp>0,x,cue);
 }
 for(int i=0;i<game.enemy_count;i++){auto&m=enemy_presentation[i];const auto&source=game.enemy_world.models[i];
  if(ui_feedback_bound(*this)&&combat_feedback.identity[i+1]!=ui_feedback_identity(game,unsigned(i+1)))continue;
  if(m.identity!=source.combat_id||m.model!=unsigned(source.kind)){
   m=EnemyPresentation{};m.identity=source.combat_id;m.model=uint8_t(source.kind);m.pose=enemy_pose(game,i,creature_layout);m.hp=combat_health(i+1).hp;m.dead=m.hp<=0;m.skin=uint8_t(ui_enemy_skin(game,i,m.model));m.facts=ui_enemy_facts(game,i);
   int initial=source.age?world_enemy_resume(m.model,ui_enemy_fact,&m.facts):world_enemy_initial(m.model,ui_enemy_fact,&m.facts);
   ui_enemy_set_state(*this,i,initial,m.facts,animation);if(m.dead)enemy_trigger(i,source.kind==EnemyId::TestSubject&&(source.flags&EnemyDown)?"DeadTrigger":"Dead");
   ui_enemy_removal(*this,i,m.dead);
  }else{
   int hp=combat_health(i+1).hp;if(hp<=0&&m.hp>0){m.dead=true;enemy_trigger(i,source.kind==EnemyId::TestSubject&&(source.flags&EnemyDown)?"DeadTrigger":"Dead");}
   else if(hp>0&&m.hp<=0){m.dead=false;enemy_trigger(i,"Revive");}
   m.hp=hp;
  }
  ui_enemy_advance(*this,i);
  ui_enemy_removal(*this,i);
 }
 bool living=false;for(int i=0;i<game.enemy_count;++i)living|=combat_health(i+1).hp>0;
 if(living)combat_status.had_live_scene=true;
 if(!living&&combat_status.had_live_scene&&!combat_phase(game.phase)&&game.phase!=Phase::Dead&&!combat_status.reward_scheduled){
  // NCombatUi.ShowRewards: Normal waits current death track +1s. Fast
  // waits .5s in ordinary rooms, half the remaining track for a boss.
  // The later corpse dissolve never gates the reward window.
  const uint32_t at=ui_feedback_bound(*this)?ui_feedback_later(animation,combat_feedback.end_at):animation;
  uint32_t remaining=0;for(int i=0;i<game.enemy_count;++i){const auto&m=enemy_presentation[i];if(!m.dead)continue;
   uint32_t end=m.until;if(m.clip<0&&(game.enemy_world.models[i].kind==EnemyId::Rocket||game.enemy_world.models[i].kind==EnemyId::Crusher))end=animation+76u;
   if(int32_t(end-at)>int32_t(remaining))remaining=end-at;
  }
  const auto*encounter=encounter_definition(game.enemy_world.encounter);const bool boss=encounter&&encounter->type==EncounterClass::Boss;
  const unsigned wait=fast_mode?(boss?(remaining+1u)/2u:16u):remaining+31u;
  combat_status.reward_scheduled=true;combat_status.reward_at=at+wait;
  combat_linger_until=ui_feedback_later(combat_linger_until,combat_status.reward_at);
 }
}
void App::observe_core(const State&s,Log log){
 if(&s!=&game)return;
 observe_ftue_log(log);
 observe_progress_log(log);
 if(log.kind==LogKind::Enemy){
  // Bind a newly entered encounter before reserving its lead-in; the first
  // identity sync resets the presentation clock to the current frame.
  if(s.enemy_world.active)sync_enemy_feedback();
  sync_turn_banner();const bool first=!turn_banner.in_enemy;turn_banner.begin_enemy(animation);
  if(first){const uint32_t cue=animation+TurnBannerPresentation::enemy_lead_ticks(fast_mode);if(int32_t(cue-enemy_visual_clock)>0)enemy_visual_clock=cue;}
  if(s.enemy_world.active)enemy_visual_clock=(int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation)+TurnBannerPresentation::intent_lead_ticks(fast_mode);}
 if(log.kind==LogKind::Turn)sync_turn_banner();
 if(!s.enemy_world.active)return;
 if(log.kind==LogKind::Enemy&&log.a>=0&&log.a<s.enemy_count){int slot=log.a;auto facts=ui_enemy_facts(s,slot);const auto*move=world_enemy_move(unsigned(s.enemy_world.models[slot].kind),log.b);
  const auto kind=s.enemy_world.models[slot].kind;
  // Respawns increments inside the original action, before its trigger.
  // That exact boundary reports through observe_enemy_animation instead.
  if(kind==EnemyId::TestSubject&&log.b==0){const uint32_t when=ui_feedback_later(enemy_visual_clock,animation);combat_feedback_move(slot,log.b,when-TurnBannerPresentation::intent_lead_ticks(fast_mode),when,when);return;}
  if(kind>=EnemyId::DecimillipedeSegmentBack&&kind<=EnemyId::DecimillipedeSegmentMiddle&&log.b>=0&&log.b<=2){
   sync_enemy_feedback();uint32_t when=int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation,end=when;
   for(int i=0;i<s.enemy_count;i++){const auto other=s.enemy_world.models[i].kind;
    if(other<EnemyId::DecimillipedeSegmentBack||other>EnemyId::DecimillipedeSegmentMiddle)continue;
    auto snapshot=ui_enemy_facts(s,i);auto&m=enemy_presentation[i];const char*action=m.dead?"dead_loop":"idle_loop";
    int clip=world_enemy_resolve_clip(m.model,action,m.skin,log.b,ui_enemy_fact,&snapshot);
    enemy_trigger(i,action,log.b,true,&snapshot,when);if(clip>=0&&int32_t(when+ui_enemy_ticks(*this,clip)-end)>0)end=when+ui_enemy_ticks(*this,clip);
   }
   combat_feedback_move(slot,log.b,when-TurnBannerPresentation::intent_lead_ticks(fast_mode),when,end);enemy_visual_clock=end;return;
  }
  if(move){sync_enemy_feedback();auto&m=enemy_presentation[slot];uint32_t when=int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation;
   const uint32_t first=when;uint32_t action_at=when;int projected=m.state;
   const uint32_t command_wait=ui_enemy_move_wait(*this,kind,log.b);
   for(unsigned i=0;i<move->action_count;i++)if(const auto*action=world_enemy_action(*move,i)){
    int clip=-1;
    if(action->direct)clip=world_enemy_resolve_clip(m.model,action->name,m.skin,log.b,ui_enemy_fact,&facts);
    else{const auto*before=world_enemy_state(m.model,projected);if(before&&!before->loop&&before->next>=0)projected=before->next;
     int state=world_enemy_transition(m.model,projected,action->name,ui_enemy_fact,&facts);if(const auto*node=world_enemy_state(m.model,state)){projected=state;clip=world_enemy_resolve_clip(m.model,node->clip,m.skin,log.b,ui_enemy_fact,&facts);}}
    const bool uncurl=kind==EnemyId::LouseProgenitor&&ui_visual_equal(action->name,"Uncurl");
    // Louse only executes this prefix when its animator is curled. An
    // uncurled Web/Pounce must not reserve an absent 900ms animation.
    if(uncurl&&clip<0)continue;
    enemy_trigger(slot,action->name,log.b,action->direct!=0,&facts,when);
    if(uncurl){when+=TurnBannerPresentation::wait_ticks(900,250,fast_mode);action_at=when;}
    else if(command_wait)when+=command_wait;
    else if(clip>=0)when+=ui_enemy_ticks(*this,clip);
   }
   combat_feedback_move(slot,log.b,first-TurnBannerPresentation::intent_lead_ticks(fast_mode),action_at,when);enemy_visual_clock=when;
  }else{
   const uint32_t when=ui_feedback_later(enemy_visual_clock,animation);combat_feedback_move(slot,log.b,when-TurnBannerPresentation::intent_lead_ticks(fast_mode),when,when);
  }
 }else if(log.kind==LogKind::Damage&&!combat_feedback.active&&log.b>=0&&log.b<s.enemy_count&&s.enemies[log.b].hp>0){
  // Room-entry relics can damage an enemy created after the observer was
  // bound. Establish its current identity before dispatching the hurt cue.
  sync_enemy_feedback();enemy_trigger(log.b,"Hit");
 }
}
void App::observe_enemy_animation(const State&s,int slot,const char*trigger){
 if(&s!=&game||!s.enemy_world.active||slot<0||slot>=s.enemy_count||!trigger)return;
 // A bound actor was synchronized before executing this command. Its
 // canonical idle predicates may already have changed inside the hook
 // (Flutter clears IsHovering before StunTrigger). Keep the displayed pose
 // until this notification's queued boundary instead of resuming that
 // future idle during synchronization. New identities still need binding.
 const auto&source=s.enemy_world.models[slot];const auto&current=enemy_presentation[slot];
 if(!ui_feedback_bound(*this)||current.identity!=source.combat_id||current.model!=unsigned(source.kind))sync_enemy_feedback();
 auto&m=enemy_presentation[slot];auto facts=ui_enemy_facts(s,slot);
 uint32_t when=int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation;
 if(ui_feedback_bound(*this))when=ui_feedback_later(when,combat_feedback.effect_at);
 int state=world_enemy_transition(m.model,m.state,trigger,ui_enemy_fact,&facts);
 const auto*node=world_enemy_state(m.model,state);int clip=node?world_enemy_resolve_clip(m.model,node->clip,m.skin,-1,ui_enemy_fact,&facts):-1;
 // Power hooks can replace a generic death/hurt animation at this exact
 // simulation boundary. Future hooks retain their order behind queued moves.
 if(when==animation&&state>=0){m.move=-1;ui_enemy_set_state(*this,slot,state,facts,animation);}
 else enemy_trigger(slot,trigger,-1,false,&facts,when);
 if(clip>=0)enemy_visual_clock=when+ui_enemy_ticks(*this,clip);
}
bool App::enemy_frame(int slot,int&clip,uint32_t&elapsed,bool&loop){
 if(slot<0||slot>=game.enemy_count||!game.enemy_world.active||!enemy_opacity(slot))return false;
 if(!ui_feedback_bound(*this)||combat_feedback.identity[slot+1]==ui_feedback_identity(game,unsigned(slot+1)))ui_enemy_advance(*this,slot);const auto&m=enemy_presentation[slot];
 clip=m.clip;elapsed=int32_t(animation-m.start)>=0?(animation-m.start)*33:0;loop=m.loop;
 if(phobia_mode&&clip>=0){
  const auto kind=EnemyId(m.model);bool skin=false,body=false,segment=false;
  switch(kind){
  case EnemyId::TheObscura:case EnemyId::FossilStalker:case EnemyId::SkulkingColony:case EnemyId::TwigSlimeM:case EnemyId::HauntedShip:skin=true;break;
  case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:segment=true;body=true;break;
  case EnemyId::Entomancer:case EnemyId::PhrogParasite:case EnemyId::TerrorEel:case EnemyId::TheInsatiable:case EnemyId::Wriggler:body=true;break;
  default:break;
  }
  int alternate=-1;
  if(skin){if(const auto*row=world_enemy_clip_directory(unsigned(clip)))alternate=content.world_clip("phobia",row->key);}
  else if(body){const auto*model=world_enemy_visual(m.model);char key[96]{};unsigned n=0;
   if(model){for(const char*p=model->name;*p&&n+12<sizeof key;++p)key[n++]=*p;
    const char*tail=segment&&combat_health(slot+1).hp<=0?".dead_body":".body";while(*tail&&n+1<sizeof key)key[n++]=*tail++;
    alternate=content.world_clip("phobia",key);}}
  if(alternate>=0){clip=alternate;if(body){elapsed=0;loop=true;}}
 }
 return clip>=0;
}
int App::combat_background()const{
 if(game.enemy_world.active){const auto*e=encounter_definition(game.enemy_world.encounter);if(e){
  if(phobia_mode){int image=content.world_image("phobia_backgrounds",e->key);if(image>=0)return image;}
  int image=content.world_image("backgrounds",e->key);if(image>=0)return image;}}
 if(game.world.active){char region[24]{};world_region_image_key(game,region,sizeof region);
  if(phobia_mode){int image=content.world_image("phobia_backgrounds",region);if(image>=0)return image;}
  int image=content.world_image("backgrounds",region);if(image>=0)return image;}
 return content.ui_image(U_BattleBackground);
}
