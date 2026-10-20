#include "app.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s phase=%d\n",__FILE__,__LINE__,#x,int(a.game.phase));std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,unsigned at,unsigned n,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=(uint8_t)(n>>(8*i));}
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count,strings=ContentStrings;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);
    put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<strings;i++){
        unsigned model=i/3;const char*text=i<CardKinds*3?(i%3?"Synthetic card description.":definitions[model].key):"Synthetic UI text.";
        if(model==DefectGeneticAlgorithm&&i%3)text="Block {Block=1}.";
        if(model==NecroTheScythe&&i%3)text="Damage {Damage=13}.";
        if(model==NecroNoEscape&&i%3)text="NoEscape.\x1c\nTarget {CalculatedDoom=10}.\x1d";
        if(model==NecroTimesUp&&i%3)text="TimesUp.\x1c\nTarget {CalculatedDamage=0}.\x1d";
        put(b,stringdir+i*8,(unsigned)b.size());put(b,stringdir+i*8+4,(unsigned)std::strlen(text));b.insert(b.end(),text,text+std::strlen(text)+1);
    }
    for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;i++)for(unsigned j=0;j<4;j++)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,(unsigned)b.size());uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}

static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static bool has(HitKind kind,int value){for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value)return true;return false;}
static void world(){a.title=true;a.character_select=true;a.practice_entry=false;a.run_settings=false;a.selected_character=Character::Ironclad;a.selected_ascension=0;a.selected_region=reference::ActKind::Overgrowth;a.selected_seed[0]=0;a.ui_action(UiAction::StartCharacter);CHECK(!a.title&&a.game.original_events.id==OriginalEventId::Neow);CHECK(validate(a.game));}
static void options_and_readers(){
 a.character_select=true;a.practice_entry=false;a.title=true;a.ui_action(UiAction::RunSettings);CHECK(a.run_settings);a.ui_action(UiAction::RunRegion);a.ui_action(UiAction::RunAscensionUp);a.ui_action(UiAction::RunAscensionUp);
 a.ui_action(UiAction::RunSeed);CHECK(a.map_preview.editing&&a.run_seed_editing);a.reference_text(-1);for(const char*p="WORLD-UI-17";*p;p++)a.reference_text(*p);a.reference_text(13);CHECK(!a.map_preview.open&&!a.run_seed_editing&&a.selected_seed[0]);
 a.ui_action(UiAction::StartCharacter);CHECK(a.game.world.active&&a.game.world.ascension==2);CHECK(a.game.original_events.id==OriginalEventId::Neow);snapshot();
 a.render();CHECK(a.hit_count>0);a.ui_action(UiAction::ReadEvent);a.render();CHECK(a.event_reader);a.pointer_scroll(1);a.render();a.ui_action(UiAction::CloseEventReader);unchanged();
 a.library=true;a.library_tab=1;a.library_selected=WorldRelicKinds-1;a.library_page=a.library_selected/18;a.render();CHECK(has(HitKind::LibraryItem,WorldRelicKinds-1));a.select_library_item(WorldRelicKinds-1);a.render();CHECK(a.detail_item==WorldRelicKinds-1);a.input(Key::Back);a.library=false;unchanged();
}
static void crystal_and_large_rewards(){
 world();CHECK(original_event_begin(a.game,OriginalEventId::CrystalSphere,31,1));a.game.gold=250;CHECK(original_event_choose(a.game,1));CHECK(a.game.phase==Phase::OriginalEventChoice);a.reset_presentation();snapshot();a.render();
 unsigned cells=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::CrystalCell)cells++;CHECK(cells==CrystalCells&&a.hit_count<=MaxHits);a.input(Key::Right);a.input(Key::Down);a.render();unchanged();
 auto&e=a.game.original_events;e.finished=1;e.selection=OriginalEventSelection::Rewards;e.select_min=0;e.select_max=1;e.select_cancelable=1;e.reward_active=1;e.reward_count=MaxOriginalEventRewards;e.reward_current=NoOriginalEventReward;e.reward_card_count=0;
 for(unsigned i=0;i<e.reward_count;i++){e.rewards[i]={};e.rewards[i].kind=1;e.rewards[i].amount=1;}a.game.phase=Phase::OriginalEventChoice;a.choice_revision=0;a.cursor=MaxOriginalEventRewards-1;snapshot();a.render();CHECK(has(HitKind::WorldChoice,MaxOriginalEventRewards-1));a.world_choice_detail(MaxOriginalEventRewards-1);unchanged();
 int gold=a.game.gold;a.toggle_world_choice(MaxOriginalEventRewards-1);CHECK(a.game.original_events.rewards[MaxOriginalEventRewards-1].claimed&&a.game.gold==gold+1);CHECK(save(a.game,after));CHECK(after.size<=sizeof after.bytes);
}
static void potion_lock_and_capacity(){
 world();a.game.potion_slots=16;for(unsigned i=0;i<16;i++)CHECK(potion_set(a.game,i,Potion::Strength));CHECK(original_event_begin(a.game,OriginalEventId::StoneOfAllTime,7,1));CHECK(original_event_blocks_potions(a.game));a.panel=4;a.scroll=15;snapshot();a.render();CHECK(has(HitKind::Potion,15));a.ui_action(UiAction::DiscardPotion);CHECK(potion_at(a.game,15)==Potion::Strength);unchanged();a.panel=0;
}
static void offer_facts(){
 world();EventCardFacts facts{};facts.science_type=1;facts.rider=unsigned(TinkerRider::Wisdom);facts.energy_offset=-1;
 a.open_card_offer({EventCardMadScience,0},facts);snapshot();a.render();CHECK(a.card_face_type({EventCardMadScience,0},-1,-1,&facts)==CardType::Skill);CHECK(a.card_offer_cost({Strike,0},&facts)==0);unchanged();a.input(Key::Back);
 CHECK(gain_world_relic(a.game,WorldRelicPaelsWing));CHECK(original_event_begin(a.game,OriginalEventId::ColorfulPhilosophers,5,0));CHECK(original_event_choose(a.game,0));CHECK(original_event_reward(a.game,0));CHECK(world_card_reward_alternatives(a.game)&1);a.reset_presentation();a.render();CHECK(has(HitKind::UiAction,int(UiAction::SacrificeReward)));snapshot();a.world_choice_detail(0);a.render();unchanged();a.input(Key::Back);
}
int main(int argc,char**argv){std::vector<uint8_t>b;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));b.resize(size);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));}else b=synthetic_profile();
 CHECK(a.init(b.data(),b.size(),{}));options_and_readers();crystal_and_large_rewards();potion_lock_and_capacity();offer_facts();std::printf("world UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");}
