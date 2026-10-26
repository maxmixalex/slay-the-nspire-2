// Textured particles use the original images, source modulation and source
// emitter rotation. The source quad is clipped before any inverse sampling.
static void random_character_particle(Canvas&c,const water_reflection::Texture&texture,
        int center_x_q16,int center_y_q16,int size_q16,int angle,unsigned alpha,
        unsigned red,unsigned green,unsigned blue,unsigned mode,bool white_texture=false){
    if(size_q16<=0||!alpha)return;
    const int sine=CombatCardMotion::sine(angle),cosine=CombatCardMotion::sine(angle+1024);
    const int half=int(int64_t(size_q16)*((sine<0?-sine:sine)+(cosine<0?-cosine:cosine))/2048);
    const int left=mx(0,(center_x_q16-half)>>16),right=mn(c.clip_w,((center_x_q16+half+65535)>>16));
    const int top=mx(0,(center_y_q16-half)>>16),bottom=mn(c.clip_h,((center_y_q16+half+65535)>>16));
    if(left>=right||top>=bottom)return;
    Canvas layer=c;layer.tint_red=uint8_t(red);layer.tint_green=uint8_t(green);layer.tint_blue=uint8_t(blue);
    const int du=int(int64_t(cosine)*65536*64/size_q16),dv=int(int64_t(sine)*65536*64/size_q16);
    for(int y=top;y<bottom;++y){
        const int dx=(left<<16)+32768-center_x_q16,dy=(y<<16)+32768-center_y_q16;
        int u=32768+int((int64_t(dx)*cosine+int64_t(dy)*sine)*64/size_q16);
        int v=32768+int((int64_t(dy)*cosine-int64_t(dx)*sine)*64/size_q16);
        for(int x=left;x<right;++x,u+=du,v-=dv){
            if(u<0||v<0||u>=65536||v>=65536)continue;
            const auto p=white_texture?water_reflection::Texel{0xffff,texture.alpha_bilinear(u,v)}:texture.bilinear(u,v);
            if(p.alpha)layer.paint(y*c.pitch+x,p.color,div255(p.alpha*alpha),mode);
        }
    }
}
static void random_character_water(Canvas&c,const water_reflection::Texture&gradient,
        const water_reflection::Texture&noise,uint32_t clock){
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
    for(int y=0;y<c.clip_h&&y<240;++y){
        const int v=(V0+y*VStep)>>8,position=((v-parameters.pan_q16)&65535)*5-32768;
        const int iy=position>>16,top=iy<0?4:iy,bottom=iy+1>=5?0:iy+1;const unsigned fraction=unsigned(position&65535)>>8;
        const auto*a=noise_rows[top],*b=noise_rows[bottom];auto*out=c.pixels+y*c.pitch;
        for(int x=0;x<c.clip_w&&x<320;++x){
            const unsigned n=(unsigned(a[x])*(256-fraction)+unsigned(b[x])*fraction+128)>>8;
            const int shift=shifts[n];const uint16_t color=gradient.opaque_bilinear(u[x]+shift,v+shift);
            if(direct)out[x]=color;else c.paint(y*c.pitch+x,color);
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
    random_character_water(c,gradient,noise,clock);
    auto&particles=c.a.character_select_presentation.random_background;particles.observe(elapsed,seed);
    // little_light_script.gd randomizes only integer preprocess time 0..5.
    const unsigned warm=RandomCharacterPresentation::hash(seed^0x51ba3f21u)%6;
    const unsigned phase=((elapsed+warm*1000)%3000)*4096/3000;
    const unsigned scale=RandomCharacterPresentation::light_scale(phase),gray=RandomCharacterPresentation::light_gray(phase);
    const int size=int(uint64_t(256*20)*scale*15832249/(4096*1000));
    random_character_particle(c,light,RandomCharacterPresentation::project_q16(1111*1024,false),
        RandomCharacterPresentation::project_q16(-83*1024,true),size,2023,
        102,171*gray/255,113*gray/255,0,presentation::Add);
    for(unsigned i=0;i<RandomCharacterPresentation::Count;++i){const auto&p=particles.particles[i];
        const unsigned alpha=RandomCharacterPresentation::fade(p.age)*(i<20?35:110)/255;
        const int size=int(uint64_t(256)*p.scale*15832249/(1024*1000));
        random_character_particle(c,dot,RandomCharacterPresentation::project_q16(p.x-107*1024,false),
            RandomCharacterPresentation::project_q16(p.y+1361*1024,true),size,-int(p.angle),alpha,128,179,69,presentation::Normal,true);
    }
    return true;
}
