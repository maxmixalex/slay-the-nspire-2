#include "app.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "ui_core_observer.hpp"
#include "content_fixture.hpp"
#include "selection_label_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>
#include "../src/app.cpp"
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static const char*scenario="setup";
static const char*captures;
static std::vector<uint32_t> glyph_keys;
static bool actual=false;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world selection [%s]:%d %s phase=%u kind=%u selected=%u preview=%d\n",scenario,__LINE__,#x,unsigned(a.game.phase),unsigned(a.world_selection_kind()),a.choice_selected_count,a.deck_selection.preview);std::exit(1);}}while(0)
static bool read_pack(void*context,uint32_t off,void*out,uint32_t count){auto*f=static_cast<FILE*>(context);return !std::fseek(f,long(off),SEEK_SET)&&std::fread(out,1,count,f)==count;}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void settle(unsigned time=600){while(time){const unsigned step=time>33?33:time;a.advance(step);a.render();time-=step;}}
static const HitRegion*hit(HitKind kind,int value){for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return &a.hits[i];return nullptr;}
static bool enabled(UiAction action){const auto*h=hit(HitKind::UiAction,int(action));return h&&h->enabled;}
static void click(HitKind kind,int value){a.render();const auto*h=hit(kind,value);if(!h||!h->enabled)std::fprintf(stderr,"missing hit=%u value=%d clock=%u detail=%d panel=%d\n",unsigned(kind),value,a.map_clock_ms,a.detail_card,a.panel);CHECK(h&&h->enabled);const auto copy=*h;a.pointer_move(copy.x+copy.w/2,copy.y+copy.h/2);a.pointer_button(true);a.pointer_button(false);a.render();}
static void action(UiAction value){click(HitKind::UiAction,int(value));}
static void capture(const char*name){if(!captures)return;const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){const unsigned char rgb[]={static_cast<unsigned char>(((p>>11)&31)*255/31),static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void reset(){new_run(a.game,0x981720,false,Character::Ironclad);a.game.phase=Phase::Map;init_world_relics(a.game);a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.progress.tutorial_flags=0;a.reset_presentation(true);a.pointer.visible=false;}
static void relic(RelicId id){reset();CHECK(gain_world_relic(a.game,id));resolve_pending(a.game);CHECK(a.game.phase==Phase::WorldRelicChoice);a.render();settle();CHECK(a.world_selection_active());}
static unsigned ids(uint16_t*out){const unsigned n=a.world_choice_options(out,MaxWorldUiChoices);CHECK(n<=MaxWorldUiChoices);return n;}
static void grid_contract(){auto&g=a.card_grids[unsigned(CardGridSurface::WorldChoice)];CHECK(g.top==12&&g.bottom==213&&g.origin==15&&g.columns==3);CHECK(!enabled(UiAction::ClearWorldChoice));CHECK(!hit(HitKind::Command,int(Key::Detail)));}
static void duplicate_and_remove(){
    scenario="Dolly native deck preview, cancellation, real duplicate";relic(WorldRelicDollysMirror);grid_contract();uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices)>1);const unsigned old=a.game.deck_count;
    CHECK(!a.world_selection_cancelable()&&!enabled(UiAction::Back));snapshot();capture("dolly-grid");click(HitKind::WorldChoice,choices[0]);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==1);unchanged();settle();capture("dolly-preview");
    CHECK(enabled(UiAction::Back)&&enabled(UiAction::ConfirmWorldChoice)&&a.active_card_grid()<0);
    a.pointer_move(90,10);a.render();
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        CHECK(h.kind!=HitKind::HudInfo&&h.kind!=HitKind::PotionSlot&&h.kind!=HitKind::OwnedRelic);
        CHECK(!(h.kind==HitKind::Command&&h.y==0));}
    unchanged();
    action(UiAction::Back);CHECK(!a.deck_selection.preview&&!a.choice_selected_count);unchanged();a.input(Key::Back);CHECK(a.game.phase==Phase::WorldRelicChoice);unchanged();
    click(HitKind::WorldChoice,choices[0]);settle();action(UiAction::ConfirmWorldChoice);CHECK(a.game.deck_count==old+1&&a.game.phase==Phase::Map);CHECK(validate(a.game));

    scenario="Empty Cage filtered removal, max preview, stale context";reset();CardKind curse_model=CardKinds;for(unsigned k=0;k<CardKinds;++k)if(definitions[k].type==CardType::Curse&&card_removable({CardKind(k),0})){curse_model=CardKind(k);break;}CHECK(curse_model<CardKinds);const int curse=event_add_deck_card(a.game,{curse_model,0});CHECK(curse>=0);CHECK(gain_world_relic(a.game,WorldRelicEmptyCage));a.render();settle();CHECK(ids(choices)>2&&choices[0]==curse);grid_contract();snapshot();
    click(HitKind::WorldChoice,choices[0]);CHECK(a.choice_selected_count==1&&!a.deck_selection.preview&&!a.world_selection_confirmable());unchanged();
    click(HitKind::WorldChoice,choices[1]);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==2);unchanged();settle();capture("empty-cage-preview");
    const uint16_t confirm=a.input_bindings.keys[0][unsigned(InputCommand::Confirm)];CHECK(a.physical_key(confirm,true));unchanged();a.paused=true;CHECK(a.physical_key(confirm,false));unchanged();a.paused=false;
    CHECK(load(a.game,before.bytes,before.size));a.reset_presentation(true);a.render();CHECK(!a.deck_selection.preview&&!a.choice_selected_count);unchanged();settle();
    click(HitKind::WorldChoice,choices[0]);click(HitKind::WorldChoice,choices[1]);settle();const unsigned deck=a.game.deck_count;CHECK(a.physical_key(confirm,true));CHECK(a.game.deck_count==deck);CHECK(a.physical_key(confirm,false));CHECK(a.game.deck_count==deck-2&&a.game.phase==Phase::Map);CHECK(validate(a.game));
}
static void enchantments(){
    scenario="single enchant preview owns cloned facts";relic(WorldRelicElectricShrymp);uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices));snapshot();click(HitKind::WorldChoice,choices[0]);CHECK(a.deck_selection.preview);settle();
    auto&grid=a.card_grids[unsigned(CardGridSurface::WorldChoice)];CHECK(grid.columns==2&&grid.card_x(1)+CardGridScroll::CardW<187);
    for(const auto&face:grid.faces)CHECK(!face.focused);
    WorldSelectionCard left{},right{};CHECK(a.world_selection_preview_card(0,left)&&a.world_selection_preview_card(1,right));CHECK(left.card.kind==right.card.kind&&left.facts.enchantment==EventEnchantment::None&&right.facts.enchantment==EventEnchantment::Imbued&&right.facts.enchantment_amount==1);
    CHECK(left.x+left.w<=right.x&&right.x+right.w<=200);
    a.pointer_move(right.x+right.w/2,right.y+right.h/2);a.render();CardHoverTarget target;CHECK(card_hover_target(a,target)&&target.card_facts()&&target.card_facts()->enchantment==EventEnchantment::Imbued);
    // A real hovered preview previously drew a second Imbued tooltip over
    // the fixed enchantment pane. One bounded column now owns the set.
    Canvas canvas{a};HoverTipList tips;collect_visible_hover_tips(canvas,target,tips);unsigned pending=0;
    for(unsigned i=0;i<tips.count;++i)pending+=tips.items[i].kind==HoverTipKind::Enchantment&&hover_key_equal(tips.items[i].key,"IMBUED");CHECK(pending==1);
    const auto pages=card_hover_page(a,target,tips);for(int page=0;page<pages.pages;++page){const auto layout=card_hover_page(a,target,tips,page);
        for(unsigned i=0;i<layout.count;++i){const auto&r=layout.pieces[i].box;CHECK(r.x>=212&&r.x+r.w<=316&&r.y>=24&&r.y+r.h<=192);
            for(unsigned j=0;j<i;++j){const auto&other=layout.pieces[j].box;CHECK(r.x+r.w<=other.x||other.x+other.w<=r.x||r.y+r.h<=other.y||other.y+other.h<=r.y);}}
    }
    std::vector<uint16_t> duplicate(ScreenW*ScreenH);canvas.pixels=duplicate.data();world_enchantment_description(canvas);
    CHECK(std::none_of(duplicate.begin(),duplicate.end(),[](uint16_t pixel){return pixel!=0;}));
    const CardHoverTarget copied=target;CHECK(copied.card_facts()!=target.card_facts()&&copied.card_facts()->enchantment==EventEnchantment::Imbued);unchanged();capture("enchant-single-preview");
    click(HitKind::WorldPreviewCard,1);CHECK(a.detail_has_facts&&a.detail_facts.enchantment==EventEnchantment::Imbued);a.input(Key::Back);CHECK(a.detail_card<0&&a.deck_selection.preview);unchanged();
    settle();action(UiAction::ConfirmWorldChoice);CHECK(a.game.event_cards.deck[choices[0]].enchantment==EventEnchantment::Imbued);CHECK(validate(a.game));

    scenario="optional count is not cancelable or free enchant skip";relic(WorldRelicGnarledHammer);CHECK(ids(choices)>1);CHECK(!a.world_selection_cancelable()&&a.world_selection_min()==0&&a.world_selection_confirmable());snapshot();
    action(UiAction::ConfirmWorldChoice);CHECK(a.deck_selection.preview&&!a.choice_selected_count&&a.world_selection_confirmable());settle();CHECK(enabled(UiAction::ConfirmWorldChoice));a.confirm_world_choice();unchanged();action(UiAction::Back);CHECK(!a.deck_selection.preview);unchanged();
    click(HitKind::WorldChoice,choices[0]);CHECK(!a.deck_selection.preview);settle();action(UiAction::ConfirmWorldChoice);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==1);settle();capture("enchant-multi-one-selected");unchanged();
    // NDeckEnchant's multi preview retains the original models; only its
    // single preview draws an enchanted clone. Both commit through the core.
    CHECK(a.world_selection_preview_card(0,left)&&left.facts.enchantment==EventEnchantment::None);action(UiAction::ConfirmWorldChoice);CHECK(a.game.event_cards.deck[choices[0]].enchantment==EventEnchantment::Sharp);CHECK(validate(a.game));

    scenario="two-card enchant preview stays out of its sidebar";relic(WorldRelicGnarledHammer);CHECK(ids(choices)>1);
    click(HitKind::WorldChoice,choices[0]);click(HitKind::WorldChoice,choices[1]);settle();action(UiAction::ConfirmWorldChoice);settle();
    CHECK(a.world_selection_preview_cards()==2&&a.world_selection_preview_card(0,left)&&a.world_selection_preview_card(1,right));
    CHECK(left.x>=0&&left.x+left.w<=right.x&&right.x+right.w<212);
    a.pointer_move(30,40);a.render();for(const auto&face:a.card_grids[unsigned(CardGridSurface::WorldChoice)].faces)CHECK(!face.focused);
    capture("enchant-two-card-preview");action(UiAction::Back);

    scenario="optional enchant keeps source MaxSelect with one eligible card";reset();while(a.game.deck_count>1)CHECK(event_remove_deck_card(a.game,a.game.deck_count-1));CHECK(gain_world_relic(a.game,WorldRelicGnarledHammer));a.render();settle();CHECK(ids(choices)==1&&a.world_choice_max()==1&&a.world_selection_max()==3);snapshot();click(HitKind::WorldChoice,choices[0]);CHECK(!a.deck_selection.preview&&a.world_selection_confirmable());unchanged();settle();action(UiAction::ConfirmWorldChoice);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==1);settle();action(UiAction::ConfirmWorldChoice);CHECK(a.game.event_cards.deck[0].enchantment==EventEnchantment::Sharp);CHECK(validate(a.game));
}
static void transformations(){
    scenario="Claws fixed transformation preview and view upgrades";relic(WorldRelicClaws);uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices)>2);a.pointer_move(310,30);a.render();CHECK(enabled(UiAction::DeckViewUpgrades));snapshot();if(actual){CHECK(spire_test::selection_upgrade_label_pixels(a));unchanged();}action(UiAction::DeckViewUpgrades);CHECK(a.deck_selection.show_upgrades);unchanged();a.input(Key::Right);a.render();CHECK(!a.deck_selection.show_upgrades&&!enabled(UiAction::DeckViewUpgrades));
    click(HitKind::WorldChoice,choices[0]);click(HitKind::WorldChoice,choices[1]);settle();action(UiAction::ConfirmWorldChoice);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==4);settle();
    WorldSelectionCard model{};for(unsigned i=2;i<4;++i){CHECK(a.world_selection_preview_card(i,model)&&model.card.kind==EventCardMaul&&model.hover);CHECK(model.x>=0&&model.x+model.w<=320);}unchanged();capture("claws-transform-preview");
    action(UiAction::Back);CHECK(!a.deck_selection.preview&&!a.choice_selected_count);unchanged();click(HitKind::WorldChoice,choices[0]);settle();action(UiAction::ConfirmWorldChoice);settle();const unsigned deck_count=a.game.deck_count;const CardKind original=a.game.deck[choices[0]].kind;unsigned original_count=0;for(unsigned i=0;i<deck_count;++i)original_count+=a.game.deck[i].kind==original;
    action(UiAction::ConfirmWorldChoice);unsigned mauls=0,remaining=0;for(unsigned i=0;i<a.game.deck_count;++i){mauls+=a.game.deck[i].kind==EventCardMaul;remaining+=a.game.deck[i].kind==original;}CHECK(a.game.deck_count==deck_count&&mauls==1&&remaining+1==original_count);CHECK(validate(a.game));
    CHECK(a.reward_presentation.transforms.count==1&&!a.reward_presentation.removed.active);
    const auto&transformed=a.reward_presentation.transforms.pairs[0];CHECK(transformed.ready&&transformed.before.model.card.kind==original&&transformed.after.model.card.kind==EventCardMaul);
    snapshot();settle(750);capture("claws-transform-original");unchanged();settle(375);capture("claws-transform-reveal");unchanged();settle(1065);CHECK(transformed.launched);unchanged();

    scenario="Astrolabe random preview does not consume gameplay RNG";relic(WorldRelicAstrolabe);CHECK(ids(choices)>=3);snapshot();for(unsigned i=0;i<3;++i)click(HitKind::WorldChoice,choices[i]);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==6);settle();
    CHECK(a.world_selection_preview_card(3,model)&&!model.hover);const Card first=model.card;settle(200);CHECK(a.world_selection_preview_card(3,model)&&!model.hover&&model.card.kind!=first.kind);unchanged();capture("astrolabe-transform-preview");
    action(UiAction::ConfirmWorldChoice);CHECK(a.game.phase==Phase::Map);CHECK(validate(a.game));
    CHECK(a.reward_presentation.transforms.count==3&&!a.reward_presentation.removed.active);
    for(unsigned i=0;i<3;++i)CHECK(a.reward_presentation.transforms.pairs[i].ready&&a.reward_presentation.transforms.pairs[i].after.model.card.upgraded);
    snapshot();settle(1125);capture("astrolabe-transform-reveal");unchanged();

    scenario="event transform keeps detached genetic facts";reset();a.game.deck[0]={DefectGeneticAlgorithm,0};a.game.defect.genetic_deck[0]=17;
    {UiCoreObserver observation(a);CHECK(event_replace_deck_card(a.game,0,{DefectBallLightning,0}));}
    CHECK(a.reward_presentation.transforms.count==1&&!a.reward_presentation.removed.active);
    const auto&event_transform=a.reward_presentation.transforms.pairs[0];CHECK(event_transform.ready&&event_transform.before.model.card.kind==DefectGeneticAlgorithm&&event_transform.before.model.genetic==17&&event_transform.after.model.card.kind==DefectBallLightning);
    snapshot();settle(1200);unchanged();
}
static void simple_and_bundle(){
    scenario="Sea Glass source SimpleGrid manual arbitrary subset";relic(WorldRelicSeaGlass);uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices)>2);CHECK(a.world_selection_kind()==WorldSelectionKind::Simple&&a.world_selection_manual()&&!a.world_selection_cancelable());grid_contract();snapshot();const unsigned old=a.game.deck_count;
    click(HitKind::WorldChoice,choices[0]);click(HitKind::WorldChoice,choices[1]);CHECK(a.choice_selected_count==2&&!a.deck_selection.preview);unchanged();settle();capture("sea-glass-simple-grid");action(UiAction::ConfirmWorldChoice);CHECK(a.game.deck_count==old+2);CHECK(validate(a.game));
    const OriginalEventId events[]={OriginalEventId::BrainLeech,OriginalEventId::RoomFullOfCheese};
    for(auto event:events){scenario="event SimpleGrid completes only at source maximum";reset();CHECK(original_event_begin(a.game,event,123,1));CHECK(original_event_choose(a.game,0));a.render();settle();CHECK(a.world_selection_kind()==WorldSelectionKind::Simple&&!a.world_selection_manual());const unsigned count=ids(choices),maximum=a.world_choice_max(),deck=a.game.deck_count;CHECK(count>maximum);grid_contract();snapshot();
        if(maximum>1){click(HitKind::WorldChoice,choices[0]);CHECK(a.choice_selected_count==1&&!a.deck_selection.preview);unchanged();capture("cheese-simple-grid");}
        else capture("brain-leech-simple-grid");click(HitKind::WorldChoice,choices[maximum-1]);CHECK(a.game.deck_count==deck+maximum&&a.game.phase==Phase::Event);CHECK(validate(a.game));}

    scenario="Scroll Boxes bundle, preview, inspect, cancel and commit";relic(WorldRelicScrollBoxes);CHECK(a.world_selection_kind()==WorldSelectionKind::Bundle&&ids(choices)==2);CHECK(a.active_card_grid()<0&&!enabled(UiAction::Back));snapshot();capture("scroll-boxes-bundles");click(HitKind::WorldChoice,1);CHECK(a.deck_selection.preview&&a.world_selection_preview_cards()==3);unchanged();settle();capture("scroll-boxes-preview");
    click(HitKind::WorldPreviewCard,1);CHECK(a.detail_card==a.game.world_relics.offers[4]);a.input(Key::Back);CHECK(a.detail_card<0&&a.deck_selection.preview);unchanged();action(UiAction::Back);CHECK(!a.deck_selection.preview&&!a.choice_selected_count);unchanged();
    click(HitKind::WorldChoice,0);settle();const unsigned deck=a.game.deck_count;action(UiAction::ConfirmWorldChoice);CHECK(a.game.deck_count==deck+3&&a.game.phase==Phase::Map);CHECK(validate(a.game));
}
static void sealed(){
    scenario="Sealed Deck remains manual after tenth selection";reset();CHECK(configure_run_options(a.game,RunMode::Custom,run_modifier_bit(RunModifier::SealedDeck)));CHECK(world_begin_run(a.game));CHECK(original_event_choose(a.game,0));a.render();settle();
    CHECK(neow_modifier_selection(a.game,RunModifier::SealedDeck));uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices)==30&&a.world_selection_kind()==WorldSelectionKind::Simple&&a.world_selection_manual());const unsigned deck=a.game.deck_count;snapshot();
    const auto press=[](uint16_t code){CHECK(a.physical_key(code,true));a.render();CHECK(a.physical_key(code,false));a.render();};
    // Exercise the same semantic Select and physical arrows as the browser.
    // NCardGrid wraps horizontally within the row; Right never selects the
    // next row's first card without its separate Down input.
    CHECK(a.card_grids[unsigned(CardGridSurface::WorldChoice)].columns==3);
    for(unsigned i=0;i<9;++i){
        CHECK(a.cursor==int(i));a.input(Key::Confirm);a.render();CHECK(a.choice_selected_count==i+1);unchanged();
        press(InputRight);CHECK(a.cursor==int(i%3==2?i-2:i+1));
        if(i%3==2){press(InputDown);CHECK(a.cursor==int(i+1));}
    }
    CHECK(a.cursor==9&&a.choice_selected_count==9);press(27);CHECK(a.choice_selected_count==9&&!a.deck_selection.preview);unchanged();
    a.input(Key::Confirm);a.render();CHECK(a.choice_selected_count==10&&!a.deck_selection.preview&&a.game.deck_count==deck);unchanged();settle();capture("sealed-ten-selected");
    a.input(Key::Confirm);a.render();CHECK(a.choice_selected_count==9&&!a.world_selection_confirmable());press('E');CHECK(a.choice_selected_count==9);unchanged();
    a.input(Key::Confirm);a.render();CHECK(a.choice_selected_count==10&&a.world_selection_confirmable());
    CHECK(a.physical_key('E',true));a.render();CHECK(a.choice_selected_count==10&&a.game.deck_count==deck);unchanged();
    CHECK(a.physical_key('E',false));a.render();CHECK(a.game.deck_count==10&&a.game.phase==Phase::Event);CHECK(validate(a.game));
}
static void cook(){
    scenario="Meat Cleaver Cook real rest option, two-level cancel";reset();CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017"));CHECK(world_begin_run(a.game));CHECK(gain_world_relic(a.game,WorldRelicMeatCleaver));
    bool entered=false;for(unsigned guard=0;guard<120;++guard){
        if(a.game.world.room_open&&a.game.world.room_type==unsigned(WorldRoom::Rest)){entered=true;break;}
        if(world_can_travel(a.game)){uint8_t points[reference::Columns];CHECK(world_available_points(a.game,points,reference::Columns));CHECK(world_choose_point(a.game,points[0]));}
        else{if(a.game.original_events.active)a.game.original_events=OriginalEventState{};if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;CHECK(world_finish_room(a.game));}
    }CHECK(entered);a.reset_presentation(true);a.render();settle(1200);
    unsigned option=0;while(option<a.game.world.rest_count&&a.game.world.rest_options[option]!=unsigned(WorldRestOption::Cook))++option;CHECK(option<a.game.world.rest_count);
    click(HitKind::Choice,int(option));CHECK(a.game.phase==Phase::WorldRelicChoice&&a.game.world.rest_pending==option+1);settle();CHECK(a.world_selection_min()==2&&a.world_selection_cancelable()&&!a.world_selection_confirmable());snapshot();
    uint16_t choices[MaxWorldUiChoices];CHECK(ids(choices)>=2);click(HitKind::WorldChoice,choices[0]);CHECK(!a.deck_selection.preview&&!a.world_selection_confirmable());unchanged();click(HitKind::WorldChoice,choices[1]);CHECK(a.deck_selection.preview);settle();capture("cook-two-card-preview");unchanged();
    action(UiAction::Back);CHECK(!a.deck_selection.preview&&!a.choice_selected_count);unchanged();action(UiAction::Back);CHECK(a.game.phase==Phase::Camp&&!a.game.world.rest_pending&&!a.game.world.rest_used&&!a.room_presentation.rest.selection.pending);CHECK(validate(a.game));
    settle(1200);click(HitKind::Choice,int(option));settle();const unsigned deck=a.game.deck_count;const int maximum=a.game.max_hp;CHECK(ids(choices)>=2);click(HitKind::WorldChoice,choices[0]);click(HitKind::WorldChoice,choices[1]);settle();action(UiAction::ConfirmWorldChoice);CHECK(a.game.deck_count==deck-2&&a.game.max_hp>maximum&&a.game.phase==Phase::Camp&&(a.game.world.rest_used&(uint32_t(1)<<option)));CHECK(validate(a.game));
}
static void potion_focus_only(){
    const auto focus=[](){
        a.render();settle();a.cursor=0;CHECK(a.world_choice_potion(0)!=Potion::Empty);snapshot();const auto phase=a.game.phase;
        a.world_choice_detail(0);CHECK(a.game.phase==phase&&a.cursor==0&&a.detail_item<0&&a.detail_card<0&&!a.panel);unchanged();
        a.input(Key::Detail);CHECK(a.game.phase==phase&&a.cursor==0&&a.detail_item<0&&a.detail_card<0&&!a.panel);unchanged();
    };
    scenario="legacy pending relic potion has hover, no inspector";reset();
    // This pending shape remains valid for existing canonical saves even
    // though current Cauldron acquisition uses its grouped reward screen.
    auto&w=a.game.world_relics;w.choice=WorldRelicChoice::Potion;w.choice_max=1;w.choice_cancelable=1;w.return_phase=uint8_t(Phase::Map);w.offer_count=1;w.offers[0]=unsigned(Potion::Strength);a.game.phase=Phase::WorldRelicChoice;focus();
    scenario="real Cauldron potion reward retains focus";reset();CHECK(gain_world_relic(a.game,WorldRelicCauldron));CHECK(a.game.phase==Phase::WorldRelicChoice&&a.game.world_relics.choice==WorldRelicChoice::OfferReward);focus();
    scenario="real Drowning Beacon potion retains focus";reset();CHECK(original_event_begin(a.game,OriginalEventId::DrowningBeacon,17,1));CHECK(original_event_choose(a.game,0));CHECK(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Potion);focus();
    scenario="real Potion Courier group retains focus";reset();CHECK(original_event_begin(a.game,OriginalEventId::PotionCourier,19,1));CHECK(original_event_choose(a.game,0));CHECK(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Rewards);focus();
}
static void exact_font(const char*text,unsigned size,unsigned face=0){
    if(actual)CHECK(spire_test::selection_exact_glyphs(a,text,size,face,glyph_keys));
}
static void fonts(){
    if(!actual)return;scenario="exact source prompt and enchantment glyphs";
    exact_font(a.content.world_string("card_selection","VIEW_UPGRADES"),8,5);
    const CombatSelectionPrompt prompts[]={{"card_selection","TO_ENCHANT"},{"relics","DOLLYS_MIRROR.selectionScreenPrompt"},{"relics","CLAWS.selectionScreenPrompt"},{"relics","SEA_GLASS.selectionScreenPrompt"},{"events","BRAIN_LEECH.pages.SHARE_KNOWLEDGE.selectionScreenPrompt"},{"events","ROOM_FULL_OF_CHEESE.pages.GORGE.selectionScreenPrompt"},{"gameplay_ui","COMBAT_REWARD_CARD_REMOVAL.selectionScreenPrompt"},{"modifiers","SEALED_DECK.selectionPrompt"}};
    for(const auto&prompt:prompts){char raw[512]{};std::snprintf(raw,sizeof raw,"%s",a.content.world_string(prompt.table,prompt.key));CHECK(*raw);CombatPromptContext context{{&a,prompt.table,{},int(WorldRelicSeaGlass),-1,10},0,10};wt_model_key(context.model,prompt.key);char text[512]{};LocalizedTemplate(text,sizeof text,combat_prompt_value,&context,"Energy","Stars",a.content.english(),combat_prompt_catalog).for_language(a.content.language()).format(raw);exact_font(text,8);}
    exact_font(a.content.world_string("gameplay_ui","CHOOSE_A_PACK"),10,5);Canvas canvas{a};
    for(unsigned i=1;i<EventEnchantmentKinds;++i){CardHoverTarget owner{};HoverTip tip{};tip.kind=HoverTipKind::Enchantment;tip.key=event_enchantment_key(EventEnchantment(i));tip.amount=3;char title[128]{},body[2048]{};format_card_hover_tip(a,owner,tip,title,sizeof title,body,sizeof body);exact_font(title,10,5);exact_font(body,10);
        const int lines=source_hover_paragraph(canvas,body,0,0,90,0),visible=(180-source_hover_overhead(canvas))/source_hover_step(canvas);
        CHECK(lines>0&&visible>0);const int overflow=std::max(0,lines-visible)*source_hover_step(canvas);
        CHECK(text_overflow_offset(1600+(overflow*1000+11)/12,overflow,12)==overflow);
    }
    scenario="source hover bodies keep partial lines inside their viewport";
    const char*paragraph="First line.\nSecond line.\nThird line.\nFinal line.";
    constexpr int x=31,y=37,w=90,lines=2;const int bottom=y+lines*source_hover_step(canvas)+2;
    std::vector<uint16_t> initial(ScreenW*ScreenH),shifted(ScreenW*ScreenH);
    for(int offset=0;offset<=1;++offset){auto&pixels=offset?shifted:initial;std::fill(pixels.begin(),pixels.end(),uint16_t(0x1234));canvas.pixels=pixels.data();
        source_hover_paragraph(canvas,paragraph,x,y,w,lines,0,rgb(255,246,226),offset);unsigned ink=0;bool bounded=true;
        for(int yy=0;yy<ScreenH;++yy)for(int xx=0;xx<ScreenW;++xx){const auto value=pixels[yy*ScreenW+xx];
            if(xx<x||xx>=x+w||yy<y||yy>=bottom)bounded&=value==0x1234;else ink+=value!=0x1234;}
        CHECK(bounded&&ink);
    }
    CHECK(initial!=shifted);
    // The visible first line translates by exactly one pixel rather than
    // replacing the entire row only after a full line has passed.
    for(int yy=y;yy<y+source_hover_size(canvas)-1;++yy)for(int xx=x;xx<x+w;++xx)
        CHECK(shifted[yy*ScreenW+xx]==initial[(yy+1)*ScreenW+xx]);
}
int main(int argc,char**argv){
    FILE*file=nullptr;if(argc>1){file=std::fopen(argv[1],"rb");CHECK(file);const unsigned size=unsigned(spire_test::pack_size(file));uint8_t header[40];CHECK(read_pack(file,0,header,sizeof header));const auto read32=[](const uint8_t*p){return unsigned(p[0])|unsigned(p[1])<<8|unsigned(p[2])<<16|unsigned(p[3])<<24;};const unsigned images=read32(header+16),glyphs=read32(header+20);for(unsigned i=0;i<glyphs;++i){uint8_t row[16];CHECK(read_pack(file,40+(images+i)*16,row,16));glyph_keys.push_back(read32(row)|(unsigned(row[8])<<24));}CHECK(std::is_sorted(glyph_keys.begin(),glyph_keys.end()));CHECK(a.content.open(ResourceSource{file,size,read_pack,nullptr}));actual=true;}if(argc>2)captures=argv[2];
    fonts();duplicate_and_remove();enchantments();transformations();simple_and_bundle();sealed();cook();potion_focus_only();CHECK(!file||a.content.healthy());if(file)CHECK(!std::fclose(file));std::printf("world selection UI: %u checks passed%s\n",checks,argc>1?" (actual source assets)":"");
}
