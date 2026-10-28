#pragma once
#include "core.hpp"
namespace spire {
// Display-only forging/attack notices share the combat effect clock. No
// canonical values, cards or RNG state are changed by this bounded queue.
struct SovereignBladePresentation {
 static constexpr unsigned Capacity=12,Cues=64;
 struct Blade {uint16_t id=0,damage=0;uint32_t forged=0,attack=0,removed=0;int16_t tx=0,ty=0;bool active=false,forging=false,attacking=false,removing=false;};
 struct Cue {uint32_t at=0;uint16_t id=0,damage=0;int16_t x=0,y=0;uint8_t kind=0;};
 Blade blades[Capacity]{};Cue cues[Cues]{};uint16_t damage[MaxCards]{};
 uint8_t head=0,count=0;uint32_t started=0;bool overflow=false;
 void reset(){*this=SovereignBladePresentation{};}
 Blade* find(unsigned id){for(auto&b:blades)if(b.active&&b.id==id)return &b;return nullptr;}
 const Blade* find(unsigned id)const{for(const auto&b:blades)if(b.active&&b.id==id)return &b;return nullptr;}
 Blade& claim(unsigned id){if(auto*b=find(id))return *b;for(auto&b:blades)if(!b.active){b={};b.id=uint16_t(id);b.active=true;return b;}
  overflow=true;auto&b=blades[id%Capacity];b={};b.id=uint16_t(id);b.active=true;return b;}
 void begin(const State&s,uint32_t now){reset();started=now;
  for(unsigned id=0;id<s.card_count;++id)if(s.cards[id].kind==SovereignBlade){damage[id]=uint16_t(s.expansion.blade_damage[id]);
   if(s.zones[id]!=Zone::Exhaust){auto&b=claim(id);b.damage=damage[id];}}
 }
 void append(Cue cue){
  if(cue.kind==0)for(unsigned i=head;i<count;++i)if(cues[i].at==cue.at&&cues[i].id==cue.id&&cues[i].kind==0){cues[i]=cue;return;}
  if(count==Cues&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Cues){overflow=true;return;}cues[count++]=cue;
 }
 void observe(const State&s,const uint8_t*zones,unsigned known,uint32_t at){
  for(unsigned id=0;id<s.card_count;++id){
   if(s.cards[id].kind!=SovereignBlade){if(damage[id]){append({at,uint16_t(id),0,0,0,2});damage[id]=0;}continue;}
   const unsigned value=unsigned(s.expansion.blade_damage[id]);
   const bool was_exhausted=id<known&&(zones[id]&127)==unsigned(Zone::Exhaust),exhausted=s.zones[id]==Zone::Exhaust;
   if(value!=damage[id]||id>=known||(was_exhausted&&!exhausted))append({at,uint16_t(id),uint16_t(value),0,0,0});
   if(exhausted&&!was_exhausted)append({at,uint16_t(id),uint16_t(value),0,0,2});
   damage[id]=uint16_t(value);
  }
 }
 void attack(unsigned id,int x,int y,uint32_t impact){append({impact>180?impact-180:0,uint16_t(id),0,int16_t(x),int16_t(y),1});}
 void advance(uint32_t now){
  // Attack notices may precede a later same-command forge notice. Process
  // every due item, preserving source order for equal timestamps.
  unsigned remaining=0;
  for(unsigned i=head;i<count;++i){const auto q=cues[i];if(int32_t(now-q.at)<0){cues[remaining++]=q;continue;}
   if(q.kind==0){auto&b=claim(q.id);b.damage=q.damage;b.forged=q.at;b.forging=true;b.removing=false;}
   else if(auto*b=find(q.id)){if(q.kind==1){b->attack=q.at;b->tx=q.x;b->ty=q.y;b->attacking=true;}
    else {b->removed=q.at;b->removing=true;}}
  }head=0;count=uint8_t(remaining);
  for(auto&b:blades){if(b.removing&&uint32_t(now-b.removed)>=200)b.active=false;
   if(b.forging&&uint32_t(now-b.forged)>=900)b.forging=false;if(b.attacking&&uint32_t(now-b.attack)>=800)b.attacking=false;}
 }
 static unsigned cubic(unsigned t,unsigned duration){if(t>=duration)return 4096;const unsigned q=4096-t*4096/duration;return 4096-q*q/4096*q/4096;}
 static unsigned size(const Blade&b,uint32_t now){
  unsigned scale=3686+4506u*(b.damage>200?200:b.damage)/200;
  if(b.forging){const unsigned age=now-b.forged;if(age<50)scale=scale*(4096+cubic(age,50)/5)/4096;
   else if(age<350)scale=scale*(4096+(4096-cubic(age-50,300))/5)/4096;}
  if(b.removing)scale=scale*(4096-cubic(now-b.removed,200))/4096;return scale;
 }
 struct Point {int x=0,y=0;unsigned phase=0;};
 Point orbit(unsigned index,uint32_t now)const{
  struct SourcePoint {int16_t x,y;};static constexpr SourcePoint path[]={
#include "sovereign_blade_path.inc"
  };
  unsigned total=0,ordinal=0;for(unsigned i=0;i<Capacity;++i)if(blades[i].active){if(i<index)++ordinal;++total;}
  const unsigned phase=unsigned(uint64_t(now-started)*65536u/21862u)+(total?ordinal*65536u/total:0);
  const unsigned segment=(phase>>10)&63,fraction=phase&1023;
  const auto a=path[segment],b=path[segment+1];
  return {a.x+(b.x-a.x)*int(fraction)/1024,a.y+(b.y-a.y)*int(fraction)/1024,phase&65535};
 }
};
static_assert(sizeof(SovereignBladePresentation)<4096,"Forging must not retain a second combat state");
}
