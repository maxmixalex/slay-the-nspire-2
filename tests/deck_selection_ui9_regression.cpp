// Actual commands, native hit regions and immutable UI observations. The
// optional resource argument adds both-locale font/pixel/visual acceptance.
#include "../src/app.cpp"
#include "card_grid_fixture.hpp"
#include "content_fixture.hpp"
#include "selection_label_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static const char*scenario="setup";
static const char*captures;
static bool actual;
static std::vector<uint32_t> glyph_keys;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"deck selection [%s]:%d %s phase=%u candidate=%d\n",scenario,__LINE__,#x,unsigned(a.game.phase),a.deck_candidate);std::exit(1);}}while(0)
static bool read_pack(void*context,uint32_t off,void*out,uint32_t count){auto*f=static_cast<FILE*>(context);return !std::fseek(f,long(off),SEEK_SET)&&std::fread(out,1,count,f)==count;}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void settle(unsigned time=400){while(time){const unsigned step=time>100?100:time;a.advance(step);time-=step;}a.render();}
static void prepare_shop(){
    // Without a host timer, production preparation advances four bounded
    // decoder steps per frame, not one complete image/card. Keep that real
    // path and wait for its complete images, glyphs and alternate idle cycle
    // before measuring the independent rug tween.
    a.render();for(unsigned steps=0;a.merchant_preparation.active&&steps<8192;++steps){a.advance(33);a.render();}
    CHECK(!a.merchant_preparation.active);
    // The no-argument command/geometry fixture intentionally has no open
    // resource source. Only the actual-resource mode can assert I/O health.
    CHECK(!actual||a.content.healthy());
}
static const HitRegion*hit(HitKind kind,int value){for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return &a.hits[i];return nullptr;}
static void click(HitKind kind,int value){a.render();const auto*h=hit(kind,value);if(!h||!h->enabled)std::fprintf(stderr,"missing hit kind=%u value=%d cursor=%d open=%d clock=%u sold=%u\n",unsigned(kind),value,a.cursor,int(a.shop_open),a.map_clock_ms,a.game.shop_sold);CHECK(h&&h->enabled);const HitRegion copy=*h;CHECK(copy.x+copy.w>0&&copy.x<320&&copy.y>=0&&copy.y+copy.h<=240);a.pointer_move(copy.x+copy.w/2,copy.y+copy.h/2);a.pointer_button(true);a.pointer_button(false);a.render();}
static void capture(const char*name){if(!captures)return;const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){const unsigned char rgb[]={static_cast<unsigned char>(((p>>11)&31)*255/31),static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static bool route_to(const reference::ReferenceMap&map,unsigned point,reference::PointType type,unsigned depth=0){if(point>=map.count||depth>reference::MaxRows+2)return false;if(map.points[point].type==type)return true;for(unsigned i=0;i<map.points[point].children.used;++i)if(route_to(map,map.points[point].children.slots[i],type,depth+1))return true;return false;}
static void room(WorldRoom target,unsigned minimum_floor=0){
    new_run(a.game,0x7195c3,false,Character::Ironclad);CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017"));CHECK(world_begin_run(a.game));
    for(unsigned guard=0;guard<120;++guard){
        if(a.game.world.room_open&&a.game.world.room_type==unsigned(target)&&a.game.floor>=minimum_floor){
            a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.reset_presentation(true);a.progress.tutorial_flags=0;a.pointer.visible=false;a.render();settle(1200);CHECK(validate(a.game));return;
        }
        if(world_can_travel(a.game)){uint8_t points[reference::Columns];const unsigned count=world_available_points(a.game,points,reference::Columns);CHECK(count);unsigned selected=points[0];
            if(target==WorldRoom::Shop){const auto*map=world_current_map(a.game);CHECK(map);bool found=false;for(unsigned i=0;i<count;++i)if(route_to(*map,points[i],reference::PointType::Shop)){selected=points[i];found=true;break;}CHECK(found);}
            CHECK(world_choose_point(a.game,selected));
        }else{if(a.game.original_events.active)a.game.original_events=OriginalEventState{};if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;CHECK(world_finish_room(a.game));}
    }CHECK(false);
}
static void two_digit_floor(){
    scenario="two-digit floor spacing";room(WorldRoom::Rest,10);CHECK(a.game.floor>=10);
    const auto*floor=hit(HitKind::HudInfo,HudFloor);const auto*boss=hit(HitKind::HudInfo,HudBoss);
    CHECK(floor&&boss&&floor->x+floor->w<boss->x);snapshot();capture("hud-two-digit-floor");unchanged();
}
static void begin_smith(){unsigned option=0;while(option<a.game.world.rest_count&&a.game.world.rest_options[option]!=unsigned(WorldRestOption::Smith))++option;CHECK(option<a.game.world.rest_count);click(HitKind::Choice,int(option));CHECK(a.game.phase==Phase::Upgrade&&a.room_presentation.rest.selection.pending);a.render();settle();}
static void font(const char*key,unsigned size,unsigned face){
    if(!actual)return;const char*text=a.content.world_string("card_selection",key);CHECK(*text);char formatted[256]{};LocalizedTemplate value(formatted,sizeof formatted,deck_selection_amount,nullptr);value.for_language(a.content.language()).format(text);
    CHECK(spire_test::selection_exact_glyphs(a,formatted,size,face,glyph_keys));
}
static void smith(){
    scenario="Smith grid/filter/toggle/cancel/confirm";room(WorldRoom::Rest);
    a.game.deck[0].upgraded=1;const unsigned expected=a.game.deck_count-1;begin_smith();
    uint16_t indices[MaxDeck];CHECK(a.deck_choice_items(indices,MaxDeck)==expected&&indices[0]==1);auto&grid=a.card_grids[unsigned(CardGridSurface::DeckChoice)];
    CHECK(grid.top==12&&grid.bottom==deck_selection_layout::UpgradeGridBottom&&grid.origin==15&&grid.count==int(expected)&&grid.columns==3);
    CHECK(!hit(HitKind::Choice,0));for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::Choice)CHECK(card_upgradeable(a.game.deck[a.hits[i].value]));
    a.pointer_move(310,30);a.render();CHECK(hit(HitKind::UiAction,int(UiAction::DeckViewUpgrades)));
    const auto*toggle=hit(HitKind::UiAction,int(UiAction::DeckViewUpgrades));
    CHECK(grid.bottom<toggle->y);for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::Choice)CHECK(a.hits[i].y+a.hits[i].h<=grid.bottom);
    snapshot();if(actual){CHECK(spire_test::selection_upgrade_label_pixels(a));unchanged();}capture("smith-grid");
    click(HitKind::UiAction,int(UiAction::DeckViewUpgrades));CHECK(a.deck_selection.show_upgrades);unchanged();capture("smith-grid-upgrades");
    a.input(Key::Right);a.render();CHECK(!a.deck_selection.show_upgrades&&!hit(HitKind::UiAction,int(UiAction::DeckViewUpgrades)));CHECK(a.cursor==2);unchanged();capture("smith-directional-grid");
    click(HitKind::Choice,1);CHECK(a.deck_candidate==1&&a.deck_candidate_valid()&&a.active_card_grid()<0);unchanged();settle();capture("smith-upgrade-preview");
    a.pointer_move(grid.card_x(0)+10,grid.card_y(0)+10);a.render();
    for(const auto&face:grid.faces)CHECK(!face.focused);
    CHECK(!hit(HitKind::GridScroll,int(CardGridSurface::DeckChoice)));
    // Readable LCD navigation reserves independent bottom corners without
    // overlapping the enlarged preview cards or their selected hit regions.
    const auto*confirm=hit(HitKind::UiAction,int(UiAction::ConfirmDeckChoice));const auto*back=hit(HitKind::UiAction,int(UiAction::Back));CHECK(confirm&&confirm->x==280&&confirm->y==215&&confirm->w==40&&confirm->h==24);CHECK(back&&back->x==0&&back->y==215&&back->w==confirm->w&&back->h==confirm->h);CHECK(!hit(HitKind::UiAction,int(UiAction::CancelDeckChoice)));
    a.pointer_move(deck_selection_layout::AfterX+20,deck_selection_layout::PreviewY+20);a.render();CardHoverTarget hover;CHECK(card_hover_target(a,hover)&&hover.card.upgraded&&hover.deck==1);
    click(HitKind::UiAction,int(UiAction::Back));CHECK(a.deck_candidate<0&&a.game.phase==Phase::Upgrade&&a.cursor==1);unchanged();settle();
    click(HitKind::UiAction,int(UiAction::Back));CHECK(a.game.phase==Phase::Camp&&!a.game.world.rest_pending&&!a.game.world.rest_used&&!a.game.deck[1].upgraded);
    CHECK(!a.room_presentation.rest.selection.pending&&!a.room_presentation.rest.choices_locked());
    settle(1000);begin_smith();click(HitKind::Choice,1);settle();snapshot();
    // The confirmation crosses a complete presentation reset. Keep every
    // already-running deck effect and acquisition cue without whole-array
    // stack copies, including the last legal MaxDeck upgrade snapshot.
    auto&upgrades=a.reward_presentation.upgrades;
    for(unsigned i=0;i<MaxDeck-1;++i){auto*v=upgrades.add({Strike,1},{},int(i),a.map_clock_ms,RewardPresentation::Upgrades::Messy,i);CHECK(v);v->genetic=uint16_t(100+i);}
    a.reward_presentation.heal.begin(10,80,a.map_clock_ms);const auto heal_start=a.reward_presentation.heal.start;
    a.hud_relic_motion.flash(0,73,a.map_clock_ms);a.reward_presentation.removed.active=true;
    a.reward_presentation.enchants.add({{Defend,0},{},0,0},1,0,0,a.map_clock_ms);
    a.reward_presentation.list_scroll.viewport=93;a.reward_presentation.list_scroll.last_y=51;
    const uint16_t confirm_key=a.input_bindings.keys[0][unsigned(InputCommand::Confirm)];CHECK(confirm_key);CHECK(a.physical_key(confirm_key,true));unchanged();CHECK(a.deck_candidate>=0);CHECK(a.physical_key(confirm_key,false));CHECK(a.game.deck[1].upgraded&&a.game.phase==Phase::Camp&&a.game.world.rest_used);CHECK(validate(a.game));
    CHECK(upgrades.count==MaxDeck);for(unsigned i=0;i<MaxDeck-1;++i){CHECK(upgrades.cards[i].genetic==100+i&&upgrades.cards[i].style==RewardPresentation::Upgrades::Messy);}
    CHECK(upgrades.cards[MaxDeck-1].card.kind==a.game.deck[1].kind&&upgrades.cards[MaxDeck-1].card.upgraded&&upgrades.cards[MaxDeck-1].style==RewardPresentation::Upgrades::Smith);
    CHECK(a.reward_presentation.heal.active&&a.reward_presentation.heal.start==heal_start&&a.hud_relic_motion.flashing(0,73,a.map_clock_ms));
    CHECK(!a.reward_presentation.removed.active&&!a.reward_presentation.enchants.count&&!a.reward_presentation.transforms.count);
    CHECK(a.reward_presentation.list_scroll.viewport==174&&a.reward_presentation.list_scroll.last_y==0&&a.reward_presentation.list_focus==-1);
    snapshot();a.reset_presentation(true);unchanged();CHECK(!upgrades.count&&!a.reward_presentation.heal.active&&!a.hud_relic_motion.flash_count);
    for(const auto&v:upgrades.cards)CHECK(v.deck==-1);for(const auto&h:a.hud_relic_motion.holders)CHECK(h.from==4096&&!h.identity);
}
static void removal(){
    scenario="Merchant curse order / cancel / real price";room(WorldRoom::Shop);a.game.gold=9999;CardKind curse=CardKinds;for(unsigned i=0;i<CardKinds;++i)if(card_type(a.game,{CardKind(i),0})==CardType::Curse&&card_removable({CardKind(i),0})){curse=CardKind(i);break;}CHECK(curse<CardKinds);const int curse_index=event_add_deck_card(a.game,{curse,0});CHECK(curse_index>=0);
    a.ui_action(UiAction::OpenShop);prepare_shop();settle(1000);click(HitKind::Choice,ShopRemove);CHECK(a.game.phase==Phase::Remove);settle();
    uint16_t indices[MaxDeck];CHECK(a.deck_choice_items(indices,MaxDeck)==a.game.deck_count&&indices[0]==curse_index);CHECK(a.card_grids[unsigned(CardGridSurface::DeckChoice)].count==a.game.deck_count);capture("merchant-remove-grid");
    const unsigned count=a.game.deck_count,removals=a.game.removals;const int gold=a.game.gold,price=remove_price(a.game);CHECK(price>0);snapshot();
    click(HitKind::Choice,curse_index);CHECK(a.deck_candidate==curse_index);unchanged();settle();capture("merchant-remove-preview");
    click(HitKind::UiAction,int(UiAction::Back));CHECK(a.game.phase==Phase::Remove&&a.deck_candidate<0);unchanged();settle();click(HitKind::UiAction,int(UiAction::Back));CHECK(a.game.phase==Phase::Shop&&a.shop_open&&a.cursor==ShopRemove&&a.game.gold==gold&&a.game.deck_count==count&&a.game.removals==removals);
    click(HitKind::Choice,ShopRemove);settle();click(HitKind::Choice,curse_index);settle();
    a.paused=true;snapshot();a.ui_action(UiAction::ConfirmDeckChoice);unchanged();CHECK(a.deck_candidate==curse_index);a.paused=false;
    a.game.gold=price-1;snapshot();a.ui_action(UiAction::ConfirmDeckChoice);CHECK(a.deck_candidate<0&&a.game.phase==Phase::Remove);unchanged();a.game.gold=gold;a.render();click(HitKind::Choice,curse_index);settle();
    // An obsolete preview never commits after a state/revision replacement.
    ++a.game.revision;snapshot();a.ui_action(UiAction::ConfirmDeckChoice);CHECK(a.deck_candidate<0);unchanged();a.render();click(HitKind::Choice,curse_index);settle();
    click(HitKind::UiAction,int(UiAction::ConfirmDeckChoice));CHECK(a.game.phase==Phase::Shop&&a.shop_open&&a.game.gold==gold-price&&a.game.deck_count==count-1&&a.game.removals==removals+1);CHECK(validate(a.game));
}
static void long_grid(){
    scenario="filtered long grid keyboard + mouse scroll";room(WorldRoom::Rest);while(a.game.deck_count<MaxDeck)CHECK(event_add_deck_card(a.game,{Strike,0})>=0);for(unsigned i=0;i<MaxDeck;++i)a.game.deck[i].upgraded=uint8_t(i%3==0);begin_smith();snapshot();uint16_t indices[MaxDeck];const unsigned count=a.deck_choice_items(indices,MaxDeck);CHECK(count==MaxDeck-MaxDeck/3);
    a.cursor=indices[0];for(unsigned i=0;i<count;++i)a.input(Key::Down);a.render();CHECK(a.cursor==indices[(count-1)/3*3]);CHECK(spire_test::focus_card_grid(a));unchanged();capture("smith-filtered-grid-bottom");
    CHECK(spire_test::drag_grid_thumb(a,false));CHECK(spire_test::drag_grid_thumb(a,true));unchanged();
    // Loading canonical state has no unchecked view or stale confirmation.
    a.cursor=indices[count-1];a.input(Key::Confirm);CHECK(a.deck_candidate>=0);a.deck_selection.show_upgrades=true;CHECK(load(a.game,before.bytes,before.size));a.reset_presentation(true);a.render();CHECK(a.deck_candidate<0&&!a.deck_selection.show_upgrades);unchanged();
}
int main(int argc,char**argv){
    FILE*file=nullptr;if(argc>1){file=std::fopen(argv[1],"rb");CHECK(file);const size_t size=spire_test::pack_size(file);uint8_t header[40];CHECK(read_pack(file,0,header,sizeof header));const auto read32=[](const uint8_t*p){return unsigned(p[0])|unsigned(p[1])<<8|unsigned(p[2])<<16|unsigned(p[3])<<24;};const unsigned images=read32(header+16),glyphs=read32(header+20);for(unsigned i=0;i<glyphs;++i){uint8_t row[16];CHECK(read_pack(file,40+(images+i)*16,row,16));glyph_keys.push_back(read32(row)|(unsigned(row[8])<<24));}CHECK(std::is_sorted(glyph_keys.begin(),glyph_keys.end()));CHECK(a.content.open(ResourceSource{file,uint32_t(size),read_pack,nullptr}));actual=true;}if(argc>2)captures=argv[2];
    font("TO_UPGRADE",8,0);font("TO_REMOVE",8,0);font("TO_TRANSFORM",8,0);font("VIEW_UPGRADES",8,5);smith();removal();long_grid();two_digit_floor();CHECK(!actual||a.content.healthy());if(file)CHECK(!std::fclose(file));std::printf("deck selection UI: %u checks passed%s\n",checks,actual?" (actual source assets)":"");
}
