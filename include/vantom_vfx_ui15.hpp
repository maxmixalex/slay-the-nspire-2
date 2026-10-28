#pragma once
#include <stdint.h>

namespace spire {
// NVantomVfx's six authored events. These are source timing facts, not a
// substitute for the independently sampled body/tail skeleton clocks.
namespace vantom_vfx {
enum Animation:uint8_t {Other,Attack,AttackDouble,Debuff,Die,Idle};
enum Event:uint8_t {DissolveTail,SprayOn,SprayOff,DeathSprayOn,DeathSprayOff,DeathExplosion};
enum Emitter:uint8_t {Spray,DeathFront,DeathBack,Explosion,EmitterCount};
// Original emitter local-to-screen matrix after the clip's registration.
// Linear coefficients useQ20; translation usesQ12. Root-scale inheritance
// is resolved by Canvas::vantom_emitter before particles use this value.
struct EmitterTransform {int32_t a=0,b=0,c=0,d=0,x=0,y=0;uint16_t depth=0,flags=0;};
struct Cue {uint16_t at;Event event;};
struct Sequence {const Cue*cues;unsigned count,duration;};
inline Sequence sequence(Animation animation){
 static constexpr Cue attack[]={{933,SprayOn},{1033,DeathSprayOff}};
 static constexpr Cue debuff[]={{467,SprayOn},{667,SprayOff}};
 static constexpr Cue die[]={{33,DissolveTail},{200,DeathSprayOn},{2133,DeathExplosion},{2133,DeathSprayOff}};
 static constexpr Cue idle[]={{33,SprayOff}};
 switch(animation){
 case Attack:case AttackDouble:return {attack,2,1267};
 case Debuff:return {debuff,2,1333};
 case Die:return {die,4,3333};
 case Idle:return {idle,1,3333};
 default:return {nullptr,0,0};
 }
}
inline bool equal(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
inline Animation animation(const char*name){
 if(equal(name,"attack"))return Attack;if(equal(name,"attack_double"))return AttackDouble;
 if(equal(name,"debuff"))return Debuff;if(equal(name,"die"))return Die;if(equal(name,"idle_loop"))return Idle;
 // Every retained tail-only request is sampled with its original idle body.
 // Track1 itself has no events, but track0 still emits spray_off at33ms.
 if(equal(name,"_tracks/attack_heavy")||equal(name,"_tracks/charge_up_1")||
    equal(name,"_tracks/charge_up_2")||equal(name,"_tracks/charge_up_3")||
    equal(name,"_tracks/charged_0")||equal(name,"_tracks/charged_1")||
    equal(name,"_tracks/charged_2")||equal(name,"_tracks/charged_3"))return Idle;
 return Other;
}
inline uint32_t hash(uint32_t value){value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;return value^(value>>16);}
inline unsigned lifetime(Emitter kind,unsigned random_fraction){
 // Only the three spray emitters have speed_scale1.2. Godot shortens a
 // particle's lifetime by randomness * random; motion uses that same clock.
 return kind==Explosion?1000u-400u*random_fraction/65535u:(800u-208u*random_fraction/65535u)*5u/6u;
}
struct Clock {
 struct Stream {uint32_t began=0,next=0;bool active=false;} streams[3]{};
 uint32_t last=0,started=0,dissolve_started=0;
 unsigned cursor=0,cycle=0;Animation current=Other;
 bool initialized=false,loop=false,dissolving=false,restart_pending=false;
 void reset(){*this={};}
 unsigned dissolve(uint32_t now)const{
  if(!dissolving)return 0;const uint32_t age=now-dissolve_started;
  return age>=1000?4096u:unsigned(uint64_t(age)*age*4096u/1000000u);
 }
 template<class Emit> void emit_until(uint32_t until,uint32_t visible_now,Emit emit){
  for(unsigned kind=0;kind<3;++kind){auto&s=streams[kind];if(!s.active)continue;
   const unsigned amount=kind==Spray?80u:50u;
   // 800ms lifetime /1.2, uniformly distributed over80 or50 particles.
   // Skip births which cannot survive a delayed render; none are painted or
   // charged to a gameplay RNG. Their deterministic ordinal still advances.
   if(int32_t(visible_now-s.began)>1000){const uint32_t cutoff=visible_now-s.began-1000;
    const uint32_t first=uint32_t(uint64_t(cutoff)*amount*3u/2000u);if(first>s.next)s.next=first;}
   for(;;){const uint32_t born=s.began+uint32_t(uint64_t(s.next)*2000u/(amount*3u));
    if(int32_t(born-until)>0)break;const uint32_t ordinal=s.next++;
    emit(Emitter(kind),born,ordinal);
   }
  }
 }
 template<class Emit,class Restart> void span(uint32_t until,uint32_t visible_now,Emit emit,Restart restart){
  const uint32_t done=dissolve_started+1000;
  if(restart_pending&&int32_t(done-until)<=0){
   emit_until(done,visible_now,emit);restart_pending=false;restart(done);
  }
  emit_until(until,visible_now,emit);
 }
 template<class Emit> void event(Event value,uint32_t at,Emit emit){
  switch(value){
  case DissolveTail:dissolve_started=at;dissolving=restart_pending=true;break;
  case SprayOn:if(!streams[Spray].active)streams[Spray]={at,0,true};break;
  case SprayOff:streams[Spray].active=false;break;
  case DeathSprayOn:
   for(unsigned i=DeathFront;i<=DeathBack;++i)if(!streams[i].active)streams[i]={at,0,true};break;
  case DeathSprayOff:streams[DeathFront].active=streams[DeathBack].active=false;break;
  case DeathExplosion:for(unsigned i=0;i<30;++i)emit(Explosion,at,i);break;
  }
 }
 template<class Emit,class Restart> void advance(uint32_t until,uint32_t visible_now,Emit emit,Restart restart){
  const auto table=sequence(current);
  while(cursor<table.count){const uint32_t at=started+cycle*table.duration+table.cues[cursor].at;
   if(int32_t(at-until)>0)break;
   span(at,visible_now,emit,restart);event(table.cues[cursor].event,at,emit);++cursor;
   if(cursor==table.count&&loop&&table.duration){
    cursor=0;++cycle;
    // Idle is the only looping source with an event. Once its stop has
    // fired, old empty cycles can be skipped without changing any stream.
    if(current==Idle&&int32_t(until-started)>int32_t(table.duration)){
     const unsigned latest=(until-started)/table.duration;if(latest>cycle)cycle=latest;
    }
   }
  }
  span(until,visible_now,emit,restart);last=until;
 }
 template<class Emit,class Restart> void observe(Animation next,uint32_t begin,uint32_t now,bool repeats,Emit emit,Restart restart,uint32_t visible_now=UINT32_MAX){
  if(visible_now==UINT32_MAX)visible_now=now;
  if(initialized&&int32_t(now-last)<0)reset();
  if(!initialized){initialized=true;current=next;started=last=begin;loop=repeats;}
  else if(current!=next||started!=begin||loop!=repeats){
   if(int32_t(begin-last)>=0)advance(begin,visible_now,emit,restart);
   current=next;started=begin;cursor=cycle=0;loop=repeats;
   // The original resets the shader on every animation start except while
   // the body is dying. Its pending tween/callback is not cancelled.
   if(next!=Die&&!restart_pending)dissolving=false;
  }
  advance(now,visible_now,emit,restart);
 }
};
constexpr unsigned BasicParticle=5404,DeathParticleStart=5405,AuxiliaryCount=5409;
}
}
