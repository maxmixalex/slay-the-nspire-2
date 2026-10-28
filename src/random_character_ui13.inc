// Retain the complete first bilinear rounding for each distinct horizontal
// color pair. The source64x64 gradient has266 pairs, but its actual input is
// checked against a fixed capacity before publication. Every frame still
// samples the current noise and applies the original vertical rounding.
struct RandomGradientCache {
    static constexpr unsigned Side=64,Capacity=272;
    uint16_t horizontal[Capacity][256]{};
    uint16_t pairs[Side*Side]{};
    uint8_t vertical_unit[Side*Side/8]{};
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
    bool bind(const Content&content,const water_reflection::Texture&texture){
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
        for(unsigned y=0;y<Side;++y)for(unsigned x=0;x<Side;++x){
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
                keys[count]=key;buckets[bucket]=uint16_t(++count);
                for(unsigned f=0;f<256;++f)horizontal[count-1][f]=mix(a,b,f);
            }
            pairs[at]=uint16_t(buckets[bucket]-1);
        }
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
static RandomGradientCache random_gradient_cache;
static_assert(sizeof(RandomGradientCache)<=153u*1024u,"Exact gradient interpolation stays within153 KiB");

// A row owns one immutable gradient and one vertical shader coordinate.
// Repeated (noise, clamped source column) values select the same horizontal
// LUT, while the current horizontal fraction still selects its exact pixel.
// Nothing survives the row, so a new clock/generation/gradient cannot reuse
// an earlier row's pointer or vertical rounding state.
class RandomGradientRow {
    const RandomGradientCache*cache_;
    const int*shifts_;
    int v_;
    unsigned key_=~0u,fraction_=0;
    const uint16_t*top_=nullptr,*bottom_=nullptr;
public:
    RandomGradientRow(const RandomGradientCache*cache,int v,const int*shifts):cache_(cache),shifts_(shifts),v_(v){}
    template<bool Interior=false> [[gnu::always_inline]] uint16_t sample(int u,unsigned noise){
        const int shift=shifts_[noise],x=u+shift-512;
        int ix=x>>10;unsigned fx=(unsigned(x)>>2)&255u;
        if(!Interior&&unsigned(ix)>=63){ix=ix<0?0:63;fx=0;}
        const unsigned key=(noise<<6)|unsigned(ix);
        if(key!=key_){
            key_=key;const int y=v_+shift-512;
            int iy=y>>10;unsigned fy=(unsigned(y)>>2)&255u;
            if(!Interior&&unsigned(iy)>=63){iy=iy<0?0:63;fy=0;}
            const unsigned at=unsigned(iy)*64+unsigned(ix);bottom_=nullptr;
            if(!fy)top_=cache_->horizontal[cache_->pairs[at]];
            else if(fy!=128&&(cache_->vertical_unit[at>>3]&(1u<<(at&7))))
                top_=cache_->horizontal[cache_->pairs[at+(fy>128?64:0)]];
            else{top_=cache_->horizontal[cache_->pairs[at]];bottom_=cache_->horizontal[cache_->pairs[at+64]];fraction_=fy;}
        }
        const uint16_t top=top_[fx];
        return bottom_?RandomGradientCache::mix(top,bottom_[fx],fraction_):top;
    }
};

// ARM926 implements each dynamic64-bit division in software. The rotated
// quad's row numerators are linear, so retain a floor quotient and a positive
// remainder. One carry reproduces the next row, including negative UVs;
// value() restores C++'s original division toward zero exactly.
class RandomRowQuotient {
    int64_t quotient_,step_;
    uint32_t remainder_,increment_,denominator_;
public:
    RandomRowQuotient(int64_t first,int64_t delta,int denominator):denominator_(unsigned(denominator)){
        quotient_=first/denominator;int64_t remainder=first-quotient_*denominator;
        if(remainder<0){--quotient_;remainder+=denominator;}remainder_=uint32_t(remainder);
        step_=delta/denominator;remainder=delta-step_*denominator;
        if(remainder<0){--step_;remainder+=denominator;}increment_=uint32_t(remainder);
    }
    int value()const{return int(quotient_+(quotient_<0&&remainder_?1:0));}
    void advance(){quotient_+=step_;remainder_+=increment_;
        if(remainder_>=denominator_){remainder_-=denominator_;++quotient_;}}
};

// With each channel's adjacent texels at most one band apart, either
// rounded interpolation selects its first or second endpoint, except at
// the exact half fraction. Verify that property for every source quad once;
// its bit permits one exact corner read, without approximating the filter.
struct RandomLightQuadCache {
    static constexpr unsigned TileShift=2,TileWidth=256>>TileShift;
    uint8_t safe[256*256/8]{};
    uint8_t maximum[TileWidth*TileWidth]{};
    uint8_t minimum[TileWidth*TileWidth]{};
    const Content*owner=nullptr;
    Picture picture{};
    uint32_t generation=0;
    unsigned logical_width=0,logical_height=0;
    bool observed=false,valid=false;
    static bool adjacent(uint16_t a,uint16_t b){
        const int red=int(a>>11)-int(b>>11),green=int((a>>5)&63)-int((b>>5)&63);
        return unsigned(red+1)<=2&&unsigned(green+1)<=2;
    }
    bool bind(const Content&content,const water_reflection::Texture&texture){
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
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){
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
        }
        valid=true;return true;
    }
};
static RandomLightQuadCache random_light_quads;
static_assert(sizeof(RandomLightQuadCache)<=17u*1024u,"Exact light quad bounds stay within17 KiB");
static_assert(sizeof(RandomGradientCache)+sizeof(RandomLightQuadCache)<=166u*1024u,
    "Water and light retain the previous combined fixed-memory allowance");

// The original light has a256x256 logical texture and a zero blue tint.
// Keep red and green in separate16-bit lanes, retaining both bilinear
// rounding steps. The palette variant decodes its256 possible colors once.
class RandomLightSampler {
    Picture picture_{};
    const uint8_t*red_=nullptr,*green_=nullptr;
    const uint8_t*quad_safe_=nullptr;
    const uint8_t*tile_maximum_=nullptr;
    const uint8_t*tile_minimum_=nullptr;
    const uint16_t*tile_contribution_=nullptr;
    unsigned zero_limit_=0;
    uint32_t palette_[256];
    bool enabled_=false;
    static uint32_t packed(uint16_t color){return (uint32_t(color&0xf800u)<<5)|((color>>5)&63u);}
    static uint32_t mix(uint32_t a,uint32_t b,unsigned f){
        if(a==b)return a;
        if(!((a^b)&0x001e003eu))return f<128?a:f>128?b:a|b;
        return (((a<<8)+(b-a)*f+0x00800080u)>>8)&0x001f003fu;
    }
    uint16_t dense(unsigned index)const{const auto*p=picture_.pixels+index*2;return uint16_t(p[0]|unsigned(p[1])<<8);}
    [[gnu::always_inline]] bool zero(unsigned quad)const{
        return tile_maximum_&&tile_maximum_[((quad>>10)<<6)|((quad>>2)&63)]<=zero_limit_;
    }
    [[gnu::always_inline]] uint16_t constant(unsigned quad)const{
        const unsigned tile=((quad>>10)<<6)|((quad>>2)&63);
        if(tile_contribution_)return tile_contribution_[tile];
        return tile_value(tile);
    }
    [[gnu::always_inline]] uint16_t tile_value(unsigned tile)const{
        const unsigned low=tile_minimum_[tile],high=tile_maximum_[tile];
        const unsigned red=red_[high>>1],green=green_[high];
        // Both source filtering roundings stay within the numeric channel
        // extrema. Equal endpoints of their monotone tint/opacity maps prove
        // this exact contribution for every filtered quad in the4x4 tile.
        if(red_[(low+1)>>1]==red&&green_[low]==green)return uint16_t((red<<11)|(green<<5));
        return 1; // An actual contribution always has a zero blue channel.
    }
    template<bool Indexed>
    [[gnu::always_inline]] uint16_t filtered(unsigned aa,unsigned bb,unsigned cc,unsigned dd,unsigned fx,unsigned fy)const{
        uint32_t a,b,c,d;
        if(Indexed){
            a=palette_[picture_.pixels[aa]];b=palette_[picture_.pixels[bb]];
            c=palette_[picture_.pixels[cc]];d=palette_[picture_.pixels[dd]];
            const uint32_t bound=a|b|c|d,lower=a&b&c&d;
            const unsigned red=red_[bound>>16],green=green_[bound&63];
            if(!red&&!green)return 0;
            if(red_[lower>>16]==red&&green_[lower&63]==green)return uint16_t((red<<11)|(green<<5));
        }else{
            const uint16_t ac=dense(aa),bc=dense(bb),ccolor=dense(cc),dc=dense(dd),bound=ac|bc|ccolor|dc;
            const unsigned red=red_[bound>>11],green=green_[(bound>>5)&63];
            if(!red&&!green)return 0;
            const uint16_t lower=ac&bc&ccolor&dc;
            if(red_[lower>>11]==red&&green_[(lower>>5)&63]==green)return uint16_t((red<<11)|(green<<5));
            a=packed(ac);b=packed(bc);c=packed(ccolor);d=packed(dc);
        }
        const uint32_t color=mix(mix(a,b,fx),mix(c,d,fx),fy);
        return uint16_t((unsigned(red_[color>>16])<<11)|(unsigned(green_[color&63])<<5));
    }
public:
    RandomLightSampler(const water_reflection::Texture&texture,const uint8_t*red,const uint8_t*green,bool enable=true,
            const uint8_t*quad_safe=nullptr,const uint8_t*tile_maximum=nullptr,const uint8_t*tile_minimum=nullptr,uint16_t*tile_contribution=nullptr):
        picture_(texture.storage()),red_(red),green_(green),quad_safe_(quad_safe),tile_maximum_(tile_maximum),tile_minimum_(tile_minimum){
        enabled_=enable&&texture.valid()&&texture.width()==256&&texture.height()==256&&picture_.format!=4;
        if(enabled_&&picture_.palette)for(unsigned i=0;i<256;++i)
            palette_[i]=packed(uint16_t(picture_.palette[i*2]|unsigned(picture_.palette[i*2+1])<<8));
        // q bounds every contributing source red by q/2 and green by q.
        // Tint and opacity are monotone, so one per-frame threshold proves
        // that an entire 4x4 group of quads contributes exactly zero.
        if(enabled_&&tile_maximum_)for(unsigned q=1;q<64&&!red_[q>>1]&&!green_[q];++q)zero_limit_=q;
        if(enabled_&&tile_minimum_&&tile_contribution){
            for(unsigned i=0;i<4096;++i)tile_contribution[i]=tile_value(i);
            tile_contribution_=tile_contribution;
        }
    }
    explicit operator bool()const{return enabled_;}
    bool indexed()const{return picture_.palette!=nullptr;}
    static bool interior_span(int u,int v,int du,int dv,unsigned count){
        if(!count)return false;
        const int64_t last_u=int64_t(u)+int64_t(du)*(count-1);
        const int64_t last_v=int64_t(v)+int64_t(dv)*(count-1);
        // UVs are linear within a row. Both endpoints in this interval
        // prove that every bilinear quad has four in-bounds source texels.
        return u>=128&&u<65408&&v>=128&&v<65408&&
            last_u>=128&&last_u<65408&&last_v>=128&&last_v<65408;
    }
    template<bool Indexed>
    [[gnu::always_inline]] uint16_t interior(int u,int v)const{
        const unsigned x=unsigned(u)-128,y=unsigned(v)-128;
        const unsigned quad=(y&0xff00u)|(x>>8);
        if(tile_minimum_){const auto value=constant(quad);if(value!=1)return value;}
        else if(zero(quad))return 0;
        return interior_variable<Indexed>(u,v);
    }
    [[gnu::always_inline]] uint16_t tile_contribution(int u,int v)const{
        if(!tile_minimum_)return 1;
        const unsigned x=unsigned(u)-128,y=unsigned(v)-128;
        return constant((y&0xff00u)|(x>>8));
    }
    template<bool Indexed>
    [[gnu::always_inline]] uint16_t interior_variable(int u,int v)const{
        const unsigned x=unsigned(u)-128,y=unsigned(v)-128;
        const unsigned quad=(y&0xff00u)|(x>>8);
        // u/v's zero low byte is the original half fraction. Keep that
        // exact tie and every unproven quad on the complete sampler path.
        if(quad_safe_&&(u&255)&&(v&255)&&(quad_safe_[quad>>3]&(1u<<(quad&7)))){
            const unsigned at=(unsigned(v)&0xff00u)|(unsigned(u)>>8);
            if(Indexed){const uint32_t color=palette_[picture_.pixels[at]];
                return uint16_t((unsigned(red_[color>>16])<<11)|(unsigned(green_[color&63])<<5));}
            const uint16_t color=dense(at);
            return uint16_t((unsigned(red_[color>>11])<<11)|(unsigned(green_[(color>>5)&63])<<5));
        }
        // This row already proved all four texels in bounds. Reuse its
        // quad and format instead of repeating edge clamps, zero/safe tests
        // and format selection in the general sampler for every shaded pixel.
        return filtered<Indexed>(quad,quad+1,quad+256,quad+257,x&255,y&255);
    }
    uint16_t contribution(int u,int v)const{
        // u,v are inside the source quad. The rare border samples retain
        // clamp-to-edge; interior texels need only two adjacent row offsets.
        const int x=u-128,y=v-128,ix=x>>8,iy=y>>8;
        unsigned left=unsigned(ix),right=left+1,top=unsigned(iy)*256,bottom=top+256;
        if(unsigned(ix)>=255){left=ix<0?0:255;right=ix<0?0:255;}
        if(unsigned(iy)>=255){top=iy<0?0:255*256;bottom=top;}
        const unsigned aa=top+left,bb=top+right,cc=bottom+left,dd=bottom+right;
        if(tile_minimum_){const auto value=constant(aa);if(value!=1)return value;}
        else if(zero(aa))return 0;
        const unsigned fx=unsigned(x)&255,fy=unsigned(y)&255;
        if(quad_safe_&&fx!=128&&fy!=128&&(quad_safe_[aa>>3]&(1u<<(aa&7)))){
            const unsigned at=(fy<128?top:bottom)+(fx<128?left:right);
            if(picture_.palette){const uint32_t color=palette_[picture_.pixels[at]];
                return uint16_t((unsigned(red_[color>>16])<<11)|(unsigned(green_[color&63])<<5));}
            const uint16_t color=dense(at);
            return uint16_t((unsigned(red_[color>>11])<<11)|(unsigned(green_[(color>>5)&63])<<5));
        }
        return picture_.palette?filtered<true>(aa,bb,cc,dd,fx,fy):filtered<false>(aa,bb,cc,dd,fx,fy);
    }
};

[[gnu::always_inline]] static inline uint16_t random_light_add(uint16_t back,uint16_t contribution){
    return uint16_t((mn(31,(back>>11)+(contribution>>11))<<11)|
        (mn(63,((back>>5)&63)+((contribution>>5)&63))<<5)|(back&31));
}
struct RandomInteriorRange {unsigned first,last;};
// Within one4x4 source-quad tile the contribution table proves either one
// exact color or the need for individual bilinear samples. Walk whole spans
// of that tile, especially the large zero-light regions, instead of looking
// up the same proof at every screen pixel. Only the two setup divisions are
// dynamic: the row loop uses a multiply-high and an exact remainder correction.
class RandomLightTileStep {
    unsigned step_,inverse_;
    bool forward_;
public:
    explicit RandomLightTileStep(int delta):step_(delta<0?0u-unsigned(delta):unsigned(delta)),
        inverse_(step_?~0u/step_:0),forward_(delta>0){}
    [[gnu::always_inline]] unsigned count(int coordinate)const{
        if(!step_)return ~0u;
        const unsigned fraction=(unsigned(coordinate)-128)&1023u;
        const unsigned distance=forward_?1023u-fraction:fraction;
        unsigned quotient=unsigned((uint64_t(distance)*inverse_)>>32);
        quotient+=distance-quotient*step_>=step_;
        return quotient+1;
    }
};
static RandomInteriorRange random_interior_range(int u,int v,int du,int dv,unsigned count){
    if(RandomLightSampler::interior_span(u,v,du,dv,count))return {0,count};
    unsigned first=0,last=count;int64_t start_u=u,start_v=v;
    const auto inside=[](int64_t x,int64_t y){return x>=128&&x<65408&&y>=128&&y<65408;};
    // A rotated quad's bounding-box row often starts and ends outside the
    // texture even though its middle is visible. Trim only those margins;
    // the intersection of a line and the interior rectangle is contiguous.
    while(first<last&&!inside(start_u,start_v)){++first;start_u+=du;start_v+=dv;}
    if(first==last)return {count,count};
    int64_t end_u=int64_t(u)+int64_t(du)*(last-1),end_v=int64_t(v)+int64_t(dv)*(last-1);
    while(last>first&&!inside(end_u,end_v)){--last;end_u-=du;end_v-=dv;}
    return {first,last};
}
template<bool Indexed>
static void random_character_light(Canvas&c,const RandomLightSampler&light,
        int left,int top,int right,int bottom,int du,int dv,RandomRowQuotient row_u,RandomRowQuotient row_v){
    const RandomLightTileStep u_tiles(du),v_tiles(-dv);
    for(int y=top;y<bottom;++y,row_u.advance(),row_v.advance()){
        int u=32768+row_u.value(),v=32768+row_v.value();auto*out=c.pixels+y*c.pitch;
        const auto interior=random_interior_range(u,v,du,-dv,unsigned(right-left));
        const int first=left+int(interior.first),last=left+int(interior.last);int x=left;
        // A rotated quad's bounding rectangle almost always has exterior end
        // pixels. That must not force its entire interior row through the
        // generic border sampler. Only the two actual edge spans need clamps.
        for(;x<first;++x,u+=du,v-=dv){
            if(unsigned(u)>=65536||unsigned(v)>=65536)continue;
            const uint16_t contribution=light.contribution(u,v);
            if(contribution)out[x]=random_light_add(out[x],contribution);
        }
        while(x<last){
            const unsigned u_count=u_tiles.count(u),v_count=v_tiles.count(v);
            const unsigned tile_count=u_count<v_count?u_count:v_count;
            const unsigned remaining=unsigned(last-x),count=remaining<tile_count?remaining:tile_count;
            const uint16_t contribution=light.tile_contribution(u,v);
            if(contribution==1){
                const int end=x+int(count);
                for(;x<end;++x,u+=du,v-=dv){
                    const uint16_t sampled=light.interior_variable<Indexed>(u,v);
                    if(sampled)out[x]=random_light_add(out[x],sampled);
                }
            }else{
                if(contribution)for(unsigned i=0;i<count;++i)
                    out[x+int(i)]=random_light_add(out[x+int(i)],contribution);
                x+=int(count);u+=du*int(count);v-=dv*int(count);
            }
        }
        for(;x<right;++x,u+=du,v-=dv){
            if(unsigned(u)>=65536||unsigned(v)>=65536)continue;
            const uint16_t contribution=light.contribution(u,v);
            if(contribution)out[x]=random_light_add(out[x],contribution);
        }
    }
}
[[gnu::always_inline]] static inline unsigned random_dot_alpha(const uint8_t*alpha,int u,int v){
    const unsigned x=unsigned(u)-128,y=unsigned(v)-128,fx=x&255,fy=y&255;
    const unsigned at=(y&0xff00u)|(x>>8);
    const unsigned top=(unsigned(alpha[at])*(256-fx)+unsigned(alpha[at+1])*fx+128)>>8;
    const unsigned bottom=(unsigned(alpha[at+256])*(256-fx)+unsigned(alpha[at+257])*fx+128)>>8;
    return (top*(256-fy)+bottom*fy+128)>>8;
}
static void random_character_white_dot(Canvas&c,const water_reflection::Texture&texture,
        int left,int top,int right,int bottom,int du,int dv,RandomRowQuotient row_u,RandomRowQuotient row_v,
        unsigned alpha,uint16_t color){
    const auto*source=texture.storage().alpha;
    for(int y=top;y<bottom;++y,row_u.advance(),row_v.advance()){
        int u=32768+row_u.value(),v=32768+row_v.value();auto*out=c.pixels+y*c.pitch;
        const auto interior=random_interior_range(u,v,du,-dv,unsigned(right-left));
        const int first=left+int(interior.first),last=left+int(interior.last);int x=left;
        for(;x<first;++x,u+=du,v-=dv){
            if(unsigned(u)>=65536||unsigned(v)>=65536)continue;
            const unsigned amount=div255(texture.alpha_bilinear(u,v)*alpha);
            if(amount)out[x]=blend(out[x],color,amount);
        }
        for(;x<last;++x,u+=du,v-=dv){
            const unsigned amount=div255(random_dot_alpha(source,u,v)*alpha);
            if(amount)out[x]=blend(out[x],color,amount);
        }
        for(;x<right;++x,u+=du,v-=dv){
            if(unsigned(u)>=65536||unsigned(v)>=65536)continue;
            const unsigned amount=div255(texture.alpha_bilinear(u,v)*alpha);
            if(amount)out[x]=blend(out[x],color,amount);
        }
    }
}

// Textured particles use the original images, source modulation and source
// emitter rotation. The source quad is clipped before any inverse sampling.
enum class RandomTextureSource:uint8_t {Color,WhiteAlpha,OpaqueColor};
static void random_character_particle(Canvas&c,const water_reflection::Texture&texture,
        int center_x_q16,int center_y_q16,int size_q16,int angle,unsigned alpha,
        unsigned red,unsigned green,unsigned blue,unsigned mode,RandomTextureSource source=RandomTextureSource::Color){
    if(size_q16<=0||!alpha)return;
    const int sine=CombatCardMotion::sine(angle),cosine=CombatCardMotion::sine(angle+1024);
    const int half=int(int64_t(size_q16)*((sine<0?-sine:sine)+(cosine<0?-cosine:cosine))/2048);
    const int left=mx(0,(center_x_q16-half)>>16),right=mn(c.clip_w,((center_x_q16+half+65535)>>16));
    const int top=mx(0,(center_y_q16-half)>>16),bottom=mn(c.clip_h,((center_y_q16+half+65535)>>16));
    if(left>=right||top>=bottom)return;
    Canvas layer=c;layer.tint_red=uint8_t(red);layer.tint_green=uint8_t(green);layer.tint_blue=uint8_t(blue);
    const bool direct=c.opacity==255&&c.brightness==255&&!c.color_matrix&&!c.occlusion&&!c.coverage;
    const bool white_fast=source==RandomTextureSource::WhiteAlpha&&direct&&mode==presentation::Normal;
    const bool add_fast=source==RandomTextureSource::OpaqueColor&&direct&&mode==presentation::Add;
    const uint16_t dot_color=presentation::tint_rgb(0xffff,red,green,blue);
    uint8_t add_red[32],add_green[64],add_blue[32];
    if(add_fast){
        // The source light has opaque alpha. Tint and additive opacity are
        // independent channel maps, so evaluate each source band once rather
        // than repeating six divisions at every covered screen pixel.
        for(unsigned i=0;i<64;++i){add_green[i]=uint8_t(div255(div255(i*green)*alpha));
            if(i<32){add_red[i]=uint8_t(div255(div255(i*red)*alpha));add_blue[i]=uint8_t(div255(div255(i*blue)*alpha));}}
    }
    const bool use_light=add_fast&&blue==0;
    const uint8_t*quad_safe=use_light&&random_light_quads.bind(c.a.content,texture)?random_light_quads.safe:nullptr;
    // A direct4096-entry contribution table costs one read per shaded pixel.
    // It lives only in the existing map workspace's render tail for this
    // particle call, avoiding both the calculator stack and a permanent pool.
    ForegroundScratch<uint16_t,4096> light_contributions;
    const RandomLightSampler light(texture,add_red,add_green,use_light,quad_safe,
        quad_safe?random_light_quads.maximum:nullptr,quad_safe?random_light_quads.minimum:nullptr,
        light_contributions?light_contributions.array():nullptr);
    const int du=int(int64_t(cosine)*65536*64/size_q16),dv=int(int64_t(sine)*65536*64/size_q16);
    const int dx=(left<<16)+32768-center_x_q16,dy=(top<<16)+32768-center_y_q16;
    RandomRowQuotient row_u((int64_t(dx)*cosine+int64_t(dy)*sine)*64,int64_t(sine)*65536*64,size_q16);
    RandomRowQuotient row_v((int64_t(dy)*cosine-int64_t(dx)*sine)*64,int64_t(cosine)*65536*64,size_q16);
    if(light){
        if(light.indexed())random_character_light<true>(c,light,left,top,right,bottom,du,dv,row_u,row_v);
        else random_character_light<false>(c,light,left,top,right,bottom,du,dv,row_u,row_v);
        return;
    }
    if(white_fast&&texture.width()==256&&texture.height()==256&&texture.storage().format!=4&&texture.storage().alpha){
        random_character_white_dot(c,texture,left,top,right,bottom,du,dv,row_u,row_v,alpha,dot_color);return;
    }
    for(int y=top;y<bottom;++y,row_u.advance(),row_v.advance()){
        int u=32768+row_u.value(),v=32768+row_v.value();
        for(int x=left;x<right;++x,u+=du,v-=dv){
            if(u<0||v<0||u>=65536||v>=65536)continue;
            const unsigned at=y*c.pitch+x;
            if(white_fast){const unsigned amount=div255(texture.alpha_bilinear(u,v)*alpha);
                if(amount)c.pixels[at]=blend(c.pixels[at],dot_color,amount);continue;}
            if(add_fast){
                const uint16_t color=texture.opaque_bilinear(u,v),back=c.pixels[at];
                c.pixels[at]=uint16_t((mn(31,(back>>11)+add_red[color>>11])<<11)|
                    (mn(63,((back>>5)&63)+add_green[(color>>5)&63])<<5)|mn(31,(back&31)+add_blue[color&31]));continue;}
            const auto p=source==RandomTextureSource::WhiteAlpha?water_reflection::Texel{0xffff,texture.alpha_bilinear(u,v)}:
                source==RandomTextureSource::OpaqueColor?water_reflection::Texel{texture.opaque_bilinear(u,v),255}:texture.bilinear(u,v);
            if(p.alpha)layer.paint(y*c.pitch+x,p.color,div255(p.alpha*alpha),mode);
        }
    }
}
// The Random control's projected UV rectangle, including the strongest
// negative noise displacement, stays strictly inside the 64x64 gradient.
// A separate row kernel leaves the frame setup and fallback renderer outside
// the ARM pixel loop; the source interpolation and its two roundings remain.
template<bool Direct> [[gnu::noinline]] static void random_character_water_cached_row(
        Canvas&c,int y,const int*u,const uint8_t*noise,unsigned width,
        const int*shifts,int v,const RandomGradientCache&cache){
    RandomGradientRow row(&cache,v,shifts);auto*out=c.pixels+y*c.pitch;
    for(unsigned x=0;x<width;++x){
        const uint16_t color=row.sample<true>(u[x],noise[x]);
        if(Direct)out[x]=color;else c.paint(y*c.pitch+int(x),color);
    }
}
static void random_character_water(Canvas&c,const water_reflection::Texture&gradient,
        const water_reflection::Texture&noise,uint32_t clock,RandomGradientCache*cache=nullptr){
    const water_reflection::Parameters parameters(clock,23,100);
    // Pixel-centre UVs after both Control pivots and responsive scale. Keep
    // Q24 increments so truncation cannot accumulate down the 240px image.
    static constexpr int U0=5196007,UStep=26366,V0=1765572,VStep=56187;
    static_assert((U0>>8)-3276>512&&((U0+319*UStep)>>8)<65024&&
                  (V0>>8)-3276>512&&((V0+239*VStep)>>8)<65024,
                  "The complete projected water control is interior");
    int u[320],shifts[256];uint8_t noise_rows[5][320],noise_row[320];
    for(unsigned i=0;i<256;++i)shifts[i]=(int(i)-255)*parameters.half_strength_q16/255;
    for(int x=0;x<320;++x){
        u[x]=(U0+x*UStep)>>8;
        const int position=((u[x]-parameters.pan_q16)&65535)*10-32768,ix=position>>16;
        const unsigned fraction=unsigned(position&65535)>>8;
        // Five source rows share all horizontal noise interpolation with
        // 240 screen rows. This is byte-exact with Texture::alpha_bilinear.
        for(int y=0;y<5;++y)noise_rows[y][x]=uint8_t((noise.at(ix,y,true).alpha*(256-fraction)+noise.at(ix+1,y,true).alpha*fraction+128)>>8);
    }
    const bool direct=c.opacity==255&&c.brightness==255&&c.tint_red==255&&c.tint_green==255&&c.tint_blue==255&&!c.color_matrix&&!c.occlusion&&!c.coverage;
    const bool lookup=cache&&cache->bind(c.a.content,gradient);
    for(int y=0;y<c.clip_h&&y<240;++y){
        const int v=(V0+y*VStep)>>8,position=((v-parameters.pan_q16)&65535)*5-32768;
        const int iy=position>>16,top=iy<0?4:iy,bottom=iy+1>=5?0:iy+1;const unsigned fraction=unsigned(position&65535)>>8;
        const auto*a=noise_rows[top],*b=noise_rows[bottom];auto*out=c.pixels+y*c.pitch;
        const unsigned width=unsigned(mx(0,mn(c.clip_w,320)));
        if(lookup){
            for(unsigned x=0;x<width;++x)
                noise_row[x]=uint8_t((unsigned(a[x])*(256-fraction)+unsigned(b[x])*fraction+128)>>8);
            if(direct)random_character_water_cached_row<true>(c,y,u,noise_row,width,shifts,v,*cache);
            else random_character_water_cached_row<false>(c,y,u,noise_row,width,shifts,v,*cache);
        }else for(unsigned x=0;x<width;++x){
            const unsigned n=(unsigned(a[x])*(256-fraction)+unsigned(b[x])*fraction+128)>>8;
            const int shift=shifts[n];
            const uint16_t color=gradient.opaque_bilinear(u[x]+shift,v+shift);
            if(direct)out[x]=color;else c.paint(y*c.pitch+int(x),color);
        }
    }
}
static bool random_character_background(Canvas&c,uint32_t clock,uint32_t elapsed,uint32_t seed){
    using namespace water_reflection;
    const Picture gp=c.a.content.image(c.a.content.ui_image(I13_RandomGradient));
    const Picture np=c.a.content.image(c.a.content.ui_image(I13_RandomNoise));
    const Picture lp=c.a.content.image(c.a.content.ui_image(I13_RandomLight));
    const Picture dp=c.a.content.image(c.a.content.ui_image(I13_RandomDot));
    if(gp.w!=64||gp.h!=64||np.w!=10||np.h!=5||lp.w!=320||lp.h!=205||dp.w!=320||dp.h!=205)return false;
    const Texture gradient(gp),noise(np),light(lp,256,256),dot(dp,256,256);
    const auto profile=c.a.callbacks.profile_render;
    if(profile)profile(RuntimeRenderPart::RandomWater,true);
    random_character_water(c,gradient,noise,clock,&random_gradient_cache);
    if(profile)profile(RuntimeRenderPart::RandomWater,false);
    if(profile)profile(RuntimeRenderPart::RandomLight,true);
    // little_light_script.gd randomizes only integer preprocess time 0..5.
    const unsigned warm=RandomCharacterPresentation::hash(seed^0x51ba3f21u)%6;
    const unsigned phase=((elapsed+warm*1000)%3000)*4096/3000;
    const unsigned scale=RandomCharacterPresentation::light_scale(phase),gray=RandomCharacterPresentation::light_gray(phase);
    const int size=int(uint64_t(256*20)*scale*15832249/(4096*1000));
    random_character_particle(c,light,RandomCharacterPresentation::project_q16(1111*1024,false),
        RandomCharacterPresentation::project_q16(-83*1024,true),size,2023,
        102,171*gray/255,113*gray/255,0,presentation::Add,RandomTextureSource::OpaqueColor);
    if(profile)profile(RuntimeRenderPart::RandomLight,false);
    if(profile)profile(RuntimeRenderPart::RandomDots,true);
    auto&particles=c.a.character_select_presentation.random_background;particles.observe(elapsed,seed);
    for(unsigned i=0;i<RandomCharacterPresentation::Count;++i){const auto&p=particles.particles[i];
        const unsigned alpha=RandomCharacterPresentation::fade(p.age)*(i<20?35:110)/255;
        const int size=int(uint64_t(256)*p.scale*15832249/(1024*1000));
        random_character_particle(c,dot,RandomCharacterPresentation::project_q16(p.x-107*1024,false),
            RandomCharacterPresentation::project_q16(p.y+1361*1024,true),size,-int(p.angle),alpha,128,179,69,presentation::Normal,RandomTextureSource::WhiteAlpha);
    }
    if(profile)profile(RuntimeRenderPart::RandomDots,false);
    return true;
}
