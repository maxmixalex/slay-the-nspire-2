#include "app.hpp"
#include "host_input_latch.hpp"
#include "host_physical_input.hpp"
#include "lzma2_block.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static unsigned checks,polls,downs,ups,directions;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"foreground pointer:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static CalculatorInputLatch latch;
static CalculatorPadInput captured,dispatched;
static TouchpadCursor cursor;
static uint16_t matrix[8],previous[8];
static uint8_t repeats[CalculatorPhysicalKeyCount];
static uint32_t revision,clock_before;
static int initial_x,initial_y,target_x,target_y;
enum Gesture {Click,Drag,Tap,Invalid};
static Gesture gesture;
static TouchSample released{5000,5000,10000,10000,false,false,true};
struct Host {
    decltype(app.pointer)&pointer=app.pointer;
    void pointer_cancel(bool keep=false){app.pointer_cancel(keep);}
    void pointer_move(int x,int y){app.pointer_move(x,y);}
    void pointer_button(bool down){if(down)++downs;else ++ups;app.pointer_button(down);}
    void physical_key(uint16_t key,bool down){if(down)++directions;app.physical_key(key,down);}
};
static Host host;
static void setup(Gesture kind){
    app.reset_presentation();new_run(app.game,7,true);app.title=app.paused=false;
    app.progress.tutorial_flags=FtueFresh|FtueSeenMask;app.calculator_keyboard=true;
    app.input_bindings=default_input_bindings(true);app.physical_keys_cancel();
    app.input(Key::Back);app.render();CHECK(app.paused);
    HitRegion button{};for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Menu&&!app.hits[i].value)button=app.hits[i];
    CHECK(button.enabled&&button.w>0);target_x=button.x+button.w/2;target_y=button.y+button.h/2;
    initial_x=target_x-(kind==Tap?0:75);initial_y=target_y-(kind==Tap?0:30);
    CHECK(initial_x>=0&&initial_y>=0);app.pointer_move(initial_x,initial_y);
    cursor.reset(initial_x,initial_y);cursor.sample(released,0);
    std::memset(matrix,0,sizeof matrix);std::memset(previous,0,sizeof previous);std::memset(repeats,0,sizeof repeats);
    latch.reset(matrix,true,0,released);captured={};dispatched={};gesture=kind;polls=downs=ups=directions=0;
    revision=app.game.revision;clock_before=app.map_clock_ms;
}
static void collect(){
    const unsigned i=polls++;TouchSample touch=released;uint32_t ms=100+i*100;
    if(gesture==Tap){touch.contact=i==0;ms=i?180+i-1:100;}
    else{
        touch.contact=i<5;if(i>=1){touch.x=6675;touch.y=4107;}
        touch.pressed=i==2||i==3;
        if(gesture==Drag&&i>=3)touch.x=9500;
        if(gesture==Invalid&&i==3)touch={};
        if(i==4)ms=480;if(i>=5)ms=580+(i-5)*100;
    }
    latch.sample(matrix,captured.capture(touch,false),touch,ms);
    CHECK(app.paused&&!app.pointer.held&&app.pointer.x==initial_x&&app.pointer.y==initial_y);
    CHECK(app.game.revision==revision&&app.map_clock_ms==clock_before);
}
static void replay(){
    const unsigned pending=latch.count;CHECK(pending>=2&&pending<CalculatorInputLatch::Capacity);
    for(unsigned i=0;i<pending;++i){uint16_t pad=0;TouchSample touch;uint32_t ms=0;
        CHECK(latch.pop(matrix,&pad,&touch,&ms));
        dispatch_calculator_sample(dispatched,cursor,touch,pad,ms,host,false,[&](bool repeat){
            calculator_physical_edges(matrix,previous,repeats,false,[&](uint16_t key,bool down){app.physical_key(key,down);},repeat);
            std::memcpy(previous,matrix,sizeof previous);
        });
    }
    CHECK(!latch.count&&!app.pointer.held&&!dispatched.dispatched&&!directions);
    if(gesture==Click||gesture==Tap){CHECK(!app.paused&&downs==1&&ups==1);CHECK(app.pointer.x==target_x&&app.pointer.y==target_y);}
    else CHECK(app.paused&&downs==1&&ups<=1);
}
static void contract(Gesture kind,bool decode){
    setup(kind);SaveData before,after;CHECK(save(app.game,before));
    if(decode){
        std::vector<uint8_t> packed(65540),out(65536);packed[0]=1;packed[1]=packed[2]=255;
        for(unsigned i=0;i<65536;++i)packed[i+3]=uint8_t(i*29);packed.back()=0;
        set_lzma2_foreground_poll(collect);CHECK(decode_lzma2_block(out.data(),out.size(),out.size(),packed.data(),packed.size(),14));set_lzma2_foreground_poll(nullptr);
        for(unsigned i=0;i<65536;++i)CHECK(out[i]==uint8_t(i*29));
    }else{app.callbacks.foreground_poll=collect;app.render();app.callbacks.foreground_poll=nullptr;}
    CHECK(polls>=6);replay();CHECK(save(app.game,after));
    CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
static void overflow_and_clock(){
    CalculatorForegroundPoll time;time.mark(16);time.mark(16-32768u);CHECK(time.milliseconds==1000&&!time.fraction);
    time.mark(16-65536u);CHECK(time.milliseconds==2000);
    CalculatorInputLatch q;uint16_t keys[8]{};q.reset(keys,true,0,released);
    TouchSample t=released;t.contact=t.pressed=true;
    for(unsigned i=0;i<100;++i){t.x=uint16_t(5000+i);q.sample(keys,0,t,i);}
    q.sample(keys,0,released,1234);CHECK(q.count==32);
    uint32_t last=0;while(q.pop(keys,nullptr,&t,&last)){}
    CHECK(!t.contact&&!t.pressed&&last==1234);
    q.reset(keys,false,0,released);t.contact=true;q.sample(keys,0,t,1400);CHECK(!q.count);
}
static void card_transaction(bool invalid_release){
    app.reset_presentation();new_run(app.game,7,true);app.title=app.paused=false;
    app.progress.tutorial_flags=FtueFresh|FtueSeenMask;app.calculator_keyboard=true;app.keyboard_only=false;
    app.input_bindings=default_input_bindings(true);app.physical_keys_cancel();
    app.game.energy=99;app.game.enemy_count=3;for(auto&enemy:app.game.enemies)enemy.hp=enemy.max_hp=100;
    app.cursor=-1;for(unsigned i=0;i<app.game.hand.count;++i){const auto id=app.game.hand.ids[i];
        if(card_targets_enemy(app.game,app.game.cards[id],id)&&can_play_card(app.game,id)){app.cursor=int(i);break;}}
    CHECK(app.cursor>=0);const auto id=app.game.hand.ids[app.cursor];const auto digit=uint16_t(app.cursor==9?'0':'1'+app.cursor);
    app.render();app.render();app.pointer_move(160,120);app.target=1;cursor.reset(160,120);cursor.sample(released,0);
    std::memset(matrix,0,sizeof matrix);std::memset(previous,0,sizeof previous);std::memset(repeats,0,sizeof repeats);
    latch.reset(matrix,true,0,released);captured={};dispatched={};downs=ups=directions=0;
    auto physical=[&](uint16_t code,bool down,uint32_t ms){bool found=false;
        calculator_physical_keys([&](unsigned,unsigned row,unsigned mask,uint16_t value){if(value!=code)return;found=true;
            if(down)matrix[row]|=mask;else matrix[row]&=~mask;});CHECK(found);latch.sample(matrix,0,released,ms);};
    physical(digit,true,100);physical(digit,false,150);
    TouchSample touch{9000,5000,10000,10000,true,true,true};
    for(unsigned i=0;i<4;++i){touch.x=uint16_t(9000+i*50);latch.sample(matrix,captured.capture(touch,false),touch,200+i*500);}
    touch=invalid_release?TouchSample{}:released;latch.sample(matrix,captured.capture(touch,false),touch,1800);
    physical(13,true,1900);physical(13,false,1950);
    const auto before=app.game.revision;const unsigned pending=latch.count;
    for(unsigned i=0;i<pending;++i){uint16_t pad=0;uint32_t ms=0;CHECK(latch.pop(matrix,&pad,&touch,&ms));
        dispatch_calculator_sample(dispatched,cursor,touch,pad,ms,host,false,[&](bool repeat){
            calculator_physical_edges(matrix,previous,repeats,false,[&](uint16_t code,bool down){app.physical_key(code,down);},repeat);
            std::memcpy(previous,matrix,sizeof previous);
        });
        if(i<7){if(app.keyboard_card!=id||app.target!=(i<2?1:2))std::fprintf(stderr,"pad case %u step %u card %d expected %u target %d\n",unsigned(invalid_release),i,app.keyboard_card,unsigned(id),app.target);CHECK(app.keyboard_card==id&&app.target==(i<2?1:2));}
    }
    CHECK(!latch.count&&!dispatched.dispatched&&!app.pointer.held&&!downs&&!ups&&directions==1);
    CHECK(app.game.revision>before&&app.game.enemies[2].hp<100&&app.keyboard_card<0);
    const auto committed=app.game.revision;app.physical_key(13,false);CHECK(app.game.revision==committed);
}
int main(){
    overflow_and_clock();
    for(bool decode:{false,true})for(Gesture kind:{Click,Drag,Tap,Invalid})contract(kind,decode);
    card_transaction(false);card_transaction(true);
    std::printf("foreground pointer: %u checks; real UI contact anchor/motion/click/tap/drag/cancel across render and CPU decode; one activation, unchanged save; side-pad selection/release/invalid/Enter; FIFO %zu bytes\n",checks,sizeof(CalculatorInputLatch));
}
