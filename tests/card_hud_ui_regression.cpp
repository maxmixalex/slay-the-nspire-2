// Production drawing functions stay private; the archive does not pull its
// app.cpp object when this test translation unit already provides it.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static unsigned checks;
static const char* captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"card/HUD UI:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t value,unsigned size=4){CHECK(at+size<=b.size());for(unsigned i=0;i<size;++i)b[at+i]=uint8_t(value>>(8*i));}
static uint32_t crc(const Bytes&b){uint32_t c=~0u;for(uint8_t value:b){c^=value;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
static unsigned group(Bytes&b,const Bytes&raw,unsigned type,unsigned id){
    if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+32);
    const unsigned fields[]={0x31524754,type,id,unsigned(raw.size()),unsigned(raw.size())+4,crc(raw),1,0};
    for(unsigned i=0;i<8;++i)put(b,at+i*4,fields[i]);
    b.push_back(1);b.push_back(uint8_t((raw.size()-1)>>8));b.push_back(uint8_t(raw.size()-1));
    b.insert(b.end(),raw.begin(),raw.end());b.push_back(0);return at;
}
static Bytes fixture(){
    // Arbitrary solid shapes make picture placement and clipping observable;
    // none of these synthetic pixels or labels comes from the original game.
    constexpr unsigned images=200,glyphs=5*96;
    const unsigned glyph_at=40+images*16,string_at=glyph_at+glyphs*16;
    const unsigned clip_at=string_at+LocaleStringCount*8,card_at=clip_at+AnimationKinds*16;
    const unsigned ui_at=card_at+CardKinds*16,base=ui_at+LocaleAuxiliaryCount*4;
    Bytes b(base+96);put(b,0,0x3250534e);put(b,4,LocaleContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,20,glyphs);put(b,24,LocaleStringCount);put(b,28,AnimationKinds);put(b,32,CardKinds);
    auto picture=[&](unsigned id,unsigned w,unsigned h,uint16_t color,unsigned shape=0){
        if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+w*h*3);
        put(b,40+id*16,at);put(b,44+id*16,w,2);put(b,46+id*16,h,2);put(b,48+id*16,1);put(b,52+id*16,w*h*3);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned p=y*w+x;put(b,at+p*2,color,2);
            b[at+w*h*2+p]=shape==1?0:shape==2&&x>=7&&x<74&&y>=11&&y<61?0:shape==3&&x>2&&x+3<w&&y>2&&y+3<h?0:255;}
    };
    picture(0,82,60,0,1);for(unsigned i=1;i<images;++i)for(unsigned k=0;k<16;++k)b[40+i*16+k]=b[40+k];
    picture(1,82,60,0x07e0);picture(2,80,112,0x001f,2);picture(3,110,28,0x2104);
    picture(4,320,22,0x2104);picture(5,30,28,0x39c7);picture(6,24,24,0xf800);picture(7,20,20,0xffff,3);
    picture(8,18,18,0xffe0);picture(9,18,18,0xf81f);picture(10,92,70,0x7bef,3);picture(11,32,16,0x7bef);picture(12,27,40,0xf800);
    unsigned index=0;
    for(unsigned size:{8u,10u,12u,16u,20u})for(unsigned n=0;n<96;++n,++index){
        const unsigned cp=n<95?32+n:0x2026,w=cp==0x2026?size:(size+1)/2,h=size+2,at=glyph_at+index*16,off=unsigned(b.size());
        put(b,at,cp);put(b,at+4,off);put(b,at+8,size,2);b[at+10]=uint8_t(w);b[at+11]=uint8_t(h);put(b,at+12,w,2);b.resize(off+w*h);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)b[off+y*w+x]=cp==' '?0:cp==0x2026?uint8_t(y==h-2&&x%3==0?255:0):255;
    }
    const unsigned empty=unsigned(b.size());b.push_back(0);for(unsigned i=0;i<LocaleStringCount;++i)put(b,string_at+i*8,empty);
    auto text=[&](unsigned id,const char*s){unsigned n=unsigned(std::strlen(s));put(b,string_at+id*8,unsigned(b.size()));put(b,string_at+id*8+4,n);b.insert(b.end(),s,s+n+1);};
    text(0,"Bounded card title");text(1,"[gold]Gain 3 Stars.[/gold] Deal 123 damage. This longer description must end visibly inside the card.");text(LocaleMarkerString,"zhs");
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i){put(b,card_at+i*16,1);put(b,card_at+i*16+8,1);put(b,card_at+i*16+12,1);}
    for(int image:{U_BigAttack,U_BigSkill,U_BigPower,U_RedAttack,U_RedSkill,U_RedPower})put(b,ui_at+image*4,2);
    for(int image:{U_CommonBanner,U_BigBanner,U_RareBanner})put(b,ui_at+image*4,3);
    put(b,ui_at+U_TopBar*4,4);put(b,ui_at+U_Star*4,9);put(b,ui_at+U_TopBarBackdrop*4,5);put(b,ui_at+U_PotionPlaceholder*4,7);
    for(unsigned i=0;i<8;++i){for(unsigned j=0;j<3;++j)put(b,ui_at+(U_CardBorderCommonAttack+i*5+j)*4,10);put(b,ui_at+(U_CardBorderCommonAttack+i*5+3)*4,11);put(b,ui_at+(U_CardBorderCommonAttack+i*5+4)*4,3);}
    for(unsigned i=0;i<5;++i)put(b,ui_at+(U_TopBarIronclad+i)*4,6);
    for(int image:{U_CharPortrait,U_SilentPortrait,U_DefectPortrait,U_RegentPortrait,U_NecrobinderPortrait})put(b,ui_at+image*4,12);
    const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,base+80,base+64,770,85,524288,617323,0,0};
    for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
    Bytes tile(770);tile[0]=tile[1]=1;Bytes record(18);put(record,0,2);put(record,4,4);put(record,8,2);
    unsigned tiles=group(b,tile,1,0),indexes=group(b,record,2,1);put(b,base+64,tiles);put(b,base+68,32+unsigned(tile.size())+4);
    put(b,base+72,indexes);put(b,base+76,32+unsigned(record.size())+4);put(b,base+80,base);put(b,base+92,unsigned(record.size()));
    put(b,40+199*16,base+80);put(b,44+199*16,1,2);put(b,46+199*16,1,2);put(b,48+199*16,6);put(b,52+199*16,16);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i)hash=(hash^b[i])*16777619u;put(b,36,hash);return b;
}
static void clear(){for(auto&p:app.pixels)p=0x0321;app.hit_count=0;}
static void capture(const char*name){
    if(!captures)return;std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
    std::fprintf(f,"P6\n320 240\n255\n");uint8_t row[960];
    for(unsigned y=0;y<240;++y){for(unsigned x=0;x<320;++x){uint16_t p=app.pixels[y*320+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,960,f)==960);}CHECK(!std::fclose(f));
}
static void bounded_canvas(){
    Canvas c{app};const char*texts[]={"A very long keyword description which will not fit.","[gold]Gain 99[/gold]\n[b]Deal damage[/b]\nMore text.\nExhaust.","[sine]Moving[/sine] [jitter]keywords[/jitter] continue here."};
    for(const char*text:texts)for(int w:{32,62,95})for(int h:{11,21,36}){
        clear();auto result=c.paragraph_fitted(text,27,41,w,h,10,White,8,true);CHECK(result.truncated||result.rows);
        unsigned changed=0;for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(app.pixels[y*320+x]!=0x0321){++changed;CHECK(x>=27&&x<27+w&&y>=41&&y<41+h);}CHECK(changed);
    }
}
static void synthetic_pixels(){
    Canvas c{app};clear();draw_card_face(c,{Strike,0},30,50,80,112,1,0,-1,-1,nullptr,"Simple description.");
    CHECK(app.pixels[85*320+60]==0x07e0);CHECK(app.pixels[130*320+60]!=0x07e0);
    std::vector<uint16_t> first(app.pixels,app.pixels+320*240);clear();draw_card_face(c,{Strike,0},30,50,80,112,1,3,-1,-1,nullptr,"Simple description.");
    for(int y=78;y<103;++y)for(int x=47;x<100;++x)CHECK(app.pixels[y*320+x]==first[y*320+x]);
    // The complete potion tray and portrait are original components in v17.
    clear();app.panel=0;hud(c);CHECK(app.pixels[8*320+8]==0xf800);CHECK(app.pixels[2*320+127]==0x39c7);
    bool potion=false,map=false,deck=false,pause=false;
    for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];if(h.kind!=HitKind::Command)continue;potion|=h.value==int(Key::Potions);map|=h.value==int(Key::Collection)||h.value==int(Key::Map);deck|=h.value==int(Key::Inspect);pause|=h.value==int(Key::Back);}
    CHECK(potion&&map&&deck&&pause);
}
static void character_hover(){
    static SaveData before,after;CHECK(save(app.game,before));const unsigned run=app.run_number;
    app.title=false;app.character_select=true;app.run_settings=false;app.pointer.held=false;app.pointer.visible=false;app.render();
    for(unsigned i=0;i<CharacterKinds;++i){
        app.pointer_move(172+int(i)*31,184);app.render();CHECK(app.selected_character==Character(i));CHECK(app.character_select&&app.run_number==run);
        CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    }
    app.pointer_move(263,225);app.render();CHECK(app.character_select&&app.run_number==run);
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.pointer.visible=false;app.character_select=false;
}
static void samples(){
    Canvas c{app};new_run(app.game,781,true,Character::Regent);app.title=false;app.character_select=false;app.panel=0;app.pointer.visible=false;app.reset_presentation();
    app.render();unsigned loading_steps=0;
    while(app.combat_preparation.active&&loading_steps++<1024){app.advance(33);app.render();}
    CHECK(!app.combat_preparation.active);app.advance(2000);app.render();capture("combat-settled");
    app.input(Key::Right);app.render();capture("combat-focused");
    clear();c.rect(0,0,320,240,Panel);hud(c);
    large_card(c,{Strike,0},21,47,false);large_card(c,{RegentComet,0},121,47,false);large_card(c,{RegentDyingStar,1},221,47,false);
    c.text_fitted("Card frame / star cost / overflow",8,209,304,12,White);capture("cards-and-hud");
    clear();c.rect(0,0,320,240,Panel);comparison_card(c,{RegentDyingStar,0},18,32);comparison_card(c,{RegentDyingStar,1},184,32);capture("card-upgrade-comparison");
    app.detail_card=card_token({RegentDyingStar,0});app.detail_instance=app.detail_deck_index=-1;clear();detail_screen(c);capture("card-detail");
    for(unsigned character=0;character<CharacterKinds;++character){
        app.character_select=true;app.selected_character=Character(character);app.map_clock_ms=1000+character*2000;app.character_select_presentation.reset();clear();character_screen(c);
        CHECK(app.character_select_presentation.panel_offset(app.map_clock_ms)==-50);
        char name[64]{};std::snprintf(name,sizeof name,"selector-%u-start",character);capture(name);
        app.map_clock_ms+=500;clear();character_screen(c);CHECK(app.character_select_presentation.panel_offset(app.map_clock_ms)==0);
        std::snprintf(name,sizeof name,"selector-%u-settled",character);capture(name);
    }
}
}
int main(int argc,char**argv){
    Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=fixture();
    if(argc>2)captures=argv[2];CHECK(app.init(data.data(),data.size(),{}));CHECK(app.content.current_profile());new_run(app.game,781,true);
    bounded_canvas();if(argc==1)synthetic_pixels();character_hover();samples();std::printf("card/HUD UI: %u checks passed; %s profile, bounded draw and selector samples\n",checks,argc>1?"actual":"synthetic");
}
