#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static App app;
static void click(int x,int y){app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);app.render();}
static void unchanged(const SaveData& old){SaveData data{};CHECK(save(app.game,data));CHECK(data.size==old.size&&!std::memcmp(data.bytes,old.bytes,data.size));}
int main(){
    new_run(app.game,7,true);app.title=false;app.render();SaveData before{};CHECK(save(app.game,before));
    app.input(Key::Inspect);app.render();app.pointer_scroll(1);app.render();CHECK(app.scroll==10);
    app.pointer_scroll(1);app.render();CHECK(app.scroll==20);
    app.pointer_scroll(1);app.render();CHECK(app.scroll==30);
    app.pointer_scroll(1);app.render();CHECK(app.scroll==30);
    app.pointer_scroll(-1);app.render();CHECK(app.scroll==20);unchanged(before);
    // Scrolling during capture cannot activate the new card under that position.
    app.pointer_move(33,80);app.pointer_button(true);app.pointer_scroll(1);app.pointer_button(false);
    CHECK(app.detail_card<0);unchanged(before);
    app.input(Key::Back);app.render();
    // Expose ten actual card instances. All ten must remain pointer-accessible.
    while(app.game.hand.count<10){auto& s=app.game;int id=s.draw.ids[--s.draw.count];s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;}
    CHECK(save(app.game,before));app.render();
    for(int k=0;k<10;k++){
        int x=60+k*23+1;app.pointer_move(x,199);app.render();
        const auto* hit=app.hit_at(x,199);CHECK(hit&&hit->kind==HitKind::Hand&&hit->value==app.game.hand.ids[k]);
        CHECK(app.cursor==k);unchanged(before);
    }
    // Drag away from a button cannot trigger it. Keyboard modal cancels capture.
    app.input(Key::Back);app.render();CHECK(app.paused);
    app.pointer_move(90,42);app.pointer_button(true);app.pointer_move(90,160);
    CHECK(app.menu==0);app.pointer_button(false);CHECK(app.paused);unchanged(before);
    app.input(Key::Back);app.render();app.pointer_move(70,199);app.pointer_button(true);
    app.input(Key::Inspect);app.pointer_button(false);CHECK(app.panel==1);unchanged(before);
    std::printf("input: %u checks passed\n",checks);
}
