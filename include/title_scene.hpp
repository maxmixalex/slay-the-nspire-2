#pragma once
#include <stdint.h>
#include "reference_viewport.hpp"

// Original main-menu logo pose, cropped from the 1920x1080 scene to 320x240.
namespace spire::title_scene {
constexpr int AtlasWidth=164,LegacyAtlasHeight=106,AtlasHeight=210;
constexpr int TextX=40,TextY=24,TextWidth=164,TextHeight=84;
constexpr int FireX=106,FireY=42,FireWidth=16,FireHeight=22;
constexpr int FireAtlasY=84,FireFrames=10,FireFrameMs=100;
constexpr int GlowAtlasY=106,GlowHeight=104,GlowCenterX=115,GlowCenterY=55;
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
// MainMenuTextButtons is centered in the FourByThree UI viewport. Its
// 269x450 source rectangle contains centered200x50 controls, not stretched
// full-width text rows. Round each source boundary rather than accumulating
// a rounded row height across the stack.
// LCD adaptation: keep the source menu under the logo, with readable rows.
constexpr int MenuCenter=111,MenuMinimumHitWidth=96,MenuVisualPadding=12;
constexpr int MenuFont=10,MenuRowHeight=17;
constexpr int menu_y(int rows,int row){return 206-rows*MenuRowHeight+row*MenuRowHeight;}
constexpr int menu_top(int rows){return menu_y(rows,0);}
constexpr int menu_height(int,int){return MenuRowHeight;}
constexpr int menu_center_y(int rows,int row){return menu_y(rows,row)+MenuRowHeight/2;}
constexpr int menu_source_y(int rows,int row=0){return menu_y(rows,row)*21/4;}
static_assert(TextY+TextHeight+8<=menu_top(5),"The saved-run menu clears the complete logo");
struct Rect {int x,y,w,h;};
inline Rect menu_bounds(int rows,int widest_label){
    // Reserve the largest focused label and both moving source reticles,
    // including focus loss while the Continue panel is fading out.
    const int visual=(widest_label*4301+4095)/4096+2*MenuVisualPadding;
    const int width=visual>MenuMinimumHitWidth?visual:MenuMinimumHitWidth;
    const int left=MenuCenter-width/2>2?MenuCenter-width/2:2;
    const int right=MenuCenter+(width+1)/2<318?MenuCenter+(width+1)/2:318;
    return {left,menu_top(rows),right-left,rows*MenuRowHeight};
}
constexpr int ContinueWidth=120,ContinueMinimumWidth=108,ContinueGap=4;
constexpr int continue_y(int rows){return menu_y(rows,0)-24;}
constexpr int continue_height(bool ascension){return ascension?79:66;}
inline Rect continue_bounds(Rect menu,bool ascension){
    const int height=continue_height(ascension),space=318-menu.x-menu.w-ContinueGap;
    if(space>=ContinueMinimumWidth){
        const int x=menu.x+menu.w+ContinueGap;
        int y=menu.y-24;
        if(x<TextX+TextWidth+ContinueGap&&y<TextY+TextHeight+ContinueGap)
            y=TextY+TextHeight+ContinueGap;
        return {x,y,space<ContinueWidth?space:ContinueWidth,height};
    }
    // An exceptionally long localized menu cannot fit two readable columns.
    // The upper-right gutter clears both the complete logo and profile button.
    // Keep its bottom above every menu row, including the fading panel.
    constexpr int x=TextX+TextWidth+ContinueGap;
    return {x,menu.y-height-ContinueGap,318-x,height};
}
}
