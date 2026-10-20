#include "app.hpp"
#include "content_fixture.hpp"
#include "combat_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}} while(0)
static SaveData snap(){SaveData b{};CHECK(save(a.game,b));return b;}
static void same(const SaveData&b){auto c=snap();CHECK(c.size==b.size&&!std::memcmp(c.bytes,b.bytes,b.size));}
static void setup(int count){
    new_run(a.game,73,false);auto&s=a.game;s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=s.enemy_count=1;
    s.energy=99;s.deck_count=s.card_count=s.hand.count=0;s.enemies[0]={46,46,0,0,0,0};
    for(int i=0;i<count;i++){s.deck[s.deck_count++]={i%2?Defend:Strike,0};s.cards[s.card_count]=s.deck[i];s.zones[s.card_count]=Zone::Hand;s.hand.ids[s.hand.count++]=s.card_count++;}
    a.title=a.library=a.paused=a.help=a.character_select=a.map_preview.open=false;a.reset_presentation();CHECK(validate(s));a.render();
}
static HitRegion rail(int index){
    for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==HitKind::Hand&&h.value==a.game.hand.ids[index]&&h.y+h.h==240)return h;}
    CHECK(false);return {};
}
static HitRegion command(HitKind kind,int value){
    for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];
    CHECK(false);return {};
}
static void click(HitRegion h){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.render();}
static void layouts(){
    for(int count=1;count<=10;count++){
        setup(count);auto before=snap();
        int starting_icons=0;for(int h=0;h<a.hit_count;h++)if(a.hits[h].kind==HitKind::OwnedRelic&&a.hits[h].value==0)++starting_icons;
        CHECK(starting_icons==1);
        auto left=hand_pose(count,0,false),right=hand_pose(count,count-1,false);
        CHECK(left.cx+right.cx>=319&&left.cx+right.cx<=321);
        for(int i=0;i<count;i++){
            auto r=rail(i);CHECK(r.w>=16&&r.h>=25&&r.x>=0&&r.x+r.w<=320);
            int x=r.x+r.w/2;
            a.pointer_move(x,224);a.render();CHECK(a.cursor==i);
            auto h=a.hit_at(x,224);CHECK(h&&h->kind==HitKind::Hand&&h->value==a.game.hand.ids[i]);same(before);
            auto p=hand_pose(count,i,true);CHECK(p.cx-HandTileW/2>48&&p.cx+HandTileW/2<254);
            // Repeated same-position hover must not oscillate the selected card.
            for(int k=0;k<4;k++){a.pointer_move(x,224);a.tick();a.render();CHECK(a.cursor==i);}same(before);
        }
        auto end=command(HitKind::Command,(int)Key::EndTurn),draw=command(HitKind::OpenPile,1),discard=command(HitKind::OpenPile,2);
        CHECK(end.x>=250&&draw.x<40&&discard.x>280);
        click(draw);CHECK(a.panel==1&&a.pile_tab==1);same(before);a.input(Key::Back);a.render();
        click(command(HitKind::OpenPile,2));CHECK(a.panel==1&&a.pile_tab==2);same(before);a.input(Key::Back);a.render();
        click(command(HitKind::Command,(int)Key::EndTurn));CHECK(a.game.turn==2&&a.game.hp==68);
    }
}
static void input_safety(){
    setup(5);auto before=snap();auto r=rail(0);a.pointer_move(r.x+r.w/2,224);a.pointer_button(true);
    a.input(Key::Inspect);a.pointer_button(false);CHECK(a.panel==1);same(before);a.input(Key::Back);a.render();
    // A real attack drag from the new hand still captures an instance, not index.
    r=rail(0);a.pointer_move(r.x+r.w/2,224);a.pointer_button(true);a.pointer_move(238,82);a.pointer_button(false);
    CHECK(a.game.hand.count==4&&a.game.enemies[0].hp==40&&a.game.energy==98);CHECK(validate(a.game));
    setup(5);r=rail(1);a.pointer_move(r.x+r.w/2,224);a.pointer_button(true);a.pointer_move(130,65);a.pointer_button(false);
    CHECK(a.game.hand.count==4&&a.game.block==5);CHECK(validate(a.game));
    setup(5);a.game.energy=0;a.render();before=snap();click(rail(0));CHECK(a.pointer.armed_card<0);same(before);
    // No card may play by releasing a drag over a new modal.
    setup(5);before=snap();r=rail(0);a.pointer_move(r.x+r.w/2,224);a.pointer_button(true);a.input(Key::Back);a.pointer_move(238,82);a.pointer_button(false);
    CHECK(a.paused);same(before);
}
static void rendering(const std::vector<uint8_t>&data){
    if(data.empty())return;
    setup(10);a.cursor=4;a.render();auto before=snap();
    std::vector<uint16_t> cold(a.pixels,a.pixels+ScreenW*ScreenH);a.render();CHECK(!std::memcmp(cold.data(),a.pixels,cold.size()*2));same(before);
    // Artwork cache must not retain a stale dynamic cost/block description.
    a.game.dexterity=7;a.render();CHECK(std::memcmp(cold.data(),a.pixels,cold.size()*2)!=0);
    a.game.dexterity=0;a.render();CHECK(!std::memcmp(cold.data(),a.pixels,cold.size()*2));
    auto r=rail(9);click(r);a.input(Key::Detail);CHECK(a.detail_card>=0);a.render();
    for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);
    a.input(Key::Back);CHECK(a.detail_card<0);
}
static int raised_hits(){
    int n=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Hand&&a.hits[i].h==68)++n;return n;
}
static void focus_transitions(){
    for(int count=1;count<=10;count++){
        setup(count);auto before=snap();CHECK(!a.hand_focus&&raised_hits()==0);
        int selected=count/2;auto r=rail(selected);
        a.pointer_move(r.x+r.w/2,224);a.render();CHECK(a.hand_focus&&a.cursor==selected&&raised_hits()==1);same(before);
        auto p=hand_pose(count,selected,true);
        a.pointer_move(p.cx,p.cy-HandTileH/2+8);a.render();CHECK(a.hand_focus&&a.cursor==selected);same(before);
        a.pointer_move(238,82);a.render();CHECK(!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Left);a.render();CHECK(a.hand_focus&&raised_hits()==1);same(before);
        a.input(Key::Up);a.render();CHECK(a.hand_focus);same(before);
        a.input(Key::Inspect);a.render();CHECK(a.panel==1&&!a.hand_focus);same(before);
        a.input(Key::Back);a.render();CHECK(!a.panel&&!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Right);CHECK(a.hand_focus);a.pointer_cancel();a.render();CHECK(!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Right);a.input(Key::Back);a.render();CHECK(a.paused&&!a.hand_focus);same(before);
        a.input(Key::Back);a.render();CHECK(!a.paused&&!a.hand_focus);same(before);
        // The collapsed upper card face is also reachable, not only its rail.
        r=rail(selected);p=hand_pose(count,selected,false);
        a.pointer_move(r.x+r.w/2,p.cy-HandTileH*p.scale/(2*1024)+2);a.render();CHECK(a.hand_focus&&a.cursor==selected);same(before);
        a.reset_presentation();CHECK(!a.hand_focus);same(before);
    }
    setup(5);auto r=rail(0);a.pointer_move(r.x+r.w/2,224);a.pointer_button(true);a.pointer_move(238,82);
    CHECK(a.hand_focus&&a.pointer.held);a.pointer_button(false);a.render();
    CHECK(!a.hand_focus&&a.game.enemies[0].hp==40&&a.game.hand.count==4);
}
static void capture(const char*prefix,bool focused){
    char filename[1024];std::snprintf(filename,sizeof filename,"%s-%s.ppm",prefix,focused?"focused":"neutral");
    FILE*f=std::fopen(filename,"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
    for(auto p:a.pixels){unsigned char rgb[3]={(unsigned char)((p>>11)*255/31),(unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}
    CHECK(std::fclose(f)==0);
}
int main(int argc,char**argv){
    std::vector<uint8_t> data;
    if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);CHECK(a.init(data.data(),n,{}));}
    layouts();input_safety();focus_transitions();rendering(data);
    if(argc>2&&!data.empty()){
        new_run(a.game,7,true);a.reset_presentation();a.title=false;a.pointer.visible=false;a.animation=0;a.render();capture(argv[2],false);
        a.input(Key::Right);a.render();capture(argv[2],true);
    }std::printf("combat UI: %u checks passed\n",checks);
}
