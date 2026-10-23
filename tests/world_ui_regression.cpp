#include "app.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "ui_core_observer.hpp"
#include "feedback_assets.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static const char* screenshot_directory;
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
static void capture(const char*name){
 if(!screenshot_directory)return;a.pointer.visible=false;a.render();
 const std::string path=std::string(screenshot_directory)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
 std::fprintf(f,"P6\n320 240\n255\n");unsigned char row[ScreenW*3];
 for(int y=0;y<ScreenH;y++){for(int x=0;x<ScreenW;x++){unsigned p=a.pixels[y*ScreenW+x];row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void world(){a.title=true;a.character_select=true;a.practice_entry=false;a.run_settings=false;a.selected_character=Character::Ironclad;a.selected_ascension=0;a.selected_region=reference::ActKind::Overgrowth;a.selected_seed[0]=0;a.ui_action(UiAction::StartCharacter);CHECK(!a.title&&a.game.original_events.id==OriginalEventId::Neow);CHECK(validate(a.game));
 // Later checks supply their own event outcomes, after verifying ordinary Neow entry.
 a.game.original_events=OriginalEventState{};CHECK(world_finish_room(a.game));}
static void options_and_readers(){
 a.character_select=true;a.practice_entry=false;a.title=true;a.ui_action(UiAction::RunSettings);CHECK(a.run_settings);a.ui_action(UiAction::RunRegion);a.ui_action(UiAction::RunAscensionUp);a.ui_action(UiAction::RunAscensionUp);
 a.ui_action(UiAction::RunSeed);CHECK(a.map_preview.editing&&a.run_seed_editing);a.reference_text(-1);for(const char*p="WORLD-UI-17";*p;p++)a.reference_text(*p);a.reference_text(13);CHECK(!a.map_preview.open&&!a.run_seed_editing&&a.selected_seed[0]);
 a.ui_action(UiAction::StartCharacter);CHECK(a.game.world.active&&a.game.world.ascension==2);CHECK(a.game.original_events.id==OriginalEventId::Neow);snapshot();
 a.render();CHECK(a.hit_count>0);capture("ancient-neow");a.ui_action(UiAction::ReadEvent);a.render();CHECK(a.event_reader);capture("ancient-reader");a.pointer_scroll(1);a.render();a.ui_action(UiAction::CloseEventReader);unchanged();
 a.library=true;a.library_tab=1;a.library_selected=WorldRelicKinds-1;a.library_page=a.library_selected/18;a.render();CHECK(has(HitKind::LibraryItem,WorldRelicKinds-1));a.select_library_item(WorldRelicKinds-1);a.render();CHECK(a.detail_item==WorldRelicKinds-1);a.input(Key::Back);a.library=false;unchanged();
}
static void crystal_and_large_rewards(){
 world();CHECK(original_event_begin(a.game,OriginalEventId::CrystalSphere,31,1));a.game.gold=250;CHECK(original_event_choose(a.game,1));CHECK(a.game.phase==Phase::OriginalEventChoice);a.reset_presentation();snapshot();a.render();
 unsigned cells=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::CrystalCell)cells++;CHECK(cells==CrystalCells&&a.hit_count<=MaxHits);capture("crystal-board");a.input(Key::Right);a.input(Key::Down);a.render();unchanged();
 auto&e=a.game.original_events;e.finished=1;e.selection=OriginalEventSelection::Rewards;e.select_min=0;e.select_max=1;e.select_cancelable=1;e.reward_active=1;e.reward_count=MaxOriginalEventRewards;e.reward_current=NoOriginalEventReward;e.reward_card_count=0;
 for(unsigned i=0;i<e.reward_count;i++){e.rewards[i]={};e.rewards[i].kind=1;e.rewards[i].amount=1;}a.game.phase=Phase::OriginalEventChoice;a.choice_revision=0;a.cursor=MaxOriginalEventRewards-1;snapshot();a.render();CHECK(has(HitKind::WorldChoice,MaxOriginalEventRewards-1));a.world_choice_detail(MaxOriginalEventRewards-1);unchanged();
 int gold=a.game.gold;a.toggle_world_choice(MaxOriginalEventRewards-1);CHECK(a.game.original_events.rewards[MaxOriginalEventRewards-1].claimed&&a.game.gold==gold+1);CHECK(save(a.game,after));CHECK(after.size<=sizeof after.bytes);
}
static void special_reward_snapshots(){
 const CardKind models[]={DefectGeneticAlgorithm,NecroTheScythe,NecroTheScythe,NecroTheScythe};
 const EventEnchantment enchants[]={EventEnchantment::Nimble,EventEnchantment::Sharp,EventEnchantment::Corrupted,EventEnchantment::TezcatarasEmber};
 const char*values[]={"17","49","60","43"};
 for(unsigned i=0;i<4;i++){
  new_run(a.game,0x1717,false);init_world_relics(a.game);a.game.phase=Phase::WorldRelicChoice;
  auto&w=a.game.world_relics;w.group_count=w.reward_count=1;w.group_active[0]=w.group_skip[0]=1;
  w.choice=WorldRelicChoice::OfferReward;w.choice_group=1;w.return_phase=uint8_t(Phase::Map);w.choice_max=1;w.choice_cancelable=1;w.offer_count=1;w.offers[0]=0;
  auto&r=w.rewards[0];r.group=1;r.kind=5;r.value=card_token({models[i],0});r.tokens[0]=i?0:12;r.tokens[1]=i?27:0;
  r.facts[0].enchantment=enchants[i];r.facts[0].enchantment_amount=i==0?4:i==1?9:1;r.facts[0].energy_offset=i==3?-1:0;
  a.title=a.library=false;a.reset_presentation();snapshot();a.world_choice_detail(0);a.render();
  CHECK(a.detail_has_facts&&a.detail_instance==-1&&a.detail_deck_index==-1);CHECK(a.detail_genetic==r.tokens[0]&&a.detail_scythe==r.tokens[1]);
  char text[2048]{};a.card_description({models[i],0},-1,text,sizeof text);CHECK(std::strstr(text,values[i]));CHECK(!std::strchr(text,'{'));
  CHECK(a.detail_energy()==a.card_offer_cost({models[i],0},&r.facts[0]));unchanged();
  if(i==2)capture("reward-grown-scythe");
  a.input(Key::Back);a.open_card_detail({models[i],0});CHECK(!a.detail_has_facts&&!a.detail_genetic&&!a.detail_scythe);a.input(Key::Back);unchanged();
 }
}
static void potion_lock_and_capacity(){
 world();a.game.potion_slots=16;for(unsigned i=0;i<16;i++)CHECK(potion_set(a.game,i,Potion::Strength));CHECK(original_event_begin(a.game,OriginalEventId::StoneOfAllTime,7,1));CHECK(original_event_blocks_potions(a.game));a.input(Key::Potions);CHECK(a.panel==4);a.pointer_scroll(1);for(int i=0;i<7;i++)a.input(Key::Right);CHECK(a.scroll==15);snapshot();a.render();CHECK(has(HitKind::Potion,15));CHECK(a.scroll==15);capture("potion-full-second-page");a.ui_action(UiAction::DiscardPotion);CHECK(potion_at(a.game,15)==Potion::Strength);unchanged();a.panel=0;
}
static void offer_facts(){
 world();EventCardFacts facts{};facts.science_type=1;facts.rider=unsigned(TinkerRider::Wisdom);facts.energy_offset=-1;
 a.open_card_offer({EventCardMadScience,0},facts);snapshot();a.render();CHECK(a.card_face_type({EventCardMadScience,0},-1,-1,&facts)==CardType::Skill);CHECK(a.card_offer_cost({Strike,0},&facts)==0);unchanged();a.input(Key::Back);
 a.open_card_offer({Strike,0},facts);CHECK(a.detail_energy()==0);a.input(Key::Back);
 CHECK(gain_world_relic(a.game,WorldRelicPaelsWing));CHECK(original_event_begin(a.game,OriginalEventId::ColorfulPhilosophers,5,0));CHECK(original_event_choose(a.game,0));CHECK(original_event_reward(a.game,0));CHECK(world_card_reward_alternatives(a.game)&1);a.reset_presentation();a.render();CHECK(has(HitKind::UiAction,int(UiAction::SacrificeReward)));snapshot();a.world_choice_detail(0);a.render();unchanged();a.input(Key::Back);
}
static void enemy_visual_fight(EncounterId encounter,Character character=Character::Ironclad){
 new_run(a.game,0x41cef1ea,false,character);a.game.phase=Phase::Combat;a.game.current_node=1;a.game.floor=a.game.turn=1;a.game.hp=a.game.max_hp=999;
 CHECK(encounter_setup(a.game,encounter));
 for(unsigned i=0;i<a.game.deck_count;i++){int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,a.game.deck[i].kind,1);resolve_pending(a.game);CHECK(a.game.card_count==id+1);a.game.cards[id].upgraded=a.game.deck[i].upgraded;notify_deck_card_created(a.game,id,i);}
 draw_cards(a.game,5);a.title=a.library=a.paused=a.help=a.character_select=false;a.reset_presentation();snapshot();a.render();
 for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}
 CHECK(!a.combat_preparation.active);unchanged();
}
static void original_enemy_presentation(){
 CHECK(world_enemy_fact_count()<=MaxEnemyVisualFacts);enemy_visual_fight(EncounterId::NibbitsWeak);
 int clip=-1;uint32_t elapsed=0;bool loop=false;CHECK(a.enemy_frame(0,clip,elapsed,loop));CHECK(clip>=38&&clip<AnimationKinds&&loop);
 CHECK(has(HitKind::EnemyIntent,0));capture("combat-nibbit-idle");const int idle=clip;const unsigned turn=a.game.turn;
 a.input(Key::EndTurn);CHECK(a.game.turn==turn+1);CHECK(a.enemy_visual_clock>a.animation);CHECK(a.enemy_frame(0,clip,elapsed,loop));CHECK(clip==idle&&loop);
 snapshot();const unsigned first_action=a.animation+TurnBannerPresentation::EnemyLeadTicks+TurnBannerPresentation::IntentLeadTicks;
 // A long prior non-loop track cannot postpone an already scheduled trigger
 // until after the enemy banner disappears.
 a.enemy_presentation[0].loop=false;a.enemy_presentation[0].until=first_action+200;
 a.animation=first_action;a.render();CHECK(a.enemy_frame(0,clip,elapsed,loop));CHECK(clip!=idle&&!loop);
 CHECK(a.turn_banner.sample_enemy(a.animation).kind==TurnBannerPresentation::Enemy&&a.turn_banner.sample_enemy(a.animation).alpha>0);
 capture("combat-nibbit-action");unchanged();
 for(unsigned i=0;i<32;i++){a.advance(250);a.render();}unchanged();CHECK(a.enemy_frame(0,clip,elapsed,loop)&&loop);CHECK(!a.enemy_presentation[0].overflow);
 {UiCoreObserver observer(a);damage_unpowered(a.game,0,9999);resolve_pending(a.game);}
 CHECK(a.enemy_presentation[0].dead);CHECK(a.enemy_frame(0,clip,elapsed,loop)&&!loop);CHECK(a.combat_linger_until>a.animation);snapshot();a.render();capture("combat-nibbit-death");unchanged();
 CHECK(a.enemy_presentation[0].removing&&a.enemy_presentation[0].hide_at>a.animation);
 a.animation=a.enemy_presentation[0].fade_start+1;CHECK(a.enemy_opacity(0)<255&&a.enemy_opacity(0)>0);
 a.animation=a.enemy_presentation[0].hide_at;a.render();CHECK(!a.enemy_opacity(0)&&!a.enemy_frame(0,clip,elapsed,loop));unchanged();
 // A restored removed actor does not replay a corpse's disappearance.
 a.reset_presentation();a.sync_enemy_feedback();CHECK(!a.enemy_opacity(0)&&!a.enemy_frame(0,clip,elapsed,loop));unchanged();
 // Actual shared-scene clips replace both arm placeholders. Rendering and
 // idle advancement cannot draw new game RNG or change a saved boss phase.
 enemy_visual_fight(EncounterId::KaiserCrabBoss);CHECK(a.game.enemy_count==2);
 for(int i=0;i<2;i++){CHECK(world_enemy_visual(unsigned(a.game.enemy_world.models[i].kind))->shared_group==1);CHECK(a.enemy_frame(i,clip,elapsed,loop)&&clip>=38);}
 snapshot();for(unsigned i=0;i<3;i++){a.advance(250);a.render();}capture("combat-kaiser-shared");unchanged();
}
static const WorldEnemyVisualClip* visual_clip(int slot){
 int clip=-1;uint32_t elapsed=0;bool loop=false;CHECK(a.enemy_frame(slot,clip,elapsed,loop));const auto*row=world_enemy_clip_directory(unsigned(clip));CHECK(row);return row;
}
static void first_enemy_cue(){
 const uint32_t cue=a.turn_banner.enemy_start+TurnBannerPresentation::EnemyLeadTicks+TurnBannerPresentation::IntentLeadTicks;
 if(int32_t(cue-a.animation)>0)a.animation=cue;
 a.sync_enemy_feedback();
}
static void settle_enemy_visuals(){
 snapshot();for(unsigned frame=0;frame<80;frame++){a.advance(250);for(int slot=0;slot<a.game.enemy_count;slot++){
  if(a.enemy_opacity(slot))visual_clip(slot);
  else CHECK(a.game.enemies[slot].hp<=0&&(a.game.enemy_world.models[slot].flags&EnemyRemoved));
 }}a.render();unchanged();
}
static void observed_enemy_kill(int slot){
 UiCoreObserver observer(a);a.game.enemies[slot].block=0;damage_unpowered(a.game,slot,9999);resolve_pending(a.game);CHECK(a.game.phase!=Phase::Error);
}
static void original_enemy_transformations(){
 // Louse's Attack is allowed only from Idle. Its real two-action move
 // finishes Uncurl at the next cue; completion must precede that trigger.
 enemy_visual_fight(EncounterId::LouseProgenitorNormal);
 auto&louse=a.enemy_presentation[0];louse.state=3;louse.loop=true;louse.clip=world_enemy_resolve_clip(louse.model,"curled_loop",louse.skin);
 a.enemy_trigger(0,"Uncurl",1);CHECK(louse.state==2&&!louse.loop);const uint32_t attack_cue=louse.until;
 a.enemy_trigger(0,"Attack",1,false,nullptr,attack_cue);a.animation=attack_cue;
 CHECK(!std::strcmp(visual_clip(0)->native_clip,"attack")&&louse.state==4&&!louse.count&&louse.start==attack_cue);

 enemy_visual_fight(EncounterId::TestSubjectBoss);
 for(int form=0;form<2;form++){
  observed_enemy_kill(0);CHECK(a.game.phase==Phase::Combat&&(a.game.enemy_world.models[0].flags&EnemyDown));
  CHECK(!std::strcmp(visual_clip(0)->native_clip,form?"knockout2":"knockout1"));CHECK(a.enemy_presentation[0].dead);
  settle_enemy_visuals();CHECK(a.enemy_opacity(0)==255&&!a.enemy_presentation[0].removing);CHECK(!std::strcmp(visual_clip(0)->native_clip,form?"knocked_out_loop2":"knocked_out_loop1"));
  if(!form)capture("combat-test-subject-down");
  a.input(Key::EndTurn);CHECK(a.game.enemy_world.models[0].data[0]==form+1&&a.game.enemies[0].hp>0);
  first_enemy_cue();
  CHECK(!std::strcmp(visual_clip(0)->native_clip,form?"regenerate2":"regenerate1"));CHECK(!a.enemy_presentation[0].dead);
  if(!form)capture("combat-test-subject-regenerate");
  settle_enemy_visuals();CHECK(!std::strcmp(visual_clip(0)->native_clip,form?"idle_loop3":"idle_loop2"));
 }
 // The third form gains Nemesis; this real end-turn route has already
 // applied its first Intangible. A damage call must not bypass that power.
 CHECK(enemy_power_value(a.game,0,EnemyPower::Intangible)==1);
 const int third_hp=a.game.enemies[0].hp;observed_enemy_kill(0);CHECK(a.game.enemies[0].hp==third_hp-1);
 settle_enemy_visuals();a.input(Key::EndTurn);settle_enemy_visuals();CHECK(!enemy_power_value(a.game,0,EnemyPower::Intangible));
 observed_enemy_kill(0);CHECK(!std::strcmp(visual_clip(0)->native_clip,"die"));settle_enemy_visuals();CHECK(a.enemy_presentation[0].dead&&!a.enemy_presentation[0].loop&&!a.enemy_opacity(0));

 enemy_visual_fight(EncounterId::DecimillipedeElite);observed_enemy_kill(0);settle_enemy_visuals();CHECK(!std::strcmp(visual_clip(0)->native_clip,"dead_loop"));
 // One real segment action shakes every segment, including a detached one.
 // The dead segment must return to dead_loop and the other two to idle_loop.
 a.game.enemy_world.models[1].node=0;
 {UiCoreObserver observer(a);record(a.game,LogKind::Enemy,1,0);queue_extension(a.game,XOp::EnemyMove,1,0);queue_extension(a.game,XOp::EnemyMoveFinish,1,0);resolve_pending(a.game);}
 first_enemy_cue();
 CHECK(a.game.phase==Phase::Combat);for(int i=0;i<3;i++){CHECK(visual_clip(i)->move==0);CHECK(a.enemy_presentation[i].direct&&!a.enemy_presentation[i].loop);}
 capture("combat-decimillipede-shared-strike");settle_enemy_visuals();
 for(int i=0;i<3;i++){CHECK(!std::strcmp(visual_clip(i)->native_clip,i?"idle_loop":"dead_loop"));CHECK(a.enemy_presentation[i].loop&&!a.enemy_presentation[i].direct&&!a.enemy_presentation[i].count&&!a.enemy_presentation[i].overflow);}

 enemy_visual_fight(EncounterId::CorpseSlugsWeak);observed_enemy_kill(0);
 CHECK(!std::strcmp(visual_clip(1)->native_clip,"devour_start"));settle_enemy_visuals();CHECK(!std::strcmp(visual_clip(1)->native_clip,"devour_loop"));
 a.input(Key::EndTurn);first_enemy_cue();CHECK(!std::strcmp(visual_clip(1)->native_clip,"devour_end"));settle_enemy_visuals();CHECK(!std::strcmp(visual_clip(1)->native_clip,"idle_loop"));
}
static void feedback_hud_and_merchant(){
 const char*names[]={"ironclad","silent","defect","regent","necrobinder"};
 for(unsigned character=0;character<5;character++){
  enemy_visual_fight(EncounterId::NibbitsWeak,Character(character));CHECK(max_energy(a.game)==3);
  init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicEctoplasm));CHECK(max_energy(a.game)==4);
  a.game.energy=2;snapshot();a.render();capture((std::string("energy-")+names[character]).c_str());unchanged();
  a.game.energy=0;snapshot();a.advance(250);a.render();capture((std::string("energy-zero-")+names[character]).c_str());unchanged();
  new_run(a.game,73,false,Character(character));a.game.map[0].room=Room::Shop;CHECK(choose_node(a.game,0));a.reset_presentation();snapshot();a.render();
  CHECK(!a.shop_open&&has(HitKind::UiAction,int(UiAction::OpenShop)));
  if(a.content.version()==FeedbackContentVersion){
   CHECK(a.content.clip(AnimMerchantIronclad+character).count>1);
   const auto*outside=a.hit_at(166,190);CHECK(!outside||outside->kind!=HitKind::UiAction||outside->value!=int(UiAction::OpenShop));
  }
  capture((std::string("merchant-")+names[character]).c_str());a.advance(250);a.render();unchanged();
  a.ui_action(UiAction::OpenShop);CHECK(a.shop_open);a.input(Key::Back);CHECK(!a.shop_open);unchanged();
 }
}
int main(int argc,char**argv){std::vector<uint8_t>b;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));b.resize(size);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));}else b=synthetic_profile();
 if(argc>2)screenshot_directory=argv[2];CHECK(a.init(b.data(),b.size(),{}));options_and_readers();crystal_and_large_rewards();potion_lock_and_capacity();offer_facts();special_reward_snapshots();original_enemy_presentation();original_enemy_transformations();feedback_hud_and_merchant();std::printf("world UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");}
