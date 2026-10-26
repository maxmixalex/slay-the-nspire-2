#pragma once
#include <stdint.h>

namespace spire {
// NCreature.ScaleTo animates Visuals with SineInOut for .75 seconds. Bounds,
// health bars and the formation itself retain their authored positions.
struct CreatureScaleMotion {
    uint32_t started=0;int from=1024,to=1024;
    static int sine_quarter(unsigned q){
        static constexpr int16_t values[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
        if(q>=1024)return 1024;const unsigned i=q/64,r=q%64;
        return (values[i]*(64-r)+values[i+1]*r)/64;
    }
    int sample(uint32_t now)const{
        const unsigned age=now-started;if(age>=750)return to;
        const unsigned q=age*2048u/750u;
        const int cosine=q<1024?sine_quarter(1024-q):-sine_quarter(q-1024);
        return from+(to-from)*(1024-cosine)/2048;
    }
    void observe(int target,uint32_t now){if(target!=to){from=sample(now);to=target;started=now;}}
};
}
