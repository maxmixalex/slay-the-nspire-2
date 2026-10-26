// A water pixel's complete time-dependent input is the shader's quantized
// noise value. Its UV, strength and immutable gradient determine the exact
// RGB565 result. Retain only the last result at each display coordinate;
// changing particles and light are still painted independently every frame.
struct RandomWaterCache {
    uint16_t colors[320*240]{};
    uint8_t noise[320*240]{};
    const Content*owner=nullptr;
    Picture gradient{};
    uint32_t generation=0;
    unsigned width=0,height=0,logical_width=0,logical_height=0;
    int strength=0;
    bool valid=false;
    bool bind(const Content&content,const water_reflection::Texture&texture,unsigned w,unsigned h,int s){
        const Picture p=texture.storage();
        if(owner!=&content||generation!=content.generation()||width!=w||height!=h||strength!=s||
           logical_width!=texture.width()||logical_height!=texture.height()||
           gradient.pixels!=p.pixels||gradient.alpha!=p.alpha||gradient.palette!=p.palette||
           gradient.w!=p.w||gradient.h!=p.h||gradient.format!=p.format){
            owner=&content;generation=content.generation();width=w;height=h;strength=s;
            logical_width=texture.width();logical_height=texture.height();gradient=p;valid=false;
        }
        return valid;
    }
};
static RandomWaterCache random_water_cache;
static_assert(sizeof(RandomWaterCache)<=226u*1024u,"Exact water memoization stays within226 KiB");

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

// The original light has a256x256 logical texture and a zero blue tint.
// Keep red and green in separate16-bit lanes, retaining both bilinear
// rounding steps. The palette variant decodes its256 possible colors once.
class RandomLightSampler {
    Picture picture_{};
    const uint8_t*red_=nullptr,*green_=nullptr;
    uint32_t palette_[256];
    bool enabled_=false;
    static uint32_t packed(uint16_t color){return (uint32_t(color&0xf800u)<<5)|((color>>5)&63u);}
    static uint32_t mix(uint32_t a,uint32_t b,unsigned f){
        if(a==b)return a;
        if(!((a^b)&0x001e003eu))return f<128?a:f>128?b:a|b;
        return (((a<<8)+(b-a)*f+0x00800080u)>>8)&0x001f003fu;
    }
    uint16_t dense(unsigned index)const{const auto*p=picture_.pixels+index*2;return uint16_t(p[0]|unsigned(p[1])<<8);}
public:
    RandomLightSampler(const water_reflection::Texture&texture,const uint8_t*red,const uint8_t*green,bool enable=true):
        picture_(texture.storage()),red_(red),green_(green){
        enabled_=enable&&texture.valid()&&texture.width()==256&&texture.height()==256&&picture_.format!=4;
        if(enabled_&&picture_.palette)for(unsigned i=0;i<256;++i)
            palette_[i]=packed(uint16_t(picture_.palette[i*2]|unsigned(picture_.palette[i*2+1])<<8));
    }
    explicit operator bool()const{return enabled_;}
    uint16_t contribution(int u,int v)const{
        // u,v are inside the source quad. The rare border samples retain
        // clamp-to-edge; interior texels need only two adjacent row offsets.
        const int x=u-128,y=v-128,ix=x>>8,iy=y>>8;
        unsigned left=unsigned(ix),right=left+1,top=unsigned(iy)*256,bottom=top+256;
        if(unsigned(ix)>=255){left=ix<0?0:255;right=ix<0?0:255;}
        if(unsigned(iy)>=255){top=iy<0?0:255*256;bottom=top;}
        const unsigned aa=top+left,bb=top+right,cc=bottom+left,dd=bottom+right;
        uint32_t a,b,c,d;
        if(picture_.palette){
            a=palette_[picture_.pixels[aa]];b=palette_[picture_.pixels[bb]];
            c=palette_[picture_.pixels[cc]];d=palette_[picture_.pixels[dd]];
            const uint32_t bound=a|b|c|d;
            if(!red_[bound>>16]&&!green_[bound&63])return 0;
        }else{
            const uint16_t ac=dense(aa),bc=dense(bb),ccolor=dense(cc),dc=dense(dd),bound=ac|bc|ccolor|dc;
            // Each channel of OR is a conservative upper bound for all four
            // texels. Monotone tint/alpha maps prove zero before any mix.
            if(!red_[bound>>11]&&!green_[(bound>>5)&63])return 0;
            a=packed(ac);b=packed(bc);c=packed(ccolor);d=packed(dc);
        }
        const unsigned fx=unsigned(x)&255,fy=unsigned(y)&255;
        const uint32_t color=mix(mix(a,b,fx),mix(c,d,fx),fy);
        return uint16_t((unsigned(red_[color>>16])<<11)|(unsigned(green_[color&63])<<5));
    }
};

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
    const RandomLightSampler light(texture,add_red,add_green,add_fast&&blue==0);
    const int du=int(int64_t(cosine)*65536*64/size_q16),dv=int(int64_t(sine)*65536*64/size_q16);
    const int dx=(left<<16)+32768-center_x_q16,dy=(top<<16)+32768-center_y_q16;
    RandomRowQuotient row_u((int64_t(dx)*cosine+int64_t(dy)*sine)*64,int64_t(sine)*65536*64,size_q16);
    RandomRowQuotient row_v((int64_t(dy)*cosine-int64_t(dx)*sine)*64,int64_t(cosine)*65536*64,size_q16);
    for(int y=top;y<bottom;++y,row_u.advance(),row_v.advance()){
        int u=32768+row_u.value(),v=32768+row_v.value();
        for(int x=left;x<right;++x,u+=du,v-=dv){
            if(u<0||v<0||u>=65536||v>=65536)continue;
            const unsigned at=y*c.pitch+x;
            if(white_fast){const unsigned amount=div255(texture.alpha_bilinear(u,v)*alpha);
                if(amount)c.pixels[at]=blend(c.pixels[at],dot_color,amount);continue;}
            if(add_fast){
                if(light){const uint16_t contribution=light.contribution(u,v);if(contribution){const uint16_t back=c.pixels[at];
                    c.pixels[at]=uint16_t((mn(31,(back>>11)+(contribution>>11))<<11)|
                        (mn(63,((back>>5)&63)+((contribution>>5)&63))<<5)|(back&31));}continue;}
                const uint16_t color=texture.opaque_bilinear(u,v),back=c.pixels[at];
                c.pixels[at]=uint16_t((mn(31,(back>>11)+add_red[color>>11])<<11)|
                    (mn(63,((back>>5)&63)+add_green[(color>>5)&63])<<5)|mn(31,(back&31)+add_blue[color&31]));continue;}
            const auto p=source==RandomTextureSource::WhiteAlpha?water_reflection::Texel{0xffff,texture.alpha_bilinear(u,v)}:
                source==RandomTextureSource::OpaqueColor?water_reflection::Texel{texture.opaque_bilinear(u,v),255}:texture.bilinear(u,v);
            if(p.alpha)layer.paint(y*c.pitch+x,p.color,div255(p.alpha*alpha),mode);
        }
    }
}
static void random_character_water(Canvas&c,const water_reflection::Texture&gradient,
        const water_reflection::Texture&noise,uint32_t clock,RandomWaterCache*cache=nullptr){
    const water_reflection::Parameters parameters(clock,23,100);
    // Pixel-centre UVs after both Control pivots and responsive scale. Keep
    // Q24 increments so truncation cannot accumulate down the 240px image.
    static constexpr int U0=5196007,UStep=26366,V0=1765572,VStep=56187;
    int u[320],shifts[256];uint8_t noise_rows[5][320];
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
    const unsigned width=unsigned(mx(0,mn(c.clip_w,320))),height=unsigned(mx(0,mn(c.clip_h,240)));
    const bool reuse=cache&&cache->bind(c.a.content,gradient,width,height,parameters.half_strength_q16);
    for(int y=0;y<c.clip_h&&y<240;++y){
        const int v=(V0+y*VStep)>>8,position=((v-parameters.pan_q16)&65535)*5-32768;
        const int iy=position>>16,top=iy<0?4:iy,bottom=iy+1>=5?0:iy+1;const unsigned fraction=unsigned(position&65535)>>8;
        const auto*a=noise_rows[top],*b=noise_rows[bottom];auto*out=c.pixels+y*c.pitch;
        for(int x=0;x<c.clip_w&&x<320;++x){
            const unsigned n=(unsigned(a[x])*(256-fraction)+unsigned(b[x])*fraction+128)>>8;
            const unsigned at=unsigned(y)*320+unsigned(x);uint16_t color;
            if(reuse&&cache->noise[at]==n)color=cache->colors[at];
            else{const int shift=shifts[n];color=gradient.opaque_bilinear(u[x]+shift,v+shift);
                if(cache){cache->noise[at]=uint8_t(n);cache->colors[at]=color;}}
            if(direct)out[x]=color;else c.paint(y*c.pitch+x,color);
        }
    }
    if(cache)cache->valid=true;
}
static bool random_character_background(Canvas&c,uint32_t clock,uint32_t elapsed,uint32_t seed){
    using namespace water_reflection;
    const Picture gp=c.a.content.image(c.a.content.ui_image(I13_RandomGradient));
    const Picture np=c.a.content.image(c.a.content.ui_image(I13_RandomNoise));
    const Picture lp=c.a.content.image(c.a.content.ui_image(I13_RandomLight));
    const Picture dp=c.a.content.image(c.a.content.ui_image(I13_RandomDot));
    if(gp.w!=64||gp.h!=64||np.w!=10||np.h!=5||lp.w!=320||lp.h!=205||dp.w!=320||dp.h!=205)return false;
    const Texture gradient(gp),noise(np),light(lp,256,256),dot(dp,256,256);
    random_character_water(c,gradient,noise,clock,&random_water_cache);
    auto&particles=c.a.character_select_presentation.random_background;particles.observe(elapsed,seed);
    // little_light_script.gd randomizes only integer preprocess time 0..5.
    const unsigned warm=RandomCharacterPresentation::hash(seed^0x51ba3f21u)%6;
    const unsigned phase=((elapsed+warm*1000)%3000)*4096/3000;
    const unsigned scale=RandomCharacterPresentation::light_scale(phase),gray=RandomCharacterPresentation::light_gray(phase);
    const int size=int(uint64_t(256*20)*scale*15832249/(4096*1000));
    random_character_particle(c,light,RandomCharacterPresentation::project_q16(1111*1024,false),
        RandomCharacterPresentation::project_q16(-83*1024,true),size,2023,
        102,171*gray/255,113*gray/255,0,presentation::Add,RandomTextureSource::OpaqueColor);
    for(unsigned i=0;i<RandomCharacterPresentation::Count;++i){const auto&p=particles.particles[i];
        const unsigned alpha=RandomCharacterPresentation::fade(p.age)*(i<20?35:110)/255;
        const int size=int(uint64_t(256)*p.scale*15832249/(1024*1000));
        random_character_particle(c,dot,RandomCharacterPresentation::project_q16(p.x-107*1024,false),
            RandomCharacterPresentation::project_q16(p.y+1361*1024,true),size,-int(p.angle),alpha,128,179,69,presentation::Normal,RandomTextureSource::WhiteAlpha);
    }
    return true;
}
