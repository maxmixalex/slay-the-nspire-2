#include "app.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "menu_overlay_layout.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
static const char*captures=nullptr;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"potion UI:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static HitRegion find(HitKind kind,int value){app.render();for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return app.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){auto h=find(kind,value);CHECK(h.enabled);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static void capture(const char*name){
    if(!captures)return;app.render();const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
    std::fprintf(f,"P6\n320 240\n255\n");uint8_t row[960];
    for(int y=0;y<240;++y){for(int x=0;x<320;++x){const auto p=app.pixels[y*320+x];row[3*x]=uint8_t((p>>11)*255/31);row[3*x+1]=uint8_t(((p>>5)&63)*255/63);row[3*x+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,960,f)==960);}CHECK(!std::fclose(f));
}
static void reset(){
    new_run(app.game,73,true);app.title=app.paused=app.help=app.library=app.character_select=false;app.reset_presentation();
    CHECK(add_potion_slots(app.game,1));
    app.game.enemy_count=2;app.game.enemies[1]=app.game.enemies[0];
    for(int i=0;i<2;++i){app.game.enemies[i].hp=app.game.enemies[i].max_hp=100;app.game.enemies[i].block=0;}
    app.game.potions[0]=Potion::Block;app.game.potions[1]=Potion::Fire;app.game.potions[2]=Potion::FairyInABottle;
    app.render();
    for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
    CHECK(!app.combat_preparation.active);
}
static void standalone_reward(){
    new_run(app.game,91,false);app.game.phase=Phase::Map;app.title=app.paused=app.help=app.library=app.character_select=false;
    for(unsigned slot=0;slot<potion_slot_count(app.game);++slot)potion_set(app.game,slot,Potion::Strength);
    CHECK(original_event_begin(app.game,OriginalEventId::DrowningBeacon,9,0)&&original_event_choose(app.game,0));
    app.reset_presentation();app.render();CHECK(app.world_choice_is_potion_reward());
    unsigned choices=0;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::WorldChoice){++choices;CHECK(app.hits[i].value==0);}CHECK(choices==1);
    snapshot();click(HitKind::WorldChoice,0);CHECK(app.game.phase==Phase::OriginalEventChoice&&app.potion_belt_motion.failed);unchanged();
    app.advance(150);CHECK(app.potion_belt_motion.error_alpha(app.map_clock_ms)==141);capture("potion-full-reward");
    click(HitKind::PotionSlot,1);click(HitKind::UiAction,int(UiAction::DiscardPotion));
    CHECK(app.game.phase==Phase::OriginalEventChoice&&potion_at(app.game,1)==Potion::Empty);
    click(HitKind::WorldChoice,0);CHECK(app.game.phase==Phase::Event&&app.game.original_events.finished);
    CHECK(potion_at(app.game,0)==Potion::Strength&&potion_at(app.game,1)==Potion::GlowwaterPotion);CHECK(validate(app.game));
}
int main(int argc,char**argv){
    std::vector<uint8_t>data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);CHECK(app.content.open(data.data(),data.size())&&app.content.current_profile());}
    if(argc>2)captures=argv[2];
    standalone_reward();reset();snapshot();auto bottle=find(HitKind::PotionSlot,1);
    app.pointer_move(bottle.x+4,8);app.render();CHECK(!app.panel);unchanged();
    click(HitKind::PotionSlot,1);CHECK(app.panel==4&&app.scroll==1&&!app.potion_targeting);app.advance(150);capture("potion-fire-menu");unchanged();
    // Original popup ratio with LCD controls, centered beneath its holder.
    const auto use_control=find(HitKind::Potion,1),discard_control=find(HitKind::UiAction,int(UiAction::DiscardPotion));
    CHECK(use_control.x+potion_popup_layout::W/2-potion_popup_layout::ControlX==hud_potion_layout::center_x(1,potion_slot_count(app.game)));
    CHECK(use_control.y==33+potion_popup_layout::UseY&&use_control.w==potion_popup_layout::ControlW&&use_control.h==potion_popup_layout::ControlH);
    CHECK(discard_control.x==use_control.x&&discard_control.y==33+potion_popup_layout::DiscardY&&discard_control.w==use_control.w&&discard_control.h==use_control.h);
    click(HitKind::Potion,1);CHECK(app.potion_targeting&&app.game.potions[1]==Potion::Fire);CHECK(app.potion_popup_exit.active&&app.potion_popup_exit.alpha==255&&app.potion_popup_exit.thrown);
    app.render();for(int i=0;i<app.hit_count;++i)CHECK(app.hits[i].kind!=HitKind::Potion&&!(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::DiscardPotion)));
    app.advance(50);capture("potion-popup-exit-half");unchanged();app.advance(150);app.render();CHECK(!app.potion_popup_exit.active);capture("potion-throw-targeting");unchanged();
    // Outside click cancels the target manager; it never consumes the bottle.
    app.pointer_move(4,230);app.pointer_button(true);app.pointer_button(false);CHECK(!app.panel&&!app.potion_targeting);unchanged();
    click(HitKind::PotionSlot,1);app.input(Key::Confirm);CHECK(app.potion_targeting);app.input(Key::Back);CHECK(!app.panel);unchanged();
    click(HitKind::PotionSlot,1);click(HitKind::Potion,1);click(HitKind::Enemy,1);
    CHECK(app.game.potions[1]==Potion::Empty&&app.game.enemies[1].hp==80&&app.game.enemies[0].hp==100&&!app.panel);
    // Releasing once more cannot throw the already consumed bottle twice.
    const auto revision=app.game.revision;app.pointer_button(false);CHECK(app.game.revision==revision&&app.game.enemies[1].hp==80);
    reset();click(HitKind::PotionSlot,2);snapshot();CHECK(app.potion_menu_item==1);auto disabled=find(HitKind::Potion,2);CHECK(!disabled.enabled);
    app.pointer_move(disabled.x+disabled.w/2,disabled.y+disabled.h/2);app.pointer_button(true);app.pointer_button(false);app.advance(150);capture("potion-automatic-menu");unchanged();
    click(HitKind::UiAction,int(UiAction::DiscardPotion));CHECK(!app.panel&&app.game.potions[2]==Potion::Empty&&app.potion_removed_discard);
    reset();click(HitKind::PotionSlot,0);const int block=app.game.block;click(HitKind::Potion,0);
    CHECK(!app.potion_targeting&&!app.panel&&app.game.potions[0]==Potion::Empty&&app.game.block==block+12);
    reset();app.game.potions[0]=Potion::Empty;snapshot();auto empty=find(HitKind::PotionSlot,0);CHECK(!empty.enabled);
    app.pointer_move(empty.x+4,8);app.render();
    CHECK(app.hit_at(empty.x+4,8)&&app.hit_at(empty.x+4,8)->kind==HitKind::PotionSlot&&!app.hit_at(empty.x+4,8)->enabled);
    capture("potion-empty-hover");app.pointer_button(true);app.pointer_button(false);CHECK(!app.panel);unchanged();
    app.input(Key::Potions);app.render();CHECK(!app.panel&&app.hud_focus==2&&!app.pointer.visible);app.input(Key::Confirm);CHECK(!app.panel&&app.hud_focus==2);unchanged();
    app.input(Key::Left);app.render();CHECK(app.hud_focus==1);app.input(Key::Right);app.input(Key::Right);CHECK(app.hud_focus==3);
    app.input(Key::Confirm);CHECK(app.panel==4&&app.scroll==1);app.input(Key::Back);CHECK(!app.panel);unchanged();
    app.input(Key::Potions);CHECK(app.hud_focus==2);app.input(Key::Potions);CHECK(app.hud_focus<0&&!app.panel);unchanged();
    // Extra physical slots stay reachable through the small top-bar pager.
    CHECK(add_potion_slots(app.game,13));CHECK(potion_slot_count(app.game)==16);app.game.potions[15]=Potion::Fruit;
    for(int i=0;i<3;++i)click(HitKind::PotionPage,1);CHECK(app.potion_page==3);
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
    const unsigned throw_tick=app.animation;app.input(Key::Confirm);CHECK(app.game.potions[0]==Potion::Empty);
    // FirePotion has no AttackCommand pre-wait: the damage occurs now.
    const int resolved=app.game.enemies[0].hp;CHECK(resolved<hp);CHECK(app.combat_health(1).hp==resolved);
    CHECK(app.last_hit[1]==hp-resolved&&app.hit_start[1]==int(throw_tick));
    for(unsigned i=0;i<180&&app.combat_feedback.active;++i){app.advance(33);app.render();}
    CHECK(app.combat_health(1).hp==resolved);
    // A potion that autoplays a real multi-hit card must retain its future
    // command cues. Do not infer those waits from the bottle animation.
    app.game.potions[0]=Potion::DistilledChaos;app.game.hand.count=app.game.draw.count=app.game.discard.count=app.game.exhaust.count=app.game.powers.count=0;
    app.game.card_count=1;app.game.cards[0]={TwinStrike,0};app.game.zones[0]=Zone::Draw;app.game.draw.ids[0]=0;app.game.draw.count=1;
    app.reset_presentation();app.render();for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
    const int old_hp=app.game.enemies[0].hp;app.open_potion(0);app.input(Key::Confirm);CHECK(app.game.potions[0]==Potion::Empty);
    CHECK(app.combat_feedback.active&&app.combat_health(1).hp==old_hp&&app.game.enemies[0].hp<old_hp);
    unsigned changes=0;int displayed=old_hp;
    for(unsigned i=0;i<180&&app.combat_feedback.active;++i){app.advance(33);app.render();const int hp_now=app.combat_health(1).hp;if(hp_now!=displayed){++changes;displayed=hp_now;}}
    CHECK(changes>=2&&displayed==app.game.enemies[0].hp);
    std::printf("potion UI: %u checks passed\n",checks);
}
