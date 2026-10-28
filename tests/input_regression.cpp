#include "app.hpp"
#include "card_grid_fixture.hpp"
#include "calculator_pad_input.hpp"
#include "host_input_latch.hpp"
#include "host_physical_input.hpp"
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
    // NScrollbar's independent 50ms spring must lag a moved Value and
    // converge without overshooting, including a capped250ms host update.
    CardGridScroll scroll;scroll.configure(1,1,64,40,220,20);scroll.thumb_drag=true;
    scroll.drag_thumb(scroll.bar_bottom());const int original_thumb=scroll.thumb_y();
    scroll.advance(16);CHECK(scroll.thumb_y()>original_thumb&&scroll.thumb_y()<scroll.bar_bottom());
    int previous=scroll.thumb_y();for(unsigned i=0;i<20;++i){scroll.advance(16);CHECK(scroll.thumb_y()>=previous&&scroll.thumb_y()<=scroll.bar_bottom());previous=scroll.thumb_y();}
    CHECK(scroll.thumb_y()==scroll.bar_bottom());scroll.drag_thumb(scroll.bar_top());scroll.advance(250);
    CHECK(scroll.thumb_y()>=scroll.bar_top()&&scroll.thumb_y()<previous);
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
    HitRegion toggle{},back{};for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];
        if(h.kind==HitKind::PileUpgrades)toggle=h;
        if(h.kind==HitKind::UiAction&&h.value==int(UiAction::Back))back=h;
    }
    CHECK(toggle.w>0&&back.w>0&&toggle.y>=grid.bottom);
    CHECK(2*toggle.x+toggle.w>=319&&2*toggle.x+toggle.w<=321);
    CHECK(toggle.x>=back.x+back.w||toggle.y+toggle.h<=back.y||toggle.y>=back.y+back.h);
    const auto*toggle_hit=app.hit_at(toggle.x+toggle.w/2,toggle.y+toggle.h/2);CHECK(toggle_hit&&toggle_hit->kind==HitKind::PileUpgrades);
    const bool upgraded=app.pile_upgraded;click(toggle.x+toggle.w/2,toggle.y+toggle.h/2);
    CHECK(app.pile_upgraded!=upgraded&&app.panel==1&&!app.paused);unchanged(before);
    const HitRegion* settings=nullptr;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Command&&app.hits[i].value==int(Key::Back)&&app.hits[i].y==0)settings=&app.hits[i];
    CHECK(settings);click(settings->x+settings->w/2,settings->y+settings->h/2);CHECK(app.paused&&app.panel==1);unchanged(before);
    app.input(Key::Back);CHECK(!app.paused&&app.panel==1);app.input(Key::Back);app.render();
    // Expose ten actual card instances. All ten must remain pointer-accessible.
    while(app.game.hand.count<10){auto& s=app.game;int id=s.draw.ids[--s.draw.count];s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;}
    CHECK(save(app.game,before));app.render();
    for(int k=0;k<10;k++){
        app.pointer_move(160,40);app.render();int x=-1,y=-1;
        // Source fan corners are rotated and partly below the viewport.
        // Find a genuinely exposed point, then inspect its raised face.
        for(int yy=239;yy>=190&&x<0;--yy)for(int xx=0;xx<320;++xx){const auto*h=app.hit_at(xx,yy);
            if(h&&h->kind==HitKind::Hand&&h->value==app.game.hand.ids[k]){x=xx;y=yy;break;}}
        CHECK(x>=0);app.pointer_move(x,y);app.render();CHECK(app.cursor==k&&app.hand_focus);
        const auto pose=app.combat_hand_pose(unsigned(k),true);app.pointer_move(pose.cx,pose.cy+5);app.render();
        const auto* hit=app.hit_at(pose.cx,pose.cy+5);CHECK(hit&&hit->kind==HitKind::Hand&&hit->value==app.game.hand.ids[k]);
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
    // Exercise the calculator host adapter, not just semantic App::input.
    // Its pointer cancellation previously cleared the selected card after a
    // side click, so Enter selected again instead of playing at the new target.
    app.calculator_keyboard=true;app.keyboard_only=false;app.input_bindings=default_input_bindings(true);
    app.physical_keys_cancel();app.target=1;
    const uint16_t digit=uint16_t(app.cursor==9?'0':'1'+app.cursor);
    const auto revision=app.game.revision;const int target_hp=app.game.enemies[2].hp;
    CalculatorInputLatch latch;CalculatorPadInput capture,pad;
    uint16_t matrix[8]{},prior[8]{},queued[8]{};uint8_t repeats[CalculatorPhysicalKeyCount]{};
    latch.reset(matrix,true);TouchSample sample{100,50,100,100,false,false,true};unsigned poll_index=0;
    const auto set_key=[&](uint16_t code,bool down){bool found=false;
        calculator_physical_keys([&](unsigned,unsigned row,unsigned mask,uint16_t key){if(key==code){found=true;if(down)matrix[row]|=mask;else matrix[row]&=~mask;}});CHECK(found);};
    // The entire digit, short side-click and Enter transaction occurs inside
    // bounded resource I/O. It must only record hardware until the host drains
    // one combined FIFO; separate pad/matrix queues would reorder this case.
    const auto poll=[&](){switch(poll_index++){
        case 0:set_key(digit,true);break;case 1:set_key(digit,false);break;
        case 2:sample.contact=sample.pressed=true;break;
        case 3:sample.contact=sample.pressed=false;break;
        case 4:set_key(13,true);break;case 5:set_key(13,false);break;}
        latch.sample(matrix,capture.capture(sample,false));
        CHECK(app.game.revision==revision&&app.keyboard_card<0&&app.target==1);
    };
    static uint8_t io_bytes[6*8192];
    CHECK(sampled_resource_read(io_bytes,sizeof io_bytes,nullptr,[](void*,unsigned,unsigned n,void*){return n;},poll)==sizeof io_bytes);
    CHECK(poll_index==7&&latch.count==6&&latch.pending_pad());unchanged(before);
    app.pointer.held=app.pointer.dragging=true;
    const unsigned pending=latch.count;for(unsigned i=0;i<pending;++i){uint16_t key=0;CHECK(latch.pop(queued,&key));
        pad.dispatch_key(key,5000,app,false);
        calculator_physical_edges(queued,prior,repeats,false,[&](uint16_t code,bool down){app.physical_key(code,down);},false);
        for(unsigned row=0;row<8;++row)prior[row]=queued[row];
        if(i<4){CHECK(app.keyboard_card==instance&&app.target==(i<2?1:2));unchanged(before);}
        if(i==3)CHECK(!app.pointer.held&&!app.pointer.dragging);
    }
    CHECK(!latch.count&&!latch.pending_pad()&&!pad.dispatched);
    CHECK(app.keyboard_card<0&&app.game.revision>revision&&app.game.enemies[2].hp<target_hp);
    const auto played_revision=app.game.revision;app.physical_key(13,false);CHECK(app.game.revision==played_revision);
    std::printf("input: %u checks passed\n",checks);
}
