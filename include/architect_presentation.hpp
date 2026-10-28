#pragma once
#include "world_card_factory.hpp"
#include "screen_shake_sources.hpp"

namespace spire {
// TheArchitect's visual-only encounter owns a second, presentation-local
// event RNG. Repainting, reloading or disabling camera shake cannot consume
// a gameplay stream or replay an already-completed opening sequence.
struct ArchitectPresentation {
 enum Vfx:uint8_t {Blunt,HeavyBlunt,Slash,BloodyImpact,RockShatter,DaggerSpray,
  FlyingSlash,DramaticStab,DaggerThrow,Lightning,Scratch,StarryImpact,Thrash};
 struct Hit {uint32_t amount=0,seed=0;uint8_t vfx=0;};
 reference::Random rng{};
 ArchitectShakeTimeline plan{};
 Hit hits[5]{};
 uint32_t start=0,entered=0,score=0;
 bool bound=false,opening=false,fast=false;
 void begin(Character character,unsigned dialogue,bool first,bool quick,uint32_t now,
            uint32_t total,const uint64_t*words,uint32_t calls,uint32_t seed){
  if(!bound||first){
   entered=now;score=total;
   for(unsigned i=0;i<4;++i)rng.words[i]=words[i];rng.calls=calls;
   rng.good=(rng.words[0]|rng.words[1]|rng.words[2]|rng.words[3])!=0;
   if(!rng.good)rng.reset(seed);
  }
  bound=true;opening=first;fast=quick;start=now;
  plan=source_architect_shake(character,dialogue,first,quick);
  static constexpr uint8_t lists[5][5]={
   {Blunt,HeavyBlunt,Slash,BloodyImpact,RockShatter},
   {DaggerSpray,FlyingSlash,DramaticStab,DaggerThrow,0},
   {Lightning,Blunt,Scratch,Slash,HeavyBlunt},
   {StarryImpact,Blunt,Slash,HeavyBlunt,Lightning},
   {Thrash,HeavyBlunt,Slash,BloodyImpact,0}
  };
  for(unsigned i=0;i<5;++i)hits[i]={};
  if(!plan.hits)return;
  uint8_t vfx[5]{};for(unsigned i=0;i<plan.hits;++i)vfx[i]=lists[unsigned(character)][i];
  reference::shuffle(vfx,plan.hits,rng);
  const unsigned big=rng.below(plan.hits);unsigned small;
  do{small=rng.below(plan.hits);}while(small==big);
  using reference::math::Real;Real weights[5]{},sum=Real::integer(0);
  for(unsigned i=0;i<plan.hits;++i){
   const Real low=i==big?Real::integer(2):i==small?world_float(0x3dcccccdU):world_float(0x3f333333U);
   const Real high=i==big?Real::integer(3):i==small?world_float(0x3f000000U):world_float(0x3fa66666U);
   weights[i]=world_float_round(Real::fraction53(rng.next()>>11)*world_float_round(high-low)+low);
   sum=sum+weights[i];
  }
  uint32_t used=0,cosmetic=seed^now^0x41cef1eaU;
  for(unsigned i=0;i<plan.hits;++i){
   const int amount=i+1==plan.hits?int(score)-int(used):world_float_truncate(Real::integer(int(score))*weights[i]/sum);
   cosmetic^=cosmetic<<13;cosmetic^=cosmetic>>17;cosmetic^=cosmetic<<5;
   hits[i]={uint32_t(amount<1?1:amount),cosmetic?cosmetic:1u,vfx[i]};used+=hits[i].amount;
  }
 }
 bool intro(uint32_t now)const{return bound&&opening&&int32_t(now-start)>=0&&now-start<plan.complete;}
 unsigned head_stop()const{return plan.complete-500u;}
};
static_assert(sizeof(ArchitectPresentation)<160,"The final event retains only bounded source cue metadata");
}
