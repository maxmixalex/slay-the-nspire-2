#pragma once
#include <stdint.h>
#include "combat_card_motion.hpp"

namespace spire { namespace fake_merchant {
constexpr int Q=4096;
struct Key {int16_t x,y;int32_t left,right;};
constexpr int fixed(double n){return int(n*Q+(n<0?-.5:.5));}
constexpr Key key(double x,double y,double left=0,double right=0){return {int16_t(fixed(x)),int16_t(fixed(y)),fixed(left),fixed(right)};}
// The source Curve resources use free Hermite tangents. Only the numeric
// projection is fixed-point; the authored keys and slopes remain unchanged.
inline int curve(const Key*keys,unsigned count,unsigned at){
 if(at<=unsigned(keys[0].x))return keys[0].y;
 for(unsigned i=1;i<count;++i)if(at<=unsigned(keys[i].x)){
  const auto&a=keys[i-1];const auto&b=keys[i];const int span=b.x-a.x;
  const int t=int(at-a.x)*Q/span,t2=t*t/Q,t3=t2*t/Q;
  const int64_t value=int64_t(2*t3-3*t2+Q)*a.y+int64_t(-2*t3+3*t2)*b.y+
   int64_t(t3-2*t2+t)*span*a.right/Q+int64_t(t3-t2)*span*b.left/Q;
  return int(value/Q);
 }return keys[count-1].y;
}
template<unsigned N>inline int curve(const Key(&keys)[N],unsigned at){return curve(keys,N,at);}
constexpr Key CandleScale[]={key(0,.417635),key(.502521,.507939),key(1,.423164)};
constexpr Key AmbientScale[]={key(0,.996333),key(.503344,.976164),key(1,.996333)};
// The light emitters use three zero-tangent keys. Retain the general source
// keys above for equivalence checks; only these compact coordinates are live.
struct LightCurve {int16_t x[3],y[3];};
constexpr bool light_curve_compatible(const Key(&keys)[3]){
 for(unsigned i=0;i<3;++i)if(keys[i].left||keys[i].right||keys[i].y<0||keys[i].y>Q||
     (i&&keys[i].x<=keys[i-1].x))return false;
 return keys[0].x==0&&keys[2].x==Q;
}
constexpr LightCurve light_curve(const Key(&keys)[3]){
 return {{keys[0].x,keys[1].x,keys[2].x},{keys[0].y,keys[1].y,keys[2].y}};
}
inline int curve(const LightCurve&keys,unsigned at){
 if(at<=unsigned(keys.x[0]))return keys.y[0];
 for(unsigned i=1;i<3;++i)if(at<=unsigned(keys.x[i])){
  const int t=int(at-keys.x[i-1])*Q/(keys.x[i]-keys.x[i-1]),t2=t*t/Q,t3=t2*t/Q;
  // Same separately rounded Hermite terms and final signed division as
  // curve(Key). Bounded y[0..Q] and t[0..Q] keep their sum within int32.
  return ((2*t3-3*t2+Q)*keys.y[i-1]+(-2*t3+3*t2)*keys.y[i])/Q;
 }return keys.y[2];
}
static_assert(light_curve_compatible(CandleScale)&&light_curve_compatible(AmbientScale),"light curve source bounds");
constexpr auto CandleLightScale=light_curve(CandleScale),AmbientLightScale=light_curve(AmbientScale);
// No preprocessing: source restart_time < time excludes an exact first
// birth, and retains the terminal pose at later exact cycle boundaries.
constexpr unsigned light_age(uint32_t clock,unsigned particle){
 const unsigned phase=(particle&1u)*1500u;
 return clock<=phase?0u:(clock-phase-1u)%3000u+1u;
}
// little_light_script chooses one integer preprocessing time in[0,5] seconds
// per node. Reduce the elapsed clock before addition to avoid uint32 wrap.
constexpr unsigned preprocessed_light_age(uint32_t clock,unsigned particle,unsigned seconds){
 const unsigned phase=(particle&1u)*1500u,preprocess=seconds*1000u;
 if(clock<=phase&&preprocess<=phase-clock)return 0;
 return (clock%3000u+preprocess+3000u-phase-1u)%3000u+1u;
}
constexpr Key SquareScale[]={key(0,0,0,3.42211),key(.0678295,.354897),key(.143411,.0674747),
 key(.234496,.457091),key(.317829,.144121),key(.395349,.254831),key(.48062,.0930233),
 key(.532946,.363413),key(.606589,.0461842),key(.705426,.450704),key(.75969,.12283),
 key(.815892,.308058),key(.872093,.0696037),key(.947674,.21225),key(1,0,-6.93645)};
constexpr Key SquareAlpha[]={key(0,1),key(1,0,-.00121226)};
constexpr Key TurbulenceLife[]={key(0,0,0,1.68527),key(1,1,2.0572)};
struct ColorKey {uint16_t at;uint8_t r,g,b,a;};
constexpr ColorKey color_key(double at,double r,double g,double b,double a=1){
 return {uint16_t(fixed(at)),uint8_t(r*255+.5),uint8_t(g*255+.5),uint8_t(b*255+.5),uint8_t(a*255+.5)};
}
constexpr ColorKey CandleColor[]={color_key(0,.427493,.427493,.427493),color_key(.607407,1,1,1),color_key(.994413,.427451,.427451,.427451)};
constexpr ColorKey AmbientColor[]={color_key(0,.720467,.720468,.720467),color_key(.607407,1,1,1),color_key(.994413,.721569,.721569,.721569)};
constexpr ColorKey StarColor[]={color_key(0,.847059,.803922,.301961,0),color_key(.258845,1,1,1),color_key(1,.780392,.65098,.403922,0)};
constexpr ColorKey SquareColor[]={color_key(0,0,0,0,0),color_key(.13986,.402864,.99397,1),
 color_key(.367619,.365884,.992627,1),color_key(.4,.360784,.992157,1,0),color_key(.422857,.352941,.992157,1),
 color_key(.539048,.308699,.992931,1),color_key(.63619,.506445,.995114,1),color_key(.666667,.560784,.996078,1,0),
 color_key(.704762,.66484,.996863,1),color_key(.798095,.94902,1,1),color_key(.868571,.94902,1,1),
 color_key(.902857,.94902,1,1,0),color_key(.931429,.94902,1,1),color_key(.967619,.94902,1,1),color_key(1,.94902,1,1,0)};
template<unsigned N>inline ColorKey gradient(const ColorKey(&keys)[N],unsigned at){
 if(at<=keys[0].at)return keys[0];for(unsigned i=1;i<N;++i)if(at<=keys[i].at){
  const auto&a=keys[i-1];const auto&b=keys[i];const unsigned q=(at-a.at)*Q/(b.at-a.at);
  const auto mix=[&](unsigned x,unsigned y){return uint8_t(int(x)+(int(y)-int(x))*int(q)/Q);};
  return {uint16_t(at),mix(a.r,b.r),mix(a.g,b.g),mix(a.b,b.b),mix(a.a,b.a)};
 }return keys[N-1];
}
inline uint32_t hash(uint32_t v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
inline unsigned root(uint64_t n){uint64_t r=0,b=uint64_t(1)<<62;while(b>n)b>>=2;
 while(b){if(n>=r+b){n-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}return unsigned(r);}
struct Square {
 int32_t x=0,y=0,vx=0,vy=0; // Q10 source coordinates and velocity.
 uint32_t birth=0,seed=0;
 uint16_t age=0,scale=0,influence=0;
 int16_t angular=0,pan_x=0,pan_y=0;
 bool active=false;
};
struct Squares {
 static constexpr unsigned Count=60,Life=840,Preprocess=900;
 Square particles[Count]{};
 uint32_t tick=0,seed=0;bool initialized=false;
 void spawn(Square&p,unsigned index){
  const uint32_t h=hash(seed^index*0x9e3779b9u^(p.birth/Life)*0x85ebca6bu);
  p.seed=h;p.active=true;p.age=0;p.vx=p.vy=p.y=0;
  // Original sphere-surface emission, flattened by scale(100,0,1).
  const int latitude=int(hash(h)%2049)-1024;
  const int radius=int(root(unsigned(1024*1024-latitude*latitude)));
  p.x=CombatCardMotion::sine(int(h>>12))*radius*100/1024;
  p.scale=uint16_t(300+hash(h+1)%701);p.angular=int16_t(int(hash(h+2)%321)-160);
  p.influence=uint16_t(fixed(.164)+hash(h+3)%unsigned(fixed(.196)-fixed(.164)+1));
  p.pan_x=int16_t(int(hash(h+4)%5121)-2560);p.pan_y=int16_t(int(hash(h+5)%5121)-2560);
 }
 void reset(uint32_t next,uint32_t source_seed){
  seed=source_seed;tick=next>Life?next-Life:0;initialized=true;
  for(unsigned i=0;i<Count;++i){auto&p=particles[i];p={};const unsigned phase=(i%30)*28;
   p.birth=phase+(tick>phase?(tick-phase+Life-1)/Life:0)*Life;}
 }
 void advance(uint32_t next,uint32_t source_seed){
  if(!initialized||seed!=source_seed||next+1<tick||(next>=tick&&next-tick>Life))reset(next,source_seed);
  for(;tick<=next;++tick){for(unsigned i=0;i<Count;++i){auto&p=particles[i];
   if(tick==p.birth){spawn(p,i);continue;}if(!p.active)continue;
   if(++p.age>=Life){p.active=false;p.birth+=Life;spawn(p,i);continue;}
   p.vx-=100*1024/60;p.vy+=50*1024/60;
   const unsigned age=unsigned(p.age)*Q/Life;
   const int influence=int(p.influence)*curve(TurbulenceLife,age)/Q;
   // Same bounded direction/speed response as ParticleProcessMaterial.
   // Its GPU curl-noise vector is evaluated by a deterministic CPU curl
   // field, with the authored8.636 scale and2.56 random panning retained.
   const int phase_x=int((int64_t(p.x)*4096/555642+int64_t(tick)*(1000+p.pan_x)*4096/376991)&4095);
   const int phase_y=int((int64_t(p.y)*4096/555642+int64_t(tick)*(1625+p.pan_y)*4096/376991)&4095);
   int dx=CombatCardMotion::sine(phase_y),dy=-CombatCardMotion::sine(phase_x);
   const unsigned norm=root(uint64_t(int64_t(dx)*dx+int64_t(dy)*dy));
   const unsigned speed=root(uint64_t(int64_t(p.vx)*p.vx+int64_t(p.vy)*p.vy));
   if(norm){const int boost=Q+(Q-influence)/5;
    dx=int(int64_t(dx)*speed*boost/(int64_t(norm)*Q));dy=int(int64_t(dy)*speed*boost/(int64_t(norm)*Q));
    p.vx=int((int64_t(p.vx)*(Q-influence)+int64_t(dx)*influence)/Q);
    p.vy=int((int64_t(p.vy)*(Q-influence)+int64_t(dy)*influence)/Q);}
   p.x+=p.vx/60;p.y+=p.vy/60;
  }}
 }
};
static_assert(sizeof(Squares)<4096,"Two source-sized square emitters have a bounded presentation state");
}}
