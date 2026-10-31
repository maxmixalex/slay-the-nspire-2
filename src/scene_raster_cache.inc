namespace {
// Both normal hand entries and the selected-description raster use the same
// face representation. Its explicit array defaults also keep the permanent
// hand array constant-initialized on the freestanding calculator target.
struct HandTile {
    bool valid=false,playable=false,resource_cross=false,description_truncated=false;
    uint8_t resource_styles=0;
    uint8_t glow=0,overlay=0;
    const Content* owner=nullptr;
    Card card{};
    uint32_t used=0,enchantment=0;
    uint32_t built=0;
    int sampled_instance=-1;
    int energy=0,stars=0,portrait=-1,frame=-1;
    char description[512]{};
    uint16_t pixels[HandTileW*HandTileH]{};
    uint8_t alpha[HandTileW*HandTileH]{};
};
struct SelectedHandRaster {
    // Claiming the shared slot starts only the inactive byte. The builder
    // copy-constructs tile from its complete source before using it, avoiding
    // an extra 42 KiB default clear that would immediately be overwritten.
    union {uint8_t inactive;HandTile tile;};
    uint16_t background[72*48];
    uint8_t coverage[72*48];
    SelectedHandRaster():inactive(0){}
};
}

// Retain the complete first bilinear rounding for each distinct horizontal
// color pair. The source64x64 gradient has266 pairs, but its actual input is
// checked against a fixed capacity before publication. Every frame still
// samples the current noise and applies the original vertical rounding.
struct RandomGradientCache {
    static constexpr unsigned Side=64,Capacity=272;
    uint16_t horizontal[Capacity][256];
    uint16_t pairs[Side*Side];
    uint8_t vertical_unit[Side*Side/8];
    const Content*owner=nullptr;
    Picture gradient{};
    uint32_t generation=0;
    unsigned logical_width=0,logical_height=0,count=0;
    bool observed=false,valid=false;
    static bool adjacent(uint16_t a,uint16_t b){
        return unsigned(int(a>>11)-int(b>>11)+1)<=2&&
            unsigned(int((a>>5)&63)-int((b>>5)&63)+1)<=2&&
            unsigned(int(a&31)-int(b&31)+1)<=2;
    }
    static uint16_t mix(uint16_t a,uint16_t b,unsigned f){
        if(a==b)return a;
        if(!((a^b)&0xf7deu))return f<128?a:f>128?b:uint16_t(a|b);
        const uint32_t ar=((uint32_t(a)&0xf800u)<<5)|(a&31u),br=((uint32_t(b)&0xf800u)<<5)|(b&31u);
        const uint32_t rb=(((ar<<8)+(br-ar)*f+0x00800080u)>>8)&0x001f001fu;
        const uint32_t ag=a&0x07e0u,bg=b&0x07e0u;
        const uint32_t g=(((ag<<8)+(bg-ag)*f+0x1000u)>>8)&0x07e0u;
        return uint16_t(((rb>>5)&0xf800u)|g|(rb&31u));
    }
    bool bind(const Content&content,const water_reflection::Texture&texture,void(*poll)()=nullptr){
        const Picture p=texture.storage();
        if(observed&&owner==&content&&generation==content.generation()&&
           logical_width==texture.width()&&logical_height==texture.height()&&
           gradient.pixels==p.pixels&&gradient.alpha==p.alpha&&gradient.palette==p.palette&&
           gradient.w==p.w&&gradient.h==p.h&&gradient.format==p.format)return valid;
        owner=&content;generation=content.generation();gradient=p;
        logical_width=texture.width();logical_height=texture.height();
        observed=true;valid=false;count=0;
        if(!texture.valid()||p.format>4||logical_width!=Side||logical_height!=Side)return false;
        for(auto&byte:vertical_unit)byte=0;
        uint32_t keys[Capacity];uint16_t buckets[512]{};
        for(unsigned y=0;y<Side;++y){if(!(y&15u)&&poll)poll();for(unsigned x=0;x<Side;++x){
            const uint16_t a=texture.at(x,y).color,b=texture.at(x+1,y).color;
            const uint16_t c=texture.at(x,y+1).color,d=texture.at(x+1,y+1).color;
            const unsigned at=y*Side+x;
            if(adjacent(a,c)&&adjacent(b,d))vertical_unit[at>>3]|=uint8_t(1u<<(at&7));
            const uint32_t key=uint32_t(a)|(uint32_t(b)<<16);
            unsigned bucket=((key*2654435761u)^(key>>13))&511u;
            while(buckets[bucket]&&keys[buckets[bucket]-1]!=key)bucket=(bucket+1)&511u;
            if(!buckets[bucket]){
                // A changed source with more distinct pairs keeps the
                // complete general sampler; no table entry may overflow.
                if(count==Capacity)return false;
                if(poll)poll();
                keys[count]=key;buckets[bucket]=uint16_t(++count);
                for(unsigned f=0;f<256;++f)horizontal[count-1][f]=mix(a,b,f);
            }
            pairs[at]=uint16_t(buckets[bucket]-1);
        }}
        valid=true;return true;
    }
    [[gnu::always_inline]] uint16_t sample(int u_q16,int v_q16)const{
        // width64 turns the source Q16 centre coordinates into these exact
        // shifts. Clamp-to-edge must also discard the border's fraction.
        const int x=u_q16-512,y=v_q16-512;
        int ix=x>>10,iy=y>>10;unsigned fx=(unsigned(x)>>2)&255u,fy=(unsigned(y)>>2)&255u;
        if(unsigned(ix)>=Side-1){ix=ix<0?0:Side-1;fx=0;}
        if(unsigned(iy)>=Side-1){iy=iy<0?0:Side-1;fy=0;}
        const unsigned at=unsigned(iy)*Side+unsigned(ix);
        if(!fy)return horizontal[pairs[at]][fx];
        // Equal horizontal weights and equal rounding preserve a per-channel
        // difference of at most one. Away from the vertical half fraction,
        // its final rounded mix is exactly one of the horizontal LUT values.
        if(fy!=128&&(vertical_unit[at>>3]&(1u<<(at&7))))
            return horizontal[pairs[at+(fy>128?Side:0)]][fx];
        const uint16_t top=horizontal[pairs[at]][fx];
        return mix(top,horizontal[pairs[at+Side]][fx],fy);
    }
};
static_assert(sizeof(RandomGradientCache)<=153u*1024u,"Exact gradient interpolation stays within153 KiB");

// With each channel's adjacent texels at most one band apart, either
// rounded interpolation selects its first or second endpoint, except at
// the exact half fraction. Verify that property for every source quad once;
// its bit permits one exact corner read, without approximating the filter.
struct RandomLightQuadCache {
    static constexpr unsigned TileShift=2,TileWidth=256>>TileShift;
    uint8_t safe[256*256/8];
    uint8_t maximum[TileWidth*TileWidth];
    uint8_t minimum[TileWidth*TileWidth];
    const Content*owner=nullptr;
    Picture picture{};
    uint32_t generation=0;
    unsigned logical_width=0,logical_height=0;
    bool observed=false,valid=false;
    static bool adjacent(uint16_t a,uint16_t b){
        const int red=int(a>>11)-int(b>>11),green=int((a>>5)&63)-int((b>>5)&63);
        return unsigned(red+1)<=2&&unsigned(green+1)<=2;
    }
    bool bind(const Content&content,const water_reflection::Texture&texture,void(*poll)()=nullptr){
        const Picture p=texture.storage();
        if(observed&&owner==&content&&generation==content.generation()&&
           logical_width==texture.width()&&logical_height==texture.height()&&
           picture.pixels==p.pixels&&picture.alpha==p.alpha&&picture.palette==p.palette&&
           picture.w==p.w&&picture.h==p.h&&picture.format==p.format)return valid;
        owner=&content;generation=content.generation();picture=p;
        logical_width=texture.width();logical_height=texture.height();observed=true;valid=false;
        if(!texture.valid()||p.format>3||logical_width!=256||logical_height!=256)return false;
        for(auto&byte:safe)byte=0;
        for(auto&byte:maximum)byte=0;
        for(auto&byte:minimum)byte=255;
        for(unsigned y=0;y<256;++y){if(!(y&15u)&&poll)poll();for(unsigned x=0;x<256;++x){
            const uint16_t a=texture.at(x,y).color,b=texture.at(x+1,y).color;
            const uint16_t c=texture.at(x,y+1).color,d=texture.at(x+1,y+1).color;
            const unsigned red=mx(mx(a>>11,b>>11),mx(c>>11,d>>11));
            const unsigned green=mx(mx((a>>5)&63,(b>>5)&63),mx((c>>5)&63,(d>>5)&63));
            const unsigned tile=(y>>TileShift)*TileWidth+(x>>TileShift);
            auto&bound=maximum[tile];auto&lower=minimum[tile];
            bound=uint8_t(mx(unsigned(bound),mx(red*2,green)));
            const unsigned red_min=mn(mn(a>>11,b>>11),mn(c>>11,d>>11));
            const unsigned green_min=mn(mn((a>>5)&63,(b>>5)&63),mn((c>>5)&63,(d>>5)&63));
            lower=uint8_t(mn(unsigned(lower),mn(red_min*2,green_min)));
            if(adjacent(a,b)&&adjacent(c,d)&&adjacent(a,c)&&adjacent(b,d)){
                const unsigned at=y*256+x;safe[at>>3]|=uint8_t(1u<<(at&7));
            }
        }}
        valid=true;return true;
    }
};
static_assert(sizeof(RandomLightQuadCache)<=17u*1024u,"Exact light quad bounds stay within17 KiB");
static_assert(sizeof(RandomGradientCache)+sizeof(RandomLightQuadCache)<=166u*1024u,
    "Water and light retain the previous combined fixed-memory allowance");

struct MerchantRugRaster {
 const Content*owner=nullptr;uint32_t generation=0;bool valid=false;
 uint16_t pixels[ScreenW*ScreenH];uint8_t alpha[ScreenW*ScreenH];
};

// Character selection, the shop and the selected combat description never
// draw each other's sampled raster in one frame. The selected description's
// small identity and scroll clock live outside this slot and survive eviction.
// Keep one lifetime-aware slot. Switching only resets metadata: each bind fully
// writes its live entries before publishing valid=true, so a scene change must
// not clear another 230 KiB on the calculator. Do not add array initializers to
// the cache types above. Foreground input collection cannot re-enter rendering.
struct RandomSceneRasters { RandomGradientCache gradient;RandomLightQuadCache light; };
union SceneRasterStorage {
    uint8_t inactive;
    RandomSceneRasters random;
    MerchantRugRaster merchant;
    SelectedHandRaster selected;
    character_select_delta::Raster character;
};
enum class SceneRasterOwner:uint8_t { None,Random,Merchant,SelectedHand,Character };
static SceneRasterStorage scene_raster_storage{0};
static SceneRasterOwner scene_raster_owner=SceneRasterOwner::None;
static_assert(sizeof(SceneRasterStorage)==sizeof(MerchantRugRaster),
    "Random, merchant, selection and selected-hand rasters share one fixed scene allocation");
static_assert(sizeof(SelectedHandRaster)<56u*1024u,
    "Only one selected description may animate");
static_assert(sizeof(RandomSceneRasters)>=160u*1024u,
    "Scene sharing removes at least 160 KiB of permanently separate tables");
static RandomSceneRasters& random_scene_rasters(){
    if(scene_raster_owner!=SceneRasterOwner::Random){
        ::new(detail::InPlaceAddress{&scene_raster_storage.random}) RandomSceneRasters;
        scene_raster_owner=SceneRasterOwner::Random;
    }
    return scene_raster_storage.random;
}
static RandomGradientCache& random_gradient_cache(){return random_scene_rasters().gradient;}
static RandomLightQuadCache& random_light_quads(){return random_scene_rasters().light;}
static MerchantRugRaster& merchant_rug_raster(){
    if(scene_raster_owner!=SceneRasterOwner::Merchant){
        ::new(detail::InPlaceAddress{&scene_raster_storage.merchant}) MerchantRugRaster;
        scene_raster_owner=SceneRasterOwner::Merchant;
    }
    return scene_raster_storage.merchant;
}
static SelectedHandRaster& selected_hand_raster(){
    if(scene_raster_owner!=SceneRasterOwner::SelectedHand){
        ::new(detail::InPlaceAddress{&scene_raster_storage.selected}) SelectedHandRaster;
        scene_raster_owner=SceneRasterOwner::SelectedHand;
    }
    return scene_raster_storage.selected;
}
static character_select_delta::Raster& character_selection_raster(){
    if(scene_raster_owner!=SceneRasterOwner::Character){
        ::new(detail::InPlaceAddress{&scene_raster_storage.character}) character_select_delta::Raster;
        scene_raster_owner=SceneRasterOwner::Character;
    }
    return scene_raster_storage.character;
}
