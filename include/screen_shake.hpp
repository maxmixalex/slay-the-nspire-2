#pragma once
#include "combat_card_motion.hpp"

namespace spire {
// NScreenShake: the latest directional punch overrides rumble; trauma is
// additive. This presentation state never owns a State, RNG or pixel buffer.
// Perlin-style lattice noise replaces Godot's floating-point FastNoiseLite;
// strengths, envelopes, durations and trauma decay keep their source units.
struct ScreenShakePresentation {
 enum Kind:uint8_t {Punch,Rumble,Drunk,Trauma};
 enum Strength:uint8_t {None,VeryWeak=2,Weak=5,Medium=20,Strong=40,TooMuch=80};
 struct Cue {uint32_t at=0;uint16_t duration=0,angle=0;uint8_t strength=0,kind=0;};
 static constexpr unsigned Capacity=48;
 Cue cues[Capacity]{},punch{},rumble{};
 uint32_t target=0,clock=0,trauma_at=0,noise_seed=1,serial=0;
 int32_t drunk_x=0,drunk_y=0;
 uint16_t trauma=0;
 uint8_t count=0;
 bool bound=false,overflow=false;
 static uint32_t hash(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
 static unsigned multiplier(unsigned option){const unsigned m[]={0,512,1024,2048,4096};return m[option<5?option:2];}
 static unsigned cubic_out(unsigned elapsed,unsigned duration){
  if(!duration||elapsed>=duration)return 0;const unsigned t=elapsed*1024u/duration;return 1024u-t*t/1024u*t/1024u;
 }
 static int noise(int64_t phase,uint32_t seed){
  // Q10 gradient noise, quintic interpolation; negative coordinates use a
  // mathematical floor rather than implementation-dependent signed shifts.
  const int64_t cell=phase>=0?phase/1024:(phase-1023)/1024;
  const int t=int(phase-cell*1024),t2=t*t/1024,t3=t2*t/1024;
  const int smooth=t3*(10*1024-15*t+6*t2)/1024;
  const int a=(hash(uint32_t(cell)^seed)&1u)?t:-t;
  const int b=(hash(uint32_t(cell+1)^seed)&1u)?t-1024:1024-t;
  return 2*(a+(b-a)*smooth/1024);
 }
 static unsigned root(uint64_t v){uint64_t out=0,bit=uint64_t(1)<<62;while(bit>v)bit>>=2;while(bit){if(v>=out+bit){v-=out+bit;out=(out>>1)+bit;}else out>>=1;bit>>=2;}return unsigned(out);}
 void reset(){*this=ScreenShakePresentation{};}
 void bind(uint32_t id,uint32_t now){if(bound&&target==id)return;reset();bound=true;target=id;clock=trauma_at=now;noise_seed=hash(id^0x74d13ab1u);}
 unsigned trauma_now(uint32_t now)const{
  const uint32_t age=now-trauma_at;return age>=500u?0u:trauma>age*2048u/1000u?trauma-age*2048u/1000u:0u;
 }
 void apply(Cue cue){
  if(cue.kind==Trauma){
   const unsigned add=cue.strength==VeryWeak?72u:cue.strength==Weak?205u:cue.strength==Medium?410u:cue.strength==Strong?614u:cue.strength==TooMuch?1024u:0u;
   const unsigned sum=trauma_now(cue.at)+add;trauma=uint16_t(sum>1024?1024:sum);trauma_at=cue.at;
  }else if(cue.kind==Punch)punch=cue;
  else{rumble=cue;drunk_x=drunk_y=0;noise_seed=hash(target^(++serial*0x9e3779b9u));}
 }
 void advance(uint32_t now){
  if(!bound)return;const uint32_t elapsed=now-clock;clock=now;
  unsigned consumed=0;while(consumed<count&&int32_t(now-cues[consumed].at)>=0)apply(cues[consumed++]);
  if(consumed){for(unsigned i=consumed;i<count;++i)cues[i-consumed]=cues[i];count=uint8_t(count-consumed);}
  if(rumble.kind==Drunk&&rumble.strength&&uint32_t(now-rumble.at)<rumble.duration){
   const int64_t phase=int64_t(rumble.duration-uint32_t(now-rumble.at))*1024/10000;
   const unsigned weight=elapsed>=100?1024:elapsed*1024u/100u;
   const int nx=noise(phase+int64_t(noise_seed%99999u)*1024,noise_seed),ny=noise(phase+int64_t(noise_seed%99999u)*2048,noise_seed);
   drunk_x+=(nx-drunk_x)*int(weight)/1024;drunk_y+=(ny-drunk_y)*int(weight)/1024;
  }
 }
 void add(uint32_t now,uint32_t at,Kind kind,unsigned strength,unsigned duration=300,unsigned angle=0){
  advance(now);Cue cue{at,uint16_t(duration>65535?65535:duration),uint16_t(angle&4095u),uint8_t(strength),uint8_t(kind)};
  if(int32_t(now-at)>=0){apply(cue);return;}
  // Coalesce only the same command instant: a later punch replaces the
  // previous one, whereas trauma additions retain their additive semantics.
  if(kind!=Trauma)for(unsigned i=0;i<count;++i)if(cues[i].at==at&&cues[i].kind==kind){cues[i]=cue;return;}
  if(count==Capacity){overflow=true;return;}
  unsigned at_index=count;while(at_index&&int32_t(at-cues[at_index-1].at)<0){cues[at_index]=cues[at_index-1];--at_index;}
  cues[at_index]=cue;++count;
 }
 static void punch_offset(const Cue&cue,uint32_t now,unsigned scale,int&x,int&y){
  const uint32_t age=now-cue.at;if(!cue.strength||age>=cue.duration)return;
  const unsigned envelope=cubic_out(age,cue.duration);
  const int phase=int((uint64_t(cue.duration-age)*391139u+5000u)/10000u);
  const int amplitude=int(unsigned(cue.strength)*scale*envelope/1024u)*CombatCardMotion::sine(phase+1024)/1024;
  x=amplitude*CombatCardMotion::sine(cue.angle+1024)/1024;y=amplitude*CombatCardMotion::sine(cue.angle)/1024;
 }
 void sample(uint32_t now,unsigned option,int&x,int&y,const Cue*damage=nullptr)const{
  x=y=0;const unsigned scale=multiplier(option);if(!scale)return;
  const Cue*latest=&punch;if(damage&&damage->strength&&(!punch.strength||int32_t(damage->at-punch.at)>=0))latest=damage;
  if(latest->strength&&uint32_t(now-latest->at)<latest->duration)punch_offset(*latest,now,scale,x,y);
  else if(rumble.strength&&uint32_t(now-rumble.at)<rumble.duration){
   const unsigned age=now-rumble.at,envelope=cubic_out(age,rumble.duration);
   const int strength=int(unsigned(rumble.strength)*scale*envelope/1024u);
   if(rumble.kind==Drunk){x=drunk_x*strength/1024*5;y=drunk_y*strength/1024*5;}
   else{const int64_t phase=int64_t(rumble.duration-age)*6144/1000;
    x=noise(phase+int64_t(noise_seed%99999u)*1024,noise_seed)*strength/2048;
    y=noise(phase+int64_t(noise_seed%99999u)*2048,noise_seed)*strength/2048;}
  }
  const unsigned t=trauma_now(now);if(t){
   const unsigned amplitude=t*root(t*1024u)/1024u*200u*scale/1024u;
   int tx=int(int64_t(noise(int64_t(now)*1024/100+1024,noise_seed^0xa6f1u))*amplitude/1024);
   int ty=int(int64_t(noise(int64_t(now)*1024/100+3072,noise_seed^0xa6f1u))*amplitude/1024);
   const unsigned length=root(uint64_t(int64_t(tx)*tx+int64_t(ty)*ty));
   if(length>50u*1024u){tx=int(int64_t(tx)*50*1024/length);ty=int(int64_t(ty)*50*1024/length);}x+=tx;y+=ty;
  }
  x=x*4/21;y=y*4/21;x=(x+(x<0?-512:512))/1024;y=(y+(y<0?-512:512))/1024;
  // Even the original debug "TooMuch/Drunk" cannot expose an unbounded
  // copy or displace the calculator's complete scene out of its viewport.
  x=x<-64?-64:x>64?64:x;y=y<-48?-48:y>48?48:y;
 }
};
static_assert(sizeof(ScreenShakePresentation)<768,"Screen motion is bounded and owns no pixels");
}
