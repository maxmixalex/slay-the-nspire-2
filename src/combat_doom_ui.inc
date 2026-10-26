static bool combat_doom_disappears(const State&s,unsigned actor){
 if(!actor||actor>unsigned(s.enemy_count))return false;
 if(!s.enemy_world.active)return true;
 const auto&m=s.enemy_world.models[actor-1];
 switch(m.kind){
 case EnemyId::CeremonialBeast:case EnemyId::Crusher:case EnemyId::DecimillipedeSegmentBack:
 case EnemyId::DecimillipedeSegmentMiddle:case EnemyId::DecimillipedeSegmentFront:
 case EnemyId::EyeWithTeeth:case EnemyId::Parafright:case EnemyId::Rocket:case EnemyId::Vantom:return false;
 case EnemyId::TestSubject:return m.data[0]>=2;
 case EnemyId::WaterfallGiant:return !enemy_power_value(s,int(actor)-1,EnemyPower::SteamEruption);
 default:return true;
 }
}
void App::observe_doom(unsigned actor){
 if(actor>unsigned(game.enemy_count))return;
 const uint32_t at=ui_feedback_bound(*this)?combat_feedback.effect_at:animation;
 auto&body=combat_doom.bodies[actor];body={};body.at=at*33u;
 body.actor=uint8_t(actor);body.identity=ui_feedback_identity(game,actor);
 body.flags=CombatDoomPresentation::Active;
 const auto pose=actor?combat_enemy_pose(int(actor)-1):combat_player_pose();
 body.x=int16_t(pose.sprite_x);body.y=int16_t(pose.sprite_y);
 body.ground_x=int16_t(pose.body_left+pose.body_width/2);body.ground_y=int16_t(pose.body_top+pose.body_height);
 body.width=int16_t(pose.body_width);body.height=int16_t(pose.body_height);body.scale=int16_t(pose.scale);
 const auto camera=game.enemy_world.active?encounter_camera(game.enemy_world.encounter):EncounterCamera{};
 const auto layout=creature_layout.right?creature_layout:combat_layout_for(game);
 body.unit=uint16_t(health_bar::bounded(combat_project_extent(camera,layout,1024)*4,1,4096));
 body.width_q12=uint16_t(health_bar::bounded(int(int64_t(body.width)*3*4096*4096/(520*body.unit)),0,65535));
 if(combat_doom_disappears(game,actor)){
  body.flags|=CombatDoomPresentation::Disappear;
  if(game.enemy_world.active){
   const auto&m=enemy_presentation[actor-1];auto facts=ui_enemy_facts(game,int(actor)-1);
   const auto*visual=world_enemy_visual(m.model);if(visual&&visual->shared_group)body.flags|=CombatDoomPresentation::Shared;
   const int state=world_enemy_transition(m.model,m.state,"Hit",ui_enemy_fact,&facts);
   const auto*node=world_enemy_state(m.model,state);
   int clip=node?world_enemy_resolve_clip(m.model,node->clip,m.skin,-1,ui_enemy_fact,&facts):m.clip;
   const auto*row=clip>=0?world_enemy_clip_directory(unsigned(clip)):nullptr;
   if(row&&ui_visual_equal(row->native_clip,"hurt")){
    body.flags|=CombatDoomPresentation::Frozen;
    body.sample_ms=game.enemy_world.models[actor-1].kind==EnemyId::Inklet?40u:100u;
   }else if(node?node->loop:m.loop)body.flags|=CombatDoomPresentation::Loop;
   if(phobia_mode&&row){const int replacement=content.world_clip("phobia",row->key);if(replacement>=0)clip=replacement;}
   body.clip=int16_t(clip);
   const unsigned units=content.clip(clip).sprite_units;
   const int baseline=creature_sprite_units(game.enemy_world.models[actor-1].kind);
   if(units&&baseline>0)body.scale=int16_t(int64_t(body.scale)*units/baseline);
  }else{body.clip=AnimEnemyHurt;body.sample_ms=100;body.flags|=CombatDoomPresentation::Frozen;}
  unsigned alive=0;for(int i=0;i<game.enemy_count;++i)alive+=game.enemies[i].hp>0;
  const uint32_t wait=(alive==1?1500u:250u)+32u;
  if(ui_feedback_bound(*this)){
   combat_feedback.effect_at=at+wait/33u;
   combat_feedback.end_at=ui_feedback_later(combat_feedback.end_at,combat_feedback.effect_at);
  }
  combat_linger_until=ui_feedback_later(combat_linger_until,at+46u);
 }
 const unsigned angle=1934u+ScreenShakePresentation::hash(body.identity^at^0x41cef1eau)%229u;
 screen_effect_at(at,ScreenShakePresentation::Punch,ScreenShakePresentation::Weak,300,angle);
 if(!actor)combat_linger_until=ui_feedback_later(combat_linger_until,at+10u);
}

static bool combat_doom_hides(const App&a,unsigned actor){
 if(actor>MaxEnemies)return false;const auto&b=a.combat_doom.bodies[actor];
 return b.started(a.animation*33u)&&(b.flags&CombatDoomPresentation::Disappear)&&
  b.identity==ui_feedback_identity(a.game,actor)&&
  (a.animation*33u-b.at<1500u||a.combat_health(int(actor)).hp<=0);
}
struct DoomTexel {uint16_t color=0;unsigned alpha=0;};
static DoomTexel doom_texel(Picture p,int u,int v,bool repeat=false){
 if(!p.pixels||p.format>3||!p.w||!p.h)return {};
 if(repeat){u&=4095;v&=4095;}else{u=health_bar::bounded(u,0,4095);v=health_bar::bounded(v,0,4095);}
 const unsigned at=unsigned(v)*p.h/4096u*p.w+unsigned(u)*p.w/4096u;
 const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
 return {uint16_t(pixel[0]|unsigned(pixel[1])<<8),p.alpha?p.alpha[at]:255u};
}
static void doom_quad(Canvas&c,Picture p,int cx,int cy,int w,int h,int angle=0,unsigned opacity=255,unsigned mode=0,int frame=-1){
 if(!p.pixels||p.format>3||w<128||h<128||!opacity)return;
 const int sn=CombatCardMotion::sine(angle),cs=CombatCardMotion::sine(angle+1024);
 const int ex=(CombatCardMotion::magnitude(cs)*w+CombatCardMotion::magnitude(sn)*h)/2048+1024;
 const int ey=(CombatCardMotion::magnitude(sn)*w+CombatCardMotion::magnitude(cs)*h)/2048+1024;
 for(int y=mx(22,(cy-ey)/1024);y<mn(c.clip_h,(cy+ey+1023)/1024);++y)
  for(int x=mx(0,(cx-ex)/1024);x<mn(c.clip_w,(cx+ex+1023)/1024);++x){
   const int dx=x*1024+512-cx,dy=y*1024+512-cy;
   int u=int((int64_t(dx)*cs+int64_t(dy)*sn)*4/w)+2048;
   int v=int((int64_t(dy)*cs-int64_t(dx)*sn)*4/h)+2048;
   if(u<0||u>=4096||v<0||v>=4096)continue;
   if(frame>=0){u=(u+(frame&1)*4096)/2;v=(v+((frame>>1)&1)*4096)/2;}
   const auto pixel=doom_texel(p,u,v);if(pixel.alpha)c.paint(y*c.pitch+x,pixel.color,div255(pixel.alpha*opacity),mode);
  }
}
static unsigned doom_smooth(int value,int begin,int end){
 const int t=health_bar::bounded((value-begin)*4096/(end-begin),0,4096);
 return unsigned(int64_t(t)*t/4096*(12288-2*t)/4096);
}
static unsigned doom_particle_curve(unsigned age,unsigned life){
 const int q=int(age*1024u/life);
 const int value=q<242?CombatSparkMotion::segment(q,0,242,19,1024,10361,0):q<754?
  CombatSparkMotion::segment(q,242,754,1024,699,0,-1340):CombatSparkMotion::segment(q,754,1024,699,0,-1340,-3515);
 return unsigned(health_bar::bounded(value,0,1024));
}
static void doom_distortion(Picture noise,int u,int v,int time,int&du,int&dv){
 // Both distortion samplers are assigned to tileable_noise_1 in the
 // scene. Only Noise1Texture and DistortionMask use the white default.
 const auto first=doom_texel(noise,u*3/2-time/2,v*3/2-time,true);
 const auto second=doom_texel(noise,u*121/100+time/2,v*1275/1000-time*8/10,true);
 du=int((first.color>>11)*205u/31u+(second.color>>11)*82u/31u);
 dv=int(((first.color>>5)&63)*205u/63u+((second.color>>5)&63)*82u/63u);
}
static void doom_flame(Canvas&c,const CombatDoomPresentation::Body&b,uint32_t now){
 const unsigned scale=b.width_scale(now);if(!scale)return;
 const auto noise=c.a.content.image(c.a.content.ui_image(I13_DoomNoise));
 const auto mask=c.a.content.image(c.a.content.ui_image(I13_DoomFlameMask));
 const auto flame=c.a.content.image(c.a.content.ui_image(I13_DoomFlame));
 if(!noise.pixels||!mask.pixels||!flame.pixels||noise.format>3||mask.format>3||flame.format>3)return;
 const int w=mx(1,int(int64_t(377)*b.unit*scale/(4096*4096)));
 const int h=mx(1,int(int64_t(694)*b.unit*mn(int(scale),4096)/(4096*4096)));
 const int left=b.ground_x-b.unit/4096-w/2,top=b.ground_y+2*b.unit/4096-h/2;
 // The source shader uses global TIME, not an age reset for each Doom.
 // Every authored pan repeats after50s, avoiding overflow with no phase loss.
 const int time=int((c.a.map_clock_ms%50000u)*4096u/1000u);
 for(int y=mx(22,top);y<mn(c.clip_h,top+h);++y)for(int x=mx(0,left);x<mn(c.clip_w,left+w);++x){
  const int u=(2*(x-left)+1)*2048/w,v=(2*(y-top)+1)*2048/h;
  int du=0,dv=0;doom_distortion(noise,u,v,time,du,dv);
  const int blue=int((doom_texel(flame,u+du,v+dv).color&31)*4096u/31u);
  if(!blue)continue;
  const int n=int((doom_texel(noise,u*122/100+time*74/100,v*2545/1000-time*144/100,true).color&31)*4096u/31u);
  const int m=int((doom_texel(mask,u,v*62/100-2375).color&31)*4096u/31u);
  const int value=int(int64_t(n*42/100+4096-m)*blue/4096);
  const unsigned alpha=doom_smooth(value,61,676)*255u/4096u;if(!alpha)continue;
  const unsigned inner=doom_smooth(value,2048,2683);
  c.paint(y*c.pitch+x,rgb(114+int(inner*88/4096),81+int(inner*52/4096),mn(255,224+int(inner*186/4096))),alpha,presentation::Add);
 }
}
static void doom_particles(Canvas&c,Picture texture,const CombatDoomPresentation::Body&b,uint32_t now,unsigned system){
 // Each source emitter remains distinct. Overdraw-equivalent LCD samples
 // replace hundreds of subpixel floor globs; all use the source textures,
 // lifetimes, gravity, size curves and independent deterministic phases.
 const unsigned age=now-b.at,stop=1800u+unsigned(b.width_q12)*150u/4096u;
 const bool globs=system>=3,gravity=system==5;
 const unsigned count=system==0?12u:system==1?20u:system==2?10u:system==3?7u:system==4?6u:10u;
 const unsigned nominal=globs?(system==3?700u:gravity?360u:1000u):500u;
 for(unsigned i=0;i<count;++i){
  const uint32_t seed=ScreenShakePresentation::hash(b.identity*0x5c9b13u+i*0x9e3779b9u+system*4711u);
  const unsigned life=nominal-(seed%((globs?(gravity?50u:30u):10u)+1u))*nominal/100u;
  const unsigned first=i*nominal/count;if(age<first)continue;
  const unsigned cycle=(age-first)/life,born=first+cycle*life,t=age-born;
  if(born>=stop)continue;const unsigned width=b.width_scale(b.at+born),curve=doom_particle_curve(t,life);
  if(!curve)continue;
  const int rx=system==0?100:system==1?120:system==2?90:100;
  const int base_y=system==0?-22:system==1?-13:system==2?-4:gravity?-13:-18;
  const int phase=int(ScreenShakePresentation::hash(seed+cycle)&4095u);
  const int radial=globs?int(seed%2001u)-1000:CombatCardMotion::sine(phase);
  const int xx=int(int64_t(radial)*rx*b.unit*width/(1024ll*4096*4096));
  int yy=base_y*b.unit/4096;
  if(!globs){const int ry=system==0?8:system==1?10:4;yy+=CombatCardMotion::sine(phase+1024)*ry*b.unit/(1024*4096);}
  else if(gravity)yy+=int((int64_t(-(150+int(seed%51u)))*t*1000+250ll*t*t)*b.unit/(1000000ll*4096));
  else yy-=int(150ll*t*t*b.unit/(1000000ll*4096));
  const unsigned scale=globs?(gravity?300u+seed%301u:400u+seed%201u):200u+seed%201u;
  const int sw=globs?32:128,sh=globs?32:43;
  const int w=int(int64_t(sw)*b.unit*scale*curve*width/(4096ll*4096*1000));
  const int vertical=system==0?64:system==1?96:system==2?44:100;
  const int h=int(int64_t(sh)*b.unit*scale*curve*vertical/(4096ll*1000*100));
  const int angle=globs?int(seed%4096u)+int((int(seed%501u)-250)*t*4096/(360*1000)):0;
  Canvas particle=c;if(!globs||gravity)particle.brightness=181;
  doom_quad(particle,texture,(b.ground_x+xx)*1024,(b.ground_y+yy)*1024,w,h,angle,255,0,globs?int((seed>>12)&3u):-1);
 }
}
static void doom_spears(Canvas&c,Picture texture,const CombatDoomPresentation::Body&b,uint32_t now,bool front){
 const unsigned age=now-b.at;if(age>=2100u)return;
 for(unsigned i=0;i<4;++i){const uint32_t seed=ScreenShakePresentation::hash(b.identity+i*619u+(front?71u:31u));
  const unsigned life=300u+seed%301u,cycle=age/life,born=cycle*life,t=age-born;if(born>=1500u)continue;
  const uint32_t random=ScreenShakePresentation::hash(seed+cycle);
  const int source_x=int(random%141u)-70-20;
  const unsigned width=b.width_scale(b.at+born);if(width<410u)continue;
  const int w=int(int64_t(40)*b.unit*400*4096/(4000ll*width));
  const int h=int(int64_t(206)*b.unit*(1000u+random%501u)*(life-t)/(4096ll*1000*life))*1024;
  const int angle=(source_x*15/100+4)*4096/360;
  const int alpha=int((102u+random%77u)*(t<200u?t*t:40000u)/40000u);
  // TextureRect position is its top-left; the authored(20,206)
  // pivot supplies the missing20px horizontally and bottom=-9px.
  const int x=b.ground_x+(source_x+20)*b.unit/4096,y=b.ground_y+(front?-16:-17)*b.unit/4096;
  doom_quad(c,texture,x*1024+h*CombatCardMotion::sine(angle)/2048,y*1024-h*CombatCardMotion::sine(angle+1024)/2048,w,h,angle,unsigned(alpha),presentation::Add);
 }
}
static void combat_doom_layers(Canvas&c,bool front){
 auto&a=c.a;const uint32_t now=a.animation*33u;bool active=false;
 for(const auto&b:a.combat_doom.bodies)active|=b.alive(now);if(!active)return;
 if(front){
  const auto glow=a.content.image(a.content.ui_image(I13_DoomGlow));
  for(const auto&b:a.combat_doom.bodies)if(b.alive(now)){
   const unsigned width=b.width_scale(now);if(!width)continue;
   const int w=int(int64_t(256)*b.unit*width/16384),h=int(int64_t(256)*b.unit*85*mn(int(width),4096)/(16384*100));
   const int center_y=b.ground_y+int((-400000ll+197568ll*mn(int(width),4096)/4096)*b.unit/(10000ll*4096));
   doom_quad(c,glow,b.ground_x*1024,center_y*1024,w,h,0,253,presentation::Add);
  }
 }else{
  const auto hole=a.content.image(a.content.ui_image(I13_DoomHole));Canvas black=c;black.brightness=0;
  for(const auto&b:a.combat_doom.bodies)if(b.alive(now)){
   const unsigned width=b.width_scale(now);if(!width)continue;
   doom_quad(black,hole,b.ground_x*1024,(b.ground_y-15*b.unit/4096)*1024,
    int(int64_t(205)*b.unit*width/16384),int(int64_t(41)*b.unit/4));
  }
  for(const auto&b:a.combat_doom.bodies)if(b.alive(now))doom_flame(c,b,now);
 }
 const auto spears=a.content.image(a.content.ui_image(I13_DoomSpear));
 for(const auto&b:a.combat_doom.bodies)if(b.alive(now))doom_spears(c,spears,b,now,front);
 const auto fill=a.content.image(a.content.ui_image(I13_DoomFill));
 for(const auto&b:a.combat_doom.bodies)if(b.alive(now)){
  doom_particles(c,fill,b,now,front?1u:0u);if(front)doom_particles(c,fill,b,now,2u);
 }
 const auto globs=a.content.image(a.content.ui_image(I13_DoomGlobs));
 for(const auto&b:a.combat_doom.bodies)if(b.alive(now)){
  doom_particles(c,globs,b,now,front?4u:3u);if(front)doom_particles(c,globs,b,now,5u);
 }
}
struct DoomBodyBounds {int left,top,right,bottom;};
static DoomBodyBounds combat_doom_bounds(const CombatDoomPresentation::Body&b){
 return {b.ground_x-b.width*3/4,b.ground_y-b.height*3/2,b.ground_x+(b.width*3+3)/4,b.ground_y-mx(1,9*b.unit/4096)};
}
static bool combat_doom_shared_frame(Canvas&c,int clip,uint32_t time,bool loop){
 DoomBodyBounds cuts[MaxEnemies]{};unsigned count=0;
 for(unsigned actor=1;actor<=unsigned(c.a.game.enemy_count);++actor){const auto&b=c.a.combat_doom.bodies[actor];
  if((b.flags&CombatDoomPresentation::Shared)&&combat_doom_hides(c.a,actor))cuts[count++]=combat_doom_bounds(b);
 }
 if(!count)return false;
 const auto cl=c.a.content.clip(clip);if(!cl.count||!cl.frame_ms)return true;
 unsigned frame=time/cl.frame_ms;frame=loop?frame%cl.count:unsigned(mn(int(frame),cl.count-1));
 const auto p=c.a.content.image(int(cl.first+frame));if(!p.pixels)return true;
 const int top=22-combat_controls::SceneLift;
 const auto visible=[&](int x,int y){for(unsigned i=0;i<count;++i)if(x>=cuts[i].left&&x<cuts[i].right&&y>=cuts[i].top&&y<cuts[i].bottom)return false;return true;};
 if(p.format==4){const uint8_t*run=p.pixels;
  for(int sy=0;sy<p.h;++sy){const int y=top+sy;const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
   for(unsigned span=0;span<spans;++span){const int x=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8;
    const unsigned stride=run[4]?3u:2u;run+=5;
    if(y>=22&&y<c.clip_h)for(int col=mx(0,x),end=mn(c.clip_w,x+n);col<end;++col)if(visible(col,y)){
     const uint8_t*pixel=run+(col-x)*stride;c.paint(y*c.pitch+col,uint16_t(pixel[0]|unsigned(pixel[1])<<8),stride==3?pixel[2]:255);
    }run+=n*stride;
   }
  }
 }else if(p.format<=3){for(int y=mx(22,top);y<mn(c.clip_h,top+p.h);++y)for(int x=0;x<mn(c.clip_w,p.w);++x)if(visible(x,y)){
  const unsigned at=unsigned(y-top)*p.w+x,alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
  const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;c.paint(y*c.pitch+x,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha);
 }}return true;
}
static uint16_t doom_grayscale(uint16_t color){
 // Despite its filename, hsv.gdshader transforms RGB to YIQ, sets
 // chroma to zero, then applies the inverse matrix (h=1,s=0,v=1).
 return presentation::yiq_material(color,0,1000);
}
static int doom_ceil(int numerator,int denominator){return numerator<0?numerator/denominator:(numerator+denominator-1)/denominator;}
static void doom_body_image(Canvas&c,Picture p,int x,int y,int width,int height){
 if(!p.pixels||width<=0||height<=0)return;
 if(p.format==4){const uint8_t*run=p.pixels;
  for(int sy=0;sy<p.h;++sy){const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
   const int y0=mx(0,y+doom_ceil(2*sy*height-p.h,2*p.h)),y1=mn(c.clip_h,y+doom_ceil(2*(sy+1)*height-p.h,2*p.h));
   for(unsigned span=0;span<spans;++span){const int sx=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8;const unsigned stride=run[4]?3u:2u;run+=5;
    if(y0<y1)for(int i=0;i<n;++i){const uint8_t*pixel=run+i*stride;const unsigned alpha=stride==3?pixel[2]:255;if(!alpha)continue;
     const int x0=mx(0,x+doom_ceil(2*(sx+i)*width-p.w,2*p.w)),x1=mn(c.clip_w,x+doom_ceil(2*(sx+i+1)*width-p.w,2*p.w));
     if(x0>=x1)continue;const auto color=doom_grayscale(uint16_t(pixel[0]|unsigned(pixel[1])<<8));
     for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx)c.paint(yy*c.pitch+xx,color,alpha);
    }run+=n*stride;
   }
  }
 }else if(p.format<=3){for(int yy=mx(0,y);yy<mn(c.clip_h,y+height);++yy)for(int xx=mx(0,x);xx<mn(c.clip_w,x+width);++xx){
  const unsigned at=unsigned((2*(yy-y)+1)*p.h/(2*height))*p.w+unsigned((2*(xx-x)+1)*p.w/(2*width));
  const unsigned alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
  c.paint(yy*c.pitch+xx,doom_grayscale(uint16_t(pixel[0]|unsigned(pixel[1])<<8)),alpha);
 }}
}
static void combat_doom_bodies(Canvas&c){
 const uint32_t now=c.a.animation*33u;
 for(const auto&b:c.a.combat_doom.bodies){
  if(!b.started(now)||!(b.flags&CombatDoomPresentation::Disappear)||now-b.at>=1500u||b.clip<0)continue;
  const auto box=combat_doom_bounds(b);const int left=mx(0,box.left),top=mx(22,box.top),right=mn(c.clip_w,box.right),bottom=mn(c.clip_h,box.bottom);
  if(left>=right||top>=bottom)continue;
  const unsigned time=b.flags&CombatDoomPresentation::Frozen?b.sample_ms:now-b.at;
  Canvas body=c;body.pixels=c.pixels+top*c.pitch+left;body.clip_w=right-left;body.clip_h=bottom-top;body.coverage=nullptr;body.occlusion=nullptr;
  const auto cl=c.a.content.clip(b.clip);if(!cl.count||!cl.frame_ms)continue;
  unsigned frame=time/cl.frame_ms;frame=b.flags&CombatDoomPresentation::Loop?frame%cl.count:unsigned(mn(int(frame),cl.count-1));
  const auto p=c.a.content.image(int(cl.first+frame));const int sink=int(b.sink(now));
  if(b.flags&CombatDoomPresentation::Shared)doom_body_image(body,p,-left,22-combat_controls::SceneLift-top+sink,p.w,p.h);
  else doom_body_image(body,p,b.x-left-cl.pivot_x*b.scale/1024,b.y-top+sink-cl.pivot_y*b.scale/1024,mx(1,p.w*b.scale/1024),mx(1,p.h*b.scale/1024));
 }
}
static void combat_doom_overlay(Canvas&c){
 c.a.combat_doom.sync_overlay(c.a.animation*33u);
 const unsigned alpha=c.a.combat_doom.overlay(c.a.animation*33u);if(!alpha)return;
 const int saturation=1000-int(alpha*300u/4096u);const unsigned value=1000-alpha*250u/4096u;
 const uint32_t time=c.a.map_clock_ms;const int wave=CombatCardMotion::sine(int(uint64_t(time)*13037973u/1000000u%4096u))+CombatCardMotion::sine(int(uint64_t(time)*6518986u/1000000u%4096u));
 const unsigned pulse=doom_smooth(wave+10240,0,11264);
 for(int y=22;y<c.clip_h;++y){const unsigned uv=unsigned(y)*4096u/240u,q2=uv*uv/4096u,q3=q2*uv/4096u;
  const unsigned glow=unsigned(uint64_t(q3)*q3/4096u*alpha/4096u*pulse/4096u*7u/10u);
  for(int x=0;x<c.clip_w;++x){const int at=y*c.pitch+x;const auto color=presentation::yiq_material(c.pixels[at],saturation,value);
   c.pixels[at]=presentation::add_opaque(color,rgb(47,0,36),glow*255u/4096u);}
 }
}
