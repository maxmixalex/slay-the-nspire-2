#include "app.hpp"
#include "card_grid_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static App app;
static void click(int x,int y){app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);app.render();}
static void unchanged(const SaveData& old){SaveData data{};CHECK(save(app.game,data));CHECK(data.size==old.size&&!std::memcmp(data.bytes,old.bytes,data.size));}
int main(){
    new_run(app.game,7,true);app.title=false;app.render();SaveData before{};CHECK(save(app.game,before));
    app.input(Key::Inspect);app.render();auto&grid=app.card_grids[unsigned(CardGridSurface::Pile)];
    CHECK(grid.bound&&grid.has_scrollbar());const int origin=grid.position;
    app.pointer_scroll(1);CHECK(grid.target>origin&&grid.position==origin);
    CHECK(spire_test::settle_card_grid(app));CHECK(grid.position>origin&&grid.position<20*CardGridScroll::Unit);
    // One continuous scroll surface contains the full deck beyond 128 cards.
    CHECK(app.game.deck_count==IroncladCards+53);CHECK(spire_test::drag_grid_thumb(app,true));
    CHECK(grid.position==grid.maximum());bool last_visible=false;
    for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::DeckCard){
        CHECK(app.hits[i].y>=grid.top&&app.hits[i].y+app.hits[i].h<=grid.bottom);
        last_visible|=app.hits[i].value==app.game.deck_count-1;
    }
    CHECK(last_visible);const int end=grid.position;app.pointer_scroll(-1);CHECK(spire_test::settle_card_grid(app));CHECK(grid.position<end);unchanged(before);
    // Scrolling during capture cannot activate the newly exposed card.
    HitRegion card{};for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::DeckCard){card=app.hits[i];break;}CHECK(card.w>0);
    app.pointer_move(card.x+card.w/2,card.y+card.h/2);app.pointer_button(true);app.pointer_scroll(1);app.pointer_button(false);
    CHECK(app.detail_card<0);unchanged(before);
    app.input(Key::Back);app.render();
    for(auto key:{Key::DrawPile,Key::DiscardPile,Key::ExhaustPile}){
        app.input(key);app.render();CHECK(app.panel==1&&app.pile_tab==(key==Key::DrawPile?1:key==Key::DiscardPile?2:3));unchanged(before);
        for(int i=0;i<app.hit_count;++i)CHECK(app.hits[i].kind!=HitKind::PileTab&&app.hits[i].kind!=HitKind::PileSort);
        app.input(key);CHECK(!app.panel);unchanged(before);
    }
    // Deck always opens its own pile, even after an exhausted-card screen.
    app.input(Key::Inspect);CHECK(app.panel==1&&app.pile_tab==0);app.render();
    const HitRegion* settings=nullptr;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Command&&app.hits[i].value==int(Key::Back)&&app.hits[i].y==0)settings=&app.hits[i];
    CHECK(settings);click(settings->x+settings->w/2,settings->y+settings->h/2);CHECK(app.paused&&app.panel==1);unchanged(before);
    app.input(Key::Back);CHECK(!app.paused&&app.panel==1);app.input(Key::Back);app.render();
    // Expose ten actual card instances. All ten must remain pointer-accessible.
    while(app.game.hand.count<10){auto& s=app.game;int id=s.draw.ids[--s.draw.count];s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;}
    CHECK(save(app.game,before));app.render();
    for(int k=0;k<10;k++){
        int x=-1;
        for(int i=0;i<app.hit_count;i++){auto h=app.hits[i];if(h.kind==HitKind::Hand&&h.value==app.game.hand.ids[k]&&h.y+h.h==240)x=h.x+h.w/2;}
        CHECK(x>=0);app.pointer_move(x,224);app.render();
        const auto* hit=app.hit_at(x,224);CHECK(hit&&hit->kind==HitKind::Hand&&hit->value==app.game.hand.ids[k]);
        CHECK(app.cursor==k);unchanged(before);
    }
    // Drag away from a button cannot trigger it. Keyboard modal cancels capture.
    app.input(Key::Back);app.render();CHECK(app.paused);
    app.pointer_move(90,42);app.pointer_button(true);app.pointer_move(90,160);
    CHECK(app.menu==0);app.pointer_button(false);CHECK(app.paused);unchanged(before);
    app.input(Key::Back);app.render();app.pointer_move(70,199);app.pointer_button(true);
    app.input(Key::Inspect);app.pointer_button(false);CHECK(app.panel==1);unchanged(before);
    // A settings overlay must own hover as soon as it opens, including an
    // event arriving before render replaces the covered pause hit regions.
    app.panel=0;app.paused=true;app.menu=0;app.render();
    const HitRegion* covered=nullptr;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Menu&&app.hits[i].value==1)covered=&app.hits[i];
    CHECK(covered);const int covered_x=covered->x+covered->w/2,covered_y=covered->y+covered->h/2;
    app.settings_open=true;app.pointer_move(covered_x,covered_y);CHECK(app.menu==0);unchanged(before);
    app.settings_open=app.paused=false;app.pointer_cancel();
    // Original directional card play is a two-step transaction. Target
    // navigation/cancel changes UI only; confirm commits exactly one card.
    app.game.energy=99;app.game.enemy_count=3;
    for(unsigned i=0;i<3;++i){app.game.enemies[i].hp=app.game.enemies[i].max_hp=100;}
    app.cursor=-1;for(unsigned i=0;i<app.game.hand.count;++i)if(card_targets_enemy(app.game,app.game.cards[app.game.hand.ids[i]],app.game.hand.ids[i])&&can_play_card(app.game,app.game.hand.ids[i])){app.cursor=int(i);break;}
    CHECK(app.cursor>=0);app.target=1;CHECK(save(app.game,before));const auto instance=app.game.hand.ids[app.cursor];
    app.input(Key::Confirm);CHECK(app.keyboard_card==instance&&app.target==1);unchanged(before);
    app.input(Key::Right);CHECK(app.target==2&&app.keyboard_card==instance);unchanged(before);
    app.input(Key::Right);CHECK(app.target==0);app.input(Key::Left);CHECK(app.target==2);unchanged(before);
    app.input(Key::Up);CHECK(app.target==2);app.input(Key::Down);CHECK(app.target==2);unchanged(before);
    app.input(Key::Back);CHECK(app.keyboard_card<0&&!app.paused);unchanged(before);
    app.input(Key::Confirm);CHECK(app.keyboard_card==instance);unchanged(before);
    const auto revision=app.game.revision;app.input(Key::Confirm);CHECK(app.keyboard_card<0&&app.game.revision>revision);
    std::printf("input: %u checks passed\n",checks);
}
