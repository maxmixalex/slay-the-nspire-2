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
static void await_combat(){
    a.render();unsigned steps=0;
    while(a.combat_preparation.active&&steps++<1024){a.advance(33);a.render();}
    CHECK(!a.combat_preparation.active);
}
static void setup(int count){
    new_run(a.game,73,false);auto&s=a.game;s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=s.enemy_count=1;
    s.energy=99;s.deck_count=s.card_count=s.hand.count=0;s.enemies[0]={46,46,0,0,0,0};
    for(int i=0;i<count;i++){s.deck[s.deck_count++]={i%2?Defend:Strike,0};s.cards[s.card_count]=s.deck[i];s.zones[s.card_count]=Zone::Hand;s.hand.ids[s.hand.count++]=s.card_count++;}
    a.title=a.library=a.paused=a.help=a.character_select=a.map_preview.open=false;a.reset_presentation();CHECK(validate(s));await_combat();
}
static HitRegion card_point(int index){
    const auto hand=a.combat_hand();CHECK(index>=0&&index<hand.count);const int id=hand.ids[index];
    // Pick an actually exposed point, respecting rotated overlap and the
    // topmost hovered face. This is a real pointer path, not a fixed rail.
    for(int row=0;row<50;++row){const int y=239-row;
        for(int x=0;x<320;++x){const auto*h=a.hit_at(x,y);if(h&&h->kind==HitKind::Hand&&h->value==id)
            return {int16_t(x),int16_t(y),1,1,h->kind,h->value,h->enabled};}}
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
            a.pointer_move(160,40);a.render();auto r=card_point(i);
            a.pointer_move(r.x,r.y);a.render();CHECK(a.cursor==i);
            auto p=a.combat_hand_pose(unsigned(i),true);const auto bounds=hand_bounds(p);
            CHECK(bounds.x>=0&&bounds.x+bounds.w<=320&&bounds.y>=0&&bounds.y+bounds.h<=240&&p.cy==174&&p.scale==1024);
            // Move inside the actual focused face; its new X target does not
            // steal a neighbour's exposed corner or invent a bottom rail.
            const int x=p.cx,y=205;a.pointer_move(x,y);a.render();
            auto h=a.hit_at(x,y);CHECK(h&&h->kind==HitKind::Hand&&h->value==a.game.hand.ids[i]);same(before);
            for(int k=0;k<4;k++){a.pointer_move(x,y);a.tick();a.render();CHECK(a.cursor==i);}same(before);
        }
        a.pointer_move(160,40);a.render();
        auto end=command(HitKind::Command,(int)Key::EndTurn),draw=command(HitKind::OpenPile,1),discard=command(HitKind::OpenPile,2);
        CHECK(end.x==246&&end.y==156&&end.w==70&&end.h==28&&draw.x<40&&discard.x>280);
        click(draw);CHECK(a.panel==1&&a.pile_tab==1);same(before);a.input(Key::Back);a.render();
        click(command(HitKind::OpenPile,2));CHECK(a.panel==1&&a.pile_tab==2);same(before);a.input(Key::Back);a.render();
        click(command(HitKind::Command,(int)Key::EndTurn));CHECK(a.game.turn==2&&a.game.hp==68);
    }
}
static HitRegion focused_point(int index){
    const auto exposed=card_point(index);a.pointer_move(exposed.x,exposed.y);a.render();
    const auto pose=a.combat_hand_pose(unsigned(index),true);
    a.pointer_move(pose.cx,pose.cy);a.render();
    const auto*hit=a.hit_at(pose.cx,pose.cy);CHECK(hit&&hit->kind==HitKind::Hand&&hit->value==a.game.hand.ids[index]);
    return {int16_t(pose.cx),int16_t(pose.cy),1,1,hit->kind,hit->value,hit->enabled};
}
static void input_safety(){
    setup(5);auto before=snap();auto r=focused_point(0);a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.pointer_button(true);
    a.input(Key::Inspect);a.pointer_button(false);CHECK(a.panel==1);same(before);a.input(Key::Back);a.render();
    // A real attack drag from the new hand still captures an instance, not index.
    r=focused_point(0);a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.pointer_button(true);a.pointer_move(238,82);a.pointer_button(false);
    CHECK(a.game.hand.count==4&&a.game.enemies[0].hp==40&&a.game.energy==98);CHECK(validate(a.game));
    setup(5);r=focused_point(1);a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.pointer_button(true);a.pointer_move(130,65);a.pointer_button(false);
    CHECK(a.game.hand.count==4&&a.game.block==5);CHECK(validate(a.game));
    setup(5);a.game.energy=0;a.render();before=snap();click(focused_point(0));CHECK(a.pointer.armed_card<0);same(before);
    // No card may play by releasing a drag over a new modal.
    setup(5);before=snap();r=focused_point(0);a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.pointer_button(true);a.input(Key::Back);a.pointer_move(238,82);a.pointer_button(false);
    CHECK(a.paused);same(before);
}
static void rendering(const std::vector<uint8_t>&data){
    if(data.empty())return;
    setup(10);a.cursor=4;a.render();auto before=snap();
    std::vector<uint16_t> cold(a.pixels,a.pixels+ScreenW*ScreenH);a.render();CHECK(!std::memcmp(cold.data(),a.pixels,cold.size()*2));same(before);
    // Artwork cache must not retain a stale dynamic cost/block description.
    a.game.dexterity=7;a.render();CHECK(std::memcmp(cold.data(),a.pixels,cold.size()*2)!=0);
    a.game.dexterity=0;a.render();CHECK(!std::memcmp(cold.data(),a.pixels,cold.size()*2));
    auto r=card_point(9);click(r);a.input(Key::Detail);CHECK(a.detail_card>=0);a.render();
    for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);
    a.input(Key::Back);CHECK(a.detail_card<0);
}
static int raised_hits(){
    if(!a.hand_focus)return 0;const auto hand=a.combat_hand();if(a.cursor<0||a.cursor>=hand.count)return 0;
    int n=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Hand&&a.hits[i].value==hand.ids[a.cursor]&&a.hits[i].y<180)++n;return n;
}
static void focus_transitions(){
    for(int count=1;count<=10;count++){
        setup(count);auto before=snap();CHECK(!a.hand_focus&&raised_hits()==0);
        int selected=count/2;auto r=card_point(selected);
        a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.render();CHECK(a.hand_focus&&a.cursor==selected&&raised_hits()==1);same(before);
        auto p=hand_pose(count,selected,true);
        a.pointer_move(p.cx,p.cy-HandFaceH*p.scale/(2*1024)+8);a.render();CHECK(a.hand_focus&&a.cursor==selected);same(before);
        a.pointer_move(238,82);a.render();CHECK(!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Left);a.render();CHECK(a.hand_focus&&raised_hits()==1);same(before);
        a.input(Key::Up);a.render();CHECK(a.hand_focus);same(before);
        a.input(Key::Inspect);a.render();CHECK(a.panel==1&&!a.hand_focus);same(before);
        a.input(Key::Back);a.render();CHECK(!a.panel&&!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Right);CHECK(a.hand_focus);a.pointer_cancel();a.render();CHECK(!a.hand_focus&&raised_hits()==0);same(before);
        a.input(Key::Right);a.input(Key::Back);a.render();CHECK(a.paused&&!a.hand_focus);same(before);
        a.input(Key::Back);a.render();CHECK(!a.paused&&!a.hand_focus);same(before);
        // The visible face remains reachable after modal focus is cleared.
        r=card_point(selected);a.pointer_move(r.x,r.y);a.render();CHECK(a.hand_focus&&a.cursor==selected);same(before);
        a.reset_presentation();CHECK(!a.hand_focus);same(before);
    }
    setup(5);auto r=focused_point(0);a.pointer_move(r.x+r.w/2,r.y+r.h/2);a.pointer_button(true);a.pointer_move(238,82);
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
        new_run(a.game,7,true);a.reset_presentation();a.title=false;a.pointer.visible=false;a.animation=0;await_combat();capture(argv[2],false);
        a.input(Key::Right);a.render();capture(argv[2],true);
    }std::printf("combat UI: %u checks passed\n",checks);
}
