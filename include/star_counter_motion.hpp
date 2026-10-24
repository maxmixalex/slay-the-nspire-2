#pragma once
#include <stdint.h>

namespace spire {
// NStarCounter's .1 s critically damped count, evaluated without libm or
// software floating-point support on the freestanding calculator target.
struct StarCounterMotion {
 static constexpr int Scale=16384;
 int32_t value=0,velocity=0;
 void set(int count){value=count*Scale;velocity=0;}
 // UpdateStarCount immediately writes the new amount on a loss, retaining
 // SmoothDamp's velocity if a previous gain is still moving.
 void decrease(int count){value=count*Scale;}
 static int64_t rounded(int64_t n,int64_t d){
  const uint64_t magnitude=uint64_t(n<0?-n:n),divisor=uint64_t(d);
  const int64_t result=int64_t((magnitude+divisor/2)/divisor);return n<0?-result:result;
 }
 void advance(int target,uint32_t elapsed){
  if(!elapsed)return;if(elapsed>1000)elapsed=1000;
  const int32_t goal=target*Scale;if(value==goal&&!velocity)return;
  // For x=20*dt and dt=milliseconds/1000, the original reciprocal
  // 1/(1+x+.48*x*x+.235*x*x*x) has this exact integer denominator.
  const uint64_t ms=elapsed;
  const uint64_t denominator=25000000u+500000u*ms+4800u*ms*ms+47u*ms*ms*ms;
  const int64_t decay=int64_t(((uint64_t(25000000u)<<30)+denominator/2)/denominator);
  const int64_t change=int64_t(value)-goal;
  const int64_t step=rounded((int64_t(velocity)+20*change)*elapsed,1000);
  const int64_t next=int64_t(goal)+rounded((change+step)*decay,int64_t(1)<<30);
  velocity=int32_t(rounded((int64_t(velocity)-20*step)*decay,int64_t(1)<<30));
  if((goal>value)==(next>goal)){value=goal;velocity=0;}else value=int32_t(next);
 }
 int displayed()const{
  const int whole=value/Scale,fraction=value%Scale;
  // Mathf.RoundToInt uses midpoint-to-even rounding.
  return whole+(fraction>Scale/2||(fraction==Scale/2&&(whole&1)));
 }
};
}
