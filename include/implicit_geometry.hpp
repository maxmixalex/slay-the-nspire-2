#pragma once
#include <stdint.h>

namespace spire::implicit_geometry {
constexpr unsigned ContentVersion=46,Width=320,Height=240,Pixels=Width*Height;
constexpr uint16_t Marker=0xfffdu;
constexpr uint16_t BitmapMarker=0xfffcu;
constexpr unsigned DenseBytes=Pixels*3u;
constexpr unsigned BitmapHeader=40,BitmapBytes=BitmapHeader+Pixels/8u;

// The converter accepts only images whose old minimum-cost sparse geometry
// is reproduced exactly. The marker replaces that geometry in-place; its
// active-cell bitmap preserves the old pixel-group read set and ordering.
inline bool mask(const uint8_t*geometry,unsigned bytes,unsigned cells,uint8_t*out,
                 unsigned marker=Marker,unsigned payload=0){
    if(!geometry||!out||!cells||cells>300u||bytes<2u+(cells+7u)/8u||
       (unsigned(geometry[0])|(unsigned(geometry[1])<<8))!=marker)return false;
    if((cells&7u)&&(geometry[2u+cells/8u]>>(cells&7u)))return false;
    const unsigned prefix=2u+(cells+7u)/8u;
    if(payload>bytes-prefix)return false;
    for(unsigned i=prefix+payload;i<bytes;++i)if(geometry[i])return false;
    for(unsigned i=0;i<cells;++i)out[i]=uint8_t((geometry[2u+i/8u]>>(i&7u))&1u);
    return true;
}

inline void word(uint8_t*p,unsigned value){p[0]=uint8_t(value);p[1]=uint8_t(value>>8);}

// Selection patches must preserve which pixels are copied, including opaque
// black. Their separate bit plane describes exact mode-0 spans, not alpha.
inline bool bitmap_row(const uint8_t*coverage,unsigned y,uint8_t*out,unsigned capacity,unsigned&written){
    written=0;if(!coverage||!out||y>=Height||capacity<2)return false;
    unsigned at=2,spans=0,x=0;const auto*bits=coverage+y*(Width/8u);
    while(x<Width){
        if(!((bits[x/8u]>>(x&7u))&1u)){++x;continue;}
        const unsigned first=x++;
        while(x<Width&&((bits[x/8u]>>(x&7u))&1u))++x;
        if(capacity-at<5)return false;
        word(out+at,first);word(out+at+2,x-first);out[at+4]=0;at+=5;++spans;
    }
    word(out,spans);written=at;return true;
}
inline bool bitmap_mask(const uint8_t*geometry,unsigned bytes,uint8_t*active){
    if(bytes<BitmapBytes||!mask(geometry,bytes,300,active,BitmapMarker,Pixels/8u))return false;
    const auto*coverage=geometry+BitmapHeader;
    for(unsigned i=0;i<300;++i){unsigned used=0;
        const unsigned tx=(i%20u)*2u,ty=(i/20u)*16u;
        for(unsigned y=ty;y<ty+16u;++y)used|=coverage[y*40u+tx]|coverage[y*40u+tx+1u];
        if(bool(used)!=bool(active[i]))return false;
    }
    return true;
}
inline bool bitmap_geometry(const uint8_t*geometry,unsigned bytes,uint8_t*out,unsigned capacity){
    uint8_t active[300];if(!bitmap_mask(geometry,bytes,active))return false;
    const auto*coverage=geometry+BitmapHeader;unsigned at=0;
    for(unsigned y=0;y<Height;++y){unsigned written=0;
        if(!bitmap_row(coverage,y,out+at,capacity-at,written))return false;at+=written;
    }
    return at==bytes;
}

// Exact closed form of sparse_image.encode's two-state dynamic program.
// A new span costs five bytes and opaque pixels save one byte each. An
// interior opaque run therefore needs >10 pixels; a leading run needs >5,
// while a trailing run needs >=5 because the source's final tie chooses mode0.
// No per-pixel DP table, allocation, quantization or alpha approximation.
inline bool row(const uint16_t*colors,const uint8_t*alpha,unsigned width,
                uint8_t*out,unsigned capacity,unsigned&written,unsigned&geometry){
    written=geometry=0;
    if(!colors||!alpha||!out||!width||width>Width||capacity<2)return false;
    unsigned position=2,spans=0;
    const auto emit=[&](unsigned first,unsigned end,bool transparent){
        const unsigned count=end-first,stride=transparent?3u:2u;
        if(!count||position>capacity||5u>capacity-position||count>(capacity-position-5u)/stride)return false;
        word(out+position,first);word(out+position+2,count);out[position+4]=uint8_t(transparent);position+=5;
        for(unsigned x=first;x<end;++x){
            word(out+position,colors[x]);position+=2;
            if(transparent)out[position++]=alpha[x];
        }
        ++spans;return true;
    };
    unsigned x=0;
    while(x<width){
        if(!alpha[x]){++x;continue;}
        const unsigned first=x;
        while(x<width&&alpha[x])++x;
        const unsigned end=x;unsigned pending=first,at=first;
        while(at<end){
            if(alpha[at]!=255u){++at;continue;}
            const unsigned start=at;
            while(at<end&&alpha[at]==255u)++at;
            const unsigned length=at-start;
            const bool opaque=start==first?(at==end||length>5u):at==end?length>=5u:length>10u;
            if(!opaque)continue;
            if(pending<start&&!emit(pending,start,true))return false;
            if(!emit(start,at,false))return false;
            pending=at;
        }
        if(pending<end&&!emit(pending,end,true))return false;
    }
    word(out,spans);written=position;geometry=2u+spans*5u;return true;
}
} // namespace spire::implicit_geometry
