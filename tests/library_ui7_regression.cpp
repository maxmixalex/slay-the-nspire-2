#include "app.hpp"
#include "card_grid_fixture.hpp"
#include "content_fixture.hpp"
#include "host_text_input.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;static unsigned checks;static const char*captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL library_ui7:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static HitRegion hit(HitKind kind,int value){a.render();for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){auto h=hit(kind,value);CHECK(h.enabled);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);}
static void control(int value){click(HitKind::LibraryControl,value);}
static void capture(const char*name){if(!captures)return;a.render();std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){uint8_t rgb[3]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void unchanged(const SaveData&before){SaveData now{};CHECK(save(a.game,now));CHECK(now.size==before.size&&!std::memcmp(now.bytes,before.bytes,now.size));}
static void pools_and_filters(){
    a.open_library(0);a.advance(200);a.advance(200);const int filters[]={1,2,5,6,4,3,7,8};const unsigned expected[]={85,86,86,86,86,53,19,67};
    bool all[CardKinds]{};CardKind ids[CardKinds];
    a.render();CHECK(a.card_grids[unsigned(CardGridSurface::Library)].columns==4);
    CHECK(CardGridScroll::CardW==46&&CardGridScroll::CardH==64&&CardGridScroll::row_offset(100)==7192);
    for(unsigned pool=0;pool<8;++pool){if(a.library_filter!=filters[pool])control(LibraryPoolFirst+int(pool));CHECK(a.library_filter==filters[pool]);const unsigned count=a.library_items(ids);CHECK(count==expected[pool]);for(unsigned i=0;i<count;++i){CHECK(ids[i]<CardKinds);all[ids[i]]=true;}}
    for(bool found:all)CHECK(found);
    control(LibraryPoolLast);CHECK(a.library_filter==0&&a.library_items()==0);control(LibraryPoolFirst);CHECK(a.library_items()==85);
    control(LibraryTypeFirst);unsigned count=a.library_items(ids);CHECK(count>0);for(unsigned i=0;i<count;++i)CHECK(definitions[ids[i]].type==CardType::Attack);
    control(LibraryTypeFirst+1);count=a.library_items(ids);CHECK(count>0);for(unsigned i=0;i<count;++i)CHECK(definitions[ids[i]].type==CardType::Attack||definitions[ids[i]].type==CardType::Skill);
    control(LibraryCostFirst+1);count=a.library_items(ids);CHECK(count>0);for(unsigned i=0;i<count;++i)CHECK(cost({ids[i],0})==1);
    control(LibraryTypeFirst);control(LibraryTypeFirst+1);control(LibraryCostFirst+1);CHECK(a.library_items()==85);
    control(LibrarySortCost);count=a.library_items(ids);for(unsigned i=1;i<count;++i)CHECK(cost({ids[i-1],0})<=cost({ids[i],0}));
    control(LibrarySortCost);a.library_items(ids);for(unsigned i=1;i<count;++i)CHECK(cost({ids[i-1],0})>=cost({ids[i],0}));
    const int offset=a.card_grids[unsigned(CardGridSurface::Library)].position;control(LibraryViewStats);CHECK(a.card_library.show_stats);a.render();CHECK(a.card_grids[unsigned(CardGridSurface::Library)].position==offset);capture("library-victory-stats");control(LibraryViewStats);CHECK(!a.card_library.show_stats);
    control(LibraryPoolFirst+6);for(int n=0;n<4;++n)CHECK(!hit(HitKind::LibraryControl,LibraryRarityFirst+n).enabled);
    capture("library-ancients-unknown");
}
static void discovery_search_and_capture(){
    a.open_library(0);std::memset(a.progress.discovered_cards,0,sizeof a.progress.discovered_cards);CardKind ids[CardKinds];const unsigned count=a.library_items(ids);CHECK(count==85);
    a.select_library_item(0);CHECK(a.detail_card<0);capture("library-unknown-cards");
    spire_test::discover_card(a,ids[0]);spire_test::discover_card(a,ids[2]);a.select_library_item(0);CHECK(a.detail_card>=0&&a.card_inspect.count==2);
    a.input(Key::Right);CHECK(card_from_token(a.detail_card).kind==ids[2]);a.input(Key::Right);CHECK(card_from_token(a.detail_card).kind==ids[2]);a.input(Key::Back);
    control(LibrarySearch);a.library_text(0x5347);a.library_text(0x7ea7);CHECK(!std::strcmp(a.card_library.draft,"升级"));a.library_text(8);CHECK(!std::strcmp(a.card_library.draft,"升"));a.library_text(8);
    for(char ch:std::string("  RaRe  "))a.library_text(ch);CHECK(a.card_library.query_pending&&!*a.card_library.query);a.advance(249);CHECK(!*a.card_library.query);a.advance(1);CHECK(!std::strcmp(a.card_library.query,"rare"));
    spire_test::discover_all_cards(a);a.card_library.invalidate();const unsigned rare=a.library_items(ids);CHECK(rare>0);for(unsigned i=0;i<rare;++i)CHECK(card_rarity(ids[i])==4);
    capture("library-rare-search");control(LibraryClearSearch);CHECK(!*a.card_library.query&&a.library_items()==85);a.library_text(13);CHECK(!a.library_search_editing);
    // A pending debounce invalidates an existing captured list index.
    a.library_text(13);control(LibrarySearch);a.library_text('x');auto first=hit(HitKind::LibraryItem,0);a.pointer_move(first.x+first.w/2,first.y+first.h/2);a.pointer_button(true);CHECK(a.pointer.held);a.advance(250);CHECK(!a.pointer.held);a.pointer_button(false);CHECK(a.detail_card<0);
    control(LibraryClearSearch);a.library_text(13);CHECK(spire_test::drag_grid_thumb(a,true));CHECK(spire_test::drag_grid_thumb(a,false));
    a.card_library.control=-1;a.library_selected=0;a.input(Key::Left);CHECK(a.card_library.control==LibraryPoolFirst);
    for(unsigned i=0;i<4;++i)a.input(Key::Right);CHECK(a.card_library.control<0&&a.library_selected==0);
}
static void hover_discovery_preserves_capture(){
    a.pointer.visible=false;std::memset(a.progress.discovered_cards,0,sizeof a.progress.discovered_cards);
    a.open_library(0);const int selected=spire_test::library_model_index(a,CloakAndDagger);CHECK(selected>=0);
    a.library_selected=selected;CHECK(spire_test::focus_card_grid(a));
    auto&grid=a.card_grids[unsigned(CardGridSurface::Library)];CHECK(grid.position>grid.minimum());
    const int position=grid.position;const uint32_t epoch=a.card_library.epoch;
    CHECK(!a.card_library.was_seen(Shiv));const auto h=hit(HitKind::LibraryItem,selected);
    a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.render();
    // NHoverTipSet discovers the real generated Shiv without rebuilding the
    // open library's visibility snapshot or moving its captured holder.
    CHECK(a.library_card_seen(Shiv)&&!a.card_library.was_seen(Shiv));
    CHECK(grid.position==position&&a.card_library.epoch==epoch);
    a.pointer_button(true);CHECK(a.pointer.held&&a.pointer.captured.kind==HitKind::LibraryItem&&a.pointer.captured.value==selected);
    a.discover_card(Soul);a.render();CHECK(grid.position==position&&a.card_library.epoch==epoch);
    a.pointer_button(false);CHECK(a.detail_card>=0&&card_from_token(a.detail_card).kind==CloakAndDagger);
    a.input(Key::Back);a.open_library(0);CHECK(a.card_library.was_seen(Shiv)&&a.card_library.was_seen(Soul));
}
static void collections(){
    for(int tab=1;tab<=2;++tab){a.open_library(tab);a.prepare_collection();const auto&v=a.collection;CHECK(v.count>0&&v.row_count>0);bool seen[WorldRelicKinds]{};
        for(unsigned i=0;i<v.count;++i){CHECK(v.ids[i]<(tab==1?WorldRelicKinds:PotionKinds));CHECK(!seen[v.ids[i]]);seen[v.ids[i]]=true;}
        if(tab==2){CHECK(v.count==PotionKinds&&v.category_count==4);for(unsigned i=0;i<PotionKinds;++i)CHECK(seen[i]);}
        else{CHECK(v.category_count==15&&seen[WorldRelicBurningBlood]&&seen[WorldRelicDivineRight]);CHECK(v.rows[0].count==5&&v.rows[1].count==5);}
        a.library_selected=0;CHECK(spire_test::focus_card_grid(a));click(HitKind::LibraryItem,0);CHECK(a.detail_item<0&&a.library_selected==0);
        a.pointer.visible=false;a.input(Key::Left);CHECK(a.library_selected==v.rows[0].count-1);a.input(Key::Right);CHECK(a.library_selected==0);
        capture(tab==1?"relic-collection-top":"potion-lab-top");CHECK(spire_test::drag_grid_thumb(a,true));capture(tab==1?"relic-collection-bottom":"potion-lab-bottom");CHECK(spire_test::drag_grid_thumb(a,false));
    }
}
static void host_input(){
    std::vector<int> codes;auto emit=[&](int code){codes.push_back(code);};host_text_utf8("A升级",7,emit);CHECK(codes.size()==3&&codes[0]=='A'&&codes[1]==0x5347&&codes[2]==0x7ea7);
    codes.clear();const char invalid[]={char(0xc0),char(0xaf),char(0xed),char(0xa0),char(0x80),'z'};host_text_utf8(invalid,sizeof invalid,emit);CHECK(codes.size()==1&&codes[0]=='z');
    uint16_t current[8]{},previous[8]{};current[4]=0x40;calculator_text_edges(current,previous,emit);CHECK(codes.back()=='a');previous[4]=current[4];const size_t size=codes.size();calculator_text_edges(current,previous,emit);CHECK(codes.size()==size);
    current[4]=0x04;current[7]=0x100;calculator_text_edges(current,previous,emit);CHECK(codes.back()=='D');current[4]=0;current[7]=0;current[6]=0x10;calculator_text_edges(current,previous,emit);CHECK(codes.back()=='2');
    CollectionPresentation motion{};CHECK(motion.observe_hover(1,true,false,100)==4096);
    for(int i=0;i<80;++i)if(i!=1)CHECK(motion.observe_hover(i,false,false,125)==4096);
    const int half=motion.observe_hover(1,true,false,125);CHECK(half>4096&&half<4915);
    CHECK(motion.observe_hover(1,false,false,125)==half);CHECK(motion.observe_hover(1,false,false,625)==4096);
    CHECK(motion.observe_hover(1,true,false,700)==4096);CHECK(motion.observe_hover(33,false,false,750)==4096);CHECK(motion.observe_hover(1,true,false,750)==4915);
}
static void observed_rewards(){
    a.library=a.compendium_open=a.statistics_open=a.history_open=a.title=false;
    new_run(a.game,7319,false);CHECK(world_begin_run(a.game));
    CHECK(a.game.original_events.id==OriginalEventId::Neow&&a.game.original_events.active);
    // Place this isolated reward test in an ordinary event room after the
    // automatically entered Ancient. It exercises the actual event reward
    // commands below, not an overlapping second active event.
    a.game.original_events=OriginalEventState{};a.game.phase=Phase::Map;a.reset_presentation();
    clear_ui_progress(a.progress);a.progress_loaded=true;a.begin_progress();
    CHECK(original_event_begin(a.game,OriginalEventId::ColorfulPhilosophers,5,0));CHECK(original_event_choose(a.game,0));
    CHECK(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Rewards);
    std::memset(a.progress.discovered_cards,0,sizeof a.progress.discovered_cards);a.card_library.invalidate();
    const auto&e=a.game.original_events;CHECK(e.reward_count&&e.rewards[0].kind==4&&e.rewards[0].count);
    CardKind offered[MaxOriginalEventOffers];const unsigned count=e.rewards[0].count;
    for(unsigned i=0;i<count;++i)offered[i]=card_from_token(e.reward_cards[e.rewards[0].first+i]).kind;
    SaveData before{};CHECK(save(a.game,before));a.render();
    for(unsigned i=0;i<count;++i)CHECK(!a.library_card_seen(offered[i]));unchanged(before);
    // CardReward.MarkContentAsSeen is deliberately empty. The actual card
    // selection binds these models only after its parent reward is opened.
    a.toggle_world_choice(0);CHECK(original_event_has_card_reward(a.game));CHECK(save(a.game,before));a.render();
    for(unsigned i=0;i<count;++i)CHECK(a.library_card_seen(offered[i]));unchanged(before);
    a.input(Key::Back);CHECK(!original_event_has_card_reward(a.game));
    // Observing all sixteen real belt holders includes expanded slots; it
    // still does not invent discovery from an uninspected canonical library.
    CHECK(add_potion_slots(a.game,MaxPotionSlots-potion_slot_count(a.game)));potion_set(a.game,MaxPotionSlots-1,Potion::ColorlessPotion);++a.game.revision;
    a.sync_progress();CHECK(a.library_item_seen(2,int(Potion::ColorlessPotion)-1));
    std::memset(a.progress.discovered_cards,0,sizeof a.progress.discovered_cards);a.open_library(0);a.render();
    for(unsigned i=0;i<CardKinds;++i)CHECK(!a.library_card_seen(CardKind(i)));
}
int main(int argc,char**argv){
    std::vector<uint8_t>bytes;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());std::fclose(f);CHECK(a.init(bytes.data(),bytes.size(),{}));}if(argc>2)captures=argv[2];
    new_run(a.game,711,true);a.title=false;a.reset_presentation();SaveData before{};CHECK(save(a.game,before));
    pools_and_filters();unchanged(before);discovery_search_and_capture();unchanged(before);hover_discovery_preserves_capture();unchanged(before);collections();unchanged(before);host_input();observed_rewards();
    std::printf("UI7 card library and collections: %u checks passed\n",checks);
}
