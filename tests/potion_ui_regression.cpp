#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"potion UI:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static HitRegion find(HitKind kind,int value){app.render();for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return app.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){auto h=find(kind,value);CHECK(h.enabled);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static void reset(){
    new_run(app.game,73,true);app.title=app.paused=app.help=app.library=app.character_select=false;app.reset_presentation();
    CHECK(add_potion_slots(app.game,1));
    app.game.enemy_count=2;app.game.enemies[1]=app.game.enemies[0];
    for(int i=0;i<2;++i){app.game.enemies[i].hp=app.game.enemies[i].max_hp=100;app.game.enemies[i].block=0;}
    app.game.potions[0]=Potion::Block;app.game.potions[1]=Potion::Fire;app.game.potions[2]=Potion::FairyInABottle;
    app.render();
}
int main(){
    reset();snapshot();auto bottle=find(HitKind::PotionSlot,1);
    app.pointer_move(bottle.x+4,8);app.render();CHECK(!app.panel);unchanged();
    click(HitKind::PotionSlot,1);CHECK(app.panel==4&&app.scroll==1&&!app.potion_targeting);unchanged();
    click(HitKind::Potion,1);CHECK(app.potion_targeting&&app.game.potions[1]==Potion::Fire);unchanged();
    // Outside click cancels the target manager; it never consumes the bottle.
    app.pointer_move(4,230);app.pointer_button(true);app.pointer_button(false);CHECK(!app.panel&&!app.potion_targeting);unchanged();
    click(HitKind::PotionSlot,1);app.input(Key::Confirm);CHECK(app.potion_targeting);app.input(Key::Back);CHECK(!app.panel);unchanged();
    click(HitKind::PotionSlot,1);click(HitKind::Potion,1);click(HitKind::Enemy,1);
    CHECK(app.game.potions[1]==Potion::Empty&&app.game.enemies[1].hp==80&&app.game.enemies[0].hp==100&&!app.panel);
    // Releasing once more cannot throw the already consumed bottle twice.
    const auto revision=app.game.revision;app.pointer_button(false);CHECK(app.game.revision==revision&&app.game.enemies[1].hp==80);
    reset();click(HitKind::PotionSlot,2);snapshot();CHECK(app.potion_menu_item==1);auto disabled=find(HitKind::Potion,2);CHECK(!disabled.enabled);
    app.pointer_move(disabled.x+disabled.w/2,disabled.y+disabled.h/2);app.pointer_button(true);app.pointer_button(false);unchanged();
    click(HitKind::UiAction,int(UiAction::DiscardPotion));CHECK(!app.panel&&app.game.potions[2]==Potion::Empty&&app.potion_removed_discard);
    reset();click(HitKind::PotionSlot,0);const int block=app.game.block;click(HitKind::Potion,0);
    CHECK(!app.potion_targeting&&!app.panel&&app.game.potions[0]==Potion::Empty&&app.game.block==block+12);
    reset();app.game.potions[0]=Potion::Empty;snapshot();auto empty=find(HitKind::PotionSlot,0);CHECK(!empty.enabled);
    app.pointer_move(empty.x+4,8);app.pointer_button(true);app.pointer_button(false);CHECK(!app.panel);unchanged();
    // Extra physical slots stay reachable through the small top-bar pager.
    CHECK(add_potion_slots(app.game,13));CHECK(potion_slot_count(app.game)==16);app.game.potions[15]=Potion::Fruit;
    for(int i=0;i<5;++i)click(HitKind::PotionPage,1);CHECK(app.potion_page==5);
    click(HitKind::PotionSlot,15);CHECK(app.panel==4&&app.scroll==15);click(HitKind::Potion,15);CHECK(app.game.potions[15]==Potion::Empty);
    // The room's merchant is a target, and its inventory overlay is not.
    reset();app.game.phase=Phase::Shop;app.game.potions[0]=Potion::FoulPotion;app.reset_presentation();
    app.open_potion(0);app.start_potion_use();CHECK(app.potion_targeting);auto merchant=find(HitKind::PotionMerchant,int(UiAction::OpenShop));CHECK(merchant.w<150&&merchant.h<180);
    app.input(Key::Back);app.shop_open=true;app.open_potion(0);CHECK(!app.potion_usable());app.close_potion();
    // The original encounter path must create a visual command projection
    // only when the target is confirmed, not while opening/cancelling menus.
    new_run(app.game,0x41cef1ea,false);app.game.phase=Phase::Combat;app.game.floor=app.game.turn=1;
    CHECK(encounter_setup(app.game,EncounterId::MechaKnightElite));app.game.potions[0]=Potion::Fire;
    app.title=false;app.reset_presentation();app.render();
    for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
    const int hp=app.game.enemies[0].hp;app.open_potion(0);app.start_potion_use();CHECK(!app.combat_feedback.active);
    app.input(Key::Confirm);CHECK(app.game.potions[0]==Potion::Empty);CHECK(app.combat_feedback.active);
    CHECK(app.combat_health(1).hp==hp);const int resolved=app.game.enemies[0].hp;CHECK(resolved<hp);
    for(unsigned i=0;i<180&&app.combat_feedback.active;++i){app.advance(33);app.render();}
    CHECK(app.combat_health(1).hp==resolved);
    std::printf("potion UI: %u checks passed\n",checks);
}
