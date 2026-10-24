// Production drawing functions stay private; the archive does not pull its
// app.cpp object when this test translation unit already provides it.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "inplace_reset.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
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
            b[at+w*h*2+p]=shape==1?0:shape==2&&x>=7&&x<74&&y>=11&&y<61?0:shape==3&&x>2&&x+3<w&&y>2&&y+3<h?0:shape==4&&(x<4||x>=31)?0:255;}
    };
    picture(0,82,60,0,1);for(unsigned i=1;i<images;++i)for(unsigned k=0;k<16;++k)b[40+i*16+k]=b[40+k];
    picture(1,82,60,0x07e0);picture(2,80,112,0x001f,2);picture(3,110,28,0x2104);
    picture(4,320,22,0x2104);picture(5,30,28,0x39c7);picture(6,24,24,0xf800);picture(7,20,20,0xffff,3);
    picture(8,18,18,0xffe0);picture(9,18,18,0xf81f);picture(10,92,70,0x7bef,3);picture(11,32,16,0x7bef);picture(12,36,40,0xf800,4);
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
static void glyph_measurement(const Bytes&data){
    struct Provider {const Bytes*data;unsigned calls=0;};Provider provider{&data};
    auto read=[](void*context,uint32_t offset,void*out,uint32_t count){auto&p=*static_cast<Provider*>(context);++p.calls;
        if(offset>p.data->size()||count>p.data->size()-offset)return false;std::memcpy(out,p.data->data()+offset,count);return true;};
    auto content=std::make_unique<Content>();CHECK(content->open({&provider,uint32_t(data.size()),read,nullptr}));
    auto word=[&](unsigned at){return uint32_t(data[at])|uint32_t(data[at+1])<<8|uint32_t(data[at+2])<<16|uint32_t(data[at+3])<<24;};
    const unsigned count=word(20),first=40+word(16)*16;
    CHECK(count>=64);
    for(unsigned i=0;i<64;++i){const unsigned at=first+i*16;CHECK(content->glyph(word(at),data[at+8]|unsigned(data[at+9])<<8).pixels);}
    const auto stats=content->read_stats();const unsigned calls=provider.calls;
    // Measuring the complete catalog must neither read pixels nor evict the
    // 64 glyphs just prepared for painting. This catches layout-cache churn.
    for(unsigned i=0;i<count;++i){const unsigned at=first+i*16;const int size=data[at+8]|unsigned(data[at+9])<<8;
        const auto g=content->glyph_metrics(word(at),size);CHECK(!g.pixels&&g.w==data[at+10]&&g.h==data[at+11]&&g.advance==(data[at+12]|unsigned(data[at+13])<<8));}
    const auto measured=content->read_stats();
    CHECK(provider.calls==calls&&measured.callback_bytes==stats.callback_bytes&&measured.callbacks==stats.callbacks&&
        measured.cache_hits==stats.cache_hits&&measured.cache_misses==stats.cache_misses&&measured.bypasses==stats.bypasses&&measured.direct_reads==stats.direct_reads);
    for(unsigned i=0;i<64;++i){const unsigned at=first+i*16;CHECK(content->glyph(word(at),data[at+8]|unsigned(data[at+9])<<8).pixels);}
    CHECK(provider.calls==calls&&content->read_stats().cache_hits==measured.cache_hits);
    const auto missing=content->glyph_metrics(0x10ffff,8),fallback=content->glyph_metrics('?',8);
    CHECK(missing.w==fallback.w&&missing.h==fallback.h&&missing.advance==fallback.advance&&content->healthy());
}
static void pattern(unsigned salt=0){
    for(unsigned i=0;i<320*240;++i)app.pixels[i]=uint16_t((i*4793u+0x185fu+salt*961u)^(i*17u));
}
static void title_layout(){
    Canvas c{app};
    for(const char*text:{"Short title","A substantially longer card title that needs an ellipsis","[b]Bold title[/b]","A\nsecond line"})
    for(int width:{23,47,96})for(int size:{8,10,12}){
        pattern();for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text_fitted(text,40+dx,45+dy,width,size,Ink,8,true);
        c.text_fitted(text,40,45,width,size,Green,8,true);const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        pattern();c.text_fitted(text,40,45,width,size,Green,8,true,true);CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
    }
}
static void card_cache_pixels(){
    Canvas c{app};invalidate_card_face_cache();
    // Merchant rest/hover sizes share the same fixed tile pool. A warm cache
    // must neither replay another size nor overwrite a changed rug pixel.
    static constexpr int dimensions[][2]={{36,50},{44,62},{47,66},{43,61},{46,64}};
    for(int size:{0,1,0,2,1,2,3,4,3,4})for(unsigned salt:{0u,1u}){
        const int w=dimensions[size][0],h=dimensions[size][1];
        pattern(salt);render_card_face(c,{Strike,0},35,69,w,h,1,0,-1,-1,nullptr,"Shop card.");
        const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        for(int repeat=0;repeat<2;++repeat){pattern(salt);begin_card_face_frame();draw_card_face(c,{Strike,0},35,69,w,h,1,0,-1,-1,nullptr,"Shop card.");CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));}
        bool cached=false;for(const auto&slot:small_card_faces)cached|=slot.valid&&slot.width==w&&slot.height==h;CHECK(cached);
    }
    invalidate_card_face_cache();
    // Compare full destination surfaces against uncached production drawing,
    // including translucent overlap, changed costs, stars and descriptions.
    for(unsigned salt:{0u,1u})for(unsigned variant=0;variant<9;++variant){
        const Card card{variant&1?RegentFallingStar:Strike,uint8_t(variant/2&1)};
        const int fee=variant<6?int(variant)-2:100,stars=variant<6?int(variant%4):variant==6?10:variant==7?99:9999;
        const char*text=variant&1?"[gold]Gain 3 Stars.[/gold] Deal 123 damage.":"Deal damage. [b]Exhaust.[/b]";
        pattern(salt);render_card_face(c,card,29,68,47,66,fee,stars,-1,-1,nullptr,text);
        const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        for(int repeat=0;repeat<2;++repeat){pattern(salt);begin_card_face_frame();draw_card_face(c,card,29,68,47,66,fee,stars,-1,-1,nullptr,text);
            CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));}
    }
    const char*animated="[sine]Moving text[/sine] [rainbow]colors[/rainbow]";
    const auto clock=app.map_clock_ms;const auto animation=app.animation;
    for(unsigned frame:{0u,7u,18u}){
        app.animation=frame;app.map_clock_ms=frame*33;pattern();render_card_face(c,{Strike,0},29,68,47,66,1,0,-1,-1,nullptr,animated);
        const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);pattern();begin_card_face_frame();draw_card_face(c,{Strike,0},29,68,47,66,1,0,-1,-1,nullptr,animated);
        CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
    }
    app.map_clock_ms=clock;app.animation=animation;
    for(int mode=0;mode<4;++mode){
        Canvas altered=c;const int x=mode==0?0:29,y=mode==1?215:68;
        if(mode==2)altered.opacity=143;if(mode==3){altered.brightness=171;altered.tint_red=91;}
        pattern();render_card_face(altered,{Strike,0},x,y,47,66,1,0,-1,-1,nullptr,"Clipped or tinted.");
        const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);pattern();begin_card_face_frame();draw_card_face(altered,{Strike,0},x,y,47,66,1,0,-1,-1,nullptr,"Clipped or tinted.");
        CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
    }
    // Ten distinct faces exceed the eight-slot budget. The same eight faces
    // must stay admitted on successive frames, with exact full-grid output.
    invalidate_card_face_cache();CardKind retained[SmallCardCacheCount]{};
    for(unsigned frame=0;frame<3;++frame){
        pattern();for(int i=0;i<10;++i)render_card_face(c,{CardKind(i),0},24+(i%5)*55,54+(i/5)*73,47,66,i%4,0,-1,-1,nullptr,"Card grid.");
        const std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);pattern();begin_card_face_frame();
        for(int i=0;i<10;++i)draw_card_face(c,{CardKind(i),0},24+(i%5)*55,54+(i/5)*73,47,66,i%4,0,-1,-1,nullptr,"Card grid.");
        CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
        for(unsigned i=0;i<SmallCardCacheCount;++i){const auto&slot=small_card_faces[i];CHECK(slot.valid&&slot.seen==small_card_frame);
            if(frame)CHECK(slot.card.kind==retained[i]);else retained[i]=slot.card.kind;}
    }
    CHECK(app.content.healthy());invalidate_card_face_cache();
}
static void focused_card_scale(){
    Canvas c{app};static HandTile transparent;transparent.playable=true;
    struct Example {HandPose pose;int left,top,right,bottom;};
    const Example examples[]={{{160,120,512,0,1024},143,94,179,144},{{160,120,672,0,1024},138,86,184,151},
        {{160,120,1024,0,1024},126,68,198,168},{{160,120,1024,1024,0},112,86,212,158}};
    for(const auto&example:examples){
        clear();draw_hand_tile(c,transparent,example.pose,true);int left=320,top=240,right=-1,bottom=-1;unsigned count=0;
        for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(app.pixels[y*320+x]!=0x0321){CHECK(app.pixels[y*320+x]==Gold);++count;left=mn(left,x);right=mx(right,x);top=mn(top,y);bottom=mx(bottom,y);}
        CHECK(left==example.left&&top==example.top&&right==example.right&&bottom==example.bottom);
        CHECK(count==unsigned(2*(right-left+bottom-top)));
        clear();draw_hand_tile(c,transparent,example.pose,false);for(auto pixel:app.pixels)CHECK(pixel==0x0321);
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
    for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];potion|=h.kind==HitKind::PotionSlot;if(h.kind!=HitKind::Command)continue;map|=h.value==int(Key::Collection)||h.value==int(Key::Map);deck|=h.value==int(Key::Inspect);pause|=h.value==int(Key::Back);}
    CHECK(potion&&map&&deck&&pause);
    // The source character row uses 22x25 pictures at centers 116..204.
    // This fixture's 27/36 opaque strip projects to 16 pixels at rest and
    // 18 at the 1.1 hover scale, with the same full-height alpha coverage.
    auto portrait_bounds=[&](unsigned character,int width,int height){
        const int center=character_select_layout::portrait_center(character);
        int left=320,right=-1,top=240,bottom=-1;
        for(int y=191;y<231;++y)for(int x=center-12;x<center+13;++x)if(app.pixels[y*320+x]==0xf800){
            left=mn(left,x);right=mx(right,x);top=mn(top,y);bottom=mx(bottom,y);
        }
        CHECK(right-left+1==width&&bottom-top+1==height);
    };
    app.selected_character=Character::Ironclad;app.run_settings=false;app.pointer.visible=true;app.pointer.x=app.pointer.y=0;
    app.map_clock_ms=100;app.character_select_presentation.reset();clear();character_screen(c);portrait_bounds(0,16,25);
    app.pointer.x=character_select_layout::portrait_center(0);app.pointer.y=210;app.map_clock_ms=150;clear();character_screen(c);portrait_bounds(0,16,25);
    app.pointer.x=character_select_layout::portrait_center(1);app.map_clock_ms=200;clear();character_screen(c);portrait_bounds(1,18,27);
    app.selected_character=Character::Silent;clear();character_screen(c);portrait_bounds(1,18,27);
    app.pointer.x=app.pointer.y=0;app.map_clock_ms=201;clear();character_screen(c);
    app.map_clock_ms=701;clear();character_screen(c);portrait_bounds(1,16,25);app.pointer.visible=false;
}
static void character_hover(){
    static SaveData before,after;CHECK(save(app.game,before));const unsigned run=app.run_number;
    app.title=false;app.character_select=true;app.run_settings=false;app.pointer.held=false;app.pointer.visible=false;
    app.selected_character=Character::Ironclad;app.character_select_presentation.reset();app.render();
    for(unsigned i=0;i<CharacterKinds;++i){
        const Character previous=app.selected_character;
        const uint32_t entered=app.character_select_presentation.panel_start;
        app.advance(33);app.pointer_move(character_select_layout::portrait_center(i),210);app.render();
        CHECK(app.selected_character==previous&&app.character_select_presentation.panel_start==entered);
        CHECK(app.character_select_presentation.focused==int(i));
        CHECK(app.character_select_presentation.portrait_scale(i,app.map_clock_ms)==(Character(i)==previous?CharacterSelectPresentation::Scale:CharacterSelectPresentation::FocusScale));
        app.pointer_button(true);app.render();CHECK(app.selected_character==previous);
        app.pointer_button(false);app.render();CHECK(app.selected_character==Character(i));
        CHECK(app.character_select&&app.run_number==run);
        CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    }
    // Releasing over a different portrait is not a selection. Only the
    // clicked portrait owns activation; mere focus still scales its picture.
    app.pointer_move(character_select_layout::portrait_center(0),210);app.pointer_button(true);app.pointer_move(character_select_layout::portrait_center(1),210);app.pointer_button(false);app.render();
    CHECK(app.selected_character==Character::Necrobinder);
    app.pointer_move(310,225);app.render();CHECK(app.character_select&&app.run_number==run);
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.pointer.visible=false;app.character_select=false;
}
static void top_bar_large_rotation(){
    const uint32_t clock=app.map_clock_ms;
    TopBarMotion motion{};app.map_clock_ms=6200;
    motion.observe(app,0,true,2);const int from=motion.angle;
    CHECK(from>350000&&from<360000);CHECK(from==355244);
    motion.observe(app,1,false,2);CHECK(motion.angle==from);
    // BackOut overshoots -180 degrees before returning to it. This span
    // exercises a product beyond signed 32-bit range at a real open angle.
    for(unsigned elapsed:{200u,350u,500u}){
        app.map_clock_ms=6200+elapsed;motion.observe(app,1,false,2);
        CHECK(motion.angle<0&&motion.angle>=-240000);
    }
    CHECK(motion.angle==-180000);
    app.map_clock_ms=6800;motion.observe(app,0,false,2);
    app.map_clock_ms=7800;motion.observe(app,0,false,2);CHECK(motion.angle==0&&motion.scale==4096);
    CHECK(top_bar_scaled(-540000,4096)==-540000);
    CHECK(top_bar_scaled(540000,4506)==594052&&top_bar_scaled(-540000,4506)==-594052);
    TopBarMotion rotation{};app.map_clock_ms=6282;rotation.observe(app,0,true,2);CHECK(rotation.angle==359942);
    app.map_clock_ms=6283;rotation.observe(app,0,true,2);CHECK(rotation.angle==0);
    TopBarMotion held{};app.map_clock_ms=0;held.observe(app,2,false,2);CHECK(held.angle==0);
    app.map_clock_ms=250;held.observe(app,2,false,2);CHECK(held.angle==45000);
    app.map_clock_ms=600000;held.observe(app,2,false,2);CHECK(held.angle==45000&&held.scale==4506);
    CHECK(top_bar_phase(765000)==512&&top_bar_phase(-765000)==-512);
    app.map_clock_ms=clock;
}
static void top_bar_session_lifecycle(const Bytes&data){
    // The host may reconstruct App at the same address. Each source overload
    // reopens Content at generation 1, as the independent WASM instances do.
    for(bool source_overload:{false,true}){
        const auto fresh=[&](){
            reset_in_place(app);
            CHECK(source_overload?app.init(ResourceSource::memory(data.data(),data.size()),{}):app.init(data.data(),data.size(),{}));
            CHECK(app.content.generation()==1);new_run(app.game,701,true);app.title=false;app.map_clock_ms=2617;
            clear();Canvas c{app};hud(c);
        };
        fresh();CHECK(top_bar_motions[1].angle==0&&top_bar_motions[1].scale==4096);
        const std::vector<uint16_t> baseline(app.pixels,app.pixels+ScreenW*22);
        Canvas c{app};app.map_clock_ms=3000;app.pointer.x=266;app.pointer.y=10;app.pointer.visible=true;hud(c);
        app.map_clock_ms=3500;hud(c);CHECK(top_bar_motions[1].angle==-12000);
        app.map_clock_ms=3617;app.pointer.visible=false;hud(c);
        CHECK(top_bar_motions[1].state==0&&top_bar_motions[1].start==3617&&top_bar_motions[1].from_angle==-12000);
        fresh();app.map_clock_ms=3617;clear();hud(c);
        CHECK(top_bar_motions[1].angle==0&&top_bar_motions[1].scale==4096);
        CHECK(!std::memcmp(baseline.data(),app.pixels,ScreenW*22*sizeof(uint16_t)));
    }
}
static void character_confirm_art(){
    const int base=app.content.world_image("common","CONFIRM"),tick=app.content.world_image("common","CONFIRM_TICK");
    if(base<0||tick<0)return; // The synthetic profile has no original control atlas.
    CHECK(app.content.image(tick).w&&app.content.image(tick).h);
    Canvas c{app};app.pointer.visible=false;app.pointer.held=false;for(auto&motion:app.button_motions)motion={};
    clear();native_original_confirm(c,UiAction::StartCharacter);const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);
    clear();const auto info=app.content.image(base);CHECK(info.w&&info.h);
    // Independent projection of the source Image/Shadow/Icon child nodes,
    // not a fit of the entire sprite into its smaller Control hit rectangle.
    const int size=15,tick_x=296,tick_y=175;
    Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(base,286,172,42,29,64);
    c.picture(base,284,170,42,29);
    unsigned changed=0;for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(actual[y*320+x]!=app.pixels[y*320+x]){
        ++changed;CHECK(x>=tick_x&&x<tick_x+size&&y>=tick_y&&y<tick_y+size);
    }
    CHECK(changed>20); // The actual atlas tick must be visible at the control center.
    Canvas cream=c;cream.tint_green=246;cream.tint_blue=226;cream.picture(tick,tick_x,tick_y,size,size);
    CHECK(!std::memcmp(actual.data(),app.pixels,sizeof app.pixels));
}
static void samples(){
    Canvas c{app};new_run(app.game,781,true,Character::Regent);app.title=false;app.character_select=false;app.panel=0;app.pointer.visible=false;app.reset_presentation();
    app.render();unsigned loading_steps=0;
    while(app.combat_preparation.active&&loading_steps++<1024){app.advance(33);app.render();}
    CHECK(!app.combat_preparation.active);
    // advance() caps one host update at 250ms; use real updates to pass the
    // 2200ms player-turn banner before inspecting the settled hand layout.
    for(int i=0;i<10;++i)app.advance(250);app.render();capture("combat-settled");
    app.input(Key::Right);app.render();capture("combat-focused");
    clear();c.rect(0,0,320,240,Panel);hud(c);
    large_card(c,{Strike,0},21,47,false);large_card(c,{RegentComet,0},121,47,false);large_card(c,{RegentDyingStar,1},221,47,false);
    c.text_fitted("Card frame / star cost / overflow",8,209,304,12,White);capture("cards-and-hud");
    clear();c.rect(0,0,320,240,Panel);comparison_card(c,{RegentDyingStar,0},18,32);comparison_card(c,{RegentDyingStar,1},184,32);capture("card-upgrade-comparison");
    app.detail_card=card_token({RegentDyingStar,0});app.detail_instance=app.detail_deck_index=-1;clear();detail_screen(c);capture("card-detail");
    app.detail_card=-1;app.open_library(0);
    clear();library_screen(c);capture("card-library");
    app.pointer.visible=true;app.pointer.x=25;app.pointer.y=25;clear();library_screen(c);capture("card-library-filter-hover");
    CHECK(app.library_filter==5);app.pointer.visible=false;app.library=false;
    app.has_save=true;app.saved_character=Character::Necrobinder;app.saved_hp=54;app.saved_max_hp=66;app.saved_floor=14;app.saved_ascension=3;app.menu=0;
    clear();title_screen(c);capture("title-continue");
    for(unsigned character=0;character<CharacterKinds;++character){
        app.character_select=true;app.selected_character=Character(character);app.map_clock_ms=1000+character*2000;app.character_select_presentation.reset();clear();character_screen(c);
        CHECK(app.character_select_presentation.panel_offset(app.map_clock_ms)==-57);
        char name[64]{};std::snprintf(name,sizeof name,"selector-%u-start",character);capture(name);
        app.map_clock_ms+=500;clear();character_screen(c);CHECK(app.character_select_presentation.panel_offset(app.map_clock_ms)==0);
        std::snprintf(name,sizeof name,"selector-%u-settled",character);capture(name);
    }
}
static void long_hand_description(){
    new_run(app.game,787,true,Character::Defect);app.title=false;app.character_select=app.library=false;app.reset_presentation();
    app.game.card_count=app.game.hand.count=1;app.game.hand.ids[0]=0;app.game.cards[0]={DefectRainbow,0};app.game.zones[0]=Zone::Hand;
    app.game.energy=99;app.cursor=0;app.hand_focus=true;app.pointer.visible=false;
    Canvas c{app};const auto&tile=hand_tile(c,0);CHECK(tile.description_truncated);
    CardHoverTarget owner{};CHECK(card_hover_target(app,owner)&&owner.instance==0);
    HoverTipList tips{};collect_visible_hover_tips(c,owner,tips);CHECK(tips.count&&tips.items[0].kind==HoverTipKind::Description);
    char title[256]{},body[2048]{},expected[2048]{};
    format_card_hover_tip(app,owner,tips.items[0],title,sizeof title,body,sizeof body);
    app.card_description(owner.card,-1,expected,sizeof expected,0);CHECK(!std::strcmp(body,expected));
    CHECK(&hand_tile(c,0)==&tile&&hand_tile(c,0).description_truncated);
    keyword_tooltips(c);capture("hand-automatic-description");
    app.reset_presentation();
}
static void ui7_cards(){
    DeckCountPulse count{};CHECK(count.observe(10,100)==6144);const int fading=count.observe(10,350);CHECK(fading>4096&&fading<4200);
    CHECK(count.observe(10,600)==4096&&count.observe(9,700)==4096&&count.observe(10,800)==4096);
    CHECK(count.observe(11,900)==6144&&count.high_water==11);
    PotionHolderMotion holder{};CHECK(holder.observe(true,false,100)==4096);CHECK(holder.observe(true,false,150)==4710);
    holder.observe(false,false,200);CHECK(holder.observe(false,false,450)>4096&&holder.scale<4120);CHECK(holder.observe(false,false,700)==4096);
    TargetReticleMotion reticle{};reticle.observe(true,100);CHECK(reticle.scale(100)==4096);CHECK(reticle.scale(101)<3800&&reticle.scale(350)>4080);CHECK(reticle.scale(600)==4096);
    new_run(app.game,713,false);auto&s=app.game;s.phase=Phase::Combat;s.floor=s.turn=1;s.current_node=0;s.energy=99;
    CHECK(encounter_setup(s,EncounterId::NibbitsNormal));s.card_count=s.deck_count=s.hand.count=3;
    s.draw.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    const Card cards[]={{Whirlwind,0},{Bash,0},{Defend,1}};
    for(unsigned i=0;i<3;++i){s.cards[i]=s.deck[i]=cards[i];s.hand.ids[i]=uint16_t(i);s.zones[i]=Zone::Hand;s.event_cards.deck_origin[i]=0;}
    app.title=app.library=app.character_select=app.paused=app.settings_open=false;app.reset_presentation();app.pointer.visible=false;
    app.render();for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}CHECK(!app.combat_preparation.active);
    SaveData before{},after{};CHECK(save(s,before));const auto unchanged=[&](){CHECK(save(s,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));};
    app.cursor=0;app.input(Key::Confirm);CHECK(app.keyboard_card==0);app.render();
    for(unsigned i=0;i<s.enemy_count;++i)CHECK(app.target_reticles[1+i].selected);CHECK(!app.target_reticles[0].selected);capture("target-reticle-all-enemies");unchanged();
    app.input(Key::Back);app.render();for(unsigned i=0;i<s.enemy_count;++i)CHECK(!app.target_reticles[1+i].selected&&app.target_reticles[1+i].active);
    app.advance(201);app.render();for(unsigned i=0;i<s.enemy_count;++i)CHECK(!app.target_reticles[1+i].active);
    app.cursor=2;app.input(Key::Confirm);app.render();CHECK(app.target_reticles[0].selected);
    for(unsigned i=0;i<s.enemy_count;++i)CHECK(!app.target_reticles[1+i].selected);capture("target-reticle-self");unchanged();app.input(Key::Back);
    app.panel=1;app.pile_tab=0;app.open_card_detail(s.deck[0],0);CHECK(app.card_inspect.index==0&&app.card_inspect.count==3);
    app.render();CHECK(app.card_inspect.card_scale(app.map_clock_ms)==7168);app.advance(250);app.render();
    CHECK(app.card_inspect.card_scale(app.map_clock_ms)==8192);CardHoverTarget owner{};CHECK(card_hover_target(app,owner)&&owner.detail);capture("inspector-original-centered");
    const int cursor=app.inspect_cursor;app.input(Key::Right);CHECK(app.detail_deck_index==1&&card_from_token(app.detail_card).kind==Bash&&app.card_inspect.card_x(app.map_clock_ms)==182);
    app.input(Key::Confirm);CHECK(card_from_token(app.detail_card).upgraded);app.input(Key::Right);CHECK(app.detail_deck_index==2&&card_from_token(app.detail_card).upgraded);
    app.input(Key::Right);CHECK(app.detail_deck_index==2);app.input(Key::Confirm);CHECK(!card_from_token(app.detail_card).upgraded);
    app.input(Key::Left);CHECK(app.detail_deck_index==1&&!card_from_token(app.detail_card).upgraded&&app.inspect_cursor==cursor);
    app.advance(250);app.render();CHECK(app.card_inspect.card_x(app.map_clock_ms)==160);capture("inspector-carousel-upgrade");unchanged();app.input(Key::Back);CHECK(app.detail_card<0&&app.panel==1);
    app.panel=0;app.reset_presentation();
}
static void ui6_screens(){
    if(app.content.version()<Ui6ContentVersion)return;
    new_run(app.game,701,true);CHECK(encounter_setup(app.game,EncounterId::NibbitsNormal));app.game.energy=99;
    for(Zone zone:{Zone::Discard,Zone::Exhaust}){auto&s=app.game;CHECK(s.draw.count);const auto id=s.draw.ids[--s.draw.count];
        auto&destination=zone==Zone::Discard?s.discard:s.exhaust;destination.ids[destination.count++]=id;s.zones[id]=zone;}
    CHECK(potion_set(app.game,0,Potion::Fire));app.title=app.character_select=app.library=app.paused=app.settings_open=false;app.reset_presentation();app.pointer.visible=false;
    app.render();for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}CHECK(!app.combat_preparation.active);
    for(unsigned i=0;i<10;++i){app.advance(250);app.render();}
    SaveData before{},after{};CHECK(save(app.game,before));
    app.input(Key::Inspect);app.render();unsigned sorters=0;
    for(int i=0;i<app.hit_count;++i){sorters+=app.hits[i].kind==HitKind::PileSort;CHECK(app.hits[i].kind!=HitKind::PileTab);}
    CHECK(sorters==4);capture("deck-original-sorters");
    app.sort_pile(3);app.pile_upgraded=true;app.render();capture("deck-original-upgrades");
    app.pointer_move(266,10);app.advance(165);app.render();capture("top-deck-hover-count");app.input(Key::Back);
    const Key piles[]={Key::DrawPile,Key::DiscardPile,Key::ExhaustPile};const char*names[]={"draw-pile-original","discard-pile-original","exhaust-pile-original"};
    for(unsigned i=0;i<3;++i){app.input(piles[i]);app.render();CHECK(app.panel==1&&app.pile_tab==int(i+1));
        for(int h=0;h<app.hit_count;++h)CHECK(app.hits[h].kind!=HitKind::PileTab&&app.hits[h].kind!=HitKind::PileSort);
        capture(names[i]);app.input(Key::Back);}
    app.cursor=-1;for(unsigned i=0;i<app.game.hand.count;++i){const int id=app.game.hand.ids[i];
        if(can_play_card(app.game,id)&&card_targets_enemy(app.game,app.game.cards[id],id)){app.cursor=int(i);break;}}
    CHECK(app.cursor>=0);app.input(Key::Confirm);CHECK(app.keyboard_card>=0);app.render();capture("card-original-target-start");
    for(unsigned frame=0;frame<20;++frame){app.advance(33);app.render();}
    CHECK(target_arrow_motion.active&&CombatCardMotion::magnitude(target_arrow_motion.x/256-enemy_pose(app.game,app.target).x)<=1);
    capture("card-original-target-arrow");app.input(Key::Back);
    app.pointer_move(135,10);app.render();capture("potion-original-hover");
    // Empty holders keep the original hover explanation but cannot activate
    // the use/discard popup. Disabled hit geometry remains discoverable.
    app.pointer_move(150,10);app.render();const auto*empty=app.hit_at(150,10);
    CHECK(empty&&empty->kind==HitKind::PotionSlot&&!empty->enabled&&potion_at(app.game,unsigned(empty->value))==Potion::Empty);
    const std::vector<uint16_t> hover_frame(app.pixels,app.pixels+ScreenW*ScreenH);
    capture("potion-empty-original-hover");app.pointer.visible=false;app.render();
    unsigned changed=0;for(int y=27;y<100;++y)for(int x=170;x<315;++x)changed+=hover_frame[y*ScreenW+x]!=app.pixels[y*ScreenW+x];
    CHECK(changed>100);app.pointer_move(150,10);app.pointer_button(true);app.pointer_button(false);CHECK(!app.panel);
    app.pointer_move(300,10);app.advance(165);app.render();capture("top-settings-hover");
    CHECK(app.content.image(app.content.ui_image(I6_CursorDefault)).w==16);
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.pointer_cancel();app.pointer.visible=false;app.reset_presentation();
}
}
int main(int argc,char**argv){
    Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=fixture();
    if(argc>2)captures=argv[2];CHECK(app.init(data.data(),data.size(),{}));CHECK(app.content.current_profile());new_run(app.game,781,true);
    glyph_measurement(data);bounded_canvas();title_layout();card_cache_pixels();focused_card_scale();if(argc==1)synthetic_pixels();character_hover();top_bar_large_rotation();if(argc>1)character_confirm_art();long_hand_description();ui7_cards();ui6_screens();samples();top_bar_session_lifecycle(data);std::printf("card/HUD UI: %u checks passed; %s profile, zero-I/O metrics, exact grid cache, automatic overflow tooltip, transformed focus, bounded draw and selector samples\n",checks,argc>1?"actual":"synthetic");
}
