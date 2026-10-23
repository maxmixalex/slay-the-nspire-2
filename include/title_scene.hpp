#pragma once
#include <stdint.h>

// Original main-menu logo pose, cropped from the 1920x1080 scene to 320x240.
namespace spire::title_scene {
constexpr int AtlasWidth=164,LegacyAtlasHeight=106,AtlasHeight=210;
constexpr int TextX=40,TextY=48,TextWidth=164,TextHeight=84;
constexpr int FireX=106,FireY=66,FireWidth=16,FireHeight=22;
constexpr int FireAtlasY=84,FireFrames=10,FireFrameMs=100;
constexpr int GlowAtlasY=106,GlowHeight=104,GlowCenterX=115,GlowCenterY=79;
constexpr unsigned GlowPeriodMs=3000,GlowOpacity=75;
struct GlowSample {int width,height;unsigned brightness;};
inline GlowSample glow_sample(uint32_t milliseconds){
    const uint32_t time=(milliseconds%GlowPeriodMs)*1000u;
    // CPUParticles2D samples the authored Curve directly. Its three points
    // have zero tangents, so each Bezier segment is an ordinary smoothstep.
    const unsigned peak_time=1507563u,peak_scale=507939u;
    const unsigned begin=time<peak_time?0u:peak_time,end=time<peak_time?peak_time:3000000u;
    const int from=time<peak_time?417635:507939,to=time<peak_time?507939:423164;
    const unsigned t=unsigned((uint64_t(time-begin)*4096u+(end-begin)/2)/(end-begin));
    const unsigned square=(t*t+2048u)/4096u;
    const unsigned smooth=(square*(12288u-2*t)+2048u)/4096u;
    const int scale=from+(to-from)*int(smooth)/4096;
    // Original Gradient: gray 89/255 -> white -> gray 89/255. CPUParticles
    // multiplies it by Color(64,205,215,75)/255 before additive blending.
    const unsigned bright_time=1234638u,dim_time=2983239u;
    const unsigned brightness=time<=bright_time?89u+(166u*time+bright_time/2)/bright_time:
        time<dim_time?255u-(166u*(time-bright_time)+(dim_time-bright_time)/2)/(dim_time-bright_time):89u;
    return {int((uint64_t(AtlasWidth)*unsigned(scale)+peak_scale/2)/peak_scale),
            int((uint64_t(GlowHeight)*unsigned(scale)+peak_scale/2)/peak_scale),brightness};
}
constexpr int MenuCenter=119,MenuLabelWidth=164,MenuHitWidth=174;
constexpr int menu_top(int rows){return rows==5?137:147;}
constexpr int MenuRowHeight=20;
}
