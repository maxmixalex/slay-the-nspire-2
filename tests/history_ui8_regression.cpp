#include "../src/app.cpp"
#include "world_run.hpp"
#include "content_fixture.hpp"
#include "source_scene_oracle.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static const char*captures;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"history_ui8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static HitRegion hit(HitKind kind,int value){a.render();for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void settle(){for(unsigned i=0;i<50;++i)a.advance(33);a.render();}
static void click(HitKind kind,int value){const auto h=hit(kind,value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);}
static void capture(const char*name){if(!captures)return;a.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){const uint8_t pixel[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(pixel,1,3,f)==3);}CHECK(!std::fclose(f));}
static void history_header_scene(const std::vector<uint8_t>&pack,const UiRunHistory&record){
    using namespace spire_test;const auto project=[](int n){return (n*4+10)/21;};
    const UiHistoryHeaderLayout ordinary(10,6,6,18,3);
    CHECK(ordinary.hp==278&&ordinary.hp_width==120&&ordinary.gold_icon==422&&ordinary.gold==472&&ordinary.gold_width==72);
    CHECK(ordinary.potions==568&&ordinary.potion_width==221&&ordinary.floor_icon==835&&ordinary.floor==885&&ordinary.floor_width==32);
    CHECK(ordinary.timer_icon==989&&ordinary.time==1039&&ordinary.time_width==95);
    CHECK(UiHistoryHeaderLayout::info_y(0)==32&&UiHistoryHeaderLayout::info_y(3)==122);
    const UiHistoryHeaderLayout many(40,30,20,36,16);CHECK(many.hp_width==210&&many.gold_width==158&&many.potion_width==1027&&many.time>1680);
    if(pack.empty())return;
    const auto letters=[&](const std::string&value,unsigned size,unsigned face,int tracking){auto result=scene_letters(a.content,pack,value,size,face);
        if(a.content.english())for(unsigned i=0;i+1<result.size();++i)result[i].advance+=project(int(i+1)*tracking)-project(int(i)*tracking);return result;};
    const auto sub=[](std::string&text,const char*key,const std::string&value){const auto at=text.find(key);CHECK(at!=std::string::npos);text.replace(at,std::strlen(key),value);};
    UiRunHistory r=record;
    for(unsigned mode=0;mode<3;++mode)for(int scroll:{0,5}){r.mode=uint8_t(mode);r.ascension=mode?10:0;r.partial=mode==2;
        const auto paint=[&](bool visible){for(auto&p:a.pixels)p=scene_rgb(21,40,65);Canvas canvas{a};if(!visible)canvas.text_visible_characters=0;history_header(canvas,r,0,scroll);};
        paint(true);const std::vector<uint16_t>actual(a.pixels,a.pixels+320*240);paint(false);std::vector<uint16_t>expected(a.pixels,a.pixels+320*240);
        const auto info=[&](std::string value,unsigned row,unsigned opacity=255){const auto glyphs=letters(value,5,0,1);CHECK(!glyphs.empty());const int width=scene_width(glyphs);
            scene_label(expected,glyphs,project(1550)-width,project(32+int(row)*30)-scroll,width,project(32),5,scene_rgb(255,246,226),128,0,0,0,false,false,255,0,0,4096,false,1,0,opacity);};
        std::string value=a.content.world_string("run_history","INFO.DATE_TIME");
        sub(value,"{Date}",a.content.english()?"October 24, 2026":"2026 十月 24");sub(value,"{Time}",a.content.english()?"10:40 AM":"10:40 上午");info(value,0);
        value=a.content.world_string("run_history","INFO.SEED");sub(value,"{Seed}","H1ST0RY");info(value,1);
        value=a.content.world_string("run_history","GAME_MODE.title");sub(value,"{PlayerCount}",a.content.locale_string("run_history","PLAYER_COUNT.singleplayer"));
        sub(value,"{GameMode}",a.content.locale_string("run_history",mode==0?"GAME_MODE.standard":mode==1?"GAME_MODE.custom":"GAME_MODE.daily"));info(value,2);info("—",3,128);
        if(r.ascension){const auto glyphs=letters("10",4,0,1);
            // The70px minimum expands the full-anchor54/57px ascension
            // rectangle symmetrically; its Label retains the14x88 offsets.
            scene_label(expected,glyphs,project(190),project(50)-scroll,project(14),project(88),4,scene_rgb(255,246,226),32,2,scene_rgb(135,41,24),0,true,false,255,0,0,4096,false,1,1);}
        const unsigned face=a.content.english()?5:0;
        const auto counter=[&](const std::string&text,int sx,int width,uint16_t color){const auto glyphs=letters(text,6,face,2);
            scene_label(expected,glyphs,project(sx),project(32)-scroll,project(width),project(83),6,color,128,0,0,0,false,false);};
        counter("61/80",278,120,scene_rgb(255,100,100));counter("145",472,72,scene_rgb(239,200,81));
        const int floor_width=(int(scene_width(letters("45",6,face,2)))*21+3)/4;
        counter("45",885,floor_width,scene_rgb(197,218,206));const std::string duration=r.partial?"—":"1:02:03";
        const int time_width=std::max(72,(int(scene_width(letters(duration,6,face,2)))*21+3)/4);
        counter(duration,885+floor_width+24+24+24+48+2,time_width,scene_rgb(239,200,81));
        unsigned ink=0,top_ink=0;for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"history header mode=%u scroll=%d pixel=%u,%u actual=%u expected=%u\n",mode,scroll,i%320,i/320,actual[i],expected[i]);CHECK(actual[i]==expected[i]);ink+=actual[i]!=a.pixels[i];top_ink+=i<6*320&&actual[i]!=a.pixels[i];}CHECK(ink>0);if(scroll)CHECK(top_ink>0);
    }
}
int main(int argc,char**argv){
    std::vector<uint8_t> bytes;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());std::fclose(f);CHECK(a.init(bytes.data(),bytes.size(),{}));}if(argc>2)captures=argv[2];
    new_run(a.game,0x8813,false);CHECK(world_begin_run(a.game));a.title=true;
    auto&r=a.progress.history[0];r.serial=1;r.character=0;r.started_at=1792838400;r.seconds=3723;r.hp=61;r.max_hp=80;r.gold=145;r.floor=45;r.floor_count=45;r.outcome=1;
    std::strcpy(r.seed_text,"H1ST0RY");r.relic_count=36;for(unsigned i=0;i<r.relic_count;++i)r.relics[i]=uint16_t(i);
    r.relics[0]=WorldRelicNinjaScroll;r.relics[1]=WorldRelicDustyTome;
    r.potion_count=3;r.potions[0]=unsigned(Potion::Fire);r.potions[2]=unsigned(Potion::FairyInABottle);
    history_header_scene(bytes,r);
    for(unsigned i=0;i<r.floor_count;++i){auto&f=r.floors[i];f.floor=uint16_t(i+1);f.act=uint8_t(i/15);f.region=f.act;f.room=unsigned(i%6==0?WorldRoom::Rest:i%7==0?WorldRoom::Shop:i%8==0?WorldRoom::Treasure:WorldRoom::Monster);f.hp=60;f.max_hp=80;f.gold=uint16_t(i*3);f.turns=4;}
    r.deck_count=60;for(unsigned i=0;i<r.deck_count;++i)r.deck[i].card={CardKind(i/2),0};
    // SerializableCard grouping ignores progress values but retains upgrades
    // and enchantment identity. Inspector receives exactly those same groups.
    r.deck[1].genetic=35;r.deck[1].facts.energy_offset=4;r.deck[3].card.upgraded=1;
    uint16_t ids[MaxDeck]{},counts[MaxDeck]{};CHECK(ui_history_groups(r,ids,counts)==31&&counts[0]==2);
    const UiHistoryLayout layout(r,31);CHECK(layout.floor(r,15).y>layout.floor(r,14).y&&layout.floor(r,15).x==layout.floor(r,0).x);
    CHECK(layout.relic_header>layout.floor(r,44).y&&layout.deck_header>layout.relic(35).y);
    a.progress.count=1;a.progress.next_history=1;a.progress_loaded=true;a.open_history();settle();
    CHECK(a.progress_page_scroll.viewport==UiHistoryLayout::ViewHeight&&a.progress_page_scroll.maximum>0);
    // A historical relic only has its recorded ID. A different live run's
    // prepared ancient offer must neither change its prose nor add its card.
    CardHoverTarget historical{};historical.relic=WorldRelicDustyTome;historical.historical=true;
    HoverTipList old_tips{};collect_relic_hover_tips(a,WorldRelicDustyTome,old_tips,true);
    char old_title[256]{},old_body[2048]{},new_title[256]{},new_body[2048]{};
    CHECK(old_tips.count==1);format_card_hover_tip(a,historical,old_tips.items[0],old_title,sizeof old_title,old_body,sizeof old_body);
    CHECK(world_relic_prepare(a.game,WorldRelicDustyTome));HoverTipList live_tips{};collect_relic_hover_tips(a,WorldRelicDustyTome,live_tips);
    bool live_card=false;for(unsigned i=0;i<live_tips.count;++i)live_card|=live_tips.items[i].kind==HoverTipKind::Card;CHECK(live_card);
    collect_relic_hover_tips(a,WorldRelicDustyTome,old_tips,true);CHECK(old_tips.count==1);
    format_card_hover_tip(a,historical,old_tips.items[0],new_title,sizeof new_title,new_body,sizeof new_body);
    CHECK(!std::strcmp(old_title,new_title)&&!std::strcmp(old_body,new_body));
    HoverTipList static_tips{};collect_relic_hover_tips(a,WorldRelicNinjaScroll,static_tips,true);
    CHECK(static_tips.find(HoverTipKind::Relic,"NINJA_SCROLL")>=0&&static_tips.find(HoverTipKind::Card,"SHIV")>=0&&static_tips.find(HoverTipKind::Keyword,"EXHAUST")>=0);
    SaveData before{},after{};CHECK(save(a.game,before));capture("history-top");
    a.pointer.visible=false;a.input(Key::Down);CHECK(a.history_presentation.focus==0);a.input(Key::Right);CHECK(a.history_presentation.focus==1);
    a.input(Key::Down);CHECK(a.history_presentation.focus>=15);a.render();CHECK(a.history_presentation.hover==a.history_presentation.focus);
    // Clicking relics focuses a tooltip; it does not introduce a new modal.
    a.progress_page_scroll.target=(layout.relic_y-80)*256;settle();click(HitKind::HistoryRelic,0);a.render();CHECK(a.detail_item<0);
    CHECK((a.keyword_hover.owner&0xff000000u)==0x73000000u);capture("history-relic-hover");
    const auto rh=hit(HitKind::HistoryRelic,0);historical.relic=WorldRelicNinjaScroll;historical.owner={rh.x,rh.y,rh.w,rh.h};
    const auto related=card_hover_page(a,historical,static_tips);bool card_piece=false;
    for(unsigned i=0;i<related.count;++i){const auto&p=related.pieces[i];CHECK(p.box.x>=0&&p.box.x+p.box.w<=320&&p.box.y>=0&&p.box.y+p.box.h<=240);card_piece|=static_tips.items[p.tip].kind==HoverTipKind::Card;}CHECK(card_piece);
    a.pointer.visible=false;a.history_presentation.focus=UiHistoryLayout::RelicFirst;a.render();CHECK((a.keyword_hover.owner&0xff000000u)==0x73000000u);
    a.pointer.visible=false;a.progress_page_scroll.target=(layout.deck_y-40)*256;settle();
    click(HitKind::HistoryCard,0);CHECK(a.detail_card>=0&&a.card_inspect.source==CardInspectSource::History&&a.card_inspect.count==31);
    CHECK(a.history_selected==0);a.input(Key::Right);CHECK(a.history_selected==2);a.input(Key::Back);CHECK(a.detail_card<0);
    // A drag originating on a card scrolls the same page without inspecting it.
    auto h=hit(HitKind::HistoryCard,0);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_move(h.x+h.w/2,h.y+h.h/2-20);a.pointer_button(false);CHECK(a.detail_card<0&&!a.pointer.held);settle();capture("history-deck");
    CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    a.progress.history[1]=r;a.progress.history[1].serial=2;a.progress.count=2;a.progress.next_history=2;a.open_history();a.history_select(1);
    CHECK(a.history_index==1&&a.history_presentation.direction==-1&&a.progress_page_scroll.position==0);settle();
    CHECK(a.history_presentation.offset(a.map_clock_ms)==0&&a.history_presentation.alpha(a.map_clock_ms)==255);
    a.history_select(1);CHECK(a.history_index==1);a.history_select(-1);CHECK(a.history_index==0&&a.history_presentation.direction==1);
    std::printf("History layout, grouping, keyboard, hover, scroll, drag and immutable gameplay: %u checks\n",checks);
}
