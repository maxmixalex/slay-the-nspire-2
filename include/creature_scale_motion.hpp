#pragma once
#include <stdint.h>

namespace spire {
// NCreature.ScaleTo animates Visuals with SineInOut for .75 seconds. Bounds,
// health bars and the formation itself retain their authored positions.
struct CreatureScaleMotion {
    uint32_t started=0;int from=1024,to=1024;uint16_t duration=750;
    static int sine_quarter(unsigned q){
        static constexpr int16_t values[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
        if(q>=1024)return 1024;const unsigned i=q/64,r=q%64;
        return (values[i]*(64-r)+values[i+1]*r)/64;
    }
    int sample(uint32_t now)const{
        if(int32_t(now-started)<0)return from;
        const unsigned age=now-started;if(age>=duration)return to;
        const unsigned q=age*2048u/duration;
        const int cosine=q<1024?sine_quarter(1024-q):-sine_quarter(q-1024);
        return from+(to-from)*(1024-cosine)/2048;
    }
    void observe(int target,uint32_t now,unsigned milliseconds=750){if(target!=to){from=sample(now);to=target;started=now;duration=uint16_t(milliseconds);}}
    // PhantasmalGardener's authored1+.1*ln(EnlargeTriggers+1). Range
    // reduction and five atanh terms keep Q10 error below one LCD unit
    // through the canonical9999-counter bound, without a per-frame libm call.
    static int enlarged(unsigned triggers){
        unsigned n=triggers+1,shift=0;while((n>>shift)>=2)++shift;
        const unsigned value=(uint64_t(n)*65536u)>>shift;
        const int z=int((uint64_t(value-65536)*65536)/(value+65536)),square=z*z/65536;
        int term=z,sum=z;for(int divisor=3;divisor<=9;divisor+=2){term=int(int64_t(term)*square/65536);sum+=term/divisor;}
        const int logarithm=int(shift)*45426+2*sum;
        return 1024+(logarithm*1024+327680)/655360;
    }
};
}
