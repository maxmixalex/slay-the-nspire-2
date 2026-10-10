#include "app.hpp"
#include "lz4_block.hpp"
namespace spire {
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t*p){return u16(p)|(uint32_t)u16(p+2)<<16;}
namespace {
// Independent large-scene/small-sprite caches avoid re-inflating a background
// for every icon. Pictures borrow these buffers until the next same-tier image.
static uint8_t large_pixels[320*240*3],small_pixels[32768];
static const uint8_t* cached_large=nullptr;
static const uint8_t* cached_small=nullptr;
const uint8_t* inflate(const uint8_t* source,unsigned length,unsigned& format){
    if(length<6)return nullptr;
    format=source[0];unsigned size=u32(source+1);
    if(format>4||!size||size>sizeof large_pixels)return nullptr;
    bool large=size>sizeof small_pixels;
    auto& cached=large?cached_large:cached_small;
    auto* pixels=large?large_pixels:small_pixels;
    if(cached!=source){
        cached=nullptr;if(!decode_lz4(source+5,length-5,pixels,size))return nullptr;
        cached=source;
    }
    return pixels;
}
bool valid_image(const uint8_t* p,unsigned len,unsigned w,unsigned h,unsigned fmt){
    if(fmt<4){unsigned expected=fmt<2?w*h*(fmt?3:2):512+w*h*(fmt==3?2:1);return len==expected;}
    unsigned at=0;
    for(unsigned y=0;y<h;y++){
        if(len-at<2)return false;unsigned spans=u16(p+at);at+=2;if(spans>w)return false;unsigned last=0;
        for(unsigned j=0;j<spans;j++){
            if(len-at<5)return false;unsigned x=u16(p+at),count=u16(p+at+2),mode=p[at+4];at+=5;
            if(!count||mode>1||x<last||x+count>w||count*(mode?3u:2u)>len-at)return false;
            last=x+count;at+=count*(mode?3u:2u);
        }
    }
    return at==len;
}
}
bool Content::open(const uint8_t*p,size_t n){
    data_=nullptr;cards_=0;ui_offset_=ui_count_=0;cached_large=cached_small=nullptr;
    if(!p||n<40||n>MaxPack||u32(p)!=0x3250534E||u32(p+8)!=0x41CEF1EA||u32(p+12)!=n)return false;
    const uint32_t version=u32(p+4),cards=u32(p+32);
    if((version!=3&&version!=4&&version!=5&&version!=6&&version!=7&&version!=8)||(version==3&&cards)||(version==4&&cards!=24)||(version==5&&cards!=26)||(version==6&&cards!=51)||(version==7&&cards!=58)||(version==8&&cards!=CardKinds))return false;
    auto ni=u32(p+16),ng=u32(p+20),ns=u32(p+24),nc=u32(p+28);
    if(ni<A_Count||ni>8192||ng>8192||ns<66||ns>2048||nc!=(version>=6?AnimationKinds:11))return false;
    uint32_t start=40+ni*16+ng*16+ns*8+nc*16+cards*16+(version>=5?(version>=8?U_Count:version==7?83:version==6?74:56)*4:0);if(start>n)return false;
    uint32_t hash=2166136261u;for(size_t i=40;i<n;i++){hash^=p[i];hash*=16777619u;}if(hash!=u32(p+36))return false;
    for(unsigned i=0;i<ni;i++){
        const uint8_t*r=p+40+16*i;uint32_t off=u32(r),w=u16(r+4),h=u16(r+6),fmt=u32(r+8),len=u32(r+12);
        if(!w||w>320||!h||h>240||fmt>5||(fmt==5&&version<5)||off<start||off%2||off>n||len>n-off)return false;
        const uint8_t* raw=p+off;
        if(fmt==5){raw=inflate(raw,len,fmt);if(!raw)return false;len=u32(p+off+1);}
        if(!valid_image(raw,len,w,h,fmt))return false;
    }
    uint32_t last=0;
    for(unsigned i=0;i<ng;i++){
        const uint8_t*r=p+40+ni*16+i*16;uint32_t cp=u32(r),off=u32(r+4),size=u16(r+8),w=r[10],h=r[11],key=(size<<24)|cp,encoded=u16(r+14);
        if(cp>0x10FFFF||!w||w>40||!h||h>40||size>32||u16(r+12)>40||off<start||off>n||(encoded?encoded:w*h)>n-off||(i&&key<=last)||(version<5&&encoded))return false;
        if(encoded){
            if(encoded%2||encoded>2*w*h)return false;
            unsigned total=0;for(unsigned j=0;j<encoded;j+=2){if(!p[off+j])return false;total+=p[off+j];}
            if(total!=w*h)return false;
        }last=key;
    }
    for(unsigned i=0;i<ns;i++){
        const uint8_t*r=p+40+ni*16+ng*16+i*8;uint32_t off=u32(r),len=u32(r+4);
        if(off<start||off>=n||len>n-off-1||p[off+len])return false;
    }
    for(unsigned i=0;i<nc;i++){
        const uint8_t*r=p+40+ni*16+ng*16+ns*8+i*16;uint32_t first=u32(r),count=u16(r+4),ms=u16(r+6);
        if(!count||count>256||first>=ni||count>ni-first||!ms||ms>1000||u32(r+12))return false;
        int px=(int16_t)u16(r+8),py=(int16_t)u16(r+10);if(px< -640||px>640||py< -480||py>480)return false;
    }
    for(unsigned i=0;i<cards;i++){
        const uint8_t*r=p+40+ni*16+ng*16+ns*8+nc*16+i*16;
        if(u32(r)>=ni||u32(r+4)>=ns||u32(r+8)>=ns||u32(r+12)>=ns)return false;
        // Card portraits are versioned independently of directory placement.
        const uint8_t* im=p+40+u32(r)*16;
        if(u16(im+4)!=(version>=5?82:41)||u16(im+6)!=(version>=5?60:30))return false;
    }
    if(version>=5){
        ui_count_=version>=8?U_Count:version==7?83:version==6?74:56;
        ui_offset_=40+ni*16+ng*16+ns*8+nc*16+cards*16;
        for(unsigned i=0;i<ui_count_;i++)if(u32(p+ui_offset_+i*4)>=ni){ui_offset_=0;return false;}
        if(ni<160||u32(p+ui_offset_+U_CampFlames*4)>ni-160)return false;
    }
    cards_=version>=4?cards:14;
    data_=p;length_=(uint32_t)n;images_=ni;glyphs_=ng;strings_=ns;clips_=nc;return true;
}
Picture Content::image(int id)const{
    if(!data_||id<0||(unsigned)id>=images_)return {nullptr,nullptr,nullptr,0,0,0};
    const uint8_t*r=data_+40+16*id;const uint8_t*p=data_+u32(r);uint16_t w=u16(r+4),h=u16(r+6);unsigned fmt=u32(r+8);
    if(fmt==5){p=inflate(p,u32(r+12),fmt);if(!p)return {nullptr,nullptr,nullptr,0,0,0};}
    if(fmt==4)return {p,nullptr,nullptr,w,h,4};
    if(fmt>=2)return {p+512,fmt==3?p+512+w*h:nullptr,p,w,h,(uint8_t)fmt};
    return {p,fmt?p+w*h*2:nullptr,nullptr,w,h,(uint8_t)fmt};
}
Glyph Content::glyph(uint32_t cp,int size)const{
    if(!data_)return {nullptr,0,0,0};uint32_t key=((uint32_t)size<<24)|cp;int lo=0,hi=(int)glyphs_;
    while(lo<hi){int mid=(lo+hi)/2;const uint8_t*r=data_+40+images_*16+mid*16;uint32_t k=(uint32_t)u16(r+8)<<24|u32(r);if(k<key)lo=mid+1;else hi=mid;}
    if((unsigned)lo<glyphs_){const uint8_t*r=data_+40+images_*16+lo*16;if(((uint32_t)u16(r+8)<<24|u32(r))==key)return {data_+u32(r+4),r[10],r[11],u16(r+12),u16(r+14)};}
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
int Content::card_image(int kind)const{
    if(!data_||kind<0||(unsigned)kind>=cards_)return -1;
    if(u32(data_+4)==3)return A_Card0+kind;
    const uint8_t*r=data_+40+images_*16+glyphs_*16+strings_*8+clips_*16+kind*16;
    return (int)u32(r);
}
const char* Content::card_string(int kind,int variant)const{
    if(!data_||kind<0||(unsigned)kind>=cards_||variant<0||variant>2)return "";
    if(u32(data_+4)==3)return string(kind*3+variant);
    const uint8_t*r=data_+40+images_*16+glyphs_*16+strings_*8+clips_*16+kind*16;
    return string((int)u32(r+4+variant*4));
}
int Content::ui_image(int key)const{
    if(!data_||!ui_offset_||key<0||(unsigned)key>=ui_count_)return -1;
    return (int)u32(data_+ui_offset_+key*4);
}
int Content::relic_image(int index)const{
    if(index<0||index>=RelicKinds)return -1;
    return index>=17?ui_image(U_HornCleat+index-17):index==16?ui_image(U_SnakeRing):index<10?relic_definitions[index].asset:ui_image(U_Strawberry+index-10);
}
int Content::frame_image(Card card)const{
    if(card.kind>=CardKinds)return -1;
    const int type=(int)definitions[card.kind].type;
    int base=card_color(card.kind)==CardColor::Silent?U_GreenAttack:card_colorless(card.kind)?U_ColorlessAttack:U_RedAttack;
    const int image=ui_image(base+type);
    return image>=0?image:ui_image(U_BigAttack+type);
}
int Content::energy_image(Card card)const{
    return card_color(card.kind)==CardColor::Silent?ui_image(U_SilentEnergy):card_colorless(card.kind)?ui_image(U_ColorlessEnergy):A_Energy;
}
int Content::potion_image(Potion p)const{
    unsigned i=(unsigned)p;if(!i||i>PotionKinds)return -1;
    return i<=3?A_FirePotion+(int)i-1:ui_image(U_EnergyPotion+(int)i-4);
}
} // namespace spire
