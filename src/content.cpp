#include "app.hpp"
#include "lz4_block.hpp"
#include "lzma2_block.hpp"
namespace spire {
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t*p){return u16(p)|(uint32_t)u16(p+2)<<16;}
namespace {
// A view lives until its entry is evicted or its owner is reopened/failed.
// Entries never move: unrelated allocations and group-cache reuse preserve
// every surviving view. All sizes and owners share the bounded image pool;
// callers must finish using a view before unbounded further image requests.
constexpr unsigned MaxImageBytes=320u*240u*8u+2u*240u;
constexpr unsigned MaxEncodedImage=MaxImageBytes+MaxImageBytes/255u+32u;
constexpr unsigned MaxGlyphBytes=40u*40u*2u,MaxStringBytes=16384;
constexpr unsigned ImageBufferBudget=64u*4096u+16u*32768u+8u*131072u+2u*MaxImageBytes;
constexpr unsigned ImagePageBytes=4096,ImagePages=ImageBufferBudget/ImagePageBytes,ImageEntries=90;
alignas(4) static uint8_t image_pixels[ImagePages*ImagePageBytes];
static uint8_t encoded_pixels[MaxEncodedImage];
#include "content_tiles.inc"
// Keep a directory index in the existing key word. The immutable, resident
// descriptor supplies offset, geometry, stored format and encoded length
// without copying the descriptor or adding range reads during lookup.
struct ImageCache {
    const Content* owner{};uint32_t image{},age{};
    uint16_t first{};uint8_t pages{},format{};
};
static ImageCache image_cache[ImageEntries];
static uint32_t image_used[(ImagePages+31)/32]{},image_age=0;
static_assert(ImagePages==748&&(MaxImageBytes+ImagePageBytes-1)/ImagePageBytes<=255,
              "Image page indexes and lengths must fit the bounded entry");
static_assert(sizeof image_pixels+sizeof image_cache+sizeof image_used+sizeof image_age<=
              ImageBufferBudget+ImageEntries*(sizeof(const Content*)+2*sizeof(unsigned))+4*sizeof(unsigned),
              "Image pixels and bookkeeping must not exceed the previous four-tier cache");
static uint8_t* image_buffer(const ImageCache& cache){return image_pixels+unsigned(cache.first)*ImagePageBytes;}
static void discard_image(ImageCache& cache){
    for(unsigned i=cache.first;i<unsigned(cache.first)+cache.pages;++i)image_used[i/32]&=~(uint32_t(1)<<(i%32));
    cache={};
}
static uint32_t next_image_age(){
    if(image_age==0xffffffffu){
        // Preserve LRU order across the bounded counter's wrap. Empty and
        // unpublished reservations have no age and cannot become victims.
        uint8_t order[ImageEntries];unsigned count=0;
        for(unsigned i=0;i<ImageEntries;++i)if(image_cache[i].owner){
            unsigned at=count;
            while(at&&image_cache[order[at-1]].age>image_cache[i].age){order[at]=order[at-1];--at;}
            order[at]=uint8_t(i);++count;
        }
        for(unsigned i=0;i<count;++i)image_cache[order[i]].age=i+1;
        image_age=count;
    }
    return ++image_age;
}
static ImageCache* reserve_image(unsigned size){
    if(!size||size>MaxImageBytes)return nullptr;
    const unsigned pages=(size+ImagePageBytes-1)/ImagePageBytes;
    // At most all 90 published entries can be evicted. No compaction or
    // allocation moves another entry while a caller still holds its view.
    for(unsigned attempt=0;attempt<=ImageEntries;++attempt){
        unsigned run=0,first=ImagePages;
        for(unsigned i=0;i<ImagePages;++i){
            run=(image_used[i/32]&(uint32_t(1)<<(i%32)))?0:run+1;
            if(run==pages){first=i+1-pages;break;}
        }
        if(first<ImagePages)for(auto& cache:image_cache)if(!cache.pages){
            cache.first=uint16_t(first);cache.pages=uint8_t(pages);
            for(unsigned i=first;i<first+pages;++i)image_used[i/32]|=uint32_t(1)<<(i%32);
            return &cache;
        }
        ImageCache* victim=nullptr;
        for(auto& cache:image_cache)if(cache.owner&&(!victim||cache.age<victim->age))victim=&cache;
        if(!victim)return nullptr;
        discard_image(*victim);
    }
    return nullptr;
}
struct ImageReservation {
    ImageCache* cache;
    explicit ImageReservation(unsigned size):cache(reserve_image(size)){}
    ImageReservation(const ImageReservation&)=delete;
    ImageReservation& operator=(const ImageReservation&)=delete;
    ~ImageReservation(){if(cache&&!cache->owner)discard_image(*cache);}
    uint8_t* bytes()const{return cache?image_buffer(*cache):nullptr;}
    void publish(const Content* owner,uint32_t image,unsigned format){
        cache->image=image;cache->format=uint8_t(format);cache->age=next_image_age();cache->owner=owner;
    }
};
struct GlyphCache {const Content* owner{};uint32_t glyph{};uint8_t bytes[MaxGlyphBytes]{};};
static GlyphCache glyph_cache[64];static unsigned glyph_cursor=0;
struct StringCache {const Content* owner{};uint32_t offset{};char bytes[MaxStringBytes]{};};
static StringCache string_cache[8];static unsigned string_cursor=0;
static void clear_caches(const Content* owner){
    clear_tile_cache(owner);
    for(auto&v:image_cache)if(v.owner==owner)discard_image(v);
    for(auto&v:glyph_cache)if(v.owner==owner)v.owner=nullptr;
    for(auto&v:string_cache)if(v.owner==owner)v.owner=nullptr;
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
const uint8_t* Content::payload(uint32_t offset,uint32_t count,uint8_t* scratch,uint32_t capacity)const{
    if(!io_good_||!reader_.contains(offset,count)){io_good_=false;return nullptr;}
    if(data_)return data_+offset;
    if(!scratch||count>capacity||!reader_.read(offset,scratch,count)){io_good_=false;return nullptr;}
    return scratch;
}
bool Content::open(const uint8_t*p,size_t n){return open(ResourceSource::memory(p,n));}
bool Content::open(ResourceSource source){
    data_=metadata_=tile_metadata_=nullptr;directory_bytes_=0;tile_validated_=nullptr;
    version_=cards_=0;ui_offset_=ui_count_=0;io_good_=true;clear_caches(this);
    if(!reader_.reset(source)||source.length<40||source.length>MaxContentFile)return false;
    uint8_t h[40];if(!reader_.read(0,h,sizeof h))return false;const uint32_t n=source.length;
    if(u32(h)!=0x3250534E||u32(h+8)!=0x41CEF1EA||u32(h+12)!=n)return false;
    const uint32_t version=u32(h+4),cards=u32(h+32);
    if(version<3||version>NativeUiContentVersion||(version==3&&cards)||(version==4&&cards!=24)||(version==5&&cards!=26)||(version==6&&cards!=51)||(version==7&&cards!=58)||(version>=8&&version<=10&&cards!=68)||(version==11&&cards!=114)||(version==12&&cards!=228)||(version==13&&cards!=497)||(version>=14&&cards!=CardKinds))return false;
    auto ni=u32(h+16),ng=u32(h+20),ns=u32(h+24),nc=u32(h+28);
    if(ni<A_Count||ni>(version>=14?65535u:8192u)||ng>(version>=14?32768u:version==13?16384u:8192u)||ns<66||ns>(version>=14?16384u:2048u)||nc!=(version>=14?AnimationKinds:version==13?38:version>=6?17:11))return false;
    if((version==11&&ns!=407)||(version==12&&ns!=749)||(version==13&&ns!=1556)||(version>=14&&ns!=ContentStrings))return false;
    const unsigned auxiliary=version==NativeUiContentVersion?unsigned(NativeUiAuxiliaryCount):version>=14?unsigned(U_Count):version==13?339:version==12?233:version==11?185:version==10?105:version==9?95:version==8?90:version==7?83:version==6?74:version==5?56:0;
    const uint32_t base=40+ni*16+ng*16+ns*8+nc*16+cards*16+auxiliary*4;
    if(base>n||base>DirectoryCapacity)return false;
    uint32_t hash=0;if(!reader_.fnv1a(40,n-40,hash)||hash!=u32(h+36))return false;
    uint32_t start=base;
    if(version>=TileContentVersion){
        uint8_t header[64];
        if(!reader_.read(base,header,sizeof header)||
           !tile_header_bounds(header,base,n,ni,DirectoryCapacity,start))return false;
    }
    if(source.direct){data_=source.direct;metadata_=source.direct;}
    else {if(!reader_.read(0,directory_,start))return false;metadata_=directory_;}
    const uint8_t*p=metadata_;length_=n;images_=ni;glyphs_=ng;strings_=ns;clips_=nc;version_=version;
    directory_bytes_=start;tile_metadata_=version>=TileContentVersion?p+base:nullptr;
    auto reject=[&](){data_=metadata_=tile_metadata_=nullptr;directory_bytes_=0;tile_validated_=nullptr;
        version_=cards_=ui_offset_=ui_count_=0;clear_caches(this);return false;};
    if(tile_metadata_&&!valid_tile_directory(tile_metadata_,n))return reject();
    const uint32_t payload_start=tile_metadata_?u32(tile_metadata_+32)+16*u32(tile_metadata_+28):start;
    for(unsigned i=0;i<ni;i++){
        const uint8_t*r=p+40+16*i;uint32_t off=u32(r),w=u16(r+4),height=u16(r+6),fmt=u32(r+8),len=u32(r+12);
        if(!w||w>320||!height||height>240||fmt>6||(fmt==5&&version<5)||
           (fmt==6&&(!tile_metadata_||len!=16))||off<(fmt==6?start:payload_start)||
           off%2||off>n||len>n-off||len>MaxEncodedImage)return reject();
        if(fmt==6){unsigned slot=0;if(!tile_descriptor_slot(tile_metadata_,off,slot))return reject();}
        else if(!image((int)i).pixels)return reject();
    }
    if(tile_metadata_){
        const unsigned bytes=(images_+7)/8;unsigned available=DirectoryCapacity-directory_bytes_;
        if(bytes<=available){
            tile_validated_=directory_+DirectoryCapacity-bytes;available-=bytes;
            for(unsigned i=0;i<bytes;++i)tile_validated_[i]=0;
        }
        if(!validate_tile_catalog(this,reader_,tile_metadata_,base,p+40,images_,
                directory_+directory_bytes_,available))return reject();
    }
    uint32_t last=0;
    for(unsigned i=0;i<ng;i++){
        const uint8_t*r=p+40+ni*16+i*16;uint32_t cp=u32(r),off=u32(r+4),size=u16(r+8),w=r[10],height=r[11],key=(size<<24)|cp,encoded=u16(r+14);
        if(cp>0x10FFFF||!w||w>40||!height||height>40||size>32||u16(r+12)>40||off<payload_start||off>n||(encoded?encoded:w*height)>n-off||(i&&key<=last)||(version<5&&encoded))return reject();
        if(encoded){
            if(encoded%2||encoded>2*w*height)return reject();
            const uint8_t*bytes=payload(off,encoded,encoded_pixels,sizeof encoded_pixels);if(!bytes)return reject();
            unsigned total=0;for(unsigned j=0;j<encoded;j+=2){if(!bytes[j])return reject();total+=bytes[j];}
            if(total!=w*height)return reject();
        }last=key;
    }
    for(unsigned i=0;i<ns;i++){
        const uint8_t*r=p+40+ni*16+ng*16+i*8;uint32_t off=u32(r),len=u32(r+4);uint8_t zero=1;
        if(off<payload_start||off>=n||len>=MaxStringBytes||len>n-off-1||!reader_.u8(off+len,zero)||zero)return reject();
    }
    for(unsigned i=0;i<nc;i++){
        const uint8_t*r=p+40+ni*16+ng*16+ns*8+i*16;uint32_t first=u32(r),count=u16(r+4),ms=u16(r+6);
        if(!count||count>256||first>=ni||count>ni-first||!ms||ms>1000||u32(r+12))return reject();
        int px=(int16_t)u16(r+8),py=(int16_t)u16(r+10);if(px< -640||px>640||py< -480||py>480)return reject();
    }
    for(unsigned i=0;i<cards;i++){
        const uint8_t*r=p+40+ni*16+ng*16+ns*8+nc*16+i*16;
        if(u32(r)>=ni||u32(r+4)>=ns||u32(r+8)>=ns||u32(r+12)>=ns)return reject();
        const uint8_t* im=p+40+u32(r)*16;
        if(u16(im+4)!=(version>=5?82:41)||u16(im+6)!=(version>=5?60:30))return reject();
    }
    if(version>=5){
        ui_count_=auxiliary;ui_offset_=40+ni*16+ng*16+ns*8+nc*16+cards*16;
        for(unsigned i=0;i<ui_count_;i++)if(u32(p+ui_offset_+i*4)>=ni)return reject();
        if(ni<160||u32(p+ui_offset_+U_CampFlames*4)>ni-160)return reject();
    }
    cards_=version>=4?cards:14;return true;
}
Picture Content::image(int id)const{
    const Picture empty{nullptr,nullptr,nullptr,0,0,0};
    if(!metadata_||!io_good_||id<0||(unsigned)id>=images_)return empty;
    const auto fail=[&](){io_good_=false;clear_caches(this);return empty;};
    const uint8_t*r=metadata_+40+16*id;const uint32_t off=u32(r),length=u32(r+12);
    const uint32_t shape=u32(r+4),stored_format=u32(r+8);
    uint16_t w=u16(r+4),h=u16(r+6);unsigned fmt=stored_format,size=length;
    const uint8_t*p=nullptr;
    for(unsigned i=0;i<ImageEntries;i++)if(image_cache[i].owner==this){
        const uint8_t*key=metadata_+40+16*image_cache[i].image;
        if(u32(key)==off&&u32(key+4)==shape&&u32(key+8)==stored_format&&u32(key+12)==length){
            p=image_buffer(image_cache[i]);fmt=image_cache[i].format;image_cache[i].age=next_image_age();
            // A byte-identical descriptor and shape can inherit a successful
            // picture's validation; merely sharing a payload offset cannot.
            if(stored_format==6&&tile_validated_)tile_validated_[unsigned(id)/8]|=uint8_t(1u<<(unsigned(id)%8));
            break;
        }
    }
    if(!p){
        if(fmt==6){
            TileFrame frame;
            if(!tile_metadata_||length!=16||!frame.open(this,reader_,tile_metadata_,
                    uint32_t(tile_metadata_-metadata_),off,w,h))return fail();
            size=frame.output_size;fmt=4;
            ImageReservation reservation(size);uint8_t* output=reservation.bytes();if(!output)return fail();
            const bool all=!tile_validated_||!(tile_validated_[unsigned(id)/8]&(1u<<(unsigned(id)%8)));
            if(!rebuild_tile_frame(this,reader_,tile_metadata_,frame,w,h,all,output))return fail();
            p=output;reservation.publish(this,uint32_t(id),fmt);
            if(tile_validated_)tile_validated_[unsigned(id)/8]|=uint8_t(1u<<(unsigned(id)%8));
        }else{
            if(fmt==5){uint8_t header[5];if(length<6||!reader_.read(off,header,5))return fail();fmt=header[0];size=u32(header+1);}
            if(fmt>4||!size||size>MaxImageBytes)return fail();
            const uint8_t* source=payload(off,length,encoded_pixels,sizeof encoded_pixels);if(!source)return fail();
            if(stored_format!=5&&data_){p=source;if(!valid_image(p,size,w,h,fmt))return fail();}
            else {
                ImageReservation reservation(size);uint8_t*output=reservation.bytes();if(!output)return fail();
                if(stored_format==5){if(!decode_lz4(source+5,length-5,output,size))return fail();}
                else for(unsigned i=0;i<size;i++)output[i]=source[i];
                p=output;if(!valid_image(p,size,w,h,fmt))return fail();reservation.publish(this,uint32_t(id),fmt);
            }
        }
    }
    if(fmt==4)return {p,nullptr,nullptr,w,h,4};
    if(fmt>=2)return {p+512,fmt==3?p+512+w*h:nullptr,p,w,h,(uint8_t)fmt};
    return {p,fmt?p+w*h*2:nullptr,nullptr,w,h,(uint8_t)fmt};
}
Glyph Content::glyph(uint32_t cp,int size)const{
    if(!metadata_||!io_good_)return {nullptr,0,0,0};uint32_t key=((uint32_t)size<<24)|cp;int lo=0,hi=(int)glyphs_;
    while(lo<hi){int mid=(lo+hi)/2;const uint8_t*r=metadata_+40+images_*16+mid*16;uint32_t k=(uint32_t)u16(r+8)<<24|u32(r);if(k<key)lo=mid+1;else hi=mid;}
    if((unsigned)lo<glyphs_){const uint8_t*r=metadata_+40+images_*16+lo*16;if(((uint32_t)u16(r+8)<<24|u32(r))==key){
        const uint32_t off=u32(r+4),len=u16(r+14)?u16(r+14):unsigned(r[10])*r[11];const uint8_t*bytes=nullptr;
        if(data_)bytes=data_+off;
        else {for(const auto&v:glyph_cache)if(v.owner==this){
                const uint8_t*cached=metadata_+40+images_*16+v.glyph*16;
                const uint32_t count=u16(cached+14)?u16(cached+14):unsigned(cached[10])*cached[11];
                // Raw/RLE bytes may be shared by different code points or
                // layouts, but a shorter read cannot supply a longer view.
                if(u32(cached+4)==off&&count==len){bytes=v.bytes;break;}
            }
            if(!bytes){auto&v=glyph_cache[glyph_cursor++%64];v.owner=nullptr;bytes=payload(off,len,v.bytes,sizeof v.bytes);if(bytes){v.owner=this;v.glyph=(uint32_t)lo;}}}
        if(!bytes)return {nullptr,0,0,0};return {bytes,r[10],r[11],u16(r+12),u16(r+14)};
    }}
    if(cp!='?')return glyph('?',size);return {nullptr,0,0,(uint16_t)(size/2)};
}
const char* Content::string(int id)const{
    if(!metadata_||!io_good_||id<0||(unsigned)id>=strings_)return "";
    const uint8_t*r=metadata_+40+images_*16+glyphs_*16+id*8;const uint32_t off=u32(r),len=u32(r+4)+1;
    if(data_)return (const char*)(data_+off);
    for(const auto&v:string_cache)if(v.owner==this&&v.offset==off)return v.bytes;
    auto&v=string_cache[string_cursor++%8];v.owner=nullptr;
    if(!payload(off,len,(uint8_t*)v.bytes,sizeof v.bytes))return "";v.owner=this;v.offset=off;return v.bytes;
}
Clip Content::clip(int id)const{
    if(!metadata_||id<0||(unsigned)id>=clips_)return {0,0,1,0,0};
    const uint8_t*r=metadata_+40+images_*16+glyphs_*16+strings_*8+id*16;
    return {u32(r),u16(r+4),u16(r+6),(int16_t)u16(r+8),(int16_t)u16(r+10)};
}
int Content::card_image(int kind)const{
    if(!metadata_||kind<0||(unsigned)kind>=cards_)return -1;
    if(version_==3)return A_Card0+kind;
    const uint8_t*r=metadata_+40+images_*16+glyphs_*16+strings_*8+clips_*16+kind*16;return (int)u32(r);
}
const char* Content::card_string(int kind,int variant)const{
    if(!metadata_||kind<0||(unsigned)kind>=cards_||variant<0||variant>2)return "";
    if(version_==3)return string(kind*3+variant);
    const uint8_t*r=metadata_+40+images_*16+glyphs_*16+strings_*8+clips_*16+kind*16;return string((int)u32(r+4+variant*4));
}
int Content::ui_image(int key)const{
    if(!metadata_||!ui_offset_||key<0||(unsigned)key>=ui_count_)return -1;return (int)u32(metadata_+ui_offset_+key*4);
}
int Content::relic_image(int index)const{
    if(index<0||index>=WorldRelicKinds)return -1;
    if(version_>=14)return world_image("relics",world_relic_key(RelicId(index)));
    if(index>=RelicKinds)return -1;
    if(index>=30)return ui_image(U_CrackedCore+index-30);
    return index>=25?ui_image(U_Kunai+index-25):index>=17?ui_image(U_HornCleat+index-17):index==16?ui_image(U_SnakeRing):index<10?relic_definitions[index].asset:ui_image(U_Strawberry+index-10);
}
int Content::frame_image(Card card)const{
    if(card.kind==Soul)return ui_image(U_SoulFrame);
    if(card.kind==SovereignBlade)return ui_image(U_BladeFrame);
    if(card.kind>=CardKinds)return -1;
    const int type=(int)definitions[card.kind].type;
    // The reference has skill-shaped status/curse frames; never index past
    // an attack/skill/power triplet with the appended non-playable types.
    if(type==(int)CardType::Curse)return ui_image(U_CurseFrame);
    if(type==(int)CardType::Status)return ui_image(U_ColorlessSkill);
    if(type==(int)CardType::Quest)return version_>=14?world_image("cards","QUEST_FRAME"):ui_image(U_ColorlessSkill);
    const CardColor color=card_color(card.kind);
    int base=color==CardColor::Silent?U_GreenAttack:color==CardColor::Defect?U_BlueAttack:
        color==CardColor::Regent?U_OrangeAttack:color==CardColor::Necrobinder?U_PinkAttack:
        (color==CardColor::Colorless||color==CardColor::Token)?U_ColorlessAttack:U_RedAttack;
    const int image=ui_image(base+type);
    return image>=0?image:ui_image(U_BigAttack+type);
}
int Content::energy_image(Card card)const{
    if(card.kind>=CardKinds||cost(card)==-2)return -1;
    if(card.kind==Soul)return ui_image(U_SoulEnergy);
    if(card.kind==SovereignBlade)return ui_image(U_BladeEnergy);
    const CardColor color=card_color(card.kind);
    return color==CardColor::Silent?ui_image(U_SilentEnergy):color==CardColor::Defect?ui_image(U_DefectEnergy):
        color==CardColor::Regent?ui_image(U_RegentEnergy):color==CardColor::Necrobinder?ui_image(U_NecrobinderEnergy):
        (color==CardColor::Colorless||color==CardColor::Token)?ui_image(U_ColorlessEnergy):A_Energy;
}
int Content::potion_image(Potion p)const{
    unsigned i=(unsigned)p;if(!i||i>PotionKinds)return -1;
    return i>16?ui_image(U_ExtPotionStart+(int)i-17):i<=3?A_FirePotion+(int)i-1:ui_image(U_EnergyPotion+(int)i-4);
}
#include "world_content.inc"
} // namespace spire
