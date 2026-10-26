// Fake Merchant's authored environment/actor order. The whole room remains
// live while the inventory slides; its body focus skin shares the same clock.
namespace fake_merchant {
constexpr int world_x(int q8){return 160*256+int(int64_t(q8-960*256)*218/1080);}
constexpr int world_y(int q8){return int(int64_t(q8)*218/1080);}
constexpr int source(double n){return int(n*256+(n<0?-.5:.5));}
constexpr int size(double n){return source(n*218/1080);}
struct Room {
 const App*owner=nullptr;uint32_t generation=0,seed=0,start=0,last=0,entry=0;int floor=-1;
 Squares squares{};
 void observe(const App&a){
  if(owner!=&a||generation!=a.content.generation()||seed!=a.game.seed||floor!=a.game.floor||
     entry!=a.room_presentation.merchant.welcome.start||int32_t(a.map_clock_ms-last)<0){
   owner=&a;generation=a.content.generation();seed=a.game.seed;floor=a.game.floor;start=a.map_clock_ms;
   entry=a.room_presentation.merchant.welcome.start;squares={};
  }last=a.map_clock_ms;
  squares.advance(Squares::Preprocess+uint32_t(uint64_t(a.map_clock_ms-start)*60/1000),hash(seed^unsigned(floor)*0x47f3u));
 }
};
static Room room;

// Fractional quad coverage matters for source stars, whose projected size
// is .5..1.6px. Keep all300 particles; cull only fragments outside the room.
// The larger soft masks retain Canvas's usual nearest LCD texture sampling.
static void quad(Canvas c,const water_reflection::Texture&tex,int cx,int cy,int w,int h,
                 int angle,unsigned red,unsigned green,unsigned blue,unsigned alpha,unsigned blend_mode){
 if(w<=0||h<=0||!alpha||!tex.valid()||c.clip_w<=0||c.clip_h<=0)return;
 const int sn=CombatCardMotion::sine(angle),cs=CombatCardMotion::sine(angle+1024);
 const int hw=(CombatCardMotion::magnitude(cs)*w+CombatCardMotion::magnitude(sn)*h)/2048;
 const int hh=(CombatCardMotion::magnitude(sn)*w+CombatCardMotion::magnitude(cs)*h)/2048;
 const int left=mx(0,(cx-hw)/256-1),right=mn(c.clip_w,(cx+hw)/256+2);
 const int top=mx(0,(cy-hh)/256-1),bottom=mn(c.clip_h,(cy+hh)/256+2);
 if(right<=left||bottom<=top)return;
 c.tint_red=uint8_t(div255(c.tint_red*red));c.tint_green=uint8_t(div255(c.tint_green*green));c.tint_blue=uint8_t(div255(c.tint_blue*blue));
 c.opacity=uint8_t(div255(c.opacity*alpha));
 const int du=int(int64_t(cs)*16384/w),dv=int(-int64_t(sn)*16384/h);
 const bool linear=w<4*256||h<4*256;
 for(int y=top;y<bottom;++y){
  const int dx=left*256+128-cx,dy=y*256+128-cy;
  int u=32768+int((int64_t(dx)*cs+int64_t(dy)*sn)*64/w);
  int v=32768+int((-int64_t(dx)*sn+int64_t(dy)*cs)*64/h);
  for(int x=left;x<right;++x,u+=du,v+=dv){
   if(unsigned(u)>=65536u||unsigned(v)>=65536u)continue;
   const auto sample=linear?tex.bilinear(u,v):tex.at(int(int64_t(u)*tex.width()>>16),int(int64_t(v)*tex.height()>>16));
   if(sample.alpha)c.paint(y*c.pitch+x,sample.color,sample.alpha,blend_mode);
  }
 }
}
template<class Source> static void atlas(Canvas&c,Source image,int x,int y){
 for(const auto&band:FakeMerchantRegentBands){
  // A plane's exact static/moving coverage is disjoint. Complete that plane
  // before the next authored blend; the eye is never moved above the arms.
  // Fetch and draw one view at a time: a subsequent Content load is allowed
  // to evict earlier decoded storage when the pool is under pressure.
  for(unsigned part=0;part<2;++part){
   const auto view=image(part!=0);
   if(!band.sx&&band.w==view.w)c.picture(Canvas::picture_rows(view,band.sy,band.h),x+band.x,y+band.y,band.w,band.h,255,band.blend);
   else c.picture_region(view,x+band.x,y+band.y,band.sx,band.sy,band.w,band.h,false,255,band.blend);
  }
 }
}
static void sequence_frame(Canvas&c,unsigned index,unsigned frame){
 const auto&s=FakeMerchantSequences[index];
 if(index==FakeMerchantRegentSequence){
  const int fixed_id=c.a.content.ui_image(s.base),moving_id=c.a.content.ui_image(s.base+1+frame);
  const auto fixed=c.a.content.image_dimensions(fixed_id),moving=c.a.content.image_dimensions(moving_id);
  if(fixed.w==FakeMerchantRegentAtlasWidth&&fixed.h==FakeMerchantRegentAtlasHeight&&
     moving.w==fixed.w&&moving.h==fixed.h){
   atlas(c,[&](bool moving){return c.a.content.image(moving?moving_id:fixed_id);},s.x,22+s.y);return;}
 }
 c.ui(s.base,s.x,22+s.y,s.w,s.h);
 c.ui(s.base+1+frame,s.x,22+s.y,s.w,s.h);
}
static void sequence(Canvas&c,unsigned index,uint64_t clock_us){
 sequence_frame(c,index,fake_merchant_frame(index,clock_us));
}
static void glitter(Canvas&c,uint32_t clock){
 const water_reflection::Texture noise(c.a.content.image(c.a.content.ui_image(I13_FakeMerchantNoise)));
 const water_reflection::Texture texture(c.a.content.image(c.a.content.ui_image(I13_FakeMerchantGlitter)));
 if(!noise.valid()||!texture.valid())return;
 const water_reflection::Parameters parameters(clock,388,58);
 constexpr int x=world_x(source(-10+1.01*1502.97)),y=world_y(source(20+1.01*531.683));
 constexpr int w=size(103*.71*1.01),h=size(103*.955*1.01);
 const int left=mx(0,x/256),right=mn(c.clip_w,(x+w+255)/256);
 const int top=mx(0,y/256),bottom=mn(c.clip_h,(y+h+255)/256);
 for(int py=top;py<bottom;++py){const int v=int(int64_t(py*256+128-y)*65536/h);
  if(unsigned(v)>=65536u)continue;
  for(int px=left;px<right;++px){const int u=int(int64_t(px*256+128-x)*65536/w);
   if(unsigned(u)>=65536u)continue;const auto p=water_reflection::sample(texture,noise,u,v,parameters,&texture);
   if(p.alpha)c.paint(py*c.pitch+px,p.color,p.alpha);
  }
 }
}
static void lights(Canvas&c,uint32_t clock,uint32_t seed,bool ambient){
 struct Light {int x,y,w,h;uint8_t r,g,b,a;};
 static constexpr Light candles[]={
  {world_x(source(-10+1.01*(1306.93+.7*106.083))),world_y(source(20+1.01*(868.317+.8*-73.02))),size(256*12*.189*.7*1.01),size(256*12*.189*.8*1.01),194,95,32,255},
  {world_x(source(-10+1.01*(1306.93+.7*4.24413))),world_y(source(20+1.01*(868.317+.8*-111.386))),size(256*12*.189*.7*1.01),size(256*12*.189*.8*1.01),194,95,32,255},
  {world_x(source(-10+1.01*(1306.93+.7*328.148))),world_y(source(20+1.01*(868.317+.8*-342.822))),size(256*12*.189*.7*1.01),size(256*12*.189*.8*1.01),160,82,220,255}
 };
 static constexpr Light background[]={
  {world_x(source(-267)),world_y(source(-7)),size(256*10*1.01),size(256*10*1.01),225,78,81,77},
  {world_x(source(2248)),world_y(source(1165)),size(256*10*1.01),size(256*10*1.01),106,78,168,77}
 };
 const water_reflection::Texture texture(c.a.content.image(c.a.content.ui_image(I13_RandomLight)),256,256);
 const auto*list=ambient?background:candles;const unsigned count=ambient?2:3;
 for(unsigned i=0;i<count;++i){const auto&l=list[i];
  // little_light_script: independent integer preprocessing in[0,5] seconds.
  const unsigned q=((clock+hash(seed+i+(ambient?90:3))%6*1000)%3000)*Q/3000;
  const auto color=ambient?gradient(AmbientColor,q):gradient(CandleColor,q);
  const int scale=ambient?curve(AmbientScale,q):curve(CandleScale,q);
  quad(c,texture,l.x,l.y,l.w*scale/Q,l.h*scale/Q,ambient?2023:0,
       div255(l.r*color.r),div255(l.g*color.g),div255(l.b*color.b),l.a,presentation::Add);
 }
}
static void stars(Canvas&c,uint32_t clock,uint32_t seed){
 const water_reflection::Texture texture(c.a.content.image(c.a.content.ui_image(I13_RandomDot)),256,256);
 for(unsigned i=0;i<300;++i){
  const uint64_t t=uint64_t(clock)+5000-i*10;const unsigned age=unsigned(t%3000),cycle=unsigned(t/3000);
  const uint32_t h=hash(seed^i*0x9e3779b9u^cycle*0x85ebca6bu);
  const int x=int(h%3000001)-1500000,y=int(hash(h+1)%2000001)-1000000;
  const int cx=world_x(source(925)+int(int64_t(x)*256*101/100000));
  const int drift=int(uint64_t(age)*age*5*256/1000000);
  const int cy=world_y(source(626)+(int(int64_t(y)*256/1000)-drift)*101/100);
  const int side=size(256*1.01)*(10+int(hash(h+2)%21))/1000;
  const auto color=gradient(StarColor,age*Q/3000);
  quad(c,texture,cx,cy,side,side,0,div255(224*color.r),div255(127*color.g),div255(100*color.b),div255(156*color.a),presentation::Normal);
 }
}
static void squares(Canvas&c,const Squares&state){
 const water_reflection::Texture texture(c.a.content.image(c.a.content.ui_image(I13_FakeMerchantSquareSpeck)));
 for(unsigned i=0;i<Squares::Count;++i){const auto&p=state.particles[i];if(!p.active)continue;
  const unsigned q=unsigned(p.age)*Q/Squares::Life;const int scale=curve(SquareScale,q)*int(p.scale)/1000;
  if(scale<=0)continue;const auto color=gradient(SquareColor,q);
  const unsigned alpha=unsigned(mx(0,curve(SquareAlpha,q)))*color.a*88/(Q*255);
  const int px=i<30?source(646.535):source(1712.87),py=i<30?source(-72.2772):source(64.3564);
  const int cx=world_x(source(-10)+(px+p.x/4)*101/100),cy=world_y(source(20)+(py+p.y/4)*101/100);
  const int w=size(24*1.01)*scale/Q,h=size(32*1.01)*scale/Q;
  const int angle=-int(int64_t(p.angular)*p.age*4096/(60*360));
  quad(c,texture,cx,cy,w,h,angle,color.r,color.g,color.b,alpha,presentation::Add);
 }
}
}
static bool fake_merchant_started_fight(const State&s){
 // FoulPotion already persists the six distinct remaining wares (or-1)
 // in these fields. Before the fight they are all zero. This retains the
 // original StartedFight fact without adding a State/RNG/save field.
 for(unsigned i=0;i<6;++i)if(s.original_events.values[2+i]!=0)return true;
 return false;
}
static bool fake_merchant_room(Canvas&c){
 auto&a=c.a;if(a.content.ui_image(I13_FakeMerchantSquareSpeck)<0)return false;
 if(c.clip_w<=0||c.clip_h<=22)return true;
 auto&v=fake_merchant::room;v.observe(a);const uint32_t elapsed=a.map_clock_ms-v.start;
 const bool fought=fake_merchant_started_fight(a.game);
 const bool focused=!fought&&!a.game.original_events.finished&&!a.shop_open&&merchant_hovered(a,0,195,92,67,95);
 const bool removed=fought&&has_world_relic(a.game,WorldRelicFakeMerchantsRug)&&a.content.ui_image(I13_FakeMerchantWithoutRug)>=0;
 fake_merchant::sequence(c,removed?9:0,uint64_t(elapsed)*1000);
 Canvas room=c;room.pixels+=22*room.pitch;if(room.coverage)room.coverage+=22*room.pitch;room.clip_h-=22;
 fake_merchant::glitter(room,a.map_clock_ms);
 fake_merchant::lights(room,elapsed,v.seed,false);
 fake_merchant::squares(room,v.squares);
 const uint32_t variation=fake_merchant::hash(v.seed^unsigned(v.floor)*0xa511e9b3u);
 const unsigned actor=4+unsigned(a.game.character),rate=900+variation%201;
 const int offset=int(fake_merchant::hash(variation)%1001)-500;
 const auto&source=FakeMerchantSequences[actor];
 const uint64_t actor_phase=uint64_t(int64_t(uint64_t(elapsed)*rate*24)+int64_t(offset)*24000+int64_t(source.count)*1000000);
 fake_merchant::sequence_frame(c,actor,unsigned(actor_phase/1000000%source.count));
 fake_merchant::lights(room,elapsed,v.seed,true);
 fake_merchant::stars(room,elapsed,v.seed);
 if(!fought)fake_merchant::sequence(c,focused?3:2,uint64_t(elapsed)*1000);
 fake_merchant::sequence(c,1,uint64_t(elapsed)*1000);
 return true;
}
