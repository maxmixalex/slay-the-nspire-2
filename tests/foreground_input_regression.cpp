// Use the actual private renderer, followed by the ordinary calculator host
// dispatch adapters. Collecting hardware during a draw must never re-enter App.
#include "../src/app.cpp"
#include "calculator_pad_input.hpp"
#include "host_input_latch.hpp"
#include "host_physical_input.hpp"
#include "lzma2_block.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static unsigned checks,polls,samples;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"foreground input:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static CalculatorInputLatch latch;
static CalculatorForegroundPoll gate;
static CalculatorPadInput capture;
static uint16_t matrix[8],digit;
static uint32_t ticks,revision,game_clock;
static TouchSample touch;
static bool transaction=false;
static void key(uint16_t code,bool down){
    bool found=false;calculator_physical_keys([&](unsigned,unsigned row,unsigned mask,uint16_t value){
        if(code==value){found=true;if(down)matrix[row]|=mask;else matrix[row]&=~mask;}});CHECK(found);
}
static void poll(){
    ++polls;ticks-=132; //4ms of synthetic CPU work per coarse boundary.
    if(!gate.due(ticks,latch.enabled))return;
    if(transaction){switch(samples){
        case 0:key(digit,true);break;case 1:key(digit,false);break;
        case 2:touch.contact=touch.pressed=true;break;
        case 3:touch.contact=touch.pressed=false;break;
        case 4:key(13,true);break;case 5:key(13,false);break;
    }}
    ++samples;latch.sample(matrix,capture.capture(touch,false));
    CHECK(app.game.revision==revision&&app.map_clock_ms==game_clock);
    if(transaction)CHECK(app.keyboard_card<0&&app.target==1);
}
static void reset_poll(bool enable,bool input){
    polls=samples=0;ticks=100000;gate={};capture={};
    std::memset(matrix,0,sizeof matrix);latch.reset(matrix,enable);
    touch={100,50,100,100,false,false,true};transaction=input;
    revision=app.game.revision;game_clock=app.map_clock_ms;
}
static void setup(){
    app.reset_presentation();new_run(app.game,7,true);app.title=false;
    app.progress.tutorial_flags=FtueFresh|FtueSeenMask;
    app.calculator_keyboard=true;app.keyboard_only=false;
    app.input_bindings=default_input_bindings(true);app.physical_keys_cancel();
    app.game.energy=99;app.game.enemy_count=3;
    for(auto&enemy:app.game.enemies)enemy.hp=enemy.max_hp=100;
    app.cursor=-1;for(unsigned i=0;i<app.game.hand.count;++i){
        const auto id=app.game.hand.ids[i];
        if(card_targets_enemy(app.game,app.game.cards[id],id)&&can_play_card(app.game,id)){app.cursor=int(i);break;}}
    CHECK(app.cursor>=0);digit=uint16_t(app.cursor==9?'0':'1'+app.cursor);app.target=1;
    app.render();app.render();
}
static void unchanged(const SaveData&before){
    SaveData after;CHECK(save(app.game,after));
    CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,after.size));
}
static void dispatch(){
    const unsigned pending=latch.count;CHECK(pending==6&&latch.pending_pad());
    const auto id=app.game.hand.ids[app.cursor];const auto before=app.game.revision;const auto hp=app.game.enemies[2].hp;
    CalculatorPadInput pad;uint16_t previous[8]{},current[8]{};uint8_t repeat[CalculatorPhysicalKeyCount]{};
    app.pointer.held=app.pointer.dragging=true;
    for(unsigned i=0;i<pending;++i){uint16_t code=0;CHECK(latch.pop(current,&code));
        pad.dispatch_key(code,5000,app,false);
        calculator_physical_edges(current,previous,repeat,false,[&](uint16_t physical,bool down){app.physical_key(physical,down);},false);
        std::memcpy(previous,current,sizeof previous);
        if(i<4)CHECK(app.keyboard_card==id&&app.target==(i<2?1:2));
    }
    CHECK(!latch.count&&!pad.dispatched&&!latch.pending_pad());
    CHECK(app.game.revision>before&&app.game.enemies[2].hp<hp&&app.keyboard_card<0);
    const auto committed=app.game.revision;app.physical_key(13,false);CHECK(app.game.revision==committed);
}
static void foreground_render(){
    setup();SaveData before;CHECK(save(app.game,before));
    const std::vector<uint16_t> expected(app.pixels,app.pixels+ScreenW*ScreenH);
    reset_poll(true,true);app.callbacks.foreground_poll=poll;app.render();
    app.callbacks.foreground_poll=nullptr;
    CHECK(polls>=12&&polls<4096&&samples>=6&&samples<polls);
    CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
    unchanged(before);const auto frame_polls=polls;dispatch();
    std::printf("real combat render: %u coarse polls, input only dispatched after pixel-identical frame\n",frame_polls);
    // Before input_ready, even a whole rendering frame cannot queue launch
    // keys. Enabling the release gate starts with an empty hardware baseline.
    setup();reset_poll(false,true);app.callbacks.foreground_poll=poll;app.render();
    app.callbacks.foreground_poll=nullptr;CHECK(polls>0&&!samples&&!latch.count);
}
static void scanline_render(){
    Canvas c{app};std::vector<uint8_t> raw(320*240*2),alpha(320*240);
    for(unsigned i=0;i<320*240;++i){const unsigned color=i*1879;raw[2*i]=uint8_t(color);raw[2*i+1]=uint8_t(color>>8);alpha[i]=uint8_t(i*17);}
    const Picture p{raw.data(),alpha.data(),nullptr,320,240,1};
    auto draw=[&](){c.picture(p,0,0);c.dim(0,0,320,240,85);c.picture_region(p,0,0,0,0,320,240,true,77);};
    std::memset(app.pixels,0,sizeof app.pixels);draw();const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
    std::memset(app.pixels,0,sizeof app.pixels);reset_poll(true,false);app.callbacks.foreground_poll=poll;draw();app.callbacks.foreground_poll=nullptr;
    CHECK(polls>=45&&polls<100&&samples<polls);CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
    std::printf("three full-screen operations: %u coarse polls, unchanged pixels\n",polls);
}
static void cpu_decode(){
    setup();SaveData before;CHECK(save(app.game,before));
    // A64KiB uncompressed LZMA2 chunk still uses the production decoder and
    // its fixed probability arena. No ResourceSource read can create a poll.
    std::vector<uint8_t> packed(65540),out(65536);packed[0]=1;packed[1]=packed[2]=255;
    for(unsigned i=0;i<65536;++i)packed[i+3]=uint8_t(i*29);packed.back()=0;
    reset_poll(true,true);set_lzma2_foreground_poll(poll);
    CHECK(decode_lzma2_block(out.data(),out.size(),out.size(),packed.data(),packed.size(),14));
    set_lzma2_foreground_poll(nullptr);CHECK(polls>=32&&polls<=34&&samples>=6);
    for(unsigned i=0;i<65536;++i)CHECK(out[i]==uint8_t(i*29));
    unchanged(before);dispatch();
}
static void cold_shader_tables(){
    std::vector<uint8_t> colors(256*256*2);
    for(unsigned i=0;i<256*256;++i){const uint16_t value=uint16_t((i%32)*2113u);colors[i*2]=uint8_t(value);colors[i*2+1]=uint8_t(value>>8);}
    const water_reflection::Texture gradient({colors.data(),nullptr,nullptr,64,64,0});
    static RandomGradientCache first_gradient,second_gradient;
    CHECK(first_gradient.bind(app.content,gradient));reset_poll(true,false);
    CHECK(second_gradient.bind(app.content,gradient,poll));CHECK(polls>=4&&polls<300);
    CHECK(first_gradient.count==second_gradient.count);
    CHECK(!std::memcmp(first_gradient.horizontal,second_gradient.horizontal,sizeof first_gradient.horizontal));
    CHECK(!std::memcmp(first_gradient.pairs,second_gradient.pairs,sizeof first_gradient.pairs));
    CHECK(!std::memcmp(first_gradient.vertical_unit,second_gradient.vertical_unit,sizeof first_gradient.vertical_unit));
    const auto cached_polls=polls;CHECK(second_gradient.bind(app.content,gradient,poll)&&polls==cached_polls);
    const water_reflection::Texture light({colors.data(),nullptr,nullptr,256,256,0});
    static RandomLightQuadCache first_light,second_light;
    CHECK(first_light.bind(app.content,light));reset_poll(true,false);
    CHECK(second_light.bind(app.content,light,poll));CHECK(polls==16);
    CHECK(!std::memcmp(first_light.safe,second_light.safe,sizeof first_light.safe));
    CHECK(!std::memcmp(first_light.maximum,second_light.maximum,sizeof first_light.maximum));
    CHECK(!std::memcmp(first_light.minimum,second_light.minimum,sizeof first_light.minimum));
    CHECK(second_light.bind(app.content,light,poll)&&polls==16);
}
static void throttle(){
    CalculatorForegroundPoll q;
    CHECK(!q.due(1000,false)&&!q.sampled);CHECK(q.due(1000,true));
    CHECK(!q.due(1000,true)&&!q.due(738,true));CHECK(q.due(737,true));
    q.mark(5);CHECK(!q.due(UINT32_MAX-256,true));CHECK(q.due(UINT32_MAX-257,true));
    q.mark(0);CHECK(!q.due(UINT32_MAX,true));
}
int main(){throttle();foreground_render();scanline_render();cpu_decode();cold_shader_tables();std::printf("foreground input: %u checks passed\n",checks);}
