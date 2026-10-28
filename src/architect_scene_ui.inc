// TheArchitectEventEncounter is a VisualOnly combat room. Its background
// contains environment/desk decoration, not the two creature nodes. Reuse
// their existing original clips without creating a canonical combat state.
static void architect_scene_effects(Canvas&);
static CreaturePose architect_scene_pose(Character character,bool architect){
 const EncounterCamera camera{};const CombatSceneLayout layout{CombatSourceWidth};
 // NCombatRoom's un-slotted single enemy/player positions are (+/-480,200)
 // relative to the 1680x1260 viewport center. The Necrobinder is shifted
 // left150 even when this VisualOnly room has no Osty node.
 const EnemyFormationPoint point{architect?480000:-480000-(character==Character::Necrobinder?150000:0),200000};
 const auto registration=player_sprite_registration(character);
 auto pose=project_creature_pose(camera,layout,point,
  architect?creature_anchors(EnemyId::Architect):player_anchors(character),architect,
  architect?creature_sprite_units(EnemyId::Architect):registration.units,1024,1024,
  architect?0:registration.x,architect?0:registration.y);
 // This event has no hand. Keep the authored room coordinate instead of
 // the combat-only14px lift used to clear the hand and health bars.
 pose.sprite_y+=combat_controls::SceneLift;pose.feet+=combat_controls::SceneLift;
 pose.body_top+=combat_controls::SceneLift;pose.center_y+=combat_controls::SceneLift;
 pose.intent+=combat_controls::SceneLift;pose.hp+=combat_controls::SceneLift;pose.status+=combat_controls::SceneLift;
 return pose;
}
static void architect_scene_begin(App&a,bool opening){
 auto&v=a.room_presentation.architect;const auto&s=a.game;
 v.begin(s.character,unsigned(s.original_events.values[0]),opening,a.fast_mode,a.map_clock_ms,
  ui_run_score(a.progress.current,true).total,s.original_events.rng_words,s.original_events.rng_calls,s.seed);
 a.combat_sparks.reset();const auto target=architect_scene_pose(s.character,true);
 for(unsigned i=0;i<v.plan.hits;++i)a.combat_sparks.emit(v.start+v.plan.hit_at(i),target.center_x,target.center_y,false,v.hits[i].seed);
}
struct ArchitectSceneFrame {int player=-1,architect=-1;uint32_t player_ms=0,architect_ms=0;bool player_loop=true,architect_loop=true;};
static ArchitectSceneFrame architect_scene_frame(const App&a){
 const auto&s=a.game;const uint32_t now=a.map_clock_ms;
 ArchitectSceneFrame frame{character_animation(s.character,AnimHeroIdle),a.content.world_clip("Architect","idle_loop"),now,now,true,true};
 const auto&v=a.room_presentation.architect;
 if(v.bound&&v.opening){
  uint32_t idle=v.entered;
  if(v.plan.hits){const uint32_t resumed=v.entered+v.plan.hit_at(v.plan.hits-1)+733u;
   if(int32_t(now-resumed)>=0)idle=resumed;}
  frame.architect_ms=int32_t(now-idle)<0?0u:now-idle;
 }
 const bool ending=s.phase==Phase::Complete;
 if(!ending&&!a.screen_motion.sequence_active(now))return frame;
 // A reloaded completed run has neither the event presentation owner nor
 // its transient cues. Show the retained idle cast, not the tail of an
 // attack inferred from the newly opened summary timestamp.
 if(ending&&!a.room_presentation.event.bound&&!a.screen_motion.sequence_active(now))return frame;
 const auto plan=source_architect_shake(s.character,unsigned(s.original_events.values[0]),!ending,a.room_presentation.event.fast);
 if(!plan.complete)return frame;
 const uint32_t end=ending?a.game_over_presentation.opened:a.screen_motion.sequence_end(now);
 const uint32_t origin=end-plan.complete;if(int32_t(now-origin)<0)return frame;
 const unsigned age=now-origin;
 int player_start=-1,architect_start=-1;const char*architect_action=nullptr;
 for(unsigned i=0;i<plan.hits;++i){
  const unsigned hit=plan.hit_at(i),attack=hit-plan.windup;
  if(age>=attack)player_start=int(attack);
  if(age>=hit){architect_start=int(hit);architect_action="hurt";}
 }
 if(plan.architect_at){const unsigned attack=plan.architect_at-plan.windup*5u;
  if(age>=attack){architect_start=int(attack);architect_action="attack";}}
 if(player_start>=0){
  const int clip=character_animation(s.character,AnimHeroAttack);const auto source=a.content.clip(clip);
  const unsigned elapsed=age-unsigned(player_start),duration=unsigned(source.count)*source.frame_ms;
  if(elapsed<duration){frame.player=clip;frame.player_ms=elapsed;frame.player_loop=false;}
  else frame.player_ms=elapsed-duration;
 }
 if(architect_start>=0){
  const int clip=a.content.world_clip("Architect",architect_action);
  const auto*source=clip<0?nullptr:world_enemy_clip_directory(unsigned(clip));
  const auto packed=a.content.clip(clip);const unsigned duration=source?source->duration_ms:unsigned(packed.count)*packed.frame_ms;
  const unsigned elapsed=age-unsigned(architect_start);
  if(elapsed<duration){frame.architect=clip;frame.architect_ms=elapsed;frame.architect_loop=false;}
  else frame.architect_ms=elapsed-duration;
 }
 return frame;
}
static void architect_scene_actors(Canvas&c,bool draw_player=true){
 if(c.a.game.original_events.id!=OriginalEventId::TheArchitect)return;
 const auto frame=architect_scene_frame(c.a);
 const auto player=architect_scene_pose(c.a.game.character,false),architect=architect_scene_pose(c.a.game.character,true);
 if(draw_player)c.scaled_clip(frame.player,player.sprite_x,player.sprite_y,frame.player_ms,player.scale,frame.player_loop);
 const auto&v=c.a.room_presentation.architect;
 unsigned reading=I13_ArchitectReading;
 if(c.a.game.character==Character::Silent)reading=v.fast?I13_ArchitectSilentFastReading:I13_ArchitectSilentReading;
 const bool read=v.intro(c.a.map_clock_ms)&&c.a.content.ui_image(reading)>=0;
 if(read)c.ui(reading+(c.a.map_clock_ms-v.start)/50u,0,22,320,218);
 else{
  int scale=architect.scale;const auto units=c.a.content.clip(frame.architect).sprite_units;
  if(units)scale=int(int64_t(scale)*units/creature_sprite_units(EnemyId::Architect));
  c.scaled_clip(frame.architect,architect.sprite_x,architect.sprite_y,frame.architect_ms,scale,frame.architect_loop);
 }
 architect_scene_effects(c);
 // CombatEventVisuals.Allies contains player creatures only. Neither Osty
 // nor another survivor from the previous boss belongs to this event.
}
