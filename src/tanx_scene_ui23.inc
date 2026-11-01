// Tanx authored order: painted scene, ash2/ash3, ash4/ash5, dialogue/HUD.
namespace tanx_scene {
using namespace tanx_scene_ui23;
static void fog(Canvas c,const Smoke&smoke,unsigned scroll,int cx,int cy,int w,int h,int angle){
 const int sn=CombatCardMotion::sine(angle),cs=CombatCardMotion::sine(angle+1024);
 const int hw=(CombatCardMotion::magnitude(cs)*w+CombatCardMotion::magnitude(sn)*h)/2048;
 const int hh=(CombatCardMotion::magnitude(sn)*w+CombatCardMotion::magnitude(cs)*h)/2048;
 const int left=mx(0,(cx-hw)/256-1),right=mn(c.clip_w,(cx+hw)/256+2);
 const int top=mx(0,(cy-hh)/256-1),bottom=mn(c.clip_h,(cy+hh)/256+2);
 const int du=int(int64_t(cs)*16384/w),dv=int(-int64_t(sn)*16384/h);
 constexpr uint16_t color=uint16_t((248>>3)<<11|(105>>2)<<5|(27>>3));
 for(int y=top;y<bottom;++y){
  const int dx=left*256+128-cx,dy=y*256+128-cy;
  int u=32768+int((int64_t(dx)*cs+int64_t(dy)*sn)*64/w);
  int v=32768+int((-int64_t(dx)*sn+int64_t(dy)*cs)*64/h);
  for(int x=left;x<right;++x,u+=du,v+=dv){
   if(unsigned(u)>=65536u||unsigned(v)>=65536u)continue;
   const unsigned alpha=smoke.alpha(u,v,scroll);
   if(alpha)c.paint(y*c.pitch+x,color,alpha,presentation::Normal);
  }
 }
}
static void draw(Canvas&c,uint32_t elapsed,uint32_t shader_clock,uint32_t entry){
 auto&a=c.a;if(a.content.ui_image(AuxiliaryFirst)!=int(FirstImage+6))return;
 auto&state=fake_merchant::room.squares;
 const uint32_t seed=fake_merchant::hash(uint32_t(a.game.seed)^unsigned(a.game.floor)*0x9e3779b9u^entry^0x54414e58u);
 tanx_particles_ui23::advance(state,elapsed,seed);
 for(unsigned emitter=0;emitter<2;++emitter){
  const auto image=a.content.image(FirstImage+emitter);
  if(!image.pixels||image.format!=1||!image.alpha||image.w!=(emitter?8:32)||image.h!=(emitter?16:32))return;
  const water_reflection::Texture texture(image);
  for(unsigned i=emitter*20;i<(emitter+1)*20;++i){const auto&p=state.particles[i];if(!p.active)continue;
   const unsigned age=mn(unsigned(p.age)*fake_merchant::Q/tanx_particles_ui23::Life,unsigned(fake_merchant::Q));
   auto color=emitter?fake_merchant::gradient(tanx_particles_ui23::RiceColor,age):fake_merchant::gradient(tanx_particles_ui23::GrainColor,age);
   if(emitter)color.a=uint8_t(div255(unsigned(color.a)*156));
   fake_merchant::quad(c,texture,
    fake_merchant::ancient_x((emitter?225:66)*256+p.x/4),fake_merchant::ancient_y(1163*256+p.y/4),
    fake_merchant::ancient_size(emitter?8:32)*p.scale/1024,fake_merchant::ancient_size(emitter?16:32)*p.scale/1024,
    p.angular,color.r,color.g,color.b,color.a,presentation::Normal);
  }
 }
 Smoke smoke;
 // At most139 pages for all Tanx views. No image request occurs after these
 // borrowed pointers are acquired; all drawing uses the existing framebuffer.
 for(unsigned page=0;page<4;++page){const auto image=a.content.image(FirstImage+2+page);
  if(!image.pixels||image.w!=200||image.h!=200||image.format!=1||!image.alpha)return;
  smoke.pages[page]=image.alpha;
 }
 const auto table=a.content.image(a.content.ui_image(AuxiliaryFirst));
 if(!parameters(table))return;smoke.gradient=table.pixels+32;smoke.lookup=table.pixels+232;
 const unsigned scroll=phase(shader_clock);
 fog(c,smoke,scroll,fake_merchant::ancient_x(1538*256),fake_merchant::ancient_y(628*256),
     fake_merchant::ancient_size(2472*.2),fake_merchant::ancient_size(3316*.2*2.765),389);
 fog(c,smoke,scroll,fake_merchant::ancient_x(221*256),fake_merchant::ancient_y(494*256),
     fake_merchant::ancient_size(2472*.5),fake_merchant::ancient_size(3316*.5*2.765),341);
}
}
