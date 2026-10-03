#include "app.hpp"
namespace spire {
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t*p){return u16(p)|(uint32_t)u16(p+2)<<16;}
bool Content::open(const uint8_t*p,size_t n){
    data_=nullptr;
    if(n<40||n>MaxPack||u32(p)!=0x3250534E||u32(p+4)!=2||u32(p+8)!=0x41CEF1EA||u32(p+12)!=n||u32(p+32))return false;
    auto ni=u32(p+16),ng=u32(p+20),ns=u32(p+24),nc=u32(p+28);
    if(ni<A_Count||ni>1024||ng>8192||ns<66||ns>2048||nc!=11)return false;
    uint32_t start=40+ni*16+ng*16+ns*8+nc*16;if(start>n)return false;
    uint32_t hash=2166136261u;for(size_t i=40;i<n;i++){hash^=p[i];hash*=16777619u;}if(hash!=u32(p+36))return false;
    for(unsigned i=0;i<ni;i++){
        const uint8_t*r=p+40+16*i;uint32_t off=u32(r),w=u16(r+4),h=u16(r+6),fmt=u32(r+8),len=u32(r+12);
        uint32_t expected=fmt<2?w*h*(fmt?3:2):512+w*h*(fmt==3?2:1);
        if(!w||w>320||!h||h>240||fmt>3||len!=expected||off<start||off%2||off>n||len>n-off)return false;
    }
    uint32_t last=0;
    for(unsigned i=0;i<ng;i++){
        const uint8_t*r=p+40+ni*16+i*16;uint32_t cp=u32(r),off=u32(r+4),size=u16(r+8),w=r[10],h=r[11],key=(size<<24)|cp;
        if(cp>0x10FFFF||!w||w>40||!h||h>40||size>32||u16(r+12)>40||off<start||off>n||w*h>n-off||(i&&key<=last))return false;last=key;
    }
    for(unsigned i=0;i<ns;i++){
        const uint8_t*r=p+40+ni*16+ng*16+i*8;uint32_t off=u32(r),len=u32(r+4);
        if(off<start||off>=n||len>n-off-1||p[off+len])return false;
    }
    for(unsigned i=0;i<nc;i++){
        const uint8_t*r=p+40+ni*16+ng*16+ns*8+i*16;uint32_t first=u32(r),count=u16(r+4),ms=u16(r+6);
        if(!count||count>120||first>=ni||count>ni-first||!ms||ms>1000||u32(r+12))return false;
        int px=(int16_t)u16(r+8),py=(int16_t)u16(r+10);if(px< -640||px>640||py< -480||py>480)return false;
    }
    data_=p;length_=(uint32_t)n;images_=ni;glyphs_=ng;strings_=ns;clips_=nc;return true;
}
Picture Content::image(int id)const{
    if(!data_||id<0||(unsigned)id>=images_)return {nullptr,nullptr,nullptr,0,0};
    const uint8_t*r=data_+40+16*id;const uint8_t*p=data_+u32(r);uint16_t w=u16(r+4),h=u16(r+6);unsigned fmt=u32(r+8);
    if(fmt>=2)return {p+512,fmt==3?p+512+w*h:nullptr,p,w,h};
    return {p,fmt?p+w*h*2:nullptr,nullptr,w,h};
}
Glyph Content::glyph(uint32_t cp,int size)const{
    if(!data_)return {nullptr,0,0,0};uint32_t key=((uint32_t)size<<24)|cp;int lo=0,hi=(int)glyphs_;
    while(lo<hi){int mid=(lo+hi)/2;const uint8_t*r=data_+40+images_*16+mid*16;uint32_t k=(uint32_t)u16(r+8)<<24|u32(r);if(k<key)lo=mid+1;else hi=mid;}
    if((unsigned)lo<glyphs_){const uint8_t*r=data_+40+images_*16+lo*16;if(((uint32_t)u16(r+8)<<24|u32(r))==key)return {data_+u32(r+4),r[10],r[11],u16(r+12)};}
    if(cp!='?')return glyph('?',size);return {nullptr,0,0,(uint16_t)(size/2)};
}
const char* Content::string(int id)const{
    if(!data_||id<0||(unsigned)id>=strings_)return "";
    return (const char*)(data_+u32(data_+40+images_*16+glyphs_*16+id*8));
}
Clip Content::clip(int id)const{
    if(!data_||id<0||(unsigned)id>=clips_)return {0,0,1,0,0};
    const uint8_t*r=data_+40+images_*16+glyphs_*16+strings_*8+id*16;
    return {u32(r),u16(r+4),u16(r+6),(int16_t)u16(r+8),(int16_t)u16(r+10)};
}
} // namespace spire
