#include "app.hpp"
#include "host_input_latch.hpp"
#include "host_physical_input.hpp"
#include "settings_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static App app;
static unsigned checks,downs,ups,keys,writes;
static UiPreferences stored;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"calculator pointer UI26:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static CalculatorPadInput capture,dispatch;
static TouchpadCursor cursor;
static TouchSample touch{5000,5000,10000,10000,false,false,true};
static uint32_t now;
struct Host {
    PointerState&pointer=app.pointer;
    void pointer_cancel(bool keep=false){app.pointer_cancel(keep);}
    void pointer_move(int x,int y){app.pointer_move(x,y);}
    void pointer_button(bool down){if(down)++downs;else ++ups;app.pointer_button(down);}
    void physical_key(uint16_t code,bool down){if(down)++keys;app.physical_key(code,down);}
} host;
static bool write_preferences(const UiPreferences&p){stored=p;++writes;return true;}
static bool read_preferences(UiPreferences&p){p=stored;return true;}
static void sample(unsigned delay=33){
    now+=delay;app.map_clock_ms=now;
    dispatch_calculator_sample(dispatch,cursor,touch,capture.capture(touch,false),now,host,false,[](bool){},app.tap_to_click,app.mouse_speed);
}
static void setup(bool keyboard_only=false){
    app.reset_presentation();new_run(app.game,7,true);app.calculator_keyboard=true;
    app.input_bindings=default_input_bindings(true);app.physical_keys_cancel();
    app.title=true;app.settings_open=true;app.settings_tab=0;app.settings_item=settings_layout::TapRow;
    app.settings_top=settings_layout::GeneralMaximum;app.keyboard_only=keyboard_only;app.tap_to_click=false;app.mouse_speed=2;
    app.pointer.visible=true;app.pointer.click_until=0;app.map_clock_ms=now=1000;app.render();
    HitRegion hit{};for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::SettingsTapToClick))hit=app.hits[i];
    CHECK(hit.enabled&&hit.w>0);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);
    capture={};dispatch={};touch={5000,5000,10000,10000,false,false,true};
    cursor.reset(app.pointer.x,app.pointer.y);cursor.sample(touch,now,false,2,true);downs=ups=keys=0;
}
static void protocol(){
    uint8_t bytes[11]={};bytes[2]=0x12;bytes[3]=0x34;bytes[4]=0x05;bytes[5]=0x67;bytes[10]=1;
    auto s=decode_touchpad_report(bytes,10000,10000);CHECK(s.valid&&!s.contact&&s.pressed&&s.x==0x1234&&s.y==0x567);
    bytes[0]=1;bytes[10]=0;s=decode_touchpad_report(bytes,10000,10000);CHECK(s.contact&&!s.pressed);
    // The CX matrix has no second physical center/arrow key to dispatch.
    unsigned centers=0;calculator_physical_keys([&](unsigned,unsigned,unsigned,uint16_t code){centers+=code==InputPad;});CHECK(!centers);
}
static void center_click(bool keyboard_only){
    setup(keyboard_only);const int x=app.pointer.x,y=app.pointer.y;
    touch.contact=true;sample();touch.x+=200;touch.y-=200;touch.pressed=true;sample();
    CHECK(downs==1&&!ups&&!keys&&app.pointer.held&&!app.tap_to_click);
    CHECK(app.pointer.x==x&&app.pointer.y==y&&app.pointer.down_x==x&&app.pointer.down_y==y);
    CHECK(app.pointer.click_until==now+180);touch.x-=100;touch.y+=100;touch.pressed=false;sample();
    CHECK(downs==1&&ups==1&&!keys&&!app.pointer.held&&app.tap_to_click);
    CHECK(app.pointer.x==x&&app.pointer.y==y);touch.contact=false;sample();CHECK(downs==1&&ups==1);
    // A click shorter than a frame must remain visibly acknowledged after up.
    CHECK(int32_t(app.pointer.click_until-now)>0);app.render();
    const int r=3+(180-int(app.pointer.click_until-now))/24;
    const uint16_t lit=app.pixels[(y-r)*ScreenW+x-r];
    app.pointer.click_until=0;app.render();CHECK(app.pixels[(y-r)*ScreenW+x-r]!=lit);
}
static void center_keyboard(bool keyboard_only){
    setup(keyboard_only);app.pointer.visible=false;cursor.reset(app.pointer.x,app.pointer.y);cursor.sample(touch,now,false,2,false);
    touch.contact=true;sample();touch.pressed=true;touch.x+=180;sample();
    CHECK(keys==1&&!downs&&!ups&&app.tap_to_click&&!app.pointer.visible);
    for(unsigned i=0;i<16;++i){touch.x=uint16_t(5200+i*200);sample(100);}
    CHECK(keys==1&&!downs&&!ups&&app.tap_to_click&&!app.pointer.visible);
    touch.pressed=false;sample();touch.contact=false;sample();CHECK(!dispatch.dispatched&&!app.pointer.visible);
}
static void taps_and_speed(){
    setup();touch.contact=true;sample();touch.contact=false;sample(80);CHECK(!downs&&!ups&&!app.tap_to_click);
    app.tap_to_click=true;touch.contact=true;sample();touch.contact=false;sample(80);
    CHECK(downs==1&&ups==1&&!app.tap_to_click); // Actual settings checkbox changed once.
    for(unsigned speed=0;speed<5;++speed){TouchpadCursor c;TouchSample s{5000,5000,10000,10000,false,false,true};
        c.sample(s,0,false,speed);s.contact=true;c.sample(s,33,false,speed);s.x+=1000;
        const auto p=c.sample(s,66,false,speed);CHECK(p.moved&&p.x==160+int(224+112*speed)/10&&p.y==120);}
}
static void navigation_ownership(){
    setup();const int x=app.pointer.x,y=app.pointer.y;
    // A real mechanical direction hides the pointer and moves focus once.
    touch.x=5000;touch.y=1000;touch.contact=touch.pressed=true;sample();
    CHECK(keys==1&&!downs&&!app.pointer.visible&&app.settings_item==settings_layout::MouseSpeedRow);
    touch.pressed=false;sample();for(unsigned i=0;i<12;++i){touch.x=uint16_t(5000+(i&1?80:0));sample();}
    touch.x=6000;sample();CHECK(!app.pointer.visible&&app.pointer.x==x&&app.pointer.y==y&&!downs);
    touch.contact=false;sample();touch.x=5000;touch.y=5000;touch.contact=true;sample();
    // Net displacement, not absolute accumulated jitter, establishes intent.
    for(unsigned i=0;i<30;++i){touch.x=uint16_t(5000+(i&1?80:0));sample();CHECK(!app.pointer.visible);}
    touch.x=5200;sample();CHECK(app.pointer.visible&&app.pointer.x==x+8&&!downs);
    // Matrix confirmation arriving during contact consumes that contact too.
    uint16_t matrix[8]{},previous[8]{};uint8_t repeat[CalculatorPhysicalKeyCount]{};
    calculator_physical_keys([&](unsigned,unsigned row,unsigned mask,uint16_t code){if(code==13)matrix[row]|=mask;});
    dispatch_calculator_sample(dispatch,cursor,touch,0,now+33,host,false,[&](bool allow){
        calculator_physical_edges(matrix,previous,repeat,false,[&](uint16_t code,bool down){app.physical_key(code,down);},allow);
    });CHECK(!app.pointer.visible);const int after=app.pointer.x;touch.x+=300;sample();CHECK(!app.pointer.visible&&app.pointer.x==after);
}
static void drag_and_invalid(){
    setup();touch.contact=true;sample();touch.pressed=true;sample();const int x=app.pointer.x;
    touch.x+=300;sample();CHECK(app.pointer.held&&app.pointer.dragging&&app.pointer.x>x);
    touch.valid=false;sample();CHECK(!app.pointer.held&&!app.tap_to_click);touch.valid=true;touch.contact=touch.pressed=false;sample();CHECK(!ups);
}
static void settings(){
    setup();app.callbacks.save_preferences=write_preferences;app.callbacks.load_preferences=read_preferences;writes=0;
    SaveData before,after;CHECK(save(app.game,before));app.pointer.visible=false;
    app.input(Key::Confirm);CHECK(app.tap_to_click&&stored.tap_to_click&&writes==1);
    app.input(Key::Down);app.input(Key::Left);CHECK(app.mouse_speed==1&&stored.mouse_speed==1);
    for(unsigned i=0;i<8;++i)app.input(Key::Right);CHECK(app.mouse_speed==4&&stored.mouse_speed==4);
    const unsigned count=writes;app.input(Key::Right);CHECK(writes==count);
    app.tap_to_click=false;app.mouse_speed=0;app.restore_preferences();CHECK(app.tap_to_click&&app.mouse_speed==4);
    for(unsigned speed=0;speed<5;++speed)for(bool tap:{false,true}){UiPreferences p;uint8_t bytes[UiPreferencesBytes];p.tap_to_click=tap;p.mouse_speed=speed;
        encode_ui_preferences(p,bytes);UiPreferences q;CHECK(decode_ui_preferences(bytes,sizeof bytes,q)&&q.tap_to_click==tap&&q.mouse_speed==speed);}
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.ui_action(UiAction::SettingsReset);app.frontend_choice=1;app.input(Key::Confirm);
    CHECK(!app.tap_to_click&&app.mouse_speed==2&&!stored.tap_to_click&&stored.mouse_speed==2);
}
int main(){protocol();center_click(false);center_click(true);center_keyboard(false);center_keyboard(true);taps_and_speed();navigation_ownership();drag_and_invalid();settings();
    std::printf("calculator pointer UI26: %u checks; protocol, center ownership/position, drift/clutch, tap, five speeds, click feedback, persistent settings, unchanged saves\n",checks);}
