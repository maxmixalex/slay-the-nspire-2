// NGameOverScreen retains combat coordinates when reparenting a creature.
// Other rooms create the ordinary character visuals and start a non-looping
// die animation. The exact terminal source sample remains visible afterward.
static void game_over_actor(Canvas&c,uint32_t elapsed,unsigned opacity){
 auto&a=c.a;const auto&s=a.game;const unsigned character=unsigned(s.character);
 if(a.content.version()<CharacterDeathContentVersion||character>=5||!opacity)return;
 const bool merchant=s.world.room_type==unsigned(WorldRoom::Shop);
 const auto clip=a.content.clip(int((merchant?MerchantDeathAnimationFirst:CharacterDeathAnimationFirst)+character));if(!clip.count)return;
 const auto registration=player_sprite_registration(s.character);
 int x=160+registration.x*4/21,y=158+registration.y*4/21,scale=int(registration.units)*4/21;
 if(s.phase==Phase::Complete&&s.original_events.id==OriginalEventId::TheArchitect){
  const auto pose=architect_scene_pose(s.character,false);x=pose.sprite_x;y=pose.sprite_y;scale=pose.scale;
 }else if(world_room_is_combat(s)||s.world.event_combat_parent||s.original_events.combat_pending){
  const auto pose=a.combat_player_pose();x=pose.sprite_x;y=pose.sprite_y;scale=pose.scale;
 }else if(s.world.room_type==unsigned(WorldRoom::Rest)){
  // Rest-site Character_1 has global(651,716); the new creature inherits
  // the .760006 actor scale, then shifts +100,+100 in the screen container.
  // Existing room art uses its unchanged1920x1080 cover transform.
  constexpr int q=760006;
  x=(651+100)*218/1080+(320-1920*218/1080)/2+int(int64_t(registration.x)*q*218/1080000000);
  y=22+(716+100)*218/1080+int(int64_t(registration.y)*q*218/1080000000);
  scale=int(int64_t(registration.units)*q*218/1080000000);
 }else if(merchant){
  x=(320-1920*218/1080)/2+564*218/1080;y=22+766*218/1080;
  scale=int(registration.units)*218/1080;
 }
 Canvas actor=c;actor.opacity=uint8_t(div255(actor.opacity*mn(opacity,255u)));
 const auto draw=[&](Clip layer,unsigned frame,unsigned mode){
  if(!layer.count)return;const unsigned units=layer.sprite_units?layer.sprite_units:registration.units;
  const int projected=int(int64_t(scale)*units/registration.units);
  const auto picture=a.content.image(int(layer.first+mn(frame,unsigned(layer.count-1))));
  actor.picture(picture,x-layer.pivot_x*projected/1024,y-layer.pivot_y*projected/1024,
                mx(1,int(picture.w)*projected/1024),mx(1,int(picture.h)*projected/1024),255,mode);
 };
 if(!merchant&&(s.character==Character::Defect||s.character==Character::Regent)){
  const auto particles=a.content.clip(s.character==Character::Defect?int(DefectDeathParticles):int(RegentDeathParticles));
  const unsigned duration=s.character==Character::Defect?CharacterDeathDurationMs[character]:5000;
  if(particles.count&&elapsed<duration)
   draw(particles,elapsed*(particles.count-1)/duration,presentation::Add);
 }
 const unsigned frame=mn(character_death_frame(character,elapsed),unsigned(clip.count-1));
 draw(clip,frame,presentation::Normal);
}
