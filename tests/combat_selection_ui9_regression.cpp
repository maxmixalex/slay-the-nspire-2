// Exercise the real pending commands and private production drawing helpers.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "resource_transfer.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;
static State before,expected;
static unsigned checks;
static bool actual;
static const char*captures;
static const char*scenario="init";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat selection [%s]:%d %s phase=%u selection=%u clock=%u\n",scenario,__LINE__,#x,unsigned(a.game.phase),a.choice_selected_count,a.map_clock_ms);std::exit(1);}}while(0)
#include "ftue_fixture.hpp"
static void advance(unsigned milliseconds){while(milliseconds){const unsigned step=milliseconds>33?33:milliseconds;a.advance(step);milliseconds-=step;}a.render();}
static void snapshot(){std::memcpy(&before,&a.game,sizeof(State));}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof(State)));}
static void matches(){CHECK(validate(a.game));CHECK(!std::memcmp(&expected,&a.game,sizeof(State)));}
static bool has(HitKind kind,int value,bool enabled=true){a.render();for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==kind&&a.hits[i].value==value&&(!enabled||a.hits[i].enabled))return true;return false;}
static HitRegion region(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){
    a.render();bool located=false;int px=0,py=0;
    // An authored hand fan has overlapping rectangles. Pick a real visible
    // inverse-affine hit, not another holder's rectangle center.
    for(int y=239;y>=0&&!located;--y)for(int x=0;x<320;++x){const auto*h=a.hit_at(x,y);if(h&&h->kind==kind&&h->value==value&&h->enabled){px=x;py=y;located=true;break;}}
    CHECK(located);a.pointer_move(px,py);a.pointer_button(true);a.pointer_button(false);a.render();
}
static void capture(const char*name){if(!captures)return;a.render();std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
    for(unsigned i=0;i<ScreenW*ScreenH;++i){const auto p=a.pixels[i];const uint8_t rgb[3]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static int append(Card card,Zone zone){auto&s=a.game;CHECK(s.card_count<MaxCards);const int id=s.card_count++;s.cards[id]=card;s.zones[id]=zone;s.event_cards.deck_origin[id]=0;
    Pile*p=zone==Zone::Hand?&s.hand:zone==Zone::Draw?&s.draw:zone==Zone::Discard?&s.discard:&s.exhaust;CHECK(p->count<MaxCards);p->ids[p->count++]=uint16_t(id);return id;}
static void setup(Card card,Character character=Character::Ironclad){
    new_run(a.game,77,true,character);a.reset_presentation();a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
    a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;a.pointer.visible=false;
    a.game.energy=99;const int id=a.game.hand.ids[0];a.game.cards[id]=card;a.game.event_cards.deck_origin[id]=0;a.game.enemies[0].hp=a.game.enemies[0].max_hp=999;
    a.render();snapshot();const auto animation=a.animation;for(unsigned i=0;i<256&&a.combat_preparation.active;++i)a.advance(33);
    CHECK(!a.combat_preparation.active);CHECK(a.animation==animation);unchanged();CHECK(validate(a.game));
}
static void played(){
    {UiCoreObserver observer(a);CHECK(play(a.game,0,0));}
    CHECK(choice_phase(a.game.phase));snapshot();for(unsigned i=0;i<256&&!a.combat_choice_ready();++i)advance(33);CHECK(a.combat_choice_ready());a.render();unchanged();
}
static void no_legacy_controls(){CHECK(!has(HitKind::UiAction,int(UiAction::ClearChoice),false));CHECK(!has(HitKind::Command,int(Key::Detail),false));
    for(int i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);}
static void source_prompt_templates(){
    scenario="Original selector model variables";snapshot();
    for(unsigned upgraded=0;upgraded<2;++upgraded){
        CombatPromptContext context{{&a,"cards",{},int(Purity),-1,1,{Purity,uint8_t(upgraded)}},0,1};
        wt_model_key(context.model,"PURITY.selectionScreenPrompt");
        const auto value=combat_prompt_value(&context,"Cards");CHECK(value.known&&value.number==int(3+upgraded*2));
        char text[128]{};LocalizedTemplate(text,sizeof text,combat_prompt_value,&context,"Energy","Stars",true,combat_prompt_catalog).format("{Cards:diff()}");
        CHECK(!std::strcmp(text,upgraded?"5":"3"));unchanged();
    }
    if(!actual)return;
    // Read every appended source prompt, not a test-authored translation.
    // Cover both card variants and singular/plural/range amounts; unresolved
    // source variables must never leak into a displayed label.
    struct Entry{const char*key;unsigned index;};static constexpr Entry prompts[]={
#include "../src/selection_text_index.inc"
    };
    CHECK(sizeof prompts/sizeof*prompts==32);
    for(const auto&entry:prompts){char table[32]{},key[128]{};const char*split=std::strchr(entry.key,'.');CHECK(split&&size_t(split-entry.key)<sizeof table);
        std::memcpy(table,entry.key,size_t(split-entry.key));std::snprintf(key,sizeof key,"%s",split+1);
        char raw[512]{};std::snprintf(raw,sizeof raw,"%s",a.content.locale_string(table,key));CHECK(*raw);
        CardKind kind=Strike;if(!std::strcmp(table,"cards")){
            kind=CardKinds;const size_t length=size_t(std::strchr(key,'.')-key);
            for(unsigned i=0;i<CardKinds;++i)if(std::strlen(definitions[i].key)==length&&!std::strncmp(definitions[i].key,key,length)){kind=CardKind(i);break;}
            CHECK(kind<CardKinds);
        }
        for(unsigned upgraded=0;upgraded<2;++upgraded)for(unsigned count:{0u,1u,3u}){
            CombatPromptContext context{{&a,table,{},int(kind),-1,int(count),{kind,uint8_t(upgraded)}},0,count};wt_model_key(context.model,key);
            char text[512]{};LocalizedTemplate(text,sizeof text,combat_prompt_value,&context,"Energy","Stars",a.content.english(),combat_prompt_catalog).format(raw);
            if(std::strchr(text,'{')||std::strchr(text,'}'))std::fprintf(stderr,"unresolved selector %s: %s\n",entry.key,text);
            CHECK(*text&&!std::strchr(text,'{')&&!std::strchr(text,'}'));unchanged();
        }
    }
}
static void source_black_backstop(unsigned alpha){
    Canvas c{a};c.rect(0,0,320,240,0xffff);card_choice_screen(c);
    // Source player_hand SelectModeBackstop is black .75; run OverlayBackstop
    // is black .85. This unobstructed corner rejects the old tinted Ink dim.
    CHECK(a.pixels[0]==blend(0xffff,0,alpha));unchanged();
}
static void offers(){
    scenario="Discovery offer";setup({Discovery,0});played();CHECK(a.combat_selection_kind()==CombatSelectionKind::Offer);CHECK(a.game.expansion.choice_min==0);
    const auto opened=a.combat_selection.row.opened;const int id=a.game.expansion.choices.ids[1];snapshot();
    a.toggle_choice(id);CHECK(!a.choice_selected_count);unchanged();
    advance(350-(a.map_clock_ms-opened));CHECK(a.map_clock_ms-opened==350);a.toggle_choice(id);CHECK(!a.choice_selected_count);unchanged();
    advance(650);source_black_backstop(217);a.pointer_move(0,30);a.render();
    // Keyboard focus expands one offer. Moving away starts the original
    // 500ms unfocus tween; SmallScale assertions apply after it settles.
    advance(500);unchanged();no_legacy_controls();CHECK(!has(HitKind::UiAction,int(UiAction::ConfirmChoice),false));
    const auto number=a.input_bindings.keys[0][unsigned(InputCommand::Card1)];CHECK(a.physical_key(number,true));CHECK(a.physical_key(number,false));unchanged();CHECK(!a.choice_selected_count);
    CHECK(has(HitKind::UiAction,int(UiAction::SkipCombatChoice)));CHECK(a.active_card_grid()<0);
    for(unsigned i=0;i<3;++i){const auto h=region(HitKind::CombatChoice,a.game.expansion.choices.ids[i]);CHECK(h.w==73&&h.h==100);CHECK(h.x+h.w/2==95+int(i)*65);CHECK(h.y+h.h/2==131);}
    a.cursor=1;a.input(Key::Detail);CHECK(a.detail_card==card_token(choice_card(a.game,id)));a.input(Key::Back);unchanged();capture("combat-offer-discovery");
    std::memcpy(&expected,&a.game,sizeof(State));const uint16_t selected=uint16_t(id);CHECK(choose_extended(expected,&selected,1));click(HitKind::CombatChoice,id);CHECK(a.game.phase==Phase::Combat);matches();
    scenario="Offer skip release";setup({Discovery,0});played();snapshot();std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_extended(expected,nullptr,0));
    const auto cancel=a.input_bindings.keys[a.keyboard_only?1:0][unsigned(InputCommand::Cancel)];CHECK(a.physical_key(cancel,true));unchanged();CHECK(a.physical_key(cancel,false));matches();
    scenario="Mandatory Abundance";setup({EventCardAbundance,0});played();CHECK(a.combat_selection_kind()==CombatSelectionKind::Offer);CHECK(a.combat_selection_min()==1);
    CHECK(!has(HitKind::UiAction,int(UiAction::SkipCombatChoice),false));snapshot();a.skip_combat_selection();unchanged();advance(1000);capture("combat-offer-abundance");
}
static void hand(){
    scenario="Purity retained hand";setup({Purity,0});played();CHECK(a.combat_selection_kind()==CombatSelectionKind::Hand);CHECK(a.combat_selection_manual());
    if(actual){char prompt[512]{};combat_selection_text(a,prompt,sizeof prompt);CHECK(std::strstr(prompt,"3")&&!std::strchr(prompt,'{')&&!std::strchr(prompt,'}'));}
    advance(500);source_black_backstop(191);CHECK(a.active_card_grid()<0);no_legacy_controls();snapshot();
    const auto&pool=choice_pile(a.game);CHECK(pool.count>=4);
    // Keyboard focus retains the hand's lifted card. Verify that separately
    // before testing every unfocused holder at its ordinary fan position.
    CHECK(region(HitKind::CombatChoice,pool.ids[a.cursor]).y<165);
    a.pointer_move(0,30);advance(500);unchanged();
    for(unsigned i=0;i<pool.count;++i){const auto h=region(HitKind::CombatChoice,pool.ids[i]);CHECK(h.y>165);}
    a.toggle_choice(pool.ids[0]);a.toggle_choice(pool.ids[1]);a.toggle_choice(pool.ids[2]);CHECK(a.choice_selected_count==3);unchanged();
    a.toggle_choice(pool.ids[3]);CHECK(a.choice_selected_count==3&&a.choice_selected[0]==pool.ids[0]&&a.choice_selected[1]==pool.ids[1]&&a.choice_selected[2]==pool.ids[3]);unchanged();
    // Render the selection change before advancing its holder transition,
    // just as the ordinary host does after the released click.
    a.render();unchanged();
    advance(500);const auto selected=region(HitKind::CombatChoice,pool.ids[3]);CHECK(selected.y<120);capture("combat-hand-selected");
    const unsigned chosen=a.choice_selected_count;a.toggle_combat_peek();CHECK(a.choice_peeking&&a.choice_selected_count==chosen);a.render();CHECK(!has(HitKind::CombatChoice,pool.ids[3],false));
    for(unsigned i=0;i<chosen;++i)CHECK(!has(HitKind::InstanceCard,a.choice_selected[i],false));unchanged();a.toggle_combat_peek();
    std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_exhaust_many(expected,a.choice_selected,a.choice_selected_count));
    CHECK(has(HitKind::UiAction,int(UiAction::ConfirmChoice)));const auto confirm=a.input_bindings.keys[a.keyboard_only?1:0][unsigned(InputCommand::Confirm)];
    CHECK(a.physical_key(confirm,true));unchanged();CHECK(a.physical_key(confirm,false));matches();
    scenario="Upgraded Purity requested maximum survives the smaller pool";setup({Purity,1});played();CHECK(a.combat_selection_max()<5);snapshot();
    if(actual){char prompt[512]{};combat_selection_text(a,prompt,sizeof prompt);CHECK(std::strstr(prompt,"5")&&!std::strchr(prompt,'{')&&!std::strchr(prompt,'}'));}
    advance(500);capture("combat-hand-purity-upgraded");unchanged();
    scenario="Mandatory hand confirm";setup({Prepared,1},Character::Silent);played();CHECK(a.game.phase==Phase::DiscardChoice);CHECK(a.game.discard_required==2);advance(500);snapshot();
    CHECK(!has(HitKind::UiAction,int(UiAction::ConfirmChoice)));a.confirm_choice();unchanged();
    const auto first=a.game.hand.ids[0],second=a.game.hand.ids[1];a.toggle_choice(first);CHECK(a.choice_selected_count==1);a.confirm_choice();unchanged();
    a.toggle_choice(second);CHECK(a.choice_selected_count==2&&a.game.phase==Phase::DiscardChoice);unchanged();advance(350);capture("combat-hand-discard-two");
    std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_discard(expected,a.choice_selected,2));click(HitKind::UiAction,int(UiAction::ConfirmChoice));matches();
    scenario="Armaments hand preview";setup({Armaments,0});played();CHECK(a.combat_selection_kind()==CombatSelectionKind::HandUpgrade);advance(500);snapshot();
    const auto id=choice_pile(a.game).ids[0];a.toggle_choice(id);advance(500);CHECK(a.game.phase==Phase::ExtendedChoice);unchanged();capture("combat-hand-upgrade-preview");
    a.pointer_move(205,110);a.render();CardHoverTarget hover;CHECK(card_hover_target(a,hover)&&hover.card.upgraded&&hover.instance==id);
    a.pointer_button(true);a.pointer_button(false);CHECK(a.choice_selected_count==1);unchanged();
    a.pointer_move(110,110);a.render();CHECK(!card_hover_target(a,hover));a.pointer_button(true);a.pointer_button(false);CHECK(!a.choice_selected_count);unchanged();
    a.toggle_choice(id);advance(500);
    std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_extended(expected,a.choice_selected,1));a.confirm_choice();matches();
}
static void relic_offers(){
    for(RelicId relic:{WorldRelicHeftyTablet,WorldRelicLeadPaperweight})for(unsigned skip=0;skip<2;++skip){
        scenario=skip?"Native relic offer skip":"Native relic offer choose";
        new_run(a.game,79,false);a.reset_presentation();a.title=a.paused=a.help=a.library=a.character_select=false;a.progress.tutorial_flags=0;
        {UiCoreObserver observer(a);CHECK(gain_world_relic(a.game,relic));}
        CHECK(a.game.phase==Phase::WorldRelicChoice&&a.game.world_relics.choice==WorldRelicChoice::OfferCard);
        a.render();CHECK(a.combat_selection_kind()==CombatSelectionKind::Offer&&a.combat_selection_min()==0);snapshot();advance(1000);unchanged();
        CHECK(has(HitKind::UiAction,int(UiAction::SkipCombatChoice)));CHECK(!has(HitKind::UiAction,int(UiAction::ConfirmWorldChoice),false));
        const uint16_t selected=0;std::memcpy(&expected,&a.game,sizeof(State));CHECK(world_relic_select(expected,skip?nullptr:&selected,skip?0:1));
        if(!skip)capture(relic==WorldRelicHeftyTablet?"relic-offer-hefty-tablet":"relic-offer-lead-paperweight");
        if(skip)a.skip_combat_selection();else click(HitKind::WorldChoice,selected);matches();
    }
}
static void piles(){
    scenario="Headbutt pile automatic";setup({Headbutt,0});for(unsigned i=0;i<26;++i)append({Defend,0},Zone::Discard);played();CHECK(a.game.phase==Phase::ReturnChoice);
    CHECK(!a.combat_selection_manual());CHECK(a.active_card_grid()==int(CardGridSurface::CombatChoice));advance(500);snapshot();
    const auto&grid=a.card_grids[unsigned(CardGridSurface::CombatChoice)];CHECK(grid.top==12&&grid.bottom==213&&grid.origin==15&&grid.columns==3);CHECK(!has(HitKind::UiAction,int(UiAction::ConfirmChoice)));
    for(unsigned i=0;i<4;++i)a.input(Key::Down);advance(500);unchanged();const int id=choice_pile(a.game).ids[a.cursor];
    a.input(Key::Detail);CHECK(a.detail_instance==id);a.input(Key::Back);unchanged();capture("combat-pile-headbutt");
    std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_return_to_draw(expected,id));a.toggle_choice(id);CHECK(a.game.phase==Phase::Combat);matches();
    scenario="Neows Fury pile manual";setup({EventCardNeowsFury,0});for(unsigned i=0;i<6;++i)append({Defend,0},Zone::Discard);played();CHECK(a.combat_selection_kind()==CombatSelectionKind::Pile);CHECK(a.combat_selection_manual());
    advance(500);snapshot();CHECK(has(HitKind::UiAction,int(UiAction::ConfirmChoice)));const auto&pool=choice_pile(a.game);const unsigned maximum=a.combat_selection_max();
    for(unsigned i=0;i<maximum;++i)a.toggle_choice(pool.ids[i]);CHECK(a.choice_selected_count==maximum);CHECK(a.game.phase==Phase::ExtendedChoice);unchanged();advance(500);capture("combat-pile-neows-fury");
    std::memcpy(&expected,&a.game,sizeof(State));CHECK(choose_extended(expected,a.choice_selected,a.choice_selected_count));a.confirm_choice();matches();
    scenario="Potion source prompt";setup({Strike,0});for(unsigned i=0;i<3;++i)append({Defend,0},Zone::Discard);a.game.potions[0]=Potion::LiquidMemories;
    {UiCoreObserver observer(a);CHECK(use_potion(a.game,0,-1));}CHECK(a.game.phase==Phase::ExtendedChoice);a.render();CHECK(!std::strcmp(a.combat_selection_prompt().key,"LIQUID_MEMORIES.selectionScreenPrompt"));
}
static void source_geometry_and_ripple(){
    scenario="Original selection geometry";
    NativeConfirmPresentation motion;CHECK(motion.observe(1,false,0,0)==34);CHECK(motion.observe(1,false,0,800)==34);
    CHECK(motion.observe(1,true,0,800)==34);CHECK(motion.sample(1150)==0);CHECK(motion.observe(1,false,0,1200)==0);CHECK(motion.sample(1550)==34);
    const auto smooth=[](double a,double b,double value){double t=(value-a)/(b-a);if(t<0)t=0;if(t>1)t=1;return t*t*(3-2*t);};
    for(unsigned alpha=14000;alpha<65536;alpha+=127)for(unsigned time:{0u,59u,211u,399u,667u,1234u,2600451u})for(unsigned width:{0u,1638u,3277u,4915u}){
        const double input=double(alpha)/65535.0*.98,ripple=std::fmod(double(time%2000)*.00003+input,.02);
        const double brightness=input-ripple+smooth(0,.005,ripple)*.02;
        const int reference=width?int(smooth(1-double(width)/65536,1,brightness)*255):0;
        const int native=int(card_highlight::alpha(uint16_t(alpha),width,time));CHECK(std::abs(reference-native)<=1);
    }
    if(actual){CHECK(a.content.has_selection_assets());const auto image=a.content.image(a.content.ui_image(I9_CardHighlightSdf));CHECK(image.w==290&&image.h==181&&image.format==0&&image.pixels);}
}
int main(int argc,char**argv){
    Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);actual=true;}else data=fixture();
    CHECK(a.init(data.data(),data.size(),{}));if(argc>2)captures=argv[2];source_prompt_templates();offers();hand();piles();relic_offers();source_geometry_and_ripple();CHECK(a.content.healthy());
    std::printf("combat selection: %u checks passed (%s)\n",checks,actual?"actual":"synthetic");
}
