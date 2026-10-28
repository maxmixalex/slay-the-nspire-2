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
    static unsigned mix(unsigned a,unsigned b,unsigned f){return ((a<<8)+(b-a)*f+128)>>8;}
    static uint16_t mix_color(uint16_t a,uint16_t b,unsigned f){
        if(a==b)return a;
        // Adjacent low-bit bands need only the rounding decision. At the
        // exact midpoint each channel rounds toward its larger endpoint.
        if(!((a^b)&0xf7deu))return f<128?a:f>128?b:uint16_t(a|b);
        // Two separated 5-bit lanes fit one 32-bit multiply. Their carry
        // spaces preserve the exact independent RGB565 round-to-nearest.
        const uint32_t ar=((uint32_t(a)&0xf800u)<<5)|(a&31u),br=((uint32_t(b)&0xf800u)<<5)|(b&31u);
        const uint32_t rb=(((ar<<8)+(br-ar)*f+0x00800080u)>>8)&0x001f001fu;
        const uint32_t ag=a&0x07e0u,bg=b&0x07e0u;
        const uint32_t g=(((ag<<8)+(bg-ag)*f+0x1000u)>>8)&0x07e0u;
        return uint16_t(((rb>>5)&0xf800u)|g|(rb&31u));
    }
    uint16_t dense_color(unsigned at)const{
        const uint8_t*p=picture_.palette?picture_.palette+2*picture_.pixels[at]:picture_.pixels+2*at;
        return uint16_t(u16(p));
    }
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
    Picture storage()const{return picture_;}
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
        const int x=u_q16*int(width_)-32768,y=v_q16*int(height_)-32768;
        const int ix=x>>16,iy=y>>16;const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
        Texel a,b,c,d;
        if(picture_.format!=4){
            // Decode the storage kind and address each row once. The old
            // four at() calls repeated format checks and bounds per texel.
            const unsigned left=unsigned(bound(ix,width_,repeat)),right=unsigned(bound(ix+1,width_,repeat));
            const unsigned top=unsigned(bound(iy,height_,repeat))*width_,bottom=unsigned(bound(iy+1,height_,repeat))*width_;
            const unsigned aa=top+left,bb=top+right,cc=bottom+left,dd=bottom+right;
            a.color=dense_color(aa);b.color=dense_color(bb);c.color=dense_color(cc);d.color=dense_color(dd);
            if(picture_.alpha){a.alpha=picture_.alpha[aa];b.alpha=picture_.alpha[bb];c.alpha=picture_.alpha[cc];d.alpha=picture_.alpha[dd];}
            else a.alpha=b.alpha=c.alpha=d.alpha=255;
        }else{a=at(ix,iy,repeat);b=at(ix+1,iy,repeat);c=at(ix,iy+1,repeat);d=at(ix+1,iy+1,repeat);}
        return {mix_color(mix_color(a.color,b.color,fx),mix_color(c.color,d.color,fx),fy),mix(mix(a.alpha,b.alpha,fx),mix(c.alpha,d.alpha,fx),fy)};
    }
    uint16_t opaque_bilinear(int u_q16,int v_q16)const{
        if(picture_.format==4)return bilinear(u_q16,v_q16).color;
        const int x=u_q16*int(width_)-32768,y=v_q16*int(height_)-32768;
        const unsigned left=unsigned(bound(x>>16,width_,false)),right=unsigned(bound((x>>16)+1,width_,false));
        const unsigned top=unsigned(bound(y>>16,height_,false))*width_,bottom=unsigned(bound((y>>16)+1,height_,false))*width_;
        const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
        return mix_color(mix_color(dense_color(top+left),dense_color(top+right),fx),
                         mix_color(dense_color(bottom+left),dense_color(bottom+right),fx),fy);
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
