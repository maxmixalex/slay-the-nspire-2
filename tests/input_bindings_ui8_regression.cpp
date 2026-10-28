#include "app.hpp"
#include "host_physical_input.hpp"
#include "settings_layout.hpp"
#include "settings_focus_oracle.hpp"
#include "world_run.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static uint8_t disk[UiPreferencesBytes];
static bool present=false,write_ok=true;
static unsigned writes,checks;
static const char*captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"input_bindings_ui8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool read_preferences(UiPreferences&out){return present&&decode_ui_preferences(disk,sizeof disk,out);}
static bool write_preferences(const UiPreferences&value){++writes;if(!write_ok)return false;encode_ui_preferences(value,disk);present=true;return true;}
static void press(uint16_t key){CHECK(app.physical_key(key,true));CHECK(app.physical_key(key,false));}
static void settle(){for(unsigned i=0;i<32;++i)app.advance(100);app.render();}
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void prepare_combat(){
    snapshot();const auto animation=app.animation;app.render();
    // Real artwork enters a bounded preparation gate. Its first visible
    // frame must be presented before updates can release ordinary input.
    for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
    CHECK(!app.combat_preparation.active);if(app.content.current_profile())CHECK(app.content.healthy());
    CHECK(app.animation==animation);unchanged();
}
static void capture(const char*name){if(!captures)return;app.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(uint16_t p:app.pixels){uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static HitRegion hit(unsigned command){app.render();for(int i=app.hit_count-1;i>=0;--i)if(app.hits[i].kind==HitKind::InputBinding&&app.hits[i].value==int(command))return app.hits[i];CHECK(false);return {};}
static void click(unsigned command){const auto h=hit(command);CHECK(h.enabled);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static void legacy(){
    UiPreferences loaded{};const uint8_t old1[]={'N','S','P','F',1,1,0,0},old2[]={'N','S','P','F',2,12,2,0};
    CHECK(decode_ui_preferences(old1,sizeof old1,loaded)&&loaded.fast_mode&&!loaded.bindings_saved&&!loaded.skip_intro_logo);
    CHECK(decode_ui_preferences(old2,sizeof old2,loaded)&&loaded.keyboard_only&&!loaded.bindings_saved&&!loaded.skip_intro_logo);
    UiPreferences custom{};custom.show_run_timer=false;CHECK(rebind_input(custom.bindings,0,unsigned(InputCommand::Map),'Y'));CHECK(rebind_input(custom.bindings,1,unsigned(InputCommand::Select),'Q'));
    for(unsigned version=3;version<=5;++version){encode_ui_preferences(custom,disk);disk[4]=uint8_t(version);
        CHECK(decode_ui_preferences(disk,sizeof disk,loaded)&&loaded.bindings_saved&&!loaded.skip_intro_logo);
        CHECK(!std::memcmp(&loaded.bindings,&custom.bindings,sizeof custom.bindings));}
    present=false;app.calculator_keyboard=true;app.restore_preferences();CHECK(app.input_bindings.keys[0][unsigned(InputCommand::Up)]==InputUp);
    app.persist_preferences();CHECK(disk[4]==6);app.calculator_keyboard=false;app.restore_preferences();
    CHECK(app.input_bindings.keys[0][unsigned(InputCommand::Map)]==9); // Saved calculator map survives a host/language change.
    app.reset_input_bindings();CHECK(app.input_bindings.keys[0][unsigned(InputCommand::Up)]==InputUp);
    std::memcpy(&loaded.bindings,&app.input_bindings,sizeof loaded.bindings);
    uint8_t broken[UiPreferencesBytes];std::memcpy(broken,disk,sizeof disk);broken[8+2*unsigned(InputCommand::Map)]=0;broken[9+2*unsigned(InputCommand::Map)]=0;
    CHECK(!decode_ui_preferences(broken,sizeof broken,loaded));CHECK(!std::memcmp(&loaded.bindings,&app.input_bindings,sizeof loaded.bindings));
}
static void focus_pixel(unsigned visible_row,unsigned alpha){
    app.render();const unsigned at=(settings_layout::input_row(visible_row)+5)*ScreenW+9;
    const uint16_t painted=app.pixels[at];const auto pointer=app.pointer;const unsigned item=app.settings_item;
    ButtonMotion motions[MaxButtonMotions];std::memcpy(motions,app.button_motions,sizeof motions);
    // Isolate the row coverage over the same-time rendered background. The
    // pixel is left of every glyph and away from the pointer/scrollbar.
    app.pointer.visible=false;app.settings_item=0;
    for(auto&motion:app.button_motions)if(settings_focus::owns(motion.identity))motion=ButtonMotion{};
    app.render();const uint16_t base=app.pixels[at];
    app.pointer=pointer;app.settings_item=item;std::memcpy(app.button_motions,motions,sizeof motions);
    const uint16_t source=uint16_t((183u>>3)<<11|63u<<5|31u);
    const auto mix=[&](unsigned back,unsigned color){return unsigned(std::lround((double(back)*(255-alpha)+double(color)*alpha)/255.0));};
    const uint16_t expected=uint16_t(mix(base>>11,source>>11)<<11|mix((base>>5)&63,(source>>5)&63)<<5|mix(base&31,source&31));
    if(painted!=expected)std::fprintf(stderr,"focus pixel row=%u alpha=%u clock=%u pointer=%d item=%u top=%u base=%04x actual=%04x expected=%04x\n",visible_row,alpha,app.map_clock_ms,app.pointer.visible,app.settings_item,app.settings_top,base,painted,expected);
    CHECK(painted==expected);app.render();
}
static void input_focus_transitions(){
    CHECK(source_settings_focus_timeline());CHECK(app.settings_open&&app.settings_tab==1&&app.settings_top==0);
    app.pointer_move(150,settings_layout::input_row(0)+5);focus_pixel(0,51);
    app.pointer_move(150,settings_layout::input_row(1)+5);focus_pixel(0,51);focus_pixel(1,51);
    app.advance(50);focus_pixel(0,26);focus_pixel(1,51);
    app.input(Key::Down);CHECK(!app.pointer.visible&&app.settings_item==1);focus_pixel(0,51);focus_pixel(1,51);
    app.advance(50);focus_pixel(1,26);app.pointer_move(151,settings_layout::input_row(1)+5);focus_pixel(1,51);
    app.ui_action(UiAction::SettingsScrollDown);app.render();CHECK(app.settings_top==1);focus_pixel(1,51);
    app.ui_action(UiAction::SettingsScrollUp);app.render();CHECK(app.settings_top==0);focus_pixel(1,51);
    // Source Back is the visible portion of its partially offscreen node.
    app.pointer_move(20,226);app.pointer_button(true);app.pointer_button(false);CHECK(!app.settings_open);
    app.advance(100);app.menu=1;app.input(Key::Confirm);CHECK(app.settings_open&&app.settings_tab==0);
    app.ui_action(UiAction::SettingsInputTab);app.pointer_move(300,230);focus_pixel(0,0);focus_pixel(1,0);
    app.pointer_move(150,settings_layout::input_row(0)+5);focus_pixel(0,51);
    app.ui_action(UiAction::SettingsGeneralTab);app.advance(100);app.ui_action(UiAction::SettingsInputTab);
    app.pointer_move(300,230);focus_pixel(0,0);unchanged();
}
static void settings(){
    new_run(app.game,0x1727,false);snapshot();app.title=true;app.menu=1;app.input(Key::Confirm);CHECK(app.settings_open);
    app.ui_action(UiAction::SettingsInputTab);CHECK(app.settings_tab==1);CHECK(!hit(unsigned(InputCommand::Select)).enabled);input_focus_transitions();capture("input-mouse-keyboard");
    click(unsigned(InputCommand::Map));CHECK(app.binding_listening==int(InputCommand::Map)&&app.binding_bank==0);capture("input-listening");
    const unsigned saved=writes;press('a');CHECK(app.settings_open&&app.binding_listening<0&&writes==saved+1);
    CHECK(app.input_bindings.keys[0][unsigned(InputCommand::Map)]=='A'&&app.input_bindings.keys[0][unsigned(InputCommand::Draw)]=='M');unchanged();
    app.ui_action(UiAction::SettingsKeyboardOnly);CHECK(app.keyboard_only);
    app.pointer.visible=false;app.settings_item=unsigned(InputCommand::Select)+1;press(13);CHECK(app.binding_listening==int(InputCommand::Select)&&app.binding_bank==1);
    press('Q');CHECK(app.input_bindings.keys[1][unsigned(InputCommand::Select)]=='Q'&&app.input_bindings.keys[1][unsigned(InputCommand::Deck)]==' ');
    app.settings_item=unsigned(InputCommand::Card10)+1;app.settings_top=settings_layout::InputMaximum;capture("input-last-shortcuts");
    for(unsigned i=InputCommandCount-settings_layout::InputVisible;i<InputCommandCount;++i)CHECK(hit(i).enabled);
    const auto expected=app.input_bindings;app.restore_preferences();CHECK(!std::memcmp(&app.input_bindings,&expected,sizeof expected));unchanged();
    app.ui_action(UiAction::SettingsReset);CHECK(!app.frontend_confirm&&app.settings_open&&valid_input_bindings(app.input_bindings));
    const auto defaults=default_input_bindings();CHECK(!std::memcmp(&app.input_bindings,&defaults,sizeof defaults));
    // Escape is a valid binding. Listening consumes it rather than closing
    // the settings page, then its conflict swaps into Map's former key.
    app.keyboard_only=false;app.pointer.visible=true;app.begin_key_rebind(unsigned(InputCommand::Map));press(27);
    CHECK(app.settings_open&&app.input_bindings.keys[0][unsigned(InputCommand::Map)]==27&&app.input_bindings.keys[0][unsigned(InputCommand::Cancel)]=='M');
    write_ok=false;app.begin_key_rebind(unsigned(InputCommand::Map));press(InputF1);CHECK(app.notice_ticks>0);write_ok=true;
    app.reset_input_bindings();app.ui_action(UiAction::SettingsBack);CHECK(!app.settings_open);unchanged();
}
static void gameplay(){
    new_run(app.game,0x1717,true);app.title=false;app.keyboard_only=false;app.reset_presentation();prepare_combat();settle();CHECK(app.game.phase==Phase::Combat&&app.game.hand.count>=2);
    CHECK(rebind_input(app.input_bindings,0,unsigned(InputCommand::Deck),'A'));
    press('A');CHECK(app.panel==1&&app.pile_tab==0);press('A');CHECK(!app.panel);
    press('D');CHECK(app.panel==1&&app.pile_tab==1);press('D');CHECK(!app.panel);
    press('S');CHECK(app.panel==1&&app.pile_tab==2);press('S');CHECK(!app.panel);
    press('X');CHECK(app.panel==1&&app.pile_tab==3);press('X');CHECK(!app.panel);
    app.reset_input_bindings();snapshot();press(' ');unchanged(); // Original Peek never ends an ordinary turn.
    unsigned chosen=0;while(chosen<app.game.hand.count&&!can_play_card(app.game,app.game.hand.ids[chosen]))++chosen;CHECK(chosen<app.game.hand.count&&chosen<10);
    const uint16_t shortcut=uint16_t(chosen==9?'0':'1'+chosen);CHECK(app.physical_key(shortcut,true));
    CHECK(app.keyboard_card==app.game.hand.ids[chosen]);unchanged();CHECK(app.physical_key(shortcut,true));unchanged();CHECK(app.physical_key(shortcut,false));
    press(27);CHECK(app.keyboard_card<0&&!app.paused);unchanged();
    app.keyboard_only=true;press('W');CHECK(app.hud_focus==0);press('W');CHECK(app.hud_focus<0);unchanged();
}
static void map_release(){
    new_run(app.game,0x7319,false);CHECK(world_begin_run(app.game));app.title=false;app.keyboard_only=false;app.reset_presentation();app.begin_progress();app.open_map_window(true);settle();CHECK(app.map_drawing_ready());
    app.map_drawing_tool(0,true);CHECK(app.map_drawing.keyboard&&app.map_drawing.mode==MapDrawingMode::Draw);
    CHECK(app.physical_key(13,true)&&app.map_drawing.pressed);CHECK(app.physical_key(InputRight,true));app.advance(100);CHECK(app.map_drawing.held_directions!=0);
    // A changed binding/mode between press and release cannot strand ink.
    CHECK(rebind_input(app.input_bindings,0,unsigned(InputCommand::Cancel),InputRight));app.keyboard_only=true;
    CHECK(app.physical_key(InputRight,false));CHECK(!app.map_drawing.held_directions);CHECK(app.physical_key(13,false));CHECK(!app.map_drawing.pressed);
    CHECK(app.physical_key(13,true));app.physical_keys_cancel();CHECK(!app.map_drawing.pressed&&!app.map_drawing.held_directions);
    app.command_key(Key::Confirm,true);CHECK(!app.map_drawing.pressed);
    app.command_key(Key::Confirm,false);CHECK(!app.map_drawing.pressed); // Ordinary toolbar tap includes release.
    app.command_key(Key::Right,true);app.command_key(Key::Right,false);CHECK(!app.map_drawing.held_directions);
    app.command_key(Key::Confirm,true);app.physical_keys_cancel();app.command_key(Key::Confirm,false);CHECK(!app.map_drawing.pressed);
}
static void calculator(){
    uint16_t now[8]{},previous[8]{};uint8_t repeats[CalculatorPhysicalKeyCount]{};unsigned down=0,up=0,last=0;
    const auto emit=[&](uint16_t code,bool pressed){last=code;if(pressed)++down;else ++up;};
    now[4]=0x20;calculator_physical_edges(now,previous,repeats,false,emit);CHECK(down==1&&last=='B');std::memcpy(previous,now,sizeof now);
    for(unsigned i=0;i<12;++i)calculator_physical_edges(now,previous,repeats,false,emit);CHECK(down==2&&last=='B');
    now[4]=0;calculator_physical_edges(now,previous,repeats,true,emit);CHECK(up==1&&last=='B');
    std::memcpy(previous,now,sizeof now);now[6]=0x200;calculator_physical_edges(now,previous,repeats,false,emit);CHECK(down==3&&last==9);
}
int main(int argc,char**argv){
    std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());}if(argc>2)captures=argv[2];
    app.callbacks.load_preferences=read_preferences;app.callbacks.save_preferences=write_preferences;
    legacy();settings();gameplay();map_release();calculator();
    std::printf("Source keyboard maps: %u checks; listening/conflict swaps, two banks, v1/2/3/4/5/6 preferences, actual pile/card input, held-key release and calculator matrix passed\n",checks);
}
