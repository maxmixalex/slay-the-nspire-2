#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "card_grid_fixture.hpp"
#include "resource_transfer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;
static State before;
static unsigned checks;
static bool actual;
static unsigned cookie_clock_calls,cookie_entropy_calls;
static uint32_t cookie_clock(){++cookie_clock_calls;return 0x13572468u;}
static uint32_t cookie_entropy(){++cookie_entropy_calls;return 0xabcdef12u;}
static const char*captures;
static const char*scenario="init";
static std::vector<uint32_t> actual_glyph_keys;
static uint32_t little_word(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"relic inspect [%s]:%d %s item=%d time=%u\n",scenario,__LINE__,#x,a.detail_item,a.map_clock_ms);std::exit(1);}}while(0)
#include "ftue_fixture.hpp"
static void snapshot(){std::memcpy(&before,&a.game,sizeof(State));}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof(State)));}
static bool exact_glyph(uint32_t codepoint,unsigned size,unsigned face){
    const uint32_t key=((size|((face&3u)<<6)|((face&4u)<<3))<<24)|codepoint;
    return std::binary_search(actual_glyph_keys.begin(),actual_glyph_keys.end(),key);
}
static void advance(unsigned time){while(time){const unsigned step=time>33?33:time;a.advance(step);time-=step;}a.render();}
static HitRegion region(UiAction action,bool required=true){a.render();for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind==HitKind::UiAction&&h.value==int(action))return h;}CHECK(!required);return {};}
static void click(int x,int y){a.pointer_move(x,y);a.pointer_button(true);a.pointer_button(false);a.render();}
static void action(UiAction action){const auto h=region(action);click(h.x+h.w/2,h.y+h.h/2);}
static void capture(const char*name){if(!captures)return;a.render();std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
    for(unsigned i=0;i<ScreenW*ScreenH;++i){const auto p=a.pixels[i];const uint8_t rgb[3]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void setup(){
    new_run(a.game,91,true);gain_relic(a.game,Anchor);gain_relic(a.game,Shuriken);a.reset_presentation();
    a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
    a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;a.pointer.visible=false;
    a.render();snapshot();const auto animation=a.animation;for(unsigned i=0;i<256&&a.combat_preparation.active;++i)a.advance(33);
    CHECK(!a.combat_preparation.active&&a.animation==animation);unchanged();
}
static void cookie_ownership(){
    scenario="Cookie canonical cache and real owner";
    const auto callbacks=a.callbacks;a.callbacks.wall_clock=cookie_clock;a.callbacks.seed_entropy=cookie_entropy;
    static const char*keys[]={"YUMMY_COOKIE_IRONCLAD","YUMMY_COOKIE_SILENT","YUMMY_COOKIE_DEFECT","YUMMY_COOKIE_REGENT","YUMMY_COOKIE_NECRO"};
    int images[5]{};snapshot();
    for(unsigned character=0;character<5;++character){images[character]=a.content.world_image("relics",keys[character]);CHECK(images[character]>=0);CHECK(a.relic_face_image(WorldRelicYummyCookie,int(character))==images[character]);
        for(unsigned previous=0;previous<character;++previous)CHECK(images[character]!=images[previous]);}
    CHECK(cookie_clock_calls==0&&cookie_entropy_calls==0);unchanged();
    const unsigned selected=a.canonical_cookie_variant();CHECK(selected<5&&cookie_clock_calls==1&&cookie_entropy_calls==0);CHECK(a.relic_face_image(WorldRelicYummyCookie)==images[selected]);unchanged();
    // Canonical/no-owner models retain the process choice across other
    // characters, profile/presentation rebuilds and even another App.
    for(unsigned character=0;character<5;++character){a.game.character=Character(character);a.map_clock_ms+=7919;a.reset_presentation();snapshot();
        CHECK(a.canonical_cookie_variant()==selected);CHECK(a.relic_face_image(WorldRelicYummyCookie)==images[selected]);
        CHECK(a.relic_face_image(WorldRelicYummyCookie,int(character))==images[character]);unchanged();}
    static App other;CHECK(other.canonical_cookie_variant()==selected);CHECK(cookie_clock_calls==1&&cookie_entropy_calls==0);
    a.callbacks=callbacks;
    // Exercise the actual inspector's owner routes, not only image lookup.
    setup();a.game.character=Character::Regent;gain_world_relic(a.game,WorldRelicYummyCookie);a.reset_presentation();
    int position=-1;for(unsigned i=0;i<ui_owned_relic_count(a.game);++i)if(ui_owned_relic_id(a.game,i)==WorldRelicYummyCookie)position=int(i);CHECK(position>=0);snapshot();
    a.open_relic_inspect(WorldRelicYummyCookie,position);CHECK(a.relic_inspect.source==RelicInspectSource::Owned&&a.relic_inspect_character()==int(Character::Regent));advance(350);capture("relic-inspect-cookie-owned-regent");unchanged();
    a.close_relic_inspect();advance(250);a.open_library(1);a.discover_relic(WorldRelicYummyCookie);a.prepare_collection();int index=-1;
    for(unsigned i=0;i<a.collection.count;++i)if(a.collection.ids[i]==WorldRelicYummyCookie)index=int(i);CHECK(index>=0);
    a.select_library_item(index);CHECK(a.relic_inspect.source==RelicInspectSource::Collection&&a.relic_inspect_character()==-1);advance(350);capture("relic-inspect-cookie-canonical");unchanged();
    // NRelicHistory restores the saved player's ownership even while the
    // currently active run uses another character. Its text remains an
    // honest historical snapshot, independent of mutable current facts.
    a.close_relic_inspect();advance(250);a.library=false;a.history_open=true;a.progress.count=1;a.progress.next_history=1;a.progress.history[0]={};
    auto&record=a.progress.history[0];record.character=uint8_t(Character::Silent);record.relic_count=1;record.relics[0]=WorldRelicYummyCookie;a.history_index=0;
    CHECK(a.history_at(0)==&record);a.open_relic_inspect(WorldRelicYummyCookie);CHECK(a.relic_inspect_character()==int(Character::Silent));advance(350);capture("relic-inspect-cookie-history-silent");unchanged();
    a.close_relic_inspect();advance(250);a.history_open=false;
    // Original EventModel.RelicOption assigns its current player before
    // offering the Ancient relic; ordinary unclaimed stock stays ownerless.
    a.game.phase=Phase::Event;a.game.original_events.active=1;a.game.original_events.finished=0;a.game.original_events.id=OriginalEventId::Tanx;a.game.original_events.offer_count=1;a.game.original_events.offers[0]=WorldRelicYummyCookie;
    snapshot();a.open_relic_inspect(WorldRelicYummyCookie);CHECK(a.relic_inspect_character()==int(Character::Regent));unchanged();a.close_relic_inspect();advance(250);
    a.game.phase=Phase::Shop;snapshot();a.open_relic_inspect(WorldRelicYummyCookie);CHECK(a.relic_inspect_character()==-1);unchanged();a.close_relic_inspect();advance(250);
    CHECK(cookie_clock_calls==1&&cookie_entropy_calls==0);
}
static void original_motion(){
    scenario="original geometry and tweens";
    RelicInspectPresentation view;view.begin(1000);CHECK(view.backstop(1000)==255&&view.backstop(1250)==230);
    CHECK(view.popup_alpha(1000)==0&&view.popup_alpha(1250)==255);CHECK(view.y(1000)==82&&view.y(1250)==44);
    CHECK(view.arrow_alpha(1100)==0&&view.arrow_alpha(1350)==255);CHECK(view.arrow_x(0,1000)==86&&view.arrow_x(1,1350)==229);
    CHECK(view.arrow_x(0,1250)<67);view.close(1350);CHECK(view.closing(1599)&&!view.closing(1600));CHECK(view.popup_alpha(1450)==0&&view.backstop(1475)==115);
    view.begin(1600);CHECK(view.backstop(1600)==0);CHECK(view.popup_brightness(1725)==127);view.direction=-1;view.slid=1700;CHECK(view.x(1700)==59&&view.x(1950)==78&&view.y(1700)==44);
    // Scene anchors are independent from the popup's animated translation.
    CHECK(relic_inspect_layout::FrameX==reference_viewport::project(840-152));CHECK(relic_inspect_layout::FrameY==reference_viewport::project(630-400+432-265));
    CHECK(relic_inspect_layout::FrameSize==reference_viewport::project(304));CHECK(relic_inspect_layout::PopupSize==reference_viewport::project(864));
}
static void inventory(){
    scenario="owned inventory modal";setup();snapshot();a.input(Key::Relics);CHECK(!a.panel&&a.hud_focus==HudRelicFocusFirst);
    a.input(Key::Confirm);CHECK(a.detail_item==ui_owned_relic_id(a.game,0));CHECK(a.relic_inspect.source==RelicInspectSource::Owned&&a.relic_inspect.count==int(ui_owned_relic_count(a.game)));unchanged();
    Canvas white{a};white.rect(0,0,320,240,0xffff);item_detail_screen(white);CHECK(a.pixels[0]==0);advance(350);
    const auto*h=a.hit_at(160,145);CHECK(h&&h->kind==HitKind::None);click(160,145);CHECK(a.detail_item>=0);unchanged();
    CHECK(region(UiAction::InspectPreviousRelic,false).kind==HitKind::None);CHECK(region(UiAction::InspectNextRelic).x==229);
    for(Key key:{Key::Confirm,Key::Detail,Key::EndTurn,Key::Inspect,Key::Map,Key::Collection,Key::Potions,Key::Relics,Key::Save,Key::Load,Key::Quit}){a.input(key);CHECK(a.detail_item>=0&&!a.quit&&!a.library&&!a.panel);unchanged();}
    const int focused=a.hud_focus;
    for(unsigned bank=0;bank<2;++bank){a.keyboard_only=bank!=0;
        for(InputCommand command:{InputCommand::Card1,InputCommand::TopPanel,InputCommand::EndTurn,InputCommand::Peek}){
            const uint16_t key=a.input_bindings.keys[bank][unsigned(command)];
            // MKb has no TopPanel binding. The real keyboard-only binding
            // below still verifies that the inspector consumes this action.
            if(key){CHECK(a.physical_key(key,true));CHECK(a.physical_key(key,false));}
            else{CHECK(!a.physical_key(key,true));CHECK(!a.physical_key(key,false));}
            unchanged();CHECK(!a.choice_peeking&&a.hud_focus==focused);
        }
    }a.keyboard_only=false;
    const auto count=a.relic_inspect.count;for(int i=1;i<count;++i){action(UiAction::InspectNextRelic);CHECK(a.detail_owned==i&&a.detail_item==ui_owned_relic_id(a.game,unsigned(i)));advance(350);unchanged();}
    CHECK(region(UiAction::InspectNextRelic,false).kind==HitKind::None);a.input(Key::Right);CHECK(a.detail_owned==count-1);unchanged();
    const uint16_t left=a.input_bindings.keys[0][unsigned(InputCommand::Left)];CHECK(a.physical_key(left,true));const int once=a.detail_owned;CHECK(a.physical_key(left,true));CHECK(a.detail_owned==once);CHECK(a.physical_key(left,false));
    advance(350);capture("relic-inspect-owned");const int old=a.detail_item;click(12,120);CHECK(a.detail_item<0&&a.detail_owned<0&&a.relic_inspect.model==old&&a.relic_inspect.closing(a.map_clock_ms));unchanged();
    advance(50);
    // Closing removes the Backstop filter, not Popup's MouseFilter=Stop.
    // Seed genuine underlying controls on both sides of that boundary.
    const auto closing_hits=[&]{a.hit_count=0;a.add_hit(0,0,320,240,HitKind::Command,int(Key::EndTurn));Canvas c{a};item_detail_screen(c);};
    closing_hits();h=a.hit_at(160,145);CHECK(h&&h->kind==HitKind::None);h=a.hit_at(12,120);CHECK(h&&h->kind==HitKind::Command);
    advance(50);CHECK(a.relic_inspect.popup_alpha(a.map_clock_ms)==0);advance(150);CHECK(!a.relic_inspect.closing(a.map_clock_ms));
    closing_hits();h=a.hit_at(160,145);CHECK(h&&h->kind==HitKind::Command);unchanged();
    // NGame retains one inspector node across run/presentation rebuilds.
    // Reset transient presentation without replaying its first black frame.
    a.reset_presentation();CHECK(a.relic_inspect.ever_opened&&!a.relic_inspect.bound);a.open_owned_relic(0);
    CHECK(a.detail_item>=0&&a.relic_inspect.backstop(a.map_clock_ms)==0);a.input(Key::Back);advance(250);unchanged();
    // Opening a modal while another native release is held discards that
    // release; it cannot end a turn or toggle the underlying Peek overlay.
    const uint16_t peek=a.input_bindings.keys[0][unsigned(InputCommand::Peek)];CHECK(a.physical_key(peek,true));a.open_owned_relic(0);CHECK(a.physical_key(peek,false));CHECK(!a.choice_peeking);unchanged();a.input(Key::Back);
}
static void collection(){
    scenario="collection inspector source list";setup();a.open_library(1);a.prepare_collection();CHECK(a.collection.count>RelicKinds);
    const int first=a.collection.ids[0];a.progress.discovered_relics[first/8]&=uint8_t(~(1u<<(first%8)));snapshot();
    a.select_library_item(0);CHECK(a.detail_item==first&&a.relic_inspect.source==RelicInspectSource::Collection);CHECK(a.relic_inspect.count==a.collection.count);advance(350);
    char name[256]{},body[4096]{},flavor[1024]{};relic_inspect_text(a,RelicId(first),name,sizeof name,body,sizeof body,flavor,sizeof flavor);
    if(actual){CHECK(!std::strcmp(name,a.content.world_string("inspect_relic_screen","UNDISCOVERED_TITLE")));CHECK(*body&&!*flavor);}
    CHECK(!a.library_item_seen(1,first));CardHoverTarget owner{};CHECK(!inspect_relic_hover_target(a,owner));capture("relic-inspect-undiscovered");unchanged();
    a.input(Key::Right);CHECK(a.detail_item==a.collection.ids[1]&&a.library_selected==0);a.input(Key::Left);CHECK(a.detail_item==first);a.input(Key::Left);CHECK(a.detail_item==first);unchanged();a.input(Key::Back);CHECK(a.library&&a.library_selected==0);
    a.discover_relic(WorldRelicFencingManual);int selected=-1;for(unsigned i=0;i<a.collection.count;++i)if(a.collection.ids[i]==WorldRelicFencingManual)selected=int(i);CHECK(selected>=0);
    a.select_library_item(selected);advance(350);CHECK(inspect_relic_hover_target(a,owner)&&owner.canonical_relic&&owner.inspect_relic);
    HoverTipList tips{};collect_keyword_hover_tips(a,owner,tips);CHECK(tips.find(HoverTipKind::Relic,"FENCING_MANUAL")<0);CHECK(tips.find(HoverTipKind::Static,"FORGE")>=0);CHECK(tips.find(HoverTipKind::Card,"SOVEREIGN_BLADE")>=0);
    if(actual){const auto page=card_hover_page(a,owner,tips);bool text=false,card=false;for(unsigned i=0;i<page.count;++i){const auto&p=page.pieces[i];if(tips.items[p.tip].kind==HoverTipKind::Card){CHECK(p.box.x==46);card=true;}else{CHECK(p.box.x==227);text=true;}}CHECK(text&&card);}capture("relic-inspect-associated-forge");unchanged();
    a.input(Key::Back);a.input(Key::Back);CHECK(!a.library);a.open_library(2);a.prepare_collection();a.select_library_item(0);CHECK(a.library&&a.detail_item<0);a.input(Key::Detail);CHECK(a.detail_item<0);unchanged();
    a.input(Key::Back);a.input(Key::Back);a.game.potions[0]=Potion::Fire;snapshot();a.hud_focus=2;a.input(Key::Detail);CHECK(a.detail_item<0);a.input(Key::Confirm);CHECK(a.panel==4);a.input(Key::Detail);CHECK(a.panel==4&&a.detail_item<0);unchanged();
}
static void source_shader(){
    scenario="source HSV matrix";
    const double source[3][3]={{.2989,.5870,.1140},{.5959,-.2774,-.3216},{.2115,-.5229,.3114}};
    double inverse[3][6]{};for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)inverse[r][c]=source[r][c];inverse[r][r+3]=1;}
    for(unsigned c=0;c<3;++c){const double divisor=inverse[c][c];for(double&v:inverse[c])v/=divisor;for(unsigned r=0;r<3;++r)if(r!=c){const double factor=inverse[r][c];for(unsigned j=0;j<6;++j)inverse[r][j]-=factor*inverse[c][j];}}
    const double materials[6][3]={{.95,.25,.9},{.426,.8,1.1},{1,.8,1.15},{.525,2.5,.85},{.23,.75,.9},{.875,3,.9}};
    for(unsigned group=0;group<6;++group)for(unsigned packed=0;packed<65536;packed+=73)for(unsigned brightness:{32u,127u,255u}){
        const unsigned rarity=group?group+2:2;const double input[]={double(packed>>11)/31,double((packed>>5)&63)/63,double(packed&31)/31};
        double yiq[3]{};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)yiq[r]+=source[r][c]*input[c];
        const auto&m=materials[group];const double angle=(1-m[0])*6.283185;
        const double converted[]={yiq[0]*m[2],(yiq[1]*std::cos(angle)-yiq[2]*std::sin(angle))*m[1]*m[2],(yiq[1]*std::sin(angle)+yiq[2]*std::cos(angle))*m[1]*m[2]};
        const auto got=relic_inspect_frame_color(uint16_t(packed),rarity,brightness);const unsigned values[]={unsigned(got>>11),unsigned((got>>5)&63),unsigned(got&31)},maxima[]={31,63,31};
        for(unsigned channel=0;channel<3;++channel){double value=0;for(unsigned k=0;k<3;++k)value+=inverse[channel][k+3]*converted[k];value*=double(brightness)/255;if(value<0)value=0;if(value>1)value=1;
            CHECK(std::abs(int(values[channel])-int(std::lround(value*maxima[channel])))<=1);}
    }
}
static void real_art_and_text(){
    if(!actual)return;scenario="actual original artwork and every relic text";CHECK(a.content.has_inspection_assets());setup();a.open_library(1);a.prepare_collection();Canvas c{a};
    CHECK(a.content.font_size(7,5,false,true)==7&&a.content.font_size(4,6)==4);
    const auto frame=a.content.image(a.content.ui_image(I9_RelicInspectFrame));CHECK(frame.pixels&&frame.w==58&&frame.h==58&&frame.format<=3);
    // Independent per-pixel source alpha/material composition, including
    // transparent frame texels. No false success from an empty picture.
    std::vector<uint16_t>expected(58*58);unsigned visible=0;
    for(unsigned i=0;i<58*58;++i){const auto*p=frame.palette?frame.palette+2*frame.pixels[i]:frame.pixels+2*i;const unsigned alpha=frame.alpha?frame.alpha[i]:255;visible+=alpha!=0;
        expected[i]=blend(0x39e7,relic_inspect_frame_color(uint16_t(p[0]|unsigned(p[1])<<8),4),alpha);}
    CHECK(visible>500);c.rect(0,0,320,240,0x39e7);relic_inspect_frame(c,131,76,4);
    for(unsigned y=0;y<58;++y)for(unsigned x=0;x<58;++x)CHECK(a.pixels[(76+y)*320+131+x]==expected[y*58+x]);
    bool rarity_seen[8]{};
    for(unsigned i=0;i<a.collection.count;++i){const auto id=RelicId(a.collection.ids[i]);a.discover_relic(id);a.close_relic_inspect();a.select_library_item(int(i));advance(350);
        char name[256]{},body[4096]{},flavor[1024]{};relic_inspect_text(a,id,name,sizeof name,body,sizeof body,flavor,sizeof flavor);CHECK(*name&&*body);CHECK(!std::strstr(flavor,"[center]")&&!std::strstr(flavor,"[/center]"));
        Canvas title{a};title.font_face=5;title.sparse_scene_sizes=true;CHECK(title.width(name,7)<=147);
        for(unsigned field=0;field<3;++field){Canvas text{a};text.font_face=field==0?5:field==2?6:0;text.bold_face=field==1?5:0;const int size=field==0?7:field==2?4:5;RichReader reader(field==0?name:field==1?body:flavor);RichChar ch{};
            while(reader.next(ch)){if(ch.codepoint=='\n'||text_format_control(ch.codepoint)||inline_icon(ch.codepoint))continue;
                const unsigned face=text.font_style(ch);const bool found=exact_glyph(ch.codepoint,size,face);
                if(!found)std::fprintf(stderr,"missing exact relic glyph: relic=%u field=%u cp=U+%04x size=%u face=%u\n",unsigned(id),field,ch.codepoint,unsigned(size),face);
                CHECK(found);const auto glyph=a.content.glyph(ch.codepoint,size,face);CHECK(glyph.pixels&&glyph.advance>0);}}
        const unsigned rarity=world_relic_definitions[id].rarity;if(rarity<8&&!rarity_seen[rarity]){rarity_seen[rarity]=true;char shot[64]{};std::snprintf(shot,sizeof shot,"relic-inspect-rarity-%u",rarity);capture(shot);}
        CHECK(a.content.healthy());
    }
    a.close_relic_inspect();
}
int main(int argc,char**argv){
    Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);actual=true;}else{
        data=fixture();const unsigned ui=40+little_word(data.data()+16)*16+little_word(data.data()+20)*16+little_word(data.data()+24)*8+little_word(data.data()+28)*16+CardKinds*16;
        const unsigned variants[]={6,8,9,10,12};for(unsigned i=0;i<5;++i)put(data,ui+(702+i)*4,variants[i]);
    }
    CHECK(a.init(data.data(),data.size(),{}));if(actual){const unsigned images=little_word(data.data()+16),glyphs=little_word(data.data()+20);actual_glyph_keys.reserve(glyphs);
        for(unsigned i=0;i<glyphs;++i){const auto*row=data.data()+40+images*16+i*16;actual_glyph_keys.push_back((uint32_t(row[8]|unsigned(row[9])<<8)<<24)|little_word(row));}
        CHECK(std::is_sorted(actual_glyph_keys.begin(),actual_glyph_keys.end()));}
    if(argc>2)captures=argv[2];original_motion();inventory();cookie_ownership();collection();source_shader();real_art_and_text();CHECK(a.content.healthy());
    std::printf("relic inspection: %u checks passed (%s)\n",checks,actual?"actual":"synthetic");
}
