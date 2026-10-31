// Keep production helpers private, as in the existing image oracle gates.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

using namespace spire;
static App app;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Random UI13:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Color {double r,g,b,a;};
struct Image {unsigned w,h;std::vector<uint16_t> colors;std::vector<uint8_t> alpha;};
static Color color(uint16_t p,unsigned a){return {double(p>>11),double((p>>5)&63),double(p&31),double(a)};}
static Color mix(Color a,Color b,double f){return {a.r+(b.r-a.r)*f,a.g+(b.g-a.g)*f,a.b+(b.b-a.b)*f,a.a+(b.a-a.a)*f};}
static Color at(const Image&i,int x,int y,bool repeat){
    if(repeat){x=(x%int(i.w)+int(i.w))%int(i.w);y=(y%int(i.h)+int(i.h))%int(i.h);}
    else{x=x<0?0:x>=int(i.w)?int(i.w)-1:x;y=y<0?0:y>=int(i.h)?int(i.h)-1:y;}
    return color(i.colors[unsigned(y)*i.w+x],i.alpha[unsigned(y)*i.w+x]);
}
static Color filtered(const Image&i,double u,double v,bool repeat=false){
    const double x=u*i.w-.5,y=v*i.h-.5;const int ix=int(std::floor(x)),iy=int(std::floor(y));
    return mix(mix(at(i,ix,iy,repeat),at(i,ix+1,iy,repeat),x-ix),
               mix(at(i,ix,iy+1,repeat),at(i,ix+1,iy+1,repeat),x-ix),y-iy);
}
static bool near(water_reflection::Texel actual,Color expected,double tolerance=1.6){
    return std::fabs(double(actual.color>>11)-expected.r)<=tolerance&&
           std::fabs(double((actual.color>>5)&63)-expected.g)<=tolerance&&
           std::fabs(double(actual.color&31)-expected.b)<=tolerance&&
           std::fabs(double(actual.alpha)-expected.a)<=tolerance;
}
static Picture picture(const Image&i){return {reinterpret_cast<const uint8_t*>(i.colors.data()),i.alpha.data(),nullptr,uint16_t(i.w),uint16_t(i.h),1};}
static uint16_t exact_mix(uint16_t a,uint16_t b,unsigned f){
    const auto channel=[&](unsigned shift,unsigned mask){return ((((a>>shift)&mask)*(256-f)+((b>>shift)&mask)*f+128)>>8)<<shift;};
    return uint16_t(channel(11,31)|channel(5,63)|channel(0,31));
}
static void exact_row_quotient_contract(){
    for(int denominator:{1,7,65536,17*65536,512*65536,0x7fffffff}){
        const int64_t d=denominator;
        const int64_t origins[]={-(int64_t(1)<<47)+37,-d-1,-d,-1,0,1,d-1,d,d+1,(int64_t(1)<<47)-1};
        const int64_t changes[]={-(int64_t(1)<<32),-1,0,1,int64_t(1)<<32};
        for(int64_t origin:origins)for(int64_t delta:changes){
            RandomRowQuotient rows(origin,delta,denominator);
            for(unsigned row=0;row<=240;++row){
                CHECK(rows.value()==int((origin+int64_t(row)*delta)/denominator));rows.advance();
            }
        }
    }
}
static void exact_interpolation_contract(){
    Image source{2,2,{0,0,0,0},{255,255,255,255}};
    const water_reflection::Texture texture(picture(source));uint32_t seed=47219;
    const auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
    for(unsigned trial=0;trial<1000;++trial){
        for(auto&value:source.colors)value=uint16_t(next()>>8);
        if(trial%4==0)source.colors[1]=uint16_t(source.colors[0]^0x0821u);
        if(trial%4==1)source.colors[1]=source.colors[0];
        for(unsigned f:{0u,1u,127u,128u,129u,254u,255u})for(unsigned g:{0u,127u,128u,129u,255u}){
            const auto expected=exact_mix(exact_mix(source.colors[0],source.colors[1],f),exact_mix(source.colors[2],source.colors[3],f),g);
            CHECK(texture.opaque_bilinear(int(16384+f*128),int(16384+g*128))==expected);
        }
    }
}
static void exact_gradient_cache_contract(){
    Image gradient{64,64,{},{}},noise{10,5,{},{}},other{64,64,{},{}},small{7,5,{},{}},many{64,64,{},{}},vertical{64,64,{},{}};
    for(unsigned i=0;i<4096;++i){const unsigned value=(i/64+i%64)%32;
        gradient.colors.push_back(uint16_t(value*1879));gradient.alpha.push_back(255);
        other.colors.push_back(uint16_t(value*3271));other.alpha.push_back(255);
        many.colors.push_back(uint16_t(i*13+1));many.alpha.push_back(255);
        // Horizontal jumps may be large; neighboring rows change each RGB
        // channel by exactly one, in opposite directions, across bit carries.
        const unsigned x=i%64,t=(i/64)&1;
        vertical.colors.push_back(uint16_t(((6+x%20+t)<<11)|((10+x%42-t)<<5)|(4+x%24+t)));vertical.alpha.push_back(255);}
    for(unsigned i=0;i<35;++i){small.colors.push_back(uint16_t(i*1879));small.alpha.push_back(255);}
    for(unsigned i=0;i<50;++i){noise.colors.push_back(0xffff);noise.alpha.push_back(uint8_t(i*29));}
    water_reflection::Texture g(picture(gradient)),n(picture(noise)),g2(picture(other)),fallback(picture(small)),overflow(picture(many)),vertical_input(picture(vertical));
    static RandomGradientCache cache;Canvas c{app};
    const unsigned times[]={0,33,66,99,100,1000,1250,7500,3599999,3600000,0};
    for(unsigned mode=0;mode<5;++mode)for(unsigned clock:times){
        c.clip_w=mode==1?197:320;c.clip_h=mode==1?111:240;
        c.opacity=mode==2?173:255;c.tint_green=mode==2?127:255;
        const auto&input=mode==3?g2:mode==4?fallback:g;
        for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
        random_character_water(c,input,n,clock);
        const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
        for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
        random_character_water(c,input,n,clock,&cache);
        CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
        CHECK(cache.valid==(mode!=4));
    }
    // The complete general sampler is the independent oracle, including
    // both rounded interpolation stages, every border and indexed storage.
    uint8_t palette[512]{},indices[4096];
    for(unsigned i=0;i<256;++i){const uint16_t color=uint16_t(i*1879);palette[i*2]=uint8_t(color);palette[i*2+1]=uint8_t(color>>8);}
    for(unsigned i=0;i<4096;++i)indices[i]=uint8_t((i/64+i%64)%32);
    const water_reflection::Texture indexed({indices,nullptr,palette,64,64,2});
    uint32_t seed=47281;const auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
    int shifts[256];const int strength=water_reflection::Parameters(0,23,100).half_strength_q16;
    for(unsigned i=0;i<256;++i)shifts[i]=(int(i)-255)*strength/255;
    const water_reflection::Texture*inputs[]={&g,&g2,&indexed,&vertical_input};
    for(const auto*texture:inputs){
        CHECK(cache.bind(app.content,*texture));CHECK(cache.count&&cache.count<=RandomGradientCache::Capacity);
        for(int u:{-65536,-8192,0,1,511,512,513,32768,65023,65024,65535,65536,73728})
            for(int v:{-65536,-8192,0,1,511,512,513,32768,65023,65024,65535,65536,73728})
                CHECK(cache.sample(u,v)==texture->opaque_bilinear(u,v));
        for(unsigned cell:{0u,1u,7u,15u,31u,62u})for(unsigned fx:{0u,127u,128u,129u,255u})for(unsigned fy:{0u,127u,128u,129u,255u}){
            const int u=int(cell*1024+512+fx*4),v=int(((cell*3+5)%63)*1024+512+fy*4);
            CHECK(cache.sample(u,v)==texture->opaque_bilinear(u,v));
        }
        for(unsigned i=0;i<10000;++i){const int u=int(next()%98304)-16384,v=int(next()%98304)-16384;
            CHECK(cache.sample(u,v)==texture->opaque_bilinear(u,v));}
        // Repeated keys retain a LUT pointer, never its already sampled
        // color. Sweep changing horizontal fractions, tile crossings,
        // clamped columns and vertical half ties against the general filter.
        for(int v:{-8192,0,512,1024,32768,65023,65024,73728}){
            RandomGradientRow row(&cache,v,shifts);
            for(unsigned block=0;block<66;++block)for(unsigned step=0;step<16;++step){
                const unsigned n=block%4?((block*29)&255):255;
                const int u=int(block)*1024+int(step)*64-1024,shift=shifts[n];
                CHECK(row.sample(u,n)==texture->opaque_bilinear(u+shift,v+shift));
            }
        }
    }
    CHECK(cache.vertical_unit[0]&1); // The vertical-only fast path was exercised.
    // A source with too many unique pairs must keep all pixels via the
    // general sampler, and repeated visits must retain that bounded result.
    CHECK(!cache.bind(app.content,overflow)&&!cache.valid&&cache.count==RandomGradientCache::Capacity);
    CHECK(!cache.bind(app.content,overflow));
    random_character_water(c,overflow,n,7500);
    const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
    random_character_water(c,overflow,n,7500,&cache);
    CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
    // Reopening even the same Content invalidates otherwise identical
    // pointers. The new resource generation can publish different texels.
    static Content other_owner;
    CHECK(cache.bind(other_owner,g));const uint16_t before=cache.sample(0,0);gradient.colors[0]^=0x1234;
    CHECK(!other_owner.open(nullptr,0));CHECK(cache.bind(other_owner,g));
    CHECK(cache.sample(0,0)==gradient.colors[0]&&cache.sample(0,0)!=before);
    CHECK(cache.bind(app.content,g2));CHECK(cache.sample(0,0)==g2.opaque_bilinear(0,0));
}
static void shader_contract(){
    Image image{7,5,{},{}},noise{10,5,{},{}},mask{1,1,{0xffff},{25}};
    for(unsigned n=0;n<35;++n){image.colors.push_back(uint16_t(((n*3%32)<<11)|((n*11%64)<<5)|(n*7%32)));image.alpha.push_back(255);}
    for(unsigned n=0;n<50;++n){noise.colors.push_back(0xffff);noise.alpha.push_back(uint8_t(n*29));}
    const water_reflection::Texture source(picture(image)),waves(picture(noise));
    const unsigned clocks[]={0,1000,7500,3599999,3600000};
    for(unsigned clock:clocks)for(unsigned mode=0;mode<2;++mode){
        const double speed=mode?.388:.023,strength=mode?.058:.1;
        const water_reflection::Parameters p(clock,mode?388:23,mode?58:100);
        for(int y=-2;y<=17;++y)for(int x=-2;x<=19;++x){
            const double u=x/17.,v=y/15.,phase=(clock%3600000)*.001*speed;
            const double shift=(filtered(noise,u-phase,v-phase,true).a/255.-1)*strength*.5;
            CHECK(near(water_reflection::sample(source,waves,int(u*65536),int(v*65536),p),filtered(image,u+shift,v+shift),2.0));
        }
    }
    for(unsigned a:{25u,26u}){mask.alpha[0]=uint8_t(a);const water_reflection::Texture m(picture(mask));
        const water_reflection::Parameters p(1000,388,58);
        const auto actual=water_reflection::sample(source,waves,32768,32768,p,&m);
        const double shift=(filtered(noise,.5-.388,.5-.388,true).a/255.-1)*.029;
        CHECK(near(actual,filtered(image,.5+(a<=25?0:shift),.5+(a<=25?0:shift))));
    }
    // Sparse rows include absent pixels and both implicit and explicit alpha.
    const uint8_t sparse[]={1,0, 1,0,2,0,0, 0,248,224,7, 1,0, 0,0,2,0,1, 31,0,64,255,255,128};
    const water_reflection::Texture compact({sparse,nullptr,nullptr,3,2,4});
    CHECK(compact.at(0,0).alpha==0&&compact.at(1,0).color==0xf800&&compact.at(2,0).color==0x07e0);
    CHECK(compact.at(0,1).color==0x001f&&compact.at(0,1).alpha==64&&compact.at(1,1).alpha==128&&compact.at(2,1).alpha==0);
    CHECK(water_reflection::Parameters(0,23,100).pan_q16==water_reflection::Parameters(3600000,23,100).pan_q16);
}
static void particle_contract(){
    RandomCharacterPresentation p;p.observe(0,91);CHECK(sizeof(p)<2048);
    for(unsigned i=0;i<40;++i){const auto&v=p.particles[i];CHECK(v.age==(15000-(i%20)*750)%15000);
        CHECK(v.radial<=1024&&v.orbit<=12353&&v.angle<=4023&&v.scale>0);
        CHECK(i<20?v.scale==1024:v.scale>=102&&v.scale<=307);}
    const auto before=p;p.observe(0,91);CHECK(!std::memcmp(&before,&p,sizeof p));
    p.observe(750,91);CHECK(p.particles[1].age==0&&p.particles[1].cycle==1);
    // Independent source one-step force/orbit order, in floating point.
    RandomCharacterPresentation::Particle q{};q.x=100*1024;q.y=-50*1024;q.radial=768;q.orbit=9000;
    const double length=std::sqrt(100.*100.+50.*50.);const double vx=(20+100/length*.75)*.033,vy=(-10-50/length*.75)*.033;
    const double angle=(9000./65536)*.033;
    const double expected_x=100*std::cos(angle)-50*std::sin(angle)+vx*.033;
    const double expected_y=-50*std::cos(angle)-100*std::sin(angle)+vy*.033;
    RandomCharacterPresentation::step(q,33);
    CHECK(std::fabs(q.x/1024.-expected_x)<.01&&std::fabs(q.y/1024.-expected_y)<.01);
    CHECK(RandomCharacterPresentation::fade(0)==0&&RandomCharacterPresentation::fade(2675)==255&&RandomCharacterPresentation::fade(14999)==255);
    CHECK(RandomCharacterPresentation::light_scale(2058)>RandomCharacterPresentation::light_scale(0));
    CHECK(RandomCharacterPresentation::light_gray(2488)==255&&RandomCharacterPresentation::light_gray(4073)==109);
    // Source anchored matrix, independently expressed in desktop coordinates.
    CHECK(std::fabs(RandomCharacterPresentation::project_q16(1111*1024,false)/65536.-177.3937714)<.001);
    CHECK(std::fabs(RandomCharacterPresentation::project_q16(-83*1024,true)/65536.+25.125)<.001);
}
static void linear_texture_contract(){
    // A logical row crosses the physical row boundary. RGB565, indexed and
    // sparse views must all retain flat source order and source UV filtering.
    Image physical{3,2,{0x1234,0xabcd,0x4567,0x89ab,0xcdef,0x1357},{17,61,101,151,201,255}};
    const Image logical{2,3,physical.colors,physical.alpha};
    uint8_t palette[512]{},indices[]={0,1,2,3,4,5};
    for(unsigned i=0;i<6;++i){palette[i*2]=uint8_t(physical.colors[i]);palette[i*2+1]=uint8_t(physical.colors[i]>>8);}
    const water_reflection::Texture dense(picture(physical),2,3),indexed({indices,physical.alpha.data(),palette,3,2,3},2,3);
    const uint8_t spans[]={1,0, 1,0,2,0,0, 0,248,224,7, 1,0, 0,0,2,0,1, 31,0,64,255,255,128};
    const water_reflection::Texture sparse({spans,nullptr,nullptr,3,2,4},2,3);
    const Image sparse_logical{2,3,{0,0xf800,0x07e0,0x001f,0xffff,0},{0,255,255,64,128,0}};
    CHECK(dense.valid()&&dense.width()==2&&dense.height()==3&&indexed.valid()&&sparse.valid());
    for(bool repeat:{false,true})for(int y=-1;y<=3;++y)for(int x=-1;x<=2;++x){
        CHECK(near(dense.at(x,y,repeat),at(logical,x,y,repeat),0));
        CHECK(near(indexed.at(x,y,repeat),at(logical,x,y,repeat),0));
        CHECK(near(sparse.at(x,y,repeat),at(sparse_logical,x,y,repeat),0));
    }
    for(bool repeat:{false,true})for(int v:{-8192,0,8192,32768,61440,65536,73728})for(int u:{-8192,0,8192,32768,61440,65536,73728}){
        const Color expected=filtered(logical,u/65536.,v/65536.,repeat);
        CHECK(near(dense.bilinear(u,v,repeat),expected));
        CHECK(near(indexed.bilinear(u,v,repeat),expected));
        CHECK(std::fabs(dense.alpha_bilinear(u,v,repeat)-expected.a)<=1.6);
        CHECK(std::fabs(indexed.alpha_bilinear(u,v,repeat)-expected.a)<=1.6);
        CHECK(near(sparse.bilinear(u,v,repeat),filtered(sparse_logical,u/65536.,v/65536.,repeat)));
    }
    CHECK(!water_reflection::Texture(picture(physical),3,3).valid());
    CHECK(!water_reflection::Texture(picture(physical),2,0).valid());
}
static void particle_fast_paths(){
    // Compare the specialized source-white dot and opaque additive light
    // against ordinary RGBA compositing, including rotated/clipped quads and
    // parent modulation which must retain the general painter path.
    Image light{7,5,{},{}},dot{7,5,{},{}};
    for(unsigned i=0;i<35;++i){light.colors.push_back(uint16_t(((i*13%32)<<11)|((i*9%64)<<5)|(i*7%32)));light.alpha.push_back(255);
        dot.colors.push_back(0xffff);dot.alpha.push_back(uint8_t(i*29));}
    for(unsigned kind=0;kind<2;++kind)for(int angle:{-735,0,942})for(unsigned modulation:{0u,1u,2u}){
        const water_reflection::Texture texture(picture(kind?light:dot));Canvas c{app};
        if(modulation==1)c.opacity=179;if(modulation==2)c.brightness=201;
        const auto render=[&](RandomTextureSource source){for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
            random_character_particle(c,texture,20*65536,22*65536,84*65536,angle,kind?102:110,128,179,69,
                kind?presentation::Add:presentation::Normal,source);};
        render(RandomTextureSource::Color);const std::vector<uint16_t> reference(app.pixels,app.pixels+320*240);
        render(kind?RandomTextureSource::OpaqueColor:RandomTextureSource::WhiteAlpha);
        CHECK(!std::memcmp(reference.data(),app.pixels,reference.size()*sizeof(uint16_t)));
    }
}
static void interior_raster_contract(){
    // Compare tile-span endpoints with independent integer division and
    // direct traversal, including both directions, zero and a step wider
    // than the complete texture. No sampled pixel may cross a proof tile.
    for(int delta:{-2147483647,-65536,-1025,-1024,-1023,-257,-33,-1,0,1,33,257,1023,1024,1025,65536,2147483647}){
        const RandomLightTileStep spans(delta);
        for(int coordinate=128;coordinate<65408;coordinate+=17){
            const unsigned count=spans.count(coordinate);
            if(!delta){CHECK(count==~0u);continue;}
            const int64_t tile=(coordinate-128)/1024;
            const int64_t edge=delta>0?(tile+1)*1024+127:tile*1024+128;
            const unsigned expected=unsigned(std::abs(edge-coordinate)/std::abs(int64_t(delta)))+1;
            CHECK(count==expected);
            for(unsigned pixel=0;pixel<count;++pixel)
                CHECK((int64_t(coordinate)-128+int64_t(delta)*pixel)/1024==tile);
            const int64_t next=int64_t(coordinate)-128+int64_t(delta)*count;
            CHECK(next<tile*1024||next>=(tile+1)*1024);
        }
    }
    // Independently visit every UV in admitted/rejected spans, including
    // descending rows, single-pixel rows and both source half-texel edges.
    for(int u:{0,127,128,129,32768,65407,65408,65535})
        for(int v:{0,127,128,129,32768,65407,65408,65535})
        for(int du:{-257,-1,0,1,257})for(int dv:{-257,-1,0,1,257})for(unsigned count:{0u,1u,2u,17u,320u}){
            bool safe=count!=0;unsigned first=count,last=count;
            for(unsigned i=0;i<count;++i){const int64_t x=int64_t(u)+int64_t(du)*i,y=int64_t(v)+int64_t(dv)*i;
                const bool inside=x>=128&&x<65408&&y>=128&&y<65408;safe=safe&&inside;
                if(inside){if(first==count)first=i;last=i+1;}}
            CHECK(RandomLightSampler::interior_span(u,v,du,dv,count)==safe);
            const auto range=random_interior_range(u,v,du,dv,count);CHECK(range.first==first&&range.last==last);
        }
    CHECK(!RandomLightSampler::interior_span(32768,32768,0x7fffffff,0,320));
    CHECK(!RandomLightSampler::interior_span(32768,32768,0,-0x7fffffff,320));
    const auto partial=random_interior_range(-1000,32768,1000,0,100);
    const auto reverse=random_interior_range(66000,32768,-1000,0,100);
    CHECK(partial.first==2&&partial.last==67&&reverse.first==1&&reverse.last==66);
    Image physical{320,205,std::vector<uint16_t>(320*205,0xffff),std::vector<uint8_t>(320*205)};
    for(unsigned i=0;i<physical.alpha.size();++i)physical.alpha[i]=uint8_t((i*17)^(i/256*31));
    const water_reflection::Texture dot(picture(physical),256,256);
    for(int u:{128,129,255,256,257,32768,65406,65407})for(int v:{128,129,255,256,257,32768,65406,65407})
        CHECK(random_dot_alpha(physical.alpha.data(),u,v)==dot.alpha_bilinear(u,v));
    uint32_t seed=4891;
    for(unsigned i=0;i<4000;++i){seed=seed*1664525u+1013904223u;const int u=128+int(seed%65280);
        seed=seed*1664525u+1013904223u;const int v=128+int(seed%65280);
        CHECK(random_dot_alpha(physical.alpha.data(),u,v)==dot.alpha_bilinear(u,v));}
    // Particle opacity may make even a nonzero source quad invisible. Verify
    // every opacity's exact rounding boundary against the general sampler;
    // one corner above the limit must never be discarded speculatively.
    for(unsigned opacity=1;opacity<=255;++opacity){
        const unsigned threshold=127/opacity;
        for(unsigned mask=0;mask<16;++mask){
            for(unsigned corner=0;corner<4;++corner)
                physical.alpha[(corner/2)*256+corner%2]=uint8_t(threshold+((mask>>corner)&1));
            for(unsigned fx:{0u,1u,127u,128u,129u,254u,255u})for(unsigned fy:{0u,1u,127u,128u,129u,254u,255u}){
                const int u=int(128+fx),v=int(128+fy);
                CHECK(div255(random_dot_alpha(physical.alpha.data(),u,v,threshold)*opacity)==
                      div255(dot.alpha_bilinear(u,v)*opacity));
            }
        }
    }
    for(unsigned i=0;i<4000;++i){seed=seed*1664525u+1013904223u;const int u=128+int(seed%65280);
        seed=seed*1664525u+1013904223u;const int v=128+int(seed%65280);
        const unsigned opacity=1+(seed>>16)%255;
        CHECK(div255(random_dot_alpha(physical.alpha.data(),u,v,127/opacity)*opacity)==
              div255(dot.alpha_bilinear(u,v)*opacity));}
    Canvas transparent{app};for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
    const std::vector<uint16_t> untouched(app.pixels,app.pixels+320*240);
    random_character_white_dot(transparent,dot,0,0,320,240,256,0,
        RandomRowQuotient(0,0,1),RandomRowQuotient(0,0,1),0,0xffff);
    CHECK(!std::memcmp(untouched.data(),app.pixels,untouched.size()*sizeof(uint16_t)));
    for(int angle:{-735,0,2023})for(int size:{84,500})for(int center:{20,177})for(unsigned clipping:{0u,1u}){
        Canvas c{app};if(clipping){c.clip_w=197;c.clip_h=111;}
        const auto render=[&](RandomTextureSource source){for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
            random_character_particle(c,dot,center*65536,-25*65536,size*65536,angle,110,128,179,69,presentation::Normal,source);};
        render(RandomTextureSource::Color);const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
        render(RandomTextureSource::WhiteAlpha);CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
    }
}
static void light_rg_contract(){
    Image physical{320,205,std::vector<uint16_t>(320*205),std::vector<uint8_t>(320*205,255)};
    Image smooth{320,205,std::vector<uint16_t>(320*205),std::vector<uint8_t>(320*205,255)};
    std::vector<uint8_t> indices(320*205),palette(512);uint32_t seed=91821;
    const auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
    for(unsigned i=0;i<256;++i){const uint16_t color=uint16_t(i*7919);palette[i*2]=uint8_t(color);palette[i*2+1]=uint8_t(color>>8);}
    for(unsigned i=0;i<physical.colors.size();++i){physical.colors[i]=i<4096?0:i<8192?31:uint16_t(next()>>8);indices[i]=uint8_t(next()>>16);
        const unsigned x=i%256,y=i/256;
        // Unit steps cross RGB bit carries as well as deliberate wrap edges.
        // Blue is unrelated and must remain irrelevant to the zero-blue tint.
        smooth.colors[i]=uint16_t((((x/4+y/8)%32)<<11)|(((x/2+y/4)%64)<<5)|((x*13+y*7)&31));}
    for(unsigned layout=0;layout<3;++layout){
        const Picture storage=layout==1?Picture{indices.data(),nullptr,palette.data(),320,205,3}:picture(layout==2?smooth:physical);
        const water_reflection::Texture texture(storage,256,256);
        CHECK(random_light_quads().bind(app.content,texture));
        for(unsigned alpha:{1u,102u,255u})for(unsigned tint:{0u,90u,171u,255u}){
            uint8_t red[32],green[64];for(unsigned i=0;i<64;++i){green[i]=uint8_t(div255(div255(i*tint)*alpha));if(i<32)red[i]=uint8_t(div255(div255(i*tint)*alpha));}
            std::vector<uint16_t> table(4096);
            const RandomLightSampler fallback(texture,red,green,true,random_light_quads().safe,random_light_quads().maximum,random_light_quads().minimum);
            const RandomLightSampler light(texture,red,green,true,random_light_quads().safe,random_light_quads().maximum,random_light_quads().minimum,table.data());CHECK(bool(light));
            const auto sample=[&](int u,int v){const uint16_t color=texture.opaque_bilinear(u,v);
                const uint16_t expected=uint16_t((unsigned(red[color>>11])<<11)|(unsigned(green[(color>>5)&63])<<5));
                CHECK(light.contribution(u,v)==expected);
                CHECK(fallback.contribution(u,v)==expected);
                if(u>=128&&u<65408&&v>=128&&v<65408)
                    CHECK((layout==1?light.interior<true>(u,v):light.interior<false>(u,v))==expected);};
            for(int u:{0,1,127,128,129,255,256,257,65407,65408,65534,65535})
                for(int v:{0,1,127,128,129,255,256,257,65407,65408,65534,65535})sample(u,v);
            for(unsigned cell:{0u,1u,7u,31u,63u,127u,254u})for(unsigned fx:{0u,127u,128u,129u,255u})for(unsigned fy:{0u,127u,128u,129u,255u})
                sample(mn(65535,int(cell*256+128+fx)),mn(65535,int(((cell*3+5)&255)*256+128+fy)));
            for(unsigned i=0;i<1000;++i)sample(int(next()&65535),int(next()&65535));
        }
        Canvas canvas{app};
        for(int angle:{-735,0,2023})for(int size:{84,500})for(int center:{20,177}){
            const auto render=[&](RandomTextureSource source){for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t(i*37);
                random_character_particle(canvas,texture,center*65536,-25*65536,size*65536,angle,102,171,113,0,presentation::Add,source);};
            render(RandomTextureSource::Color);const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
            render(RandomTextureSource::OpaqueColor);CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
        }
    }
    // A changed generation at the same resource/pixel addresses must replace
    // a formerly safe quad when its contents acquire a larger channel jump.
    const water_reflection::Texture generation_input(picture(physical),256,256);
    CHECK(random_light_quads().bind(app.content,generation_input));CHECK(random_light_quads().safe[0]&1);
    CHECK(random_light_quads().maximum[0]==0);
    physical.colors[0]=0xffe0;CHECK(!app.content.open(nullptr,0));
    CHECK(random_light_quads().bind(app.content,generation_input));CHECK(!(random_light_quads().safe[0]&1));
    CHECK(random_light_quads().maximum[0]==63);
    // The first 4x4 tile bounds its quads, including source texel (4,4)
    // beyond that tile's origin cells. Omitting this halo can skip real light.
    physical.colors[0]=0;physical.colors[4*256+4]=0xffe0;CHECK(!app.content.open(nullptr,0));
    CHECK(random_light_quads().bind(app.content,generation_input));CHECK(random_light_quads().maximum[0]==63);
    CHECK(random_light_quads().maximum[2]==0);
}
static std::vector<uint8_t> read(const char*path){FILE*f=std::fopen(path,"rb");CHECK(f);std::vector<uint8_t> data(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));return data;}
static Image decode(Picture p){
    Image out{p.w,p.h,std::vector<uint16_t>(unsigned(p.w)*p.h),std::vector<uint8_t>(unsigned(p.w)*p.h)};
    Canvas c{app,out.colors.data(),p.w,p.w,p.h,out.alpha.data()};c.picture(p,0,0);return out;
}
static void capture(const char*directory,const char*name){
    if(!directory)return;char path[1024];std::snprintf(path,sizeof path,"%s/%s.ppm",directory,name);FILE*f=std::fopen(path,"wb");CHECK(f);
    CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);
    for(unsigned y=0;y<240;++y){uint8_t row[960];for(unsigned x=0;x<320;++x){const uint16_t p=app.pixels[y*320+x];row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static unsigned profile_count,profile_parts[6];
static void render_part_probe(RuntimeRenderPart part,bool begin){
    CHECK(profile_count<6);profile_parts[profile_count++]=unsigned(part)*2+(begin?0:1);
}
static void actual(const char*captures){
    const int ids[]={I13_RandomGradient,I13_RandomNoise,I13_RandomLight,I13_RandomDot};const unsigned widths[]={64,10,320,320},heights[]={64,5,205,205};
    for(unsigned i=0;i<4;++i){const auto p=app.content.image(app.content.ui_image(ids[i]));CHECK(p.pixels&&p.w==widths[i]&&p.h==heights[i]);}
    const auto gp=app.content.image(app.content.ui_image(I13_RandomGradient)),np=app.content.image(app.content.ui_image(I13_RandomNoise));
    const Image gradient=decode(gp),noise=decode(np);const water_reflection::Texture g(gp),n(np);
    for(unsigned value:gradient.alpha)CHECK(value==255);
    new_run(app.game,91919,true);SaveData before,after;CHECK(save(app.game,before));Canvas canvas{app};
    std::vector<uint16_t> previous;const unsigned times[]={0,1250,7500,16000};
    for(unsigned clock:times){
        random_character_water(canvas,g,n,clock);
        for(int y=0;y<240;++y)for(int x=0;x<320;++x){
            const double scale=1.2683*4/21;
            const double u=(x+.5+196.573542857)/(2634*scale),v=(y+.5+30.922933333)/(1236*scale);
            const double shift=(filtered(noise,u-clock*.000023,v-clock*.000023,true).a/255.-1)*.05;
            CHECK(near({app.pixels[y*320+x],255},filtered(gradient,u+shift,v+shift),1.6));
        }
        const auto water=std::vector<uint16_t>(app.pixels,app.pixels+320*240);
        random_character_water(canvas,g,n,clock,&random_gradient_cache());
        CHECK(random_gradient_cache().valid&&random_gradient_cache().count<=RandomGradientCache::Capacity);
        CHECK(!std::memcmp(water.data(),app.pixels,water.size()*sizeof(uint16_t)));
        profile_count=0;app.callbacks.profile_render=render_part_probe;
        CHECK(random_character_background(canvas,clock,clock,91));
        app.callbacks.profile_render=nullptr;CHECK(profile_count==6);
        for(unsigned i=0;i<6;++i)CHECK(profile_parts[i]==i);
        unsigned decoration=0;for(unsigned i=0;i<water.size();++i)decoration+=water[i]!=app.pixels[i];CHECK(decoration>1000);
        if(!previous.empty()){unsigned changed=0;for(unsigned i=0;i<previous.size();++i)changed+=previous[i]!=app.pixels[i];CHECK(changed>1000);}
        previous.assign(app.pixels,app.pixels+320*240);
        CHECK(random_character_background(canvas,clock,clock,91));CHECK(!std::memcmp(previous.data(),app.pixels,previous.size()*2));
        char name[80];std::snprintf(name,sizeof name,"random-background-%u",clock);capture(captures,name);
        CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    }
    app.title=app.character_select=app.character_random=true;app.selected_character=Character::Ironclad;
    app.progress.tutorial_flags=FtueFresh|FtueSeenMask;app.map_clock_ms=24000;app.render();
    capture(captures,"random-screen-24000");CHECK(app.character_random);
    app.map_clock_ms+=600;app.render();capture(captures,"random-screen-settled-24600");
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
int main(int argc,char**argv){
    shader_contract();exact_row_quotient_contract();exact_interpolation_contract();exact_gradient_cache_contract();particle_contract();linear_texture_contract();particle_fast_paths();interior_raster_contract();light_rg_contract();
    if(argc>1){auto data=read(argv[1]);std::vector<uint8_t> locale;SplitResourceSource split;
        if(argc>2){locale=read(argv[2]);CHECK(split.open(ResourceSource::memory(data.data(),data.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage(argc>3?std::atoi(argv[3]):1)));CHECK(app.init(split.source(),{}));}
        else CHECK(app.init(data.data(),data.size(),{}));
        actual(argc>4?argv[4]:nullptr);
    }
    std::printf("Random source water, complete particle emitters and unchanged canonical state: %u checks passed (%s)\n",checks,argc>1?"actual source textures":"independent synthetic");
}
