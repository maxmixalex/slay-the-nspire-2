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
static void actual(const char*captures){
    const int ids[]={I13_RandomGradient,I13_RandomNoise,I13_RandomLight,I13_RandomDot};const unsigned widths[]={64,10,256,256},heights[]={64,5,256,256};
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
        CHECK(random_character_background(canvas,clock,clock,91));
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
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
int main(int argc,char**argv){
    shader_contract();particle_contract();
    if(argc>1){auto data=read(argv[1]);std::vector<uint8_t> locale;SplitResourceSource split;
        if(argc>2){locale=read(argv[2]);CHECK(split.open(ResourceSource::memory(data.data(),data.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage(argc>3?std::atoi(argv[3]):1)));CHECK(app.init(split.source(),{}));}
        else CHECK(app.init(data.data(),data.size(),{}));
        actual(argc>4?argv[4]:nullptr);
    }
    std::printf("Random source water, complete particle emitters and unchanged canonical state: %u checks passed (%s)\n",checks,argc>1?"actual source textures":"independent synthetic");
}
