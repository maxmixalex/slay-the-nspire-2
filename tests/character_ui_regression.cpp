#include "app.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData stored;
static unsigned checks,acquisitions;
static bool real_profile,catalog_navigation_verified;
static const char* screenshot_directory;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static void progress(const char* stage,unsigned character=CharacterKinds,unsigned model=CardKinds,unsigned upgraded=0){
    std::printf("character UI progress: %s character=%u model=%u key=%s upgraded=%u page=%d acquisitions=%u checks=%u\n",
        stage,character,model,model<CardKinds?definitions[model].key:"-",upgraded,a.library_page,acquisitions,checks);
    std::fflush(stdout);
}
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned width=4){for(unsigned i=0;i<width;i++)b[at+i]=(uint8_t)(value>>(8*i));}
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned first=CardKinds>A_Count+160?CardKinds:A_Count+160;
    constexpr unsigned images=first>U_Count?first:U_Count,strings=ContentStrings;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);
    put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<strings;i++){
        const unsigned card=i/3;
        const char* text=i<CardKinds*3?(i%3?"Synthetic card description.":definitions[card].key):"Synthetic UI text.";
        if(card==GiantRock&&i%3)text=i%3==1?"Damage {Damage=20}.":"Damage {Damage=24}.";
        put(b,stringdir+i*8,(unsigned)b.size());put(b,stringdir+i*8+4,(unsigned)std::strlen(text));
        b.insert(b.end(),text,text+std::strlen(text)+1);
    }
    for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;i++)for(unsigned j=0;j<4;j++)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,(unsigned)b.size());uint32_t hash=2166136261u;
    for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);
    return b;
}
static HitRegion hit(HitKind kind,int value){
    a.render();for(int i=a.hit_count-1;i>=0;i--)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];
    std::fprintf(stderr,"missing hit %d/%d\n",(int)kind,value);CHECK(false);return {};
}
static void tap(HitKind kind,int value){
    auto h=hit(kind,value);
    if(!h.enabled)std::fprintf(stderr,"disabled hit %d/%d detail=%d page=%d phase=%d acquisitions=%u\n",(int)kind,value,a.detail_card,a.library_page,(int)a.game.phase,acquisitions);
    CHECK(h.enabled);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);
}
static void action(UiAction x){tap(HitKind::UiAction,(int)x);}
// Content/description fixtures create their inputs directly; production no
// longer exposes the old practice injection controls.
static void fixture_card(){CHECK(a.detail_card>=0&&lab_card(a.game,card_from_token(a.detail_card)));a.library=false;a.reset_presentation();}
static SaveData snapshot(){SaveData data{};CHECK(validate(a.game));CHECK(save(a.game,data));return data;}
static void unchanged(const SaveData&before){auto after=snapshot();CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void start(Character character){
    new_run(a.game,71,true,character);a.title=a.paused=a.help=a.library=a.character_select=a.map_preview.open=a.settings_open=false;
    a.selected_character=character;a.reset_presentation();
    a.library_tab=a.library_page=a.library_selected=a.library_filter=a.library_sort=0;a.library_upgraded=false;
    CHECK(!a.title&&!a.character_select&&a.game.character==character&&a.game.practice);
    CHECK(a.game.deck_count==(character==Character::Ironclad?138:139));
    CHECK(a.game.deck_count>128&&a.game.deck_count<=MaxDeck);CHECK(validate(a.game));
}
static void catalog_page_click(UiAction action,bool enabled,int expected_page){
    CHECK(a.library&&a.library_tab==0&&a.detail_card<0);
    CHECK(action==UiAction::NextPage||action==UiAction::PreviousPage);
    const int previous_page=a.library_page;
    CHECK(expected_page==previous_page+(enabled?(action==UiAction::NextPage?1:-1):0));
    progress(action==UiAction::NextPage?"catalog-next":"catalog-previous");
    a.render();int index=-1;
    for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==(int)action){
        CHECK(index<0);index=i;
    }
    CHECK(index>=0);const HitRegion h=a.hits[index];CHECK(h.w>0&&h.h>0&&h.enabled==enabled);
    const int x=h.x+h.w/2,y=h.y+h.h/2;
    CHECK(a.hit_at(x,y)==&a.hits[index]); // Unique action is also the topmost real pointer target.
    a.pointer_move(x,y);a.pointer_button(true);
    CHECK(a.pointer.held&&!a.pointer.dragging);
    CHECK(a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==(int)action&&a.pointer.captured.enabled==enabled);
    CHECK(a.library_page==previous_page); // A press alone must not turn the page.
    a.pointer_button(false);CHECK(!a.pointer.held);
    CHECK(a.library_page==expected_page&&a.library_selected==expected_page*8);
}
static void catalog_navigation(){
    CHECK(!catalog_navigation_verified);progress("catalog-navigation-start");
    const uint32_t run_number=a.run_number;
    start(Character::Ironclad);auto before=snapshot();
    a.input(Key::Collection);action(UiAction::LibraryCards);action(UiAction::FilterAll);
    CardKind ids[CardKinds];CHECK(a.library_items(ids)==CardKinds);
    const int last=(CardKinds-1)/8;CHECK(a.library_page==0&&last>0);
    catalog_page_click(UiAction::PreviousPage,false,0);unchanged(before);
    for(int page=1;page<=last;++page)catalog_page_click(UiAction::NextPage,true,page);
    catalog_page_click(UiAction::NextPage,false,last);unchanged(before);
    for(int page=last-1;page>=0;--page)catalog_page_click(UiAction::PreviousPage,true,page);
    catalog_page_click(UiAction::PreviousPage,false,0);unchanged(before);
    a.input(Key::Back);CHECK(!a.library);unchanged(before);
    // This independent navigation run must not shift the existing acquisition
    // fixtures' deterministic practice seeds.
    CHECK(a.run_number==run_number+1);a.run_number=run_number;
    catalog_navigation_verified=true;progress("catalog-navigation-complete");
}
static void open_model(unsigned model,unsigned upgraded){
    CHECK(catalog_navigation_verified);progress("open-model",(unsigned)a.game.character,model,upgraded);
    if(!a.library)a.input(Key::Collection);action(UiAction::LibraryCards);
    action(UiAction::FilterAll);CardKind ids[CardKinds];unsigned count=a.library_items(ids),index=0;
    CHECK(count==CardKinds);while(index<count&&ids[index]!=model)++index;CHECK(index<count);
    // Every real page-button transition was checked before any model fixture.
    // Position this isolated fixture without replaying all preceding pages;
    // the model itself is still opened, upgraded and acquired by real clicks.
    a.library_page=(int)index/8;a.library_selected=a.library_page*8;
    tap(HitKind::LibraryItem,(int)index);CHECK(card_from_token(a.detail_card).kind==model);
    if(card_from_token(a.detail_card).upgraded!=(int)upgraded)action(UiAction::ToggleCardUpgrade);
    CHECK(a.detail_card==card_token({(CardKind)model,(uint8_t)upgraded}));
    progress("model-detail",(unsigned)a.game.character,model,upgraded);
}
static void description(Card card,int instance=-1){
    char text[1024]={};auto before=snapshot();a.card_description(card,0,text,sizeof text,instance);
    CHECK(*text&&!std::strchr(text,'{')&&!std::strchr(text,'}')&&!std::strchr(text,'\x1e')&&!std::strchr(text,'\x1f'));
    unchanged(before);
}
static void capture(const char* name){
    if(!screenshot_directory)return;
    a.render();std::string path=std::string(screenshot_directory)+"/"+name+".ppm";
    FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n%d %d\n255\n",ScreenW,ScreenH)>0);
    uint8_t row[ScreenW*3];
    for(int y=0;y<ScreenH;y++){
        for(int x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=(uint8_t)(((p>>11)&31)*255/31);row[x*3+1]=(uint8_t)(((p>>5)&63)*255/63);row[x*3+2]=(uint8_t)((p&31)*255/31);}
        CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);
    }
    CHECK(std::fclose(f)==0);
}
static void all_models(){
    CHECK(catalog_navigation_verified);progress("all-models-start");
    for(unsigned character=0;character<CharacterKinds;character++){
        unsigned count=0;
        for(unsigned model=0;model<CardKinds;model++)if(card_in_pool(model,(Character)character)){
            ++count;
            for(unsigned upgraded=0;upgraded<2;upgraded++){
                progress("variant-start",character,model,upgraded);
                start((Character)character);auto before=snapshot();open_model(model,upgraded);
                Card card{(CardKind)model,(uint8_t)upgraded};description(card);unchanged(before);
                if(real_profile){CHECK(a.content.image(a.content.card_image(model)).pixels);CHECK(a.content.image(a.content.frame_image(card)).pixels);}
                if(cost(card)==-2)CHECK(a.content.energy_image(card)==-1);
                const unsigned old_count=a.game.card_count;fixture_card();CHECK(!a.library&&a.detail_card<0);
                CHECK(a.game.card_count==old_count+1&&a.game.cards[old_count].kind==model&&a.game.cards[old_count].upgraded==upgraded);
                CHECK(a.game.zones[old_count]==Zone::Hand);description(card,(int)old_count);
                auto saved=snapshot();State loaded{};CHECK(load(loaded,saved.bytes,saved.size));
                CHECK(loaded.card_count==a.game.card_count&&loaded.cards[old_count].kind==model&&loaded.cards[old_count].upgraded==upgraded);
                ++acquisitions;progress("variant-complete",character,model,upgraded);
            }
        }
        CHECK(count==(character==0?85u:86u));
    }
    for(unsigned upgraded=0;upgraded<2;upgraded++){
        progress("variant-start",(unsigned)Character::Ironclad,GiantRock,upgraded);
        start(Character::Ironclad);auto before=snapshot();open_model(GiantRock,upgraded);description({GiantRock,(uint8_t)upgraded});unchanged(before);
        unsigned id=a.game.card_count;fixture_card();CHECK(a.game.cards[id].kind==GiantRock&&a.game.cards[id].upgraded==upgraded);
        CHECK(!a.library&&a.detail_card<0&&a.game.card_count==id+1&&a.game.zones[id]==Zone::Hand);
        description({GiantRock,(uint8_t)upgraded},(int)id);
        auto saved=snapshot();State loaded{};CHECK(load(loaded,saved.bytes,saved.size));
        CHECK(loaded.card_count==a.game.card_count&&loaded.cards[id].kind==GiantRock&&loaded.cards[id].upgraded==upgraded);
        ++acquisitions;progress("variant-complete",(unsigned)Character::Ironclad,GiantRock,upgraded);
    }
    CHECK(acquisitions==2*(IroncladCards+SilentCards+DefectCards+RegentCards+NecroCards)+2);
    CHECK(acquisitions==860);progress("all-models-complete");
}
static void overflow_and_catalog(){
    for(Character character:{Character::Ironclad,Character::Silent}){
        start(character);auto&s=a.game;
        while(s.hand.count<MaxHand){CHECK(s.draw.count);unsigned id=s.draw.ids[--s.draw.count];s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;}
        unsigned id=s.card_count;open_model(GiantRock,1);fixture_card();
        CHECK(s.card_count==id+1&&s.zones[id]==Zone::Discard&&s.cards[id].kind==GiantRock&&s.cards[id].upgraded);
        CHECK(validate(s));
        a.input(Key::Collection);action(UiAction::LibraryCards);action(UiAction::FilterAll);
        while(a.library_page<(CardKinds-1)/8){
            const int page=a.library_page;progress("overflow-catalog-next",(unsigned)character);
            action(UiAction::NextPage);CHECK(a.library_page==page+1);
        }
        CHECK(a.library_page==(CardKinds-1)/8);CardKind ids[CardKinds];CHECK(a.library_items(ids)==CardKinds);
        CHECK(ids[CardKinds-1]==EventCardWrithe);auto before=snapshot();
        auto next=hit(HitKind::UiAction,(int)UiAction::NextPage);CHECK(!next.enabled);
        a.pointer_move(next.x+next.w/2,next.y+next.h/2);a.pointer_button(true);a.pointer_button(false);
        CHECK(a.library_page==(CardKinds-1)/8);unchanged(before);
        tap(HitKind::LibraryItem,CardKinds-1);CHECK(card_from_token(a.detail_card).kind==EventCardWrithe);
    }
}
static void named_value_and_screens(){
    start(Character::Ironclad);open_model(GiantRock,0);char catalog[1024]={};a.card_description({GiantRock,0},0,catalog,sizeof catalog);
    CHECK(a.content.frame_image({GiantRock,0})==a.content.ui_image(U_ColorlessAttack));
    CHECK(a.content.energy_image({GiantRock,0})==a.content.ui_image(U_ColorlessEnergy));
    CHECK(std::strstr(catalog,"20"));capture("giant-rock-catalog");unsigned id=a.game.card_count;fixture_card();
    a.game.strength=3;a.game.enemies[0].vulnerable=2;a.open_choice_detail((int)id);char live[1024]={};
    a.card_description({GiantRock,0},0,live,sizeof live,(int)id);CHECK(std::strstr(live,"34"));CHECK(!std::strchr(live,'{'));
    capture("giant-rock-live-damage");
    start(Character::Ironclad);open_model(ExpectAFight,1);capture("ironclad-expect-a-fight");
    start(Character::Silent);open_model(Sidestep,1);capture("silent-sidestep-beta");
    start(Character::Silent);open_model(Skewer,0);CHECK(cost({Skewer,0})==-1);capture("silent-skewer-x");
    // Reflex is a playable three-energy Sly card in the pinned STS2 release.
    start(Character::Silent);open_model(Reflex,0);CHECK(cost({Reflex,0})==3&&a.content.energy_image({Reflex,0})>=0);capture("silent-reflex-sly");
}
static void playability_and_instance_cost(){
    start(Character::Silent);open_model(GrandFinale,0);unsigned id=a.game.card_count;fixture_card();
    CHECK(a.game.draw.count>0&&cost(a.game,(int)id)==0);CHECK(!can_play_card(a.game,(int)id));
    CHECK(!hit(HitKind::Hand,(int)id).enabled);
    // Disabled play targets remain hoverable and can still open their detail.
    auto region=hit(HitKind::Hand,(int)id);a.pointer_move(region.x+region.w/2,region.y+region.h/2);a.input(Key::Detail);
    CHECK(a.detail_instance==(int)id);a.input(Key::Back);
    auto&s=a.game;
    while(s.draw.count){unsigned card=s.draw.ids[--s.draw.count];s.discard.ids[s.discard.count++]=(uint16_t)card;s.zones[card]=Zone::Discard;}
    CHECK(validate(s)&&can_play_card(s,(int)id));CHECK(hit(HitKind::Hand,(int)id).enabled);
    start(Character::Ironclad);open_model(Stomp,0);id=a.game.card_count;fixture_card();
    a.open_choice_detail((int)id);CHECK(a.detail_cost_override==3);a.input(Key::Back);
    int strike=-1;for(int i=0;i<a.game.hand.count;i++)if(a.game.cards[a.game.hand.ids[i]].kind==Strike)strike=i;
    CHECK(strike>=0&&play(a.game,strike,0));a.reset_presentation();CHECK(cost(a.game,(int)id)==2);
    a.open_choice_detail((int)id);CHECK(a.detail_cost_override==2&&a.detail_energy()==2);capture("stomp-instance-discount");
    start(Character::Ironclad);open_model(Barricade,0);id=a.game.card_count;fixture_card();
    a.open_choice_detail((int)id);CHECK(a.detail_energy()==3);auto before=snapshot();
    action(UiAction::ToggleCardUpgrade);CHECK(a.detail_energy()==2);unchanged(before);
    action(UiAction::ToggleCardUpgrade);CHECK(a.detail_energy()==3);unchanged(before);
    // A genuine explicit zero Set keeps both previews free.
    a.game.expansion.turn_cost[id]=1;a.open_choice_detail((int)id);CHECK(a.detail_energy()==0);
    action(UiAction::ToggleCardUpgrade);CHECK(a.detail_energy()==0);
}
static int hand_index(unsigned id){
    for(int i=0;i<a.game.hand.count;i++)if(a.game.hand.ids[i]==id)return i;
    CHECK(false);return -1;
}
static void inky_and_dynamic_targets(){
    start(Character::Silent);open_model(BladeOfInk,0);unsigned id=a.game.card_count;fixture_card();
    CHECK(play(a.game,hand_index(id),0));a.reset_presentation();CHECK(a.game.phase==Phase::Combat&&validate(a.game));
    unsigned ink[2]={},count=0;
    for(unsigned i=0;i<a.game.hand.count;i++){
        unsigned instance=a.game.hand.ids[i];
        if(a.game.silent.inky[instance]){CHECK(count<2);ink[count++]=instance;}
    }
    CHECK(count==2&&card_targets_enemy(a.game,a.game.cards[ink[0]]));
    a.open_choice_detail((int)ink[0]);char text[1024]={};auto before=snapshot();
    a.card_description(a.game.cards[ink[0]],0,text,sizeof text,(int)ink[0]);
    CHECK(std::strstr(text,"墨影")&&std::strstr(text,"虚弱")&&!std::strstr(text,"所有敌人"));unchanged(before);
    if(real_profile)CHECK(a.content.image(a.content.ui_image(U_Inky)).pixels);
    capture("silent-inky-target");a.input(Key::Back);
    // Opening the same model in the compendium must not inherit its enchantment.
    open_model(Shiv,0);a.card_description({Shiv,0},0,text,sizeof text,(int)ink[0]);
    CHECK(!std::strstr(text,"墨影"));a.input(Key::Back);a.input(Key::Back);CHECK(!a.library);
    open_model(FanOfKnives,0);id=a.game.card_count;fixture_card();
    CHECK(play(a.game,hand_index(id),0));a.reset_presentation();CHECK(a.game.phase==Phase::Combat&&validate(a.game));
    CHECK(!card_targets_enemy(a.game,a.game.cards[ink[0]]));
    a.open_choice_detail((int)ink[0]);a.card_description(a.game.cards[ink[0]],-1,text,sizeof text,(int)ink[0]);
    CHECK(std::strstr(text,"墨影")&&std::strstr(text,"所有敌人"));capture("silent-inky-all-enemies");a.input(Key::Back);
    int hp[MaxEnemies]={},weak[MaxEnemies]={};
    for(int t=0;t<a.game.enemy_count;t++){hp[t]=a.game.enemies[t].hp;weak[t]=a.game.enemies[t].weak;}
    // A second click on this non-targeted hand card plays it immediately.
    tap(HitKind::Hand,(int)ink[0]);CHECK(a.pointer.armed_card==(int)ink[0]);tap(HitKind::Hand,(int)ink[0]);
    CHECK(a.game.zones[ink[0]]==Zone::Exhaust);
    for(int t=0;t<a.game.enemy_count;t++)CHECK(a.game.enemies[t].hp<hp[t]&&a.game.enemies[t].weak==weak[t]+1);
    a.reset_presentation();
    // Dragging another enchanted Shiv onto empty battlefield needs no enemy hit.
    auto region=hit(HitKind::Hand,(int)ink[1]);CHECK(region.enabled);
    a.pointer_move(region.x+region.w/2,region.y+region.h/2);a.pointer_button(true);a.pointer_move(104,58);
    CHECK(a.hit_at(104,58)&&a.hit_at(104,58)->kind==HitKind::Battlefield);a.pointer_button(false);
    CHECK(a.game.zones[ink[1]]==Zone::Exhaust);
    for(int t=0;t<a.game.enemy_count;t++)CHECK(a.game.enemies[t].weak==weak[t]+2);
    CHECK(validate(a.game));
}
int main(int argc,char**argv){
    std::vector<uint8_t> bytes;
    if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));size_t n=std::fread(bytes.data(),1,bytes.size(),f);CHECK(!std::ferror(f));std::fclose(f);bytes.resize(n);real_profile=true;}
    else bytes=synthetic_profile();
    if(argc>2)screenshot_directory=argv[2];
    CHECK(a.init(bytes.data(),bytes.size(),{save_cb,load_cb}));CHECK(a.content.current_profile());
    catalog_navigation();all_models();
    progress("overflow-and-catalog");overflow_and_catalog();
    progress("named-value-and-screens");named_value_and_screens();
    progress("playability-and-instance-cost");playability_and_instance_cost();
    progress("inky-and-dynamic-targets");inky_and_dynamic_targets();
    std::printf("character UI: %u checks passed, %u acquired variants, %s content\n",checks,acquisitions,real_profile?"actual":"synthetic");
    std::fflush(stdout);
}
