#pragma once
#include <stdint.h>

namespace spire::title_scene {
constexpr unsigned StarCount=200,StarPeriodMs=5000,FogCount=40,FogPeriodMs=400000;
constexpr int ParticleAtlasWidth=270,ParticleAtlasHeight=216,FogFrameWidth=135,FogFrameHeight=54;
constexpr int DotAtlasX=135,DotAtlasY=162,DotSize=3;
struct ParticleSample {
    int x,y,width,height;
    unsigned red,green,blue,opacity,frame;
};
// Cosmetic samples are derived only from emitter/index/cycle. They never
// observe or advance the gameplay RNG and need no per-particle allocation.
inline uint32_t particle_random(uint32_t&state){
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
}
inline unsigned particle_unit(uint32_t&state){return particle_random(state)>>16;}
inline unsigned particle_age(uint32_t time,unsigned index,unsigned count,unsigned period,unsigned&cycle){
    unsigned phase=time%period+index*(period/count);cycle=time/period;
    if(phase>=period){phase-=period;++cycle;}return phase;
}
inline int particle_round(int32_t fixed){return (fixed+(fixed<0?-32768:32768))/65536;}
// Authored world positions stay within -1005..7005 pixels in Q16. Even
// after the cover offset and multiplication by two they fit signed 32-bit.
// Keep those divisions in the calculator's existing 32-bit integer ABI.
inline int particle_screen_x(int64_t world){return particle_round((int32_t(world)-240*65536)*2/9);}
inline int particle_screen_y(int64_t world){return particle_round((int32_t(world)+24*65536)*2/9);}
inline unsigned particle_lerp(unsigned a,unsigned b,unsigned phase,unsigned span){
    return unsigned(int(a)+(int(b)-int(a))*int(phase)/int(span));
}
inline ParticleSample star_sample(unsigned index,uint32_t time){
    unsigned cycle;const unsigned age=particle_age(time,index,StarCount,StarPeriodMs,cycle);
    uint32_t random=0x47198ab3u^((index+cycle*StarCount+1u)*0x9e3779b9u);
    const int world_x=926-1500+int(particle_unit(random)*3000u/65535u);
    const int world_y=600-300+int(particle_unit(random)*600u/65535u);
    // 256px dot, authored scale 0.01..0.04, then 2/9 viewport scale.
    const unsigned scale=655u+particle_unit(random)*1966u/65535u;
    const int size=int((256u*scale*2u+9u*32768u)/(9u*65536u));
    const unsigned peak=1294,phase=age<peak?age:age-peak,span=age<peak?peak:StarPeriodMs-peak;
    const unsigned red=age<peak?particle_lerp(216,255,phase,span):particle_lerp(255,199,phase,span);
    const unsigned green=age<peak?particle_lerp(205,255,phase,span):particle_lerp(255,166,phase,span);
    const unsigned blue=age<peak?particle_lerp(77,255,phase,span):particle_lerp(255,103,phase,span);
    const unsigned alpha=age<peak?phase*255u/span:(span-phase)*255u/span;
    const int width=size?size:1;
    return {particle_screen_x(int64_t(world_x)*65536)-width/2,
            particle_screen_y(int64_t(world_y)*65536)-width/2,width,width,
            (71u*red+127u)/255u,(214u*green+127u)/255u,(224u*blue+127u)/255u,alpha,0};
}
// Integral twice of gravity minus the authored linear damping curve.
// Curve(t)=1+166.555*t^2-67.555*t^3, t=age/400s. Velocity stays positive
// for the entire 0..0.001 damping range, so no zero-velocity clamp is needed.
// Continuous integration avoids replaying the original 400-second preprocess.
inline int64_t fog_displacement(unsigned age,unsigned damping){
    const uint64_t u=uint64_t(age)*65536u/FogPeriodMs;
    const uint64_t u2=u*u/65536u,u4=u2*u2/65536u,u5=u4*u/65536u;
    const uint64_t drag=80u*u2+33311u*u4/15u-13511u*u5/25u;
    return int64_t(8000u*u2-drag*damping/65535u);
}
inline unsigned particle_sqrt(unsigned value){
    unsigned root=0,bit=1u<<30;
    while(bit>value)bit>>=2;
    while(bit){if(value>=root+bit){value-=root+bit;root=(root>>1)+bit;}else root>>=1;bit>>=2;}
    return root;
}
inline int particle_sine(unsigned phase){
    static constexpr int16_t quarter[]={0,3212,6393,9512,12539,15446,18204,20787,23170,
        25329,27245,28898,30273,31356,32138,32609,32767};
    const unsigned p=phase&63u,index=(p&16u)?16u-(p&15u):p&15u;
    return p&32u?-quarter[index]:quarter[index];
}
inline int32_t particle_sphere_axis(unsigned radius,int sine){
    const unsigned magnitude=unsigned(sine<0?-sine:sine);
    const int32_t result=int32_t(uint64_t(radius)*magnitude/32767u);
    return sine<0?-result:result;
}
inline ParticleSample fog_sample(unsigned layer,unsigned index,uint32_t time){
    unsigned cycle;const unsigned age=particle_age(time,index,FogCount,FogPeriodMs,cycle);
    uint32_t random=0x19c364bdu^((1u+index+layer*FogCount+cycle*2u*FogCount)*0x9e3779b9u);
    const unsigned damping=particle_unit(random),scale_random=particle_unit(random),frame=particle_unit(random)*7u/65536u;
    // Project the source sphere emission into 2D. Sixty-four directions are
    // enough for its 1.11 screen-pixel radius; no floating-point trigonometry.
    const int s=int(particle_unit(random))*2-65535;
    const unsigned angle=particle_unit(random)>>10,p=particle_unit(random);
    const unsigned radius=particle_sqrt(unsigned(uint64_t(65535)*65535-int64_t(s)*s));
    const unsigned radius_fixed=unsigned(uint64_t(radius)*p*5u/65535u);
    const int32_t jitter_x=particle_sphere_axis(radius_fixed,particle_sine(angle+16u));
    const int32_t jitter_y=particle_sphere_axis(radius_fixed,particle_sine(angle));
    const unsigned scale=layer?117965u+scale_random*13107u/65535u:131072u+scale_random*26214u/65535u;
    const int width=int((252u*scale*2u+9u*32768u)/(9u*65536u));
    const int height=int((100u*scale*2u+9u*32768u)/(9u*65536u));
    return {particle_screen_x(-int64_t(1000)*65536+jitter_x+fog_displacement(age,damping))-width/2,
            particle_screen_y(int64_t(layer?850:800)*65536+jitter_y)-height/2,
            width,height,74,90,99,layer?255u:112u,frame};
}
}
