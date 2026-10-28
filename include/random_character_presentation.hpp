#pragma once
#include <stdint.h>

namespace spire {
// The Random scene's two 20-particle CPU emitters. All source particles are
// retained, with the source 15s warmup, uniform emission times, gravity,
// radial acceleration and clockwise orbit. Q10 integration uses <=34ms
// steps, including Godot's 30Hz preprocessing, without a gameplay RNG.
struct RandomCharacterPresentation {
    static constexpr unsigned Count=40,LifeMs=15000;
    struct Particle {
        int32_t x=0,y=0,vx=0,vy=0;
        uint32_t cycle=0;
        uint16_t age=0,radial=0,orbit=0,scale=1024,angle=0;
    } particles[Count]{};
    uint32_t seed=0,elapsed=0;
    bool active=false;
    static uint32_t hash(uint32_t v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
    static unsigned root(unsigned n){unsigned result=0,bit=1u<<30;while(bit>n)bit>>=2;
        while(bit){if(n>=result+bit){n-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;}
    void birth(unsigned index,uint32_t cycle){
        auto&p=particles[index];p={};p.cycle=cycle;
        uint32_t r=hash(seed+index*0x9e3779b9u+cycle*0x8da6b343u);
        p.angle=uint16_t((r&65535)*4023u/65535u);r=hash(r);
        p.scale=uint16_t(index<20?1024:102+(r&65535)*205u/65535u);r=hash(r);
        p.x=(int(r&65535)-32768)*25;r=hash(r); // [-800,800] in Q10.
        p.y=(int(r&65535)-32768)*125/8;r=hash(r); // [-500,500].
        p.radial=uint16_t((r&65535)*1024u/65535u);r=hash(r);
        // 0..0.03 turns/second, in Q16 radians/second.
        p.orbit=uint16_t((r&65535)*12353u/65535u);
    }
    static void step(Particle&p,unsigned ms){
        const int ix=p.x/1024,iy=p.y/1024;
        const unsigned distance=root(unsigned(ix*ix+iy*iy));
        int ax=20*1024,ay=-10*1024;
        if(distance){ax+=ix*int(p.radial)/int(distance);ay+=iy*int(p.radial)/int(distance);}
        p.vx+=ax*int(ms)/1000;p.vy+=ay*int(ms)/1000;
        const int angle=int(p.orbit)*int(ms)/1000;
        // Source clockwise rotation followed by velocity integration. These
        // tiny <=0.0065rad steps permit a bounded Q16 sine/cosine expansion.
        const int cosine=65536-int(int64_t(angle)*angle/131072);
        const int x=p.x,y=p.y;
        p.x=int((int64_t(x)*cosine+int64_t(y)*angle)/65536)+p.vx*int(ms)/1000;
        p.y=int((int64_t(y)*cosine-int64_t(x)*angle)/65536)+p.vy*int(ms)/1000;
        p.age=uint16_t(p.age+ms);
    }
    void advance_particle(unsigned index,unsigned ms){
        auto&p=particles[index];
        while(ms){unsigned dt=ms>34?34:ms;const unsigned remaining=LifeMs-p.age;
            if(dt>remaining)dt=remaining;
            if(dt){step(p,dt);ms-=dt;}
            if(p.age>=LifeMs)birth(index,p.cycle+1);
        }
    }
    void reset_at(uint32_t now,uint32_t cosmetic_seed){
        seed=cosmetic_seed;active=true;elapsed=now;
        for(unsigned i=0;i<Count;++i){const uint64_t total=uint64_t(now)+LifeMs-(i%20)*750u;
            birth(i,uint32_t(total/LifeMs));
            unsigned age=unsigned(total%LifeMs),tick=0;
            while(age){const unsigned dt=age<(tick%3==2?34u:33u)?age:(tick%3==2?34u:33u);step(particles[i],dt);age-=dt;++tick;}}
    }
    void observe(uint32_t now,uint32_t cosmetic_seed){
        if(!active||seed!=cosmetic_seed||now<elapsed||now-elapsed>500){reset_at(now,cosmetic_seed);return;}
        const unsigned dt=now-elapsed;for(unsigned i=0;i<Count;++i)advance_particle(i,dt);elapsed=now;
    }
    static unsigned fade(unsigned age){return age>=2675?255:age*255/2675;}
    static int project_q16(int source_q10,bool vertical){
        // RandomBg's anchored parent plus AnimatedBg's responsive 1.2683
        // scale, projected from the source 1680x1260 camera to 320x240.
        return (vertical?-332515:-5963951)+int(int64_t(source_q10)*15832249/1024000);
    }
    static unsigned smooth(unsigned q){return unsigned(uint64_t(q)*q*(12288-2*q)/16777216);}
    static unsigned light_scale(unsigned phase){
        const unsigned split=2058;
        unsigned a=phase<split?1711:2081,b=phase<split?2081:1733;
        const unsigned t=phase<split?phase*4096/split:(phase-split)*4096/(4096-split);
        return unsigned(int(a)+(int(b)-int(a))*int(smooth(t))/4096);
    }
    static unsigned light_gray(unsigned phase){
        const unsigned peak=2488,last=4073;
        if(phase>=last)return 109;
        return phase<peak?109+146*phase/peak:255-146*(phase-peak)/(last-peak);
    }
};
static_assert(sizeof(RandomCharacterPresentation)<2048,"Random particles must stay below two KiB");
}
