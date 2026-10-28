// Optional actual-pack checks exercise the production renderer without a
// public debug drawing API. No source artwork is embedded in this test.
#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static State before;
static unsigned checks;
static const char* output;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"character death:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Frame=std::vector<uint16_t>;
static const char* names[]={"ironclad","silent","defect","regent","necrobinder"};
static void transition_equation(){
    // Independent rearrangement of the original GLSL equation, before its
    // scalar cut-off optimization. Include every representable half-red tap
    // and every source-clock phase, not just the all-clear/all-covered ends.
    for(unsigned phase=0;phase<=4096;++phase)for(unsigned sample=0;sample<=510;++sample){
        const bool expected=int64_t(sample)*10*4096>=int64_t(11*4096-int(12*phase))*510;
        CHECK(game_over_transition::covered(sample,phase)==expected);
    }
    GameOverPresentation m;m.bound=true;m.opened=1000;
    CHECK(m.black_alpha(1100)==0&&m.backstop_phase(1000)==0&&m.backstop_phase(2500)==4096);
    m.event_room=true;CHECK(m.black_alpha(1000)==0&&m.black_alpha(1200)==255&&m.backstop_phase(1200)==0&&m.backstop_phase(2700)==4096);
}
static void transition_pixels(){
    if(app.content.version()<GameOverTransitionContentVersion)return;
    const auto mask=app.content.image(app.content.ui_image(I9_GameOverTransition));
    CHECK(mask.pixels&&mask.format==0&&mask.w==320&&mask.h==240);
    Frame expected(320*240);Canvas reference{app};reference.pixels=expected.data();
    for(unsigned phase:{0u,341u,342u,512u,1024u,2048u,3072u,3754u,4096u})for(unsigned opacity:{0u,137u,255u}){
        for(unsigned i=0;i<320*240;++i)app.pixels[i]=expected[i]=uint16_t((i*3917u)^(i>>2)^0x8410);
        Canvas c{app};c.opacity=reference.opacity=uint8_t(opacity);before=app.game;
        for(unsigned i=0;i<320*240;++i){const unsigned scalar=unsigned(mask.pixels[2*i])|(unsigned(mask.pixels[2*i+1])<<8);
            CHECK(scalar<=510);
            if(int64_t(scalar)*10*4096>=int64_t(11*4096-int(12*phase))*510)reference.paint(int(i),rgb(31,0,11),230);
        }
        game_over_shader_backstop(c,phase);CHECK(!std::memcmp(app.pixels,expected.data(),320*240*2));CHECK(!std::memcmp(&before,&app.game,sizeof before));
    }
}
static void capture(const char*name){
    if(!output)return;char path[1024];const int n=std::snprintf(path,sizeof path,"%s/%s.ppm",output,name);CHECK(n>0&&unsigned(n)<sizeof path);
    FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);
    uint8_t row[320*3];for(unsigned y=0;y<240;++y){for(unsigned x=0;x<320;++x){const auto p=app.pixels[y*320+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static Frame actor(unsigned ms,unsigned opacity=255){
    before=app.game;Canvas c{app};c.rect(0,0,320,240,0);game_over_actor(c,ms,opacity);
    CHECK(!std::memcmp(&before,&app.game,sizeof before));return Frame(app.pixels,app.pixels+320*240);
}
static void setup(unsigned character,WorldRoom room){
    new_run(app.game,0x41cef1ea,false,Character(character));enter_combat(app.game);CHECK(encounter_setup(app.game,EncounterId::CorpseSlugsNormal));
    app.game.world.active=1;app.game.world.room_type=unsigned(room);app.game.hp=0;app.game.phase=Phase::Dead;app.game.world.finished=1;
    app.title=app.paused=app.help=app.library=app.character_select=false;app.reset_presentation();app.pointer.visible=false;
}
static void actual(const char*path){
    FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>data(size);CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));
    CHECK(app.init(data.data(),data.size(),{}));CHECK(app.content.version()>=CharacterDeathContentVersion&&app.content.current_profile());
    transition_pixels();
    const WorldRoom rooms[]={WorldRoom::Monster,WorldRoom::Event,WorldRoom::Rest,WorldRoom::Shop};const char*room_names[]={"combat","event","rest","merchant"};
    for(unsigned character=0;character<5;++character){
        for(unsigned merchant=0;merchant<2;++merchant){const auto clip=app.content.clip(int((merchant?MerchantDeathAnimationFirst:CharacterDeathAnimationFirst)+character));CHECK(clip.count==CharacterDeathFrameCount[character]&&clip.sprite_units>0);CHECK(app.content.image(clip.first).pixels&&app.content.image(clip.first+clip.count-1).pixels);}
        for(unsigned room=0;room<4;++room){
            setup(character,rooms[room]);const auto first=actor(0);CHECK(std::any_of(first.begin(),first.end(),[](uint16_t p){return p!=0;}));
            const auto middle=actor(CharacterDeathDurationMs[character]/2);CHECK(first!=middle);
            char name[128];std::snprintf(name,sizeof name,"%s-%s-mid",room_names[room],names[character]);if(room==1)capture(name);
            const auto last=actor(6000);CHECK(last==actor(100000));
            const auto hidden=actor(500,0);CHECK(std::all_of(hidden.begin(),hidden.end(),[](uint16_t p){return p==0;}));
            actor(6000);std::snprintf(name,sizeof name,"%s-%s-terminal",room_names[room],names[character]);capture(name);
            if(room==1){actor(0);std::snprintf(name,sizeof name,"event-%s-first",names[character]);capture(name);}
        }
    }
    for(unsigned index:{DefectDeathParticles,RegentDeathParticles}){
        const auto clip=app.content.clip(int(index));CHECK(clip.count==(index==DefectDeathParticles?68:151)&&clip.sprite_units>0);
        Canvas c{app};c.rect(0,0,320,240,0);const auto image=app.content.image(clip.first+clip.count-1);c.picture(image,0,0,image.w,image.h);
        CHECK(std::all_of(std::begin(app.pixels),std::end(app.pixels),[](uint16_t p){return p==0;}));
    }
    setup(unsigned(Character::Regent),WorldRoom::Event);actor(2500);capture("event-regent-explosion");
    setup(unsigned(Character::Ironclad),WorldRoom::Monster);app.game.phase=Phase::Combat;app.game.hp=50;app.game.world.finished=0;app.show_run_timer=true;app.progress.current.seconds=3723;CHECK(app.run_timer_visible());
    Canvas c{app};c.rect(0,0,320,240,0);hud(c);const auto timer=app.content.image(app.content.ui_image(I9_RunTimerIcon));CHECK(timer.pixels&&timer.w<=8&&timer.h<=15);capture("original-run-timer");
}
int main(int argc,char**argv){
    transition_equation();
    for(unsigned character=0;character<5;++character){const unsigned duration=CharacterDeathDurationMs[character],last=CharacterDeathFrameCount[character]-1;CHECK(character_death_frame(character,0)==0);CHECK(character_death_frame(character,duration-1)<last);CHECK(character_death_frame(character,duration)==last);CHECK(character_death_frame(character,UINT32_MAX)==last);}
    if(argc>2)output=argv[2];if(argc>1)actual(argv[1]);
    std::printf("character death: %u checks; %s\n",checks,argc>1?"actual body clips, source room registration, terminal hold, particle decay and original timer":"source clocks; actual private pack optional");
}
