#pragma once
#include <stdint.h>
#include "resource_transfer.hpp"

namespace spire {
// NMainMenu's BlurBackstop animates the screen-texture shader to LOD 3 and
// black mix .7 in 250ms, and back to zero in 150ms. This is presentation-only.
struct MainMenuBackdrop {
    uint32_t started=0;
    uint16_t from=0;
    bool open=false;
    unsigned sample(uint32_t now)const {
        const unsigned duration=open?250:150,age=now-started;
        const unsigned t=age>=duration?duration:age;
        return unsigned(int(from)+(int(open?4096:0)-int(from))*int(t)/int(duration));
    }
    void observe(bool active,uint32_t now) {
        if(active!=open){from=uint16_t(sample(now));started=now;open=active;}
    }
};

namespace main_menu_backdrop {
constexpr unsigned One=4096;
constexpr unsigned MaximumLod=3*One;
constexpr unsigned MaximumBlackMix=(7*One+5)/10;
constexpr unsigned ScratchPixels=160*120+80*60+40*30;
struct MipScratch {uint16_t pixels[ScratchPixels];};
static_assert(sizeof(MipScratch)<=ResourceTransferCapacity&&alignof(ForegroundWorkspace)>=alignof(MipScratch),
    "The complete mip chain fits the existing aligned foreground workspace");

inline uint16_t channels(unsigned r,unsigned g,unsigned b) {
    return uint16_t((r<<11)|(g<<5)|b);
}
inline uint16_t average4(uint16_t a,uint16_t b,uint16_t c,uint16_t d) {
    return channels(((a>>11)+(b>>11)+(c>>11)+(d>>11)+2)/4,
        (((a>>5)&63)+((b>>5)&63)+((c>>5)&63)+((d>>5)&63)+2)/4,
        ((a&31)+(b&31)+(c&31)+(d&31)+2)/4);
}
inline uint16_t interpolate(uint16_t a,uint16_t b,unsigned q) {
    return channels(((a>>11)*(One-q)+(b>>11)*q+One/2)/One,
        (((a>>5)&63)*(One-q)+((b>>5)&63)*q+One/2)/One,
        ((a&31)*(One-q)+(b&31)*q+One/2)/One);
}
// Pixel-centre coordinates and clamp-to-edge match SCREEN_UV linear sampling.
// The native framebuffer is RGB565, so each generated mip keeps that precision.
inline uint16_t sample(const uint16_t*image,int width,int height,int x,int y) {
    int fx=((2*x+1)*width*int(One))/640-int(One/2);
    int fy=((2*y+1)*height*int(One))/480-int(One/2);
    int ix=fx/int(One),iy=fy/int(One);unsigned qx=0,qy=0;
    if(fx<=0)ix=0;else if(ix>=width-1)ix=width-1;else qx=unsigned(fx%int(One));
    if(fy<=0)iy=0;else if(iy>=height-1)iy=height-1;else qy=unsigned(fy%int(One));
    const int nx=ix+int(qx!=0),ny=iy+int(qy!=0);
    const uint16_t a=image[iy*width+ix],b=image[iy*width+nx];
    const uint16_t c=image[ny*width+ix],d=image[ny*width+nx];
    const unsigned wa=(One-qx)*(One-qy),wb=qx*(One-qy),wc=(One-qx)*qy,wd=qx*qy;
    constexpr unsigned denominator=One*One,round=denominator/2;
    return channels(((a>>11)*wa+(b>>11)*wb+(c>>11)*wc+(d>>11)*wd+round)/denominator,
        (((a>>5)&63)*wa+((b>>5)&63)*wb+((c>>5)&63)*wc+((d>>5)&63)*wd+round)/denominator,
        ((a&31)*wa+(b&31)*wb+(c&31)*wc+(d&31)*wd+round)/denominator);
}
inline void apply(uint16_t*pixels,int pitch,unsigned lod,unsigned black_mix) {
    if(!pixels||pitch<320)return;
    if(lod>MaximumLod)lod=MaximumLod;
    if(black_mix>One)black_mix=One;
    if(!lod&&!black_mix)return;
    const unsigned lower=lod/One,fraction=lod%One;
    const unsigned upper=lower+unsigned(fraction!=0);
    // All source reads are complete before this pixel-only operation. Borrow
    // the existing foreground transfer bytes for its mip chain; the calculator
    // stack cannot hold even the first 38,400-byte level. No resource callback
    // runs inside this lease and no mip pointer escapes to the next frame.
    ResourceTransferLease transfer(ResourceTransferUse::Bytes);if(!transfer)return;
    auto* scratch=(::new(detail::InPlaceAddress{transfer.bytes()}) MipScratch)->pixels;
    const uint16_t*levels[4]={pixels,scratch,scratch+160*120,scratch+160*120+80*60};
    const int widths[4]={320,160,80,40},heights[4]={240,120,60,30};
    for(unsigned level=1;level<=upper;++level){
        const auto*source=levels[level-1];auto*target=const_cast<uint16_t*>(levels[level]);
        const int source_pitch=level==1?pitch:widths[level-1],width=widths[level];
        for(int y=0;y<heights[level];++y)for(int x=0;x<width;++x){
            const int at=2*y*source_pitch+2*x;
            target[y*width+x]=average4(source[at],source[at+1],source[at+source_pitch],source[at+source_pitch+1]);
        }
    }
    const unsigned keep=One-black_mix;
    for(int y=0;y<240;++y)for(int x=0;x<320;++x){
        const int at=y*pitch+x;
        uint16_t color=lower?sample(levels[lower],widths[lower],heights[lower],x,y):pixels[at];
        if(fraction)color=interpolate(color,sample(levels[upper],widths[upper],heights[upper],x,y),fraction);
        pixels[at]=channels(((color>>11)*keep+One/2)/One,(((color>>5)&63)*keep+One/2)/One,((color&31)*keep+One/2)/One);
    }
}
inline void apply_progress(uint16_t*pixels,int pitch,unsigned progress) {
    if(progress>One)progress=One;
    apply(pixels,pitch,3*progress,(MaximumBlackMix*progress+One/2)/One);
}
}
}
