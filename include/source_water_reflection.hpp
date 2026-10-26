#pragma once
#include "app.hpp"
#include <stdint.h>

namespace spire { namespace water_reflection {
// vfx_water_reflection.gdshader. The private converter carries the noise's
// original R8 channel in alpha; color textures keep their ordinary RGBA.
struct Texel {uint16_t color=0;unsigned alpha=0;};
class Texture {
    Picture picture_{};
    unsigned width_=0,height_=0;
    const uint8_t* rows_[256]{};
    static unsigned u16(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
    static int bound(int n,int extent,bool repeat){
        if(repeat){if(n>=0&&n<extent)return n;if(n<0&&n>=-extent)return n+extent;if(n>=extent&&n<extent*2)return n-extent;n%=extent;return n<0?n+extent:n;}
        return n<0?0:n>=extent?extent-1:n;
    }
    static unsigned mix(unsigned a,unsigned b,unsigned f){return (a*(256-f)+b*f+128)>>8;}
public:
    explicit Texture(Picture p,unsigned logical_width=0,unsigned logical_height=0):picture_(p){
        width_=logical_width?logical_width:p.w;height_=logical_height?logical_height:p.h;
        if(!p.pixels||!p.w||!p.h||p.h>256||!width_||!height_||width_>320||height_>256||
           bool(logical_width)!=bool(logical_height)||width_*height_>unsigned(p.w)*p.h){picture_={};width_=height_=0;return;}
        if(p.format==4){const uint8_t*row=p.pixels;
            for(unsigned y=0;y<p.h;++y){rows_[y]=row;unsigned count=u16(row);row+=2;
                for(unsigned i=0;i<count;++i){unsigned n=u16(row+2),stride=row[4]?3:2;row+=5+n*stride;}}}
    }
    bool valid()const{return picture_.pixels!=nullptr;}
    unsigned width()const{return width_;}
    unsigned height()const{return height_;}
    Texel at(int x,int y,bool repeat=false)const{
        if(!valid())return {};x=bound(x,width_,repeat);y=bound(y,height_,repeat);
        const unsigned index=unsigned(y)*width_+unsigned(x);
        if(picture_.format==4){
            // A linear texture view preserves every source texel without
            // widening Content's 320x240 image bounds. Dense and indexed
            // images below use its flat index directly; only sparse spans
            // need to recover a physical row and column.
            if(width_!=picture_.w){y=int(index/picture_.w);x=int(index-unsigned(y)*picture_.w);}
            const uint8_t*p=rows_[y];unsigned count=u16(p);p+=2;
            for(unsigned i=0;i<count;++i){unsigned left=u16(p),n=u16(p+2),alpha=p[4];p+=5;
                if(unsigned(x)<left)return {};if(unsigned(x)<left+n){p+=(unsigned(x)-left)*(alpha?3:2);return {uint16_t(u16(p)),alpha?p[2]:255u};}
                p+=n*(alpha?3:2);}return {};}
        const uint8_t*p=picture_.pixels;
        const uint8_t*color=picture_.palette?picture_.palette+2*p[index]:p+2*index;
        return {uint16_t(u16(color)),picture_.alpha?picture_.alpha[index]:255u};
    }
    Texel bilinear(int u_q16,int v_q16,bool repeat=false)const{
        if(!valid())return {};
        if(repeat){u_q16&=65535;v_q16&=65535;}
        // Texture coordinates address pixel centres, as in the source GPU
        // sampler. Eight fractional bits retain subpixel motion on RGB565.
        int x=u_q16*int(width_)-32768,y=v_q16*int(height_)-32768;
        int ix=x>>16,iy=y>>16;unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
        const auto a=at(ix,iy,repeat),b=at(ix+1,iy,repeat),c=at(ix,iy+1,repeat),d=at(ix+1,iy+1,repeat);
        const auto channel=[&](unsigned shift,unsigned mask){return mix(mix((a.color>>shift)&mask,(b.color>>shift)&mask,fx),mix((c.color>>shift)&mask,(d.color>>shift)&mask,fx),fy);};
        return {uint16_t((channel(11,31)<<11)|(channel(5,63)<<5)|channel(0,31)),mix(mix(a.alpha,b.alpha,fx),mix(c.alpha,d.alpha,fx),fy)};
    }
    unsigned alpha_bilinear(int u_q16,int v_q16,bool repeat=false)const{
        if(!valid())return 0;
        if(picture_.format==4)return bilinear(u_q16,v_q16,repeat).alpha;
        if(!picture_.alpha)return 255;
        if(repeat){u_q16&=65535;v_q16&=65535;}
        const int x=u_q16*int(width_)-32768,y=v_q16*int(height_)-32768;
        const int left=bound(x>>16,width_,repeat),right=bound((x>>16)+1,width_,repeat);
        const int top=bound(y>>16,height_,repeat),bottom=bound((y>>16)+1,height_,repeat);
        const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
        const uint8_t*a=picture_.alpha+top*width_,*b=picture_.alpha+bottom*width_;
        return mix(mix(a[left],a[right],fx),mix(b[left],b[right],fx),fy);
    }
};
struct Parameters {
    int pan_q16=0,half_strength_q16=0;
    Parameters(uint32_t clock_ms,unsigned speed_milli,unsigned strength_milli){
        // Godot shader TIME wraps every 3600 seconds. Reduce the repeated
        // noise phase before the pixel loop and leave all divisions here.
        pan_q16=int((uint64_t(clock_ms%3600000u)*speed_milli*65536/1000000)&65535);
        half_strength_q16=int(strength_milli*65536/2000);
    }
};
inline Texel sample(const Texture&texture,const Texture&noise,int u_q16,int v_q16,
                    const Parameters&p,const Texture*mask=nullptr){
    if(mask&&mask->alpha_bilinear(u_q16,v_q16)<=25)return texture.bilinear(u_q16,v_q16);
    const unsigned n=noise.alpha_bilinear(u_q16-p.pan_q16,v_q16-p.pan_q16,true);
    const int shift=(int(n)-255)*p.half_strength_q16/255;
    return texture.bilinear(u_q16+shift,v_q16+shift);
}
}}
