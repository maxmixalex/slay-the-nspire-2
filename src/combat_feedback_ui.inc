// EndTurn still resolves the authoritative State synchronously. This small
// view records settled effect values and releases them on the animation
// clock; it never copies State, invokes a command, or advances an RNG stream.
static const enemy_command_program::Step*ui_enemy_program_command(App&,unsigned,unsigned);
static void ui_enemy_program_finish(App&);
static void ui_enemy_program_wait(App&,unsigned);
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
static bool ui_combat_ending(const State&s){
 return s.world.combat_ending||s.phase==Phase::Dead||s.phase==Phase::Reward||s.phase==Phase::Complete||
  (s.phase==Phase::WorldRelicChoice&&s.world_relics.return_phase==uint8_t(Phase::Reward));
}
struct CombatOrbPoint {int x,y,size,radius;};
static CombatOrbPoint combat_orb_point(const App&a,OrbSourcePoint position){
 const auto&s=a.game;const auto camera=s.enemy_world.active?encounter_camera(s.enemy_world.encounter):EncounterCamera{};
 const auto layout=a.creature_layout.right?a.creature_layout:combat_layout_for(s);const auto root=player_formation_point(s,camera.zoom);
 const auto player=a.combat_player_pose();const auto anchors=player_anchors(s.character);
 const auto base=player_pose(s,layout);const int scale=player.scale*1024/(base.scale>0?base.scale:1);
 // NCreature.DoScaleTween updates OrbManager with the creature: growth
 // moves its anchor only up to1.25, while shrink halves the scale delta.
 const int manager=scale>1024?1024:(scale+1024)/2,anchor=scale<1280?scale:1280;
 const int xx=anchors.intent_x*anchor+int(int64_t(position.x)*manager/1024),yy=(anchors.intent_y+195)*anchor+int(int64_t(position.y)*manager/1024);
 const int x=s.enemy_world.active?combat_project_x(camera,layout,root.x+int(int64_t(xx)*camera.zoom/1024),false):player.x+xx*4/(21*1024);
 const int y=s.enemy_world.active?combat_project_y(camera,layout,root.y+int(int64_t(yy)*camera.zoom/1024)):player.feet+yy*4/(21*1024);
 return {x,y,health_bar::bounded(combat_project_extent(camera,layout,90,manager,1024),9,90),health_bar::bounded(combat_project_extent(camera,layout,30,manager,1024),5,30)};
}
void App::sync_orb_presentation(){
 auto&m=orb_presentation;const uint32_t now=animation*33u;m.advance(now);
 // Pending original commands own topology until their display clock is
 // reached. A bare load/debug setup can still initialize from live facts.
 if(m.count)return;
 const unsigned slots=unsigned(health_bar::bounded(combat_status_fact(CombatStatus::OrbSlots),0,MaxOrbs));
 const unsigned count=unsigned(health_bar::bounded(combat_status_fact(CombatStatus::OrbCount),0,int(slots)));
 m.layout(slots,now);for(unsigned i=0;i<slots;++i)m.model(i,i<count?unsigned(combat_status_fact(CombatStatus::OrbKind,int(i))):255u,now);
}
void App::observe_orb_activation(const State&s,const CoreOrbNotice&notice){
 if(&s!=&game)return;
 auto&m=orb_presentation;const uint32_t now=animation*33u;
 if(!m.bound){m.layout(s.expansion.orb_slots,now);for(unsigned i=0;i<m.slots;++i)m.model(i,i<s.expansion.orb_count?s.defect.orb_kind[i]:255u,now);}
 const uint32_t at=(ui_feedback_bound(*this)?combat_feedback.effect_at:animation)*33u;
 if(notice.event!=CoreOrbNotice::Activation){
  int passive=0,evoke=0;if(notice.event==CoreOrbNotice::Removed)orb_values(s,notice.slot,passive,evoke);
  m.enqueue({at,int16_t(passive),int16_t(evoke),notice.event,notice.slot,notice.slots,notice.kind},now);return;
 }
 if(notice.kind>=unsigned(DOrb::Count)||!notice.slots||notice.slot>=notice.slots)return;
 // Repeated evokes do not dequeue. The explicit Removed observation
 // captures the old object and the remaining objects' current positions.
 const auto p=combat_orb_point(*this,m.activation(notice.slot,notice.kind,notice.removed,at));
 const uint32_t seed=at^(uint32_t(notice.kind+1)*0x4a56b39u)^(uint32_t(notice.slot+1)*0x906b1a4du);
 combat_sparks.secondary(at,p.x,p.y,notice.evoke?CombatSparkMotion::OrbEvoke:CombatSparkMotion::OrbPassive,notice.kind,seed);
 if(notice.evoke)for(unsigned slot=0;slot<unsigned(s.enemy_count);++slot)if(notice.targets&(1u<<slot)){
  const auto target=combat_enemy_pose(int(slot));combat_sparks.secondary(at,p.x,p.y,CombatSparkMotion::OrbBeam,notice.kind,seed^slot,target.center_x,target.center_y);
 }
}
static const WorldEnemyVisualAction*ui_enemy_attack_action(EnemyId kind,int node){
 // These source AttackCommands suppress their animator entirely. A zero
 // delay alone cannot identify that case: Cubex still triggers Attack.
 if((kind==EnemyId::CeremonialBeast&&node==1)||(kind==EnemyId::ThievingHopper&&node==0)||
    (kind==EnemyId::Tunneler&&node==2)||(kind==EnemyId::Vantom&&node==2))return nullptr;
 const auto*move=world_enemy_move(unsigned(kind),node);if(!move)return nullptr;
 for(unsigned i=0;i<move->action_count;++i){const auto*action=world_enemy_action(*move,i);if(!action||action->direct)continue;
  // The move directory preserves source order. Only these two commands
  // precede the AttackCommand's own animation; do not replay that prefix.
  if((kind==EnemyId::LouseProgenitor||kind==EnemyId::LivingFog)&&i==0&&move->action_count>1)continue;
  return action;
 }return nullptr;
}
static const char*ui_status_key(int);
void App::observe_power_flash(const State&s,int receiver,const char*key){
 if(&s!=&game||!key||receiver<-1||receiver>=s.enemy_count)return;
 const unsigned actor=unsigned(receiver+1);int id=-1;
 for(int candidate=0;candidate<StatusKinds;++candidate){const char*stored=ui_status_key(candidate);if(!stored)continue;
  const char*left=key,*right=stored;while(*left&&*left==*right){++left;++right;}if(!*left&&!*right){id=candidate;break;}}
 if(id<0)return;
 const uint32_t at=ui_feedback_bound(*this)?combat_feedback.effect_at:animation;
 combat_status.flash(actor,unsigned(id),ui_feedback_identity(s,actor),at,animation);
}
static void ui_damage_apply(App&a,const CombatFeedback::DamageCue&cue){
 const unsigned actor=cue.actor;
 const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[actor]:ui_feedback_identity(a.game,actor);
 if(identity!=cue.identity)return;
 a.combat_status.damage(actor,cue.amount,cue.identity,cue.at,a.animation,cue.flags,cue.shake_strength);
 if(cue.amount>0||(cue.flags&CoreDamageResult::FullyBlocked)){
  const bool pet=actor==MaxEnemies+1;const auto pose=pet?a.combat_player_pose(true):actor?a.combat_enemy_pose(int(actor)-1):a.combat_player_pose();
  a.combat_sparks.emit(cue.at*33u,pose.center_x,pose.center_y,bool(cue.flags&CoreDamageResult::FullyBlocked),a.combat_status.cosmetic);
 }
 // Self-inflicted damage and explicit kills do not play a fabricated Hit.
 // DamageResult still owns its number/sparks when a command suppresses hurt.
 if(!(cue.flags&CoreDamageResult::Hurt)||cue.amount<=0||int32_t(a.animation-cue.at)>=24)return;
 if(actor==0)a.motion[0]={AnimHeroHurt,cue.at};
 else if(actor<=MaxEnemies&&a.combat_health(int(actor)).hp>0)a.enemy_trigger(int(actor)-1,"Hit");
}
void App::observe_combat_damage(const State&s,const CoreDamageResult&result){
 if(&s!=&game||(!combat_phase(s.phase)&&!ui_feedback_bound(*this)))return;
 const unsigned actor=result.receiver==-2?MaxEnemies+1:unsigned(result.receiver+1);
 if(actor>=CombatFeedback::Energy)return;
 const auto identity=ui_feedback_identity(s,actor);auto&f=combat_feedback;
 if(ui_feedback_bound(*this)&&result.receiver>=0){
  int card=-1;for(const auto&attack:f.attacks)if(attack.active&&attack.card>=0)card=attack.card;
  if(card>=0&&card<s.card_count){const auto impact=source_card_impact_shake(s.cards[card].kind);
   if(s.cards[card].kind==SovereignBlade){const auto target=combat_enemy_pose(result.receiver);
    combat_cards.blades.attack(unsigned(card),target.center_x,target.center_y,f.effect_at*33u);}
   if(impact.strength){sync_screen_motion();screen_motion.add(screen_motion_clock(),f.effect_at*33u+impact.delay,ScreenShakePresentation::Punch,impact.strength,impact.duration);}
   if(s.cards[card].kind==DefectHyperbeam){sync_screen_motion();
    // The card awaits .5s; the laser begins at .525s and ends .5s later.
    screen_motion.add(screen_motion_clock(),f.effect_at*33u+25u,ScreenShakePresentation::Punch,ScreenShakePresentation::Medium,800);
    screen_motion.add(screen_motion_clock(),f.effect_at*33u+525u,ScreenShakePresentation::Punch,ScreenShakePresentation::Strong);
   }
  }
 }
 if(ui_feedback_bound(*this))f.append_damage(actor,result,identity,f.effect_at);
 else if(result.amount()>0||(result.flags&(CoreDamageResult::FullyBlocked|CoreDamageResult::ShowZero)))
  ui_damage_apply(*this,{identity,animation,result.amount(),uint8_t(actor),result.flags,result.shake_strength});
}
void App::combat_scene_offset(int&x,int&y)const{
 ScreenShakePresentation::Cue damage{};
 for(unsigned i=combat_status.damage_count;i>0;--i){const auto&d=combat_status.damages[i-1];
  if(int32_t(animation-d.at)<0||!d.shake_strength)continue;
  damage={d.at*33u,300,d.angle,d.shake_strength,ScreenShakePresentation::Punch};break;
 }
 screen_motion.sample(animation*33u,screen_shake,x,y,&damage);
}
void App::sync_combat_damage(){
 auto&f=combat_feedback;
 while(f.damage_head<f.damage_count&&int32_t(animation-f.damages[f.damage_head].at)>=0)ui_damage_apply(*this,f.damages[f.damage_head++]);
 if(f.damage_head==f.damage_count)f.damage_head=f.damage_count=0;
}
#include "combat_card_motion_ui.inc"
#include "combat_status_ui.inc"
HealthBarValues App::combat_health(int actor)const{
 if(actor<0||actor>=int(CombatFeedback::Actors))return {};
 if(!ui_feedback_bound(*this))return ui_health_values(game,actor);
 // Identity travels with each sample. A reused slot keeps the old actor's
 // displayed values until its replacement cue, then switches atomically.
 return combat_feedback.shown[actor];
}
int App::combat_energy(bool maximum)const{const auto v=combat_health(CombatFeedback::Energy);return maximum?v.max_hp:v.hp;}
CombatHand::Cards App::combat_hand()const{
 return ui_feedback_bound(*this)?combat_feedback.hand.shown:CombatHand::capture(game.hand);
}
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
 if(!ui_feedback_bound(*this)){
  // A resumed pending choice has no old presentation queue. Bind its actual
  // current state before accepting the answer so its transfers and effects
  // receive the same observers as an uninterrupted selection.
  if(combat_selection_kind()!=CombatSelectionKind::None)begin_combat_feedback();return;
 }
 if(!combat_feedback.choice_pending||!combat_choice_ready())return;
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
 if(combat_health(slot+1).hp<=0||!combat_intent_alpha(slot))return {};
 return f.intents[slot];
}
unsigned App::combat_intent_alpha(int slot)const{
 if(slot<0||slot>=game.enemy_count)return 0;
 if(!ui_feedback_bound(*this))return 255;
 const auto&f=combat_feedback;if(!(f.hidden&(1u<<slot)))return 255;
 // NCreature.AnimHideIntent: 400 ms delay, then 500 ms fade. The command
 // continues independently, and revealing the next intent cancels this fade.
 const int elapsed=int32_t(animation-f.hide_at[slot])*33;
 return elapsed<=400?255:elapsed>=900?0:unsigned((900-elapsed)*255/500);
}
void App::begin_combat_feedback(int manual_card){
 sync_combat_cards();combat_feedback.reset();if(!game.enemy_world.active||(!combat_phase(game.phase)&&combat_selection_kind()==CombatSelectionKind::None))return;
 sync_enemy_feedback();auto&f=combat_feedback;f.active=true;f.seed=game.seed;f.floor=game.floor;f.combat=game.world.combat_count;f.encounter=int(game.enemy_world.encounter);
 f.effect_at=f.move_at=f.move_end=f.end_at=animation;
 f.hand.begin(game.hand);
 auto&m=combat_cards;if(!ui_cards_bound(*this))m.begin(game,animation);
 m.manual=manual_card;m.manual_started=m.wait_at=animation;m.wait_excess=0;
 m.card_count=game.card_count;for(unsigned id=0;id<m.card_count;++id)m.zones[id]=uint8_t(unsigned(game.zones[id])|(m.zones[id]&CombatCardMotion::Tracked));
 m.set_focus(hand_focus&&cursor>=0&&cursor<game.hand.count?game.hand.ids[cursor]:-1);
 if(manual_card>=0&&manual_card<int(game.card_count)){
  m.manual_energy=int16_t(cost(game,manual_card));m.manual_stars=int16_t(card_star_cost(game,game.cards[manual_card],manual_card));
  // Power cards await their 250 ms play-pile scale tween. Ordinary manual
  // cards execute effects in parallel with that movement.
  const bool power=card_type(game,game.cards[manual_card],manual_card)==CardType::Power;
  if(power){f.effect_at+=TurnBannerPresentation::wait_ticks(250,250,fast_mode);m.wait_at=f.effect_at;m.wait_excess=uint8_t((f.effect_at-animation)*33-250);}
  m.manual_min_end=animation+TurnBannerPresentation::wait_ticks(power?550:300,power?400:150,fast_mode);
 }
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor){f.shown[actor]=f.observed[actor]=ui_health_values(game,int(actor));f.identity[actor]=f.observed_identity[actor]=ui_feedback_identity(game,actor);}
 for(int i=0;i<MaxEnemies;++i){f.intents[i]=i<game.enemy_count?enemy_intent(game,i):EnemyIntent{};f.hide_at[i]=animation;}
 ui_capture_combat_status(*this,animation,true);
}
void App::combat_feedback_move(int slot,int move,uint32_t start,uint32_t action,uint32_t end){
 if(!ui_feedback_bound(*this)||slot<0||slot>=game.enemy_count)return;auto&f=combat_feedback;
 f.move_slot=slot;f.move=move;f.move_at=action;f.move_end=end;
 f.commands.begin(unsigned(game.enemy_world.models[slot].kind),move,action,enemy_presentation[slot].state);
 f.commands.first_speech=game.enemy_world.models[slot].kind==EnemyId::GremlinMerc&&!game.enemy_world.models[slot].data[0];
 f.effect_at=ui_feedback_later(f.effect_at,action);f.end_at=ui_feedback_later(f.end_at,end);
 f.hidden|=uint8_t(1u<<slot);f.hide_at[slot]=start;
}
void App::before_combat_effect(const State&s,Effect effect){
 if(&s!=&game)return;
 if(world_effect_clears_combat_powers(effect))
  ui_freeze_combat_powers(*this);
 if(!ui_feedback_bound(*this))return;auto&f=combat_feedback;
 const XOp op=effect.kind==EffectKind::Extended?XOp(effect.target):XOp::Count;
 if(op==XOp::DrawOne&&effect.value>0&&!s.draw.count&&s.discard.count&&!s.no_draw&&s.hand.count<MaxHand&&
    !seen_ftue(FtueId::Shuffle)&&!ftue.shuffle_waiting){
  ftue.shuffle_at=ui_feedback_later(animation+1,f.effect_at);ftue.shuffle_waiting=true;
  ftue.shuffle_draw=s.draw.count;ftue.shuffle_discard=s.discard.count;ftue.shuffle_exhaust=s.exhaust.count;
  // A command beginning on this frame must not publish its shuffled piles
  // or newly drawn hand before tick() reaches the modal barrier. Only this
  // presentation cue moves; the canonical command and RNG still resolve.
  f.effect_at=ftue.shuffle_at;
 }
 const EnemyId kind=f.move_slot>=0&&f.move_slot<s.enemy_count?s.enemy_world.models[f.move_slot].kind:EnemyId::Count;
 f.commands.operation=uint16_t(op);f.after_at=0;
 // Existing saved core boundaries identify an AttackCommand without
 // altering the command queue. Repeated wrappers start a new hit; distinct
 // targets between them belong to one simultaneous all-opponents hit.
 auto&attack=f.attacks[s.expansion.depth<=MaxAutoFrames?s.expansion.depth:0];
 if(op==XOp::SharedAttackStart&&effect.value>=0&&effect.value<s.card_count){
  const auto timing=combat_card_attack_timing(s,s.cards[effect.value].kind,fast_mode);
  attack.begin(f.effect_at,timing.delay,timing.wait,timing.once,effect.value);
 }else if(op==XOp::SharedAttackEnd){
  for(auto&pending:f.attacks)if(pending.active&&pending.card==effect.value)f.effect_at=ui_feedback_later(f.effect_at,pending.finish(fast_mode,ui_combat_ending(s)));
 }else if(attack.active){
  const bool group=effect.kind==EffectKind::RandomDamage||effect.kind==EffectKind::WhirlwindHits||
   op==XOp::RandomHits||op==XOp::IHits||op==XOp::IAllHits||op==XOp::SHits||
   op==XOp::DHits||op==XOp::RHits||op==XOp::RAllHits||op==XOp::NRepeatedHit;
  if(group&&attack.hit)attack.next=true;
  if((effect.kind==EffectKind::Damage||effect.kind==EffectKind::GreedDamage)&&effect.target>=0&&effect.target<s.enemy_count&&f.observed[effect.target+1].hp>0){
   // FromOsty uses 300 ms, irrespective of the player's character.
   if(effect.aux==2)attack.delay_ms=fast_mode?150:300;
   f.effect_at=ui_feedback_later(f.effect_at,attack.impact(f.effect_at,unsigned(effect.target),fast_mode));
  }
 }
 if(op==XOp::EnemyAttackStart&&effect.value==f.move_slot){
  ui_enemy_program_command(*this,enemy_command_program::Attack,0);
  const auto timing=combat_enemy_attack_timing(kind,f.move,fast_mode);
  f.enemy_attack.begin(f.commands.active?f.commands.base:f.effect_at,timing.delay,timing.wait,timing.once);
  if(f.commands.active){f.commands.align(f.effect_at);f.enemy_attack.clock_ms=f.commands.milliseconds;}
 }else if(op==XOp::EnemyHit&&effect.aux%8==f.move_slot&&f.enemy_attack.active&&f.observed[f.move_slot+1].hp>0&&f.observed[0].hp>0){
  const bool repeat=f.enemy_attack.hit&&!f.enemy_attack.once;
  if(f.commands.active&&f.commands.milliseconds>f.enemy_attack.clock_ms)f.enemy_attack.clock_ms=f.commands.milliseconds;
  f.effect_at=ui_feedback_later(f.effect_at,f.enemy_attack.impact(f.effect_at,0,fast_mode));
  if(f.commands.active)f.commands.milliseconds=f.enemy_attack.clock_ms;
  if((kind==EnemyId::CeremonialBeast&&(f.move==1||f.move==4||f.move==5))||(kind==EnemyId::Vantom&&f.move==2))
   screen_effect_at(f.effect_at,ScreenShakePresentation::Punch,ScreenShakePresentation::Strong,800,2048);
  else if(kind==EnemyId::Crusher||kind==EnemyId::Rocket){sync_screen_motion();screen_motion.add(screen_motion_clock(),f.effect_at*33u+200u,ScreenShakePresentation::Punch,ScreenShakePresentation::Strong);}
  else if(kind==EnemyId::MechaKnight){sync_screen_motion();screen_motion.add(screen_motion_clock(),f.effect_at*33u+(f.move==1?0u:100u),ScreenShakePresentation::Punch,
    f.move==1?ScreenShakePresentation::Weak:ScreenShakePresentation::Strong,f.move==1?800u:300u);}
  if(repeat&&combat_enemy_attack_timing(kind,f.move,fast_mode).known)if(const auto*action=ui_enemy_attack_action(kind,f.move)){
   // AttackCommand restarts its animator after the previous Damage wait,
   // before this hit's anim delay and optional callback wait. Retain the
   // millisecond accumulator so repeated rounding cannot drift the cue.
   const auto&attack=f.enemy_attack;
   const uint32_t at=attack.base+(attack.clock_ms-attack.delay_ms-attack.wait_ms+32u)/33u;
   enemy_trigger(f.move_slot,action->name,f.move,false,nullptr,at);
  }
 }else if(op==XOp::EnemyAttackEnd&&effect.value==f.move_slot&&f.enemy_attack.active){
  if(f.commands.active&&f.commands.milliseconds>f.enemy_attack.clock_ms)f.enemy_attack.clock_ms=f.commands.milliseconds;
  f.effect_at=ui_feedback_later(f.effect_at,f.enemy_attack.finish(fast_mode,ui_combat_ending(s)));
  if(f.commands.active)f.commands.milliseconds=f.enemy_attack.clock_ms+(f.enemy_attack.hit&&!ui_combat_ending(s)?(fast_mode?100u:200u):0u);
  f.move_end=ui_feedback_later(f.move_end,f.effect_at);
  enemy_visual_clock=ui_feedback_later(enemy_visual_clock,f.effect_at);
 }
 if(op==XOp::EnemyMoveFinish){
  ui_enemy_program_finish(*this);f.effect_at=ui_feedback_later(f.effect_at,ui_feedback_later(f.move_end,enemy_visual_clock));f.move_slot=-1;
 }else if(op==XOp::EnemyGenerateCard&&effect.aux%1024>0){
  auto&p=f.commands;const unsigned pile=unsigned(effect.aux)/1024u;
  if(!p.card_batch){const auto*step=ui_enemy_program_command(*this,enemy_command_program::Cards,pile==4?2:pile);
   p.card_wait=uint16_t(step?step->normal_ms:pile==1?100u:1000u);p.card_batch=true;p.card_pile=uint8_t(pile);}
 }else if(op==XOp::EnemyKnowledgeChoice)ui_enemy_program_command(*this,enemy_command_program::Choice,0);
 else if(op==XOp::EnemyEscape)ui_enemy_program_command(*this,enemy_command_program::Escape,0);
 else if(op==XOp::EnemyDie&&effect.aux==1)ui_enemy_program_command(*this,enemy_command_program::Kill,0);
}
void App::observe_combat_effect(const State&s,Effect effect){
 if(&s!=&game||!ui_feedback_bound(*this))return;auto&f=combat_feedback;
 // Primary and nested commands share the exact source await accumulator;
 // independent card/skeleton animation lifetimes do not extend it.
 ui_card_finish_wait(*this);
 const bool generated=effect.kind==EffectKind::Extended&&XOp(effect.target)==XOp::EnemyGenerateCard;
 const unsigned old_card_count=combat_cards.card_count;
 const bool hand_add=ui_capture_card_changes(*this,f.effect_at,generated);
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)f.append(actor,ui_health_values(s,int(actor)),ui_feedback_identity(s,actor),f.effect_at);
 ui_capture_combat_status(*this,f.effect_at);
 // Observe all settled commands: retain/flush, returned cards and enemy
 // card effects can change the hand without going through ordinary Draw.
 f.hand.append(s.hand,f.effect_at);
 // DrawInternal awaits each real Add. A full hand or prevented draw makes
 // no transfer and contributes no invented delay. Parallel return/add
 // commands share this one command boundary rather than serializing a batch.
 if(hand_add)f.effect_at=ui_card_add_wait(*this,f.effect_at);
 if(generated&&effect.aux%1024==1){
  auto&p=f.commands;
  // AddToCombatAndPreview owns one wait for the whole source batch.
  // Mixed draw/discard AddGeneratedCardToCombat calls share the explicit
  // wait in the move program, rather than each inventing another second.
  if(game.card_count>old_card_count)ui_enemy_program_wait(*this,p.card_wait);
  p.card_batch=false;p.card_pile=255;p.card_wait=0;
 }
 // Round cumulative command time once, rather than adding rounded waits.
 // Otherwise Fast's 175ms attack +100ms block drifts one 33ms tick later.
 if(f.after_at)f.effect_at=ui_feedback_later(f.effect_at,f.after_at);
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
 // A saved combat choice may suspend a hook in the same side. Terminal
 // room rewards also use WorldRelicChoice, but their parent is Reward:
 // NCombatUi.OnCombatEnded has already withdrawn hand, piles and counters.
 // Leaving this display trace suspended there would retain combat controls
 // beneath the entire reward list until the player claimed every reward.
 const bool room_rewards=game.phase==Phase::WorldRelicChoice&&
     game.world_relics.return_phase==uint8_t(Phase::Reward);
 if(choice_phase(game.phase)&&!room_rewards){
  if(!f.choice_pending){f.choice_pending=true;f.choice_at=ui_feedback_later(f.effect_at,animation);}
  return;
 }
 f.choice_pending=false;
 if(game.effect_count||game.playing>=0)return;
 if(ui_combat_ending(game)){
  // Victory/death can clear the continuation queue before MoveFinish. Do
  // not invent the unexecuted suffix or leave its program armed forever.
  f.commands.cursor=f.commands.end;f.commands.active=f.commands.card_batch=false;
  f.commands.card_pile=255;f.commands.card_wait=0;f.move_slot=-1;
  f.enemy_attack.active=false;for(auto&attack:f.attacks)attack.active=false;
 }
 f.end_at=ui_feedback_later(f.end_at,enemy_visual_clock);f.effect_at=f.end_at;
 ui_card_finish_wait(*this);
 const bool hand_add=ui_capture_card_changes(*this,f.effect_at);
 f.end_at=f.effect_at;
 for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)f.append(actor,ui_health_values(game,int(actor)),ui_feedback_identity(game,actor),f.end_at);
 ui_capture_combat_status(*this,f.end_at);
 // Some selections move their card before resuming the effect stream.
 // Capture that final result at its queued endpoint, never immediately.
 f.hand.append(game.hand,f.end_at);
 if(hand_add)f.end_at=ui_card_add_wait(*this,f.end_at);
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
 }
}
void App::sync_combat_feedback(){
 combat_status.expire(animation);
 auto&f=combat_feedback;if(!f.active){sync_combat_cards();return;}
 if(title||!ui_feedback_bound(*this)||game.phase==Phase::Error){f.reset();sync_combat_cards();return;}
 sync_combat_cards();
 while(f.head<f.count&&int32_t(animation-f.cues[f.head].at)>=0){const auto cue=f.cues[f.head++];ui_feedback_apply(*this,cue.actor,cue.value(),cue.identity,cue.at);}
 if(f.head==f.count)f.head=f.count=0;
 sync_combat_damage();
 combat_status.sync(animation);
 if(f.finished&&int32_t(animation-f.end_at)>=0){
  // Overflow only drops intermediate presentation entries. At the scheduled
  // side boundary every displayed value converges to the captured result.
  for(unsigned actor=0;actor<CombatFeedback::Actors;++actor)ui_feedback_apply(*this,actor,f.observed[actor],f.observed_identity[actor],f.end_at);
  combat_status.finish();
  f.hand.finish();
  combat_cards.set_hand(f.hand.shown);
  combat_cards.counts=combat_cards.observed_counts;
  if(combat_cards.overflow){for(auto&v:combat_cards.sprites)if(v.mode!=CombatCardMotion::Hand)v.mode=CombatCardMotion::Empty;}
  combat_cards.head=combat_cards.count=combat_cards.count_head=combat_cards.count_count=0;
  combat_cards.overflow=combat_cards.count_overflow=false;
  f.reset();
 }
}
