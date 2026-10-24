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
static bool actual_pack;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s phase=%d\n",__FILE__,__LINE__,#x,int(a.game.phase));std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,unsigned at,unsigned n,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=(uint8_t)(n>>(8*i));}
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count,strings=ContentStrings,glyphs=22;
    constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);
    put(b,20,glyphs);put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    // Numeric labels must have real pixels in the public fixture too. The
    // rest of the synthetic alphabet remains absent; no original font ships.
    static const uint8_t digits[][7]={
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14},{0,0,17,10,4,10,17}};
    for(unsigned size=0;size<2;++size)for(unsigned ch=0;ch<11;++ch){
        const unsigned at=glyphdir+(size*11+ch)*16,height=size?7:5;
        put(b,at,ch<10?'0'+ch:'x');put(b,at+4,unsigned(b.size()));put(b,at+8,size?10:8,2);
        put(b,at+10,5,1);put(b,at+11,height,1);put(b,at+12,6,2);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<5;++x)b.push_back(digits[ch][y*7/height]&(16u>>x)?255:0);
    }
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
 a.character_select=true;a.practice_entry=false;a.title=true;a.run_settings=false;a.selected_ascension=0;
 a.ui_action(UiAction::RunSettings);CHECK(!a.run_settings);a.render();
 CHECK(!has(HitKind::UiAction,int(UiAction::RunSettings))&&!has(HitKind::UiAction,int(UiAction::RunRegion)));
 CHECK(has(HitKind::UiAction,int(UiAction::RunAscensionUp)));a.input(Key::ExhaustPile);a.input(Key::ExhaustPile);CHECK(a.selected_ascension==2);
 a.ui_action(UiAction::RunSeed);CHECK(a.map_preview.editing&&a.run_seed_editing);a.reference_text(-1);for(const char*p="WORLD-UI-17";*p;p++)a.reference_text(*p);a.reference_text(13);CHECK(!a.map_preview.open&&!a.run_seed_editing&&a.selected_seed[0]);
 a.ui_action(UiAction::StartCharacter);CHECK(a.game.world.active&&a.game.world.ascension==2);CHECK(a.game.original_events.id==OriginalEventId::Neow);snapshot();
 a.render();CHECK(a.hit_count>0);capture("ancient-neow");a.ui_action(UiAction::ReadEvent);a.render();CHECK(a.event_reader);capture("ancient-reader");a.pointer_scroll(1);a.render();a.ui_action(UiAction::CloseEventReader);unchanged();
 a.open_library(1);a.prepare_collection();int cookie=-1;
 for(unsigned i=0;i<a.collection.count;++i)if(a.collection.ids[i]==WorldRelicYummyCookie)cookie=int(i);
 CHECK(cookie>=0);a.library_selected=cookie;a.focus_collection();for(unsigned i=0;i<45;++i)a.advance(33);
 a.render();CHECK(has(HitKind::LibraryItem,cookie));a.select_library_item(cookie);a.render();CHECK(a.detail_item<0);
 a.input(Key::Back);CHECK(a.compendium_open&&!a.library);a.input(Key::Back);CHECK(!a.compendium_open);unchanged();
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
 world();a.game.potion_slots=16;for(unsigned i=0;i<16;i++)CHECK(potion_set(a.game,i,Potion::Strength));CHECK(original_event_begin(a.game,OriginalEventId::StoneOfAllTime,7,1));CHECK(original_event_blocks_potions(a.game));a.input(Key::Potions);CHECK(a.hud_focus==2&&!a.panel);a.input(Key::Confirm);CHECK(a.panel==4);for(int i=0;i<15;i++)a.input(Key::Right);CHECK(a.scroll==15);snapshot();a.render();CHECK(has(HitKind::Potion,15));CHECK(a.scroll==15);capture("potion-full-second-page");a.ui_action(UiAction::DiscardPotion);CHECK(potion_at(a.game,15)==Potion::Strength);unchanged();a.panel=0;
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
static void original_aspect_layout(){
 for(unsigned character=0;character<5;++character){
  enemy_visual_fight(EncounterId::CorpseSlugsNormal,Character(character));
  CHECK(a.creature_layout.right==1740000);
  const auto player=a.combat_player_pose();CHECK(player.feet==156&&player.body_top<player.feet&&player.body_width>0);
  CHECK(a.health_bars[0].width==health_bar::inner_width(player.width));
  for(int i=0;i<a.game.enemy_count;++i){const auto pose=a.combat_enemy_pose(i);CHECK(pose.feet==156&&pose.body_top<pose.feet);
   CHECK(a.health_bars[i+1].width==health_bar::inner_width(pose.width));}
  if(character==unsigned(Character::Necrobinder)){const auto pet=a.combat_player_pose(true);CHECK(pet.x>player.x&&pet.feet<player.feet);}
  char name[48];std::snprintf(name,sizeof name,"combat-43-character-%u",character);capture(name);unchanged();
 }
 enemy_visual_fight(EncounterId::AeonglassBoss);const auto boss=a.combat_enemy_pose(0);
 CHECK(boss.x==253&&boss.feet==164&&boss.body_height>80);capture("combat-43-large-enemy");unchanged();
 enemy_visual_fight(EncounterId::LivingFogNormal);CHECK(a.game.enemy_count==1);
 const auto actor=a.combat_enemy_pose(0),player=a.combat_player_pose();const auto layout=a.creature_layout;
 {UiCoreObserver observer(a);for(int slot=0;slot<5;++slot){queue_extension(a.game,XOp::EnemySpawn,int(EnemyId::GasBomb),slot+8);resolve_pending(a.game);}}
 CHECK(a.game.enemy_count==6&&combat_layout_for(a.game).right>layout.right);snapshot();
 // Ready fits the actors present at entry. AddCreature does not repeat
 // that pass; a newly spawned far-right slot cannot move the old actors.
 for(unsigned frame=0;frame<24;++frame){a.advance(33);a.render();
  CHECK(a.creature_layout.right==layout.right);
  const auto held=a.combat_enemy_pose(0),ally=a.combat_player_pose();
  CHECK(!std::memcmp(&actor,&held,sizeof actor)&&!std::memcmp(&player,&ally,sizeof player));unchanged();
 }
 const int last=a.game.enemy_count-1;const auto added=a.combat_enemy_pose(last),want=enemy_pose(a.game,last,layout);
 CHECK(!std::memcmp(&added,&want,sizeof added));capture("combat-43-summon-stable");
 {UiCoreObserver observer(a);damage_unpowered(a.game,last,9999);resolve_pending(a.game);}snapshot();a.render();
 const auto after_death=a.combat_enemy_pose(0);CHECK(a.creature_layout.right==layout.right&&!std::memcmp(&actor,&after_death,sizeof actor));unchanged();
 // Reset/load is an explicitly new presentation scene; its pre-render
 // fallback and its first bound render must agree on the current actors.
 a.reset_presentation();const auto fallback=a.combat_enemy_pose(0);a.render();const auto rebuilt=a.combat_enemy_pose(0);
 CHECK(!std::memcmp(&fallback,&rebuilt,sizeof fallback));unchanged();
}
static void original_osty_and_orbs(){
 enemy_visual_fight(EncounterId::NibbitsWeak,Character::Necrobinder);const auto initial=a.combat_player_pose(true);
 const uint32_t gain=a.animation+10;
 {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=gain;queue_extension(a.game,XOp::NSummon,75,1);resolve_pending(a.game);}
 CHECK(a.game.expansion.osty_hp>=75);snapshot();a.animation=gain-1;a.render();
 CHECK(a.combat_player_pose(true).scale==initial.scale&&a.combat_player_pose(true).x==initial.x);unchanged();
 a.animation=gain;a.render();const auto begin=a.combat_player_pose(true);
 CHECK(begin.scale==initial.scale&&begin.body_width==initial.body_width);
 a.animation=gain+12;a.render();const auto middle=a.combat_player_pose(true);
 CHECK(middle.scale>begin.scale&&middle.body_width==begin.body_width&&middle.x!=begin.x);capture("osty-growing-bounds-held");unchanged();
 a.animation=gain+23;a.render();const auto grown=a.combat_player_pose(true);
 CHECK(grown.scale>middle.scale&&grown.body_width>begin.body_width);
 CHECK(a.health_bars[MaxEnemies+1].width==health_bar::inner_width(grown.width));capture("osty-grown-original-position");unchanged();
 {UiCoreObserver observer(a);a.begin_combat_feedback();queue_extension(a.game,XOp::NKillOsty);resolve_pending(a.game);}
 snapshot();a.render();CHECK(a.osty_motion.clip==AnimOstyDie);
 const uint32_t death_begin=a.osty_presentation.started;CHECK(int32_t(death_begin-a.animation*33u)>0);
 CHECK(a.combat_player_pose(true).scale==grown.scale);
 a.animation=(death_begin+782)/33u;a.render();const auto down=a.combat_player_pose(true);
 CHECK(down.scale<grown.scale&&a.osty_presentation.bounds_scale==1024);capture("osty-death-terminal-pose");unchanged();
 enemy_visual_fight(EncounterId::NibbitsWeak,Character::Defect);a.animation+=15;a.render();
 CHECK(a.orb_presentation.slots==3);const auto edge=CombatOrbPresentation::target(3,0),center=CombatOrbPresentation::target(3,1);
 CHECK(edge.x>0&&center.y<edge.y);const uint32_t slots_at=a.animation+5;
 {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=slots_at;queue_extension(a.game,XOp::DSlots,2);resolve_pending(a.game);}
 snapshot();a.animation=slots_at-1;a.render();CHECK(a.orb_presentation.slots==3&&!has(HitKind::Orb,3));unchanged();
 a.animation=slots_at;a.render();CHECK(a.orb_presentation.slots==5&&has(HitKind::Orb,4));
 a.animation=slots_at+14;a.render();unsigned holders=0;int left=320,right=-1,top=240,bottom=-1;
 for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind!=HitKind::Orb)continue;++holders;
  left=std::min(left,int(h.x));right=std::max(right,int(h.x+h.w));top=std::min(top,int(h.y));bottom=std::max(bottom,int(h.y+h.h));}
 CHECK(holders==5&&right-left>60&&bottom-top>20&&top>22);capture("defect-source-orb-arc");unchanged();
}
static std::vector<uint16_t> intent_background;
static void spatial_card_targeting(){
 const auto select_potion=[](int target){
  CHECK(potion_set(a.game,0,Potion::Fire));a.target=target;snapshot();a.open_potion(0);
  CHECK(a.panel==4&&!a.potion_targeting);a.input(Key::Confirm);CHECK(a.potion_targeting&&a.target==target);unchanged();
 };
 const auto select_strike=[](int target){
  const int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,Strike,1025);resolve_pending(a.game);
  CHECK(a.game.card_count==id+1&&a.game.zones[id]==Zone::Hand);a.game.energy=99;a.reset_presentation();a.render();
  for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);a.cursor=a.game.hand.count-1;a.target=target;snapshot();
  a.input(Key::Confirm);CHECK(a.keyboard_card==id&&a.target==target);unchanged();
 };
 // The original Kaiser encounter places its two arms on opposite sides of
 // the player. Target navigation follows those real positions and wraps.
 enemy_visual_fight(EncounterId::KaiserCrabBoss);CHECK(a.game.enemy_count==2);
 CHECK(enemy_pose(a.game,0).x<combat_player_x(a.game)&&enemy_pose(a.game,1).x>combat_player_x(a.game));
 select_strike(1);a.input(Key::Left);CHECK(a.target==0);a.input(Key::Right);CHECK(a.target==1);
 a.input(Key::Right);CHECK(a.target==0);a.input(Key::Up);a.input(Key::Down);CHECK(a.target==0);unchanged();
 a.render();capture("keyboard-kaiser-target");a.input(Key::Back);CHECK(a.keyboard_card<0&&!a.paused);unchanged();
 select_potion(1);a.input(Key::Left);CHECK(a.target==0);a.input(Key::Right);CHECK(a.target==1);
 a.input(Key::Right);CHECK(a.target==0);a.input(Key::Up);a.input(Key::Down);CHECK(a.target==0);unchanged();
 a.render();capture("keyboard-kaiser-potion-target");a.input(Key::Back);CHECK(a.panel==0&&!a.potion_targeting);unchanged();
 // Living Fog occupies named slot 5. Its summoned slots 0..4 are stored
 // after it, and slot 2 lies just to its right: raw slot cycling is wrong.
 enemy_visual_fight(EncounterId::LivingFogNormal);
 for(int slot=0;slot<5;++slot){queue_extension(a.game,XOp::EnemySpawn,int(EnemyId::GasBomb),slot+8);resolve_pending(a.game);}
 CHECK(a.game.enemy_count==6);const int spatial[]={1,2,0,3,4,5};
 for(unsigned i=1;i<6;++i)CHECK(enemy_pose(a.game,spatial[i-1]).x<enemy_pose(a.game,spatial[i]).x);
 select_strike(0);a.input(Key::Right);CHECK(a.target==3);a.input(Key::Left);CHECK(a.target==0);
 a.input(Key::Left);CHECK(a.target==2);a.input(Key::Left);CHECK(a.target==1);a.input(Key::Left);CHECK(a.target==5);
 a.input(Key::Right);CHECK(a.target==1);unchanged();a.render();capture("keyboard-summoned-spatial-target");
 a.input(Key::Back);CHECK(a.keyboard_card<0&&!a.paused);unchanged();
 select_potion(0);a.input(Key::Right);CHECK(a.target==3);a.input(Key::Left);CHECK(a.target==0);
 a.input(Key::Left);CHECK(a.target==2);a.input(Key::Up);a.input(Key::Down);CHECK(a.target==2);
 a.input(Key::Left);CHECK(a.target==1);a.input(Key::Left);CHECK(a.target==5);a.input(Key::Right);CHECK(a.target==1);unchanged();
 a.render();capture("keyboard-summoned-potion-target");a.input(Key::Back);CHECK(a.panel==0&&!a.potion_targeting);unchanged();
}
static void capture_intent_background(){
 // Remove only display intents at this same clock. The actual room/actor
 // pixels beneath the numeric labels are the independent blend background.
 uint16_t flags[MaxEnemies]{};for(int i=0;i<MaxEnemies;++i){flags[i]=a.combat_feedback.intents[i].flags;a.combat_feedback.intents[i].flags=0;}
 a.render();intent_background.assign(a.pixels,a.pixels+ScreenW*ScreenH);
 for(int i=0;i<MaxEnemies;++i)a.combat_feedback.intents[i].flags=flags[i];a.render();
}
static int rendered_number(const char*text,int size,int left,int top,int right,int bottom,const char*repeat=""){
 // Search for the requested number's independently composed pixels. Small
 // real font strokes need not contain alpha=255, so use their actual alpha
 // over the captured room, not a predicate that can vacuously match no ink.
 struct Point {int x,y;unsigned alpha;};std::vector<Point> ink;int width=0,height=0;
 const auto append=[&](const char*string,int px,int offset_y){
  for(const char*p=string;*p;++p){const auto g=a.content.glyph(unsigned(*p),px);CHECK(g.pixels&&g.w&&g.h);
   std::vector<uint8_t> alpha(unsigned(g.w)*g.h,0);
   if(g.encoded_size){unsigned pos=0;for(unsigned i=0;i+1<g.encoded_size&&pos<alpha.size();i+=2)
    for(unsigned n=0;n<g.pixels[i]&&pos<alpha.size();++n)alpha[pos++]=g.pixels[i+1];
   }else std::memcpy(alpha.data(),g.pixels,alpha.size());
   for(unsigned y=0;y<g.h;++y)for(unsigned x=0;x<g.w;++x)if(alpha[y*g.w+x])ink.push_back({width+int(x),offset_y+int(y),alpha[y*g.w+x]});
   if(offset_y+int(g.h)>height)height=offset_y+g.h;width+=g.advance;
  }
 };
 append(text,size,0);append(repeat,8,2);CHECK(!ink.empty()&&intent_background.size()==ScreenW*ScreenH);
 constexpr uint16_t cream=uint16_t((31u<<11)|(61u<<5)|28u),outline=uint16_t((2u<<11)|(5u<<5)|2u);
 const auto mix=[](uint16_t back,uint16_t fore,unsigned alpha){
  const unsigned r=((back>>11)*(255-alpha)+(fore>>11)*alpha+127)/255;
  const unsigned g=(((back>>5)&63)*(255-alpha)+((fore>>5)&63)*alpha+127)/255;
  const unsigned b=((back&31)*(255-alpha)+(fore&31)*alpha+127)/255;return uint16_t((r<<11)|(g<<5)|b);
 };
 const int stride=width+2,rows=height+2;std::vector<uint16_t> expected(unsigned(stride*rows));
 for(int y=top;y+height<bottom;++y)for(int x=left;x+width<right;++x){
  for(int yy=0;yy<rows;++yy)for(int xx=0;xx<stride;++xx)expected[yy*stride+xx]=intent_background[(y+yy-1)*ScreenW+x+xx-1];
  for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)for(const auto&p:ink){const int at=(p.y+dy+1)*stride+p.x+dx+1;expected[at]=mix(expected[at],outline,p.alpha);}
  for(const auto&p:ink){const int at=(p.y+1)*stride+p.x+1;expected[at]=mix(expected[at],cream,p.alpha);}
  bool match=true;unsigned visible=0;
  for(int yy=0;yy<rows&&match;++yy)for(int xx=0;xx<stride;++xx){
   // The icon occupies the first 14 rows; its last two rows may overlap a
   // synthetic font's top. Compare all remaining pixels, including holes.
   if(y+yy-1<top+4)continue;const int at=(y+yy-1)*ScreenW+x+xx-1;
   visible+=expected[yy*stride+xx]!=intent_background[at];if(a.pixels[at]!=expected[yy*stride+xx]){match=false;break;}
  }
  if(match&&visible)return x;
 }return -1;
}
static void compound_intent_values(){
 struct Case {EncounterId encounter;int move,ascension,damage,status;const char*name;};
 const Case cases[]={
  {EncounterId::MechaKnightElite,1,0,8,4,"intent-mecha-flamethrower"},
  {EncounterId::MechaKnightElite,1,9,12,4,"intent-mecha-flamethrower-a9"},
  {EncounterId::SoulFyshBoss,2,0,7,1,"intent-soul-fysh-gaze"},
  {EncounterId::VantomBoss,2,0,26,3,"intent-vantom-dismember"}};
 for(const auto&test:cases){
  enemy_visual_fight(test.encounter);a.game.enemy_world.ascension=uint8_t(test.ascension);a.game.enemy_world.models[0].node=uint8_t(test.move);
  a.pointer.visible=false;a.reset_presentation();a.render();const auto intent=enemy_intent(a.game,0);
  CHECK(intent.flags==(EnemyIntentAttack|EnemyIntentStatus)&&intent.damage==test.damage&&intent.hits==1&&intent.status_cards==test.status);
  snapshot();a.begin_combat_feedback();capture_intent_background();const auto pos=enemy_pose(a.game,0);
  char damage[16]{},status[16]{};std::snprintf(damage,sizeof damage,"%d",test.damage);std::snprintf(status,sizeof status,"%d",test.status);
  const int damage_x=rendered_number(damage,10,pos.x-32,pos.intent+10,pos.x,pos.intent+34);
  const int status_x=rendered_number(status,10,pos.x,pos.intent+10,pos.x+32,pos.intent+34);
  CHECK(damage_x>=0&&status_x>damage_x);CHECK(has(HitKind::EnemyIntent,0));
  for(int x:{damage_x,status_x}){const auto*hit=a.hit_at(x,pos.intent+16);CHECK(hit&&hit->kind==HitKind::EnemyIntent&&hit->value==0);}
  const std::vector<uint16_t> shown(a.pixels,a.pixels+ScreenW*ScreenH);
  // Removing only the display count must erase visible pixels, while keeping
  // the attack value intact. The former Attack/else-if branch fails this.
  a.combat_feedback.intents[0].status_cards=0;a.render();
  CHECK(rendered_number(damage,10,pos.x-32,pos.intent+10,pos.x,pos.intent+34)>=0);
  CHECK(rendered_number(status,10,pos.x,pos.intent+10,pos.x+32,pos.intent+34)<0);
  bool changed=false;for(int y=pos.intent+10;y<pos.intent+34;++y)for(int x=pos.x;x<pos.x+32;++x)changed|=shown[y*ScreenW+x]!=a.pixels[y*ScreenW+x];CHECK(changed);
  a.combat_feedback.intents[0]=intent;a.render();unchanged();capture(test.name);
  if(test.encounter==EncounterId::MechaKnightElite){
   // Canonical settlement can already point at Windup; the retained labels
   // must still describe the visible Flamethrower until its display cue.
   a.game.enemy_world.models[0].node=2;snapshot();capture_intent_background();CHECK(!(enemy_intent(a.game,0).flags&EnemyIntentAttack));
   CHECK(rendered_number(damage,10,pos.x-32,pos.intent+10,pos.x,pos.intent+34)>=0);
   CHECK(rendered_number("4",10,pos.x,pos.intent+10,pos.x+32,pos.intent+34)>=0);unchanged();
  }
 }
 // Two neighboring real compound attacks retain separate hit targets and
 // put each damage value under its attack, rather than between both icons.
 enemy_visual_fight(EncounterId::NibbitsNormal);for(int i=0;i<2;++i)a.game.enemy_world.models[i].node=1;
 a.reset_presentation();snapshot();a.begin_combat_feedback();capture_intent_background();for(int i=0;i<2;++i){const auto pos=enemy_pose(a.game,i);const auto intent=enemy_intent(a.game,i);
  CHECK(intent.flags==(EnemyIntentAttack|EnemyIntentBlock)&&intent.damage==6);
  const int x=rendered_number("6",10,pos.x-32,pos.intent+10,pos.x,pos.intent+34);CHECK(x>=0);
  const auto*hit=a.hit_at(x,pos.intent+16);CHECK(hit&&hit->kind==HitKind::EnemyIntent&&hit->value==i);
 }capture("intent-two-nibbits-slice");unchanged();
 // Keep the original three-hit display and its smaller suffix, using actual
 // Byrdonis intent data instead of synthesizing a new unsupported flag set.
 enemy_visual_fight(EncounterId::ByrdonisElite);a.game.enemy_world.models[0].node=0;snapshot();a.begin_combat_feedback();capture_intent_background();const auto intent=enemy_intent(a.game,0);const auto pos=enemy_pose(a.game,0);
 CHECK(intent.damage==3&&intent.hits==3&&intent.flags==EnemyIntentAttack);
 const int repeat_x=rendered_number("3",10,pos.x-25,pos.intent+10,pos.x+25,pos.intent+34,"x3");CHECK(repeat_x>=0&&repeat_x<pos.x);
 capture("intent-byrdonis-three-hits");unchanged();
}
static void original_enemy_presentation(){
 compound_intent_values();
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
 CHECK(enemy_pose(a.game,0).x<combat_player_x(a.game)&&enemy_pose(a.game,1).x>combat_player_x(a.game));
 for(int i=0;i<2;i++){CHECK(world_enemy_visual(unsigned(a.game.enemy_world.models[i].kind))->shared_group==1);CHECK(a.enemy_frame(i,clip,elapsed,loop)&&clip>=38);}
 snapshot();for(unsigned i=0;i<3;i++){a.advance(250);a.render();}capture("combat-kaiser-shared");unchanged();
}
static const WorldEnemyVisualClip* visual_clip(int slot){
 int clip=-1;uint32_t elapsed=0;bool loop=false;CHECK(a.enemy_frame(slot,clip,elapsed,loop));const auto*row=world_enemy_clip_directory(unsigned(clip));CHECK(row);return row;
}
static void first_enemy_cue(){
 const uint32_t cue=a.turn_banner.enemy_start+TurnBannerPresentation::enemy_lead_ticks(a.fast_mode)+TurnBannerPresentation::intent_lead_ticks(a.fast_mode);
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
static void original_enemy_command_waits(){
 // Source command waits are shorter than some skeleton tails. Two genuine
 // enemy turns must hand off at the command boundary in both speed modes.
 const unsigned nibbit_ticks[2][3]={{11,19,26},{6,9,11}};
 const unsigned louse_ticks[2][3]={{20,13,23},{10,7,10}};
 for(unsigned fast=0;fast<2;++fast){
  for(int move=0;move<3;++move){
   enemy_visual_fight(EncounterId::NibbitsNormal);a.fast_mode=fast!=0;
   CHECK(a.game.enemy_count==2);a.game.enemy_world.models[0].node=uint8_t(move);a.game.enemy_world.models[1].node=0;
   if(move==0)a.enemy_visual_encounter=-1; // First observer sync cannot erase the lead-in.
   const uint32_t start=a.animation;a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat);
   auto&first=a.enemy_presentation[0];auto&second=a.enemy_presentation[1];
   CHECK(first.count==1&&second.count==1&&first.queue[0].move==move&&second.queue[0].move==0);
   const uint32_t first_cue=first.queue[0].earliest,second_cue=second.queue[0].earliest;
   CHECK(first_cue==start+(fast?24u:38u));
   CHECK(second_cue==first_cue+nibbit_ticks[fast][move]+(fast?8u:13u));
   CHECK(a.enemy_visual_clock==second_cue+nibbit_ticks[fast][0]);
   snapshot();a.animation=first_cue;CHECK(!std::strcmp(visual_clip(0)->native_clip,move==2?"hiss":"attack"));
   const uint32_t tail=first.until;CHECK(tail>first.start);
   a.animation=second_cue;CHECK(!std::strcmp(visual_clip(1)->native_clip,"attack")&&second.start==second_cue);
   if(move==2){CHECK(first.until==tail&&int32_t(tail-a.animation)>0);CHECK(!std::strcmp(visual_clip(0)->native_clip,"hiss")&&!first.loop);}
   unchanged();
  }
  for(int move=0;move<3;++move){
   enemy_visual_fight(EncounterId::LouseProgenitorNormal);a.fast_mode=fast!=0;a.game.enemy_world.models[0].node=uint8_t(move);
   const uint32_t start=a.animation;a.input(Key::EndTurn);auto&louse=a.enemy_presentation[0];
   // Uncurled Web/Pounce skip the conditional Uncurl prefix entirely.
   CHECK(louse.count==1&&louse.queue[0].move==move);const uint32_t cue=louse.queue[0].earliest;
   CHECK(cue==start+(fast?24u:38u)&&a.enemy_visual_clock==cue+louse_ticks[fast][move]);
   snapshot();a.animation=cue;const char*name=move==0?"attack_web":move==1?"attack":"curl";
   CHECK(!std::strcmp(visual_clip(0)->native_clip,name)&&!louse.loop&&louse.start==cue);
   CHECK(louse.until==cue+(move==2?33u:26u));
   if(move==2){a.animation=louse.until+1;a.sync_enemy_feedback();CHECK(!std::strcmp(visual_clip(0)->native_clip,"curled_loop")&&louse.loop&&louse.state==3);}
   unchanged();
  }
  enemy_visual_fight(EncounterId::LouseProgenitorNormal);a.fast_mode=fast!=0;a.game.enemy_world.models[0].node=1;
  auto&louse=a.enemy_presentation[0];louse.state=3;louse.loop=true;louse.clip=world_enemy_resolve_clip(louse.model,"curled_loop",louse.skin);
  a.input(Key::EndTurn);CHECK(louse.count==2);const uint32_t first=louse.queue[0].earliest,attack=louse.queue[1].earliest;
  CHECK(!std::strcmp(louse.queue[0].action,"Uncurl")&&!std::strcmp(louse.queue[1].action,"Attack"));
  CHECK(attack==first+(fast?8u:28u)&&a.enemy_visual_clock==attack+louse_ticks[fast][1]);
  snapshot();a.animation=first;CHECK(!std::strcmp(visual_clip(0)->native_clip,"uncurl"));
  CHECK(louse.until==first+28u);a.animation=attack;
  // Animator triggers remain conditional. Fast's 250ms command wait does
  // not invent an Attack branch from an unfinished Uncurl state.
  CHECK(!std::strcmp(visual_clip(0)->native_clip,fast?"uncurl":"attack")&&!louse.count);unchanged();
 }
 a.fast_mode=false;
}
static void audited_leaf_and_cultist_timing(){
 // Run the actual end-turn dispatcher, with the same serialized start state
 // also resolved directly. Presentation must neither change its outcome nor
 // delay the next creature until this one's complete skeleton has finished.
 static State direct;static SaveData expected;
 const EnemyId models[]={EnemyId::LeafSlimeS,EnemyId::LeafSlimeM,EnemyId::DampCultist};
 const unsigned normal_impact[3][2]={{150,500},{150,1000},{950,200}};
 const unsigned fast_impact[3][2]={{75,250},{75,250},{475,100}};
 const unsigned normal_end[3][2]={{350,1500},{350,2000},{1200,400}};
 const unsigned fast_end[3][2]={{175,1250},{175,1250},{575,200}};
 for(unsigned fast=0;fast<2;++fast)for(unsigned model=0;model<3;++model)for(unsigned move=0;move<2;++move){
  enemy_visual_fight(model==2?EncounterId::CultistsNormal:EncounterId::SlimesNormal);a.fast_mode=fast!=0;
  int slot=-1;for(int i=0;i<a.game.enemy_count;++i){a.game.enemy_world.models[i].node=0;if(a.game.enemy_world.models[i].kind==models[model])slot=i;}
  CHECK(slot>=0);a.game.enemy_world.models[slot].node=uint8_t(move);a.game.block=0;
  const auto intent=enemy_intent(a.game,slot);const bool attack=model==2?move==1:move==0;
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));CHECK(save(direct,expected));
  a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&a.combat_feedback.active);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  const auto&actor=a.enemy_presentation[slot];CHECK(actor.count==1&&actor.queue[0].move==int(move));
  const uint32_t start=actor.queue[0].earliest;
  const uint32_t impact=start+((fast?fast_impact[model][move]:normal_impact[model][move])+32)/33;
  const uint32_t end=start+((fast?fast_end[model][move]:normal_end[model][move])+32)/33;
  if(slot+1<a.game.enemy_count){const auto&next=a.enemy_presentation[slot+1];CHECK(next.count==1&&next.queue[0].earliest==end+TurnBannerPresentation::intent_lead_ticks(fast!=0));}
  else CHECK(a.enemy_visual_clock==end);
  snapshot();a.animation=start;a.render();CHECK(a.combat_intent(slot).flags&&a.combat_intent_alpha(slot)>0);
  const int hp_before=a.combat_health(0).hp,ritual=EnemyStatusStart+int(EnemyPower::Ritual);
  const auto piles_before=a.combat_pile_counts();
  a.animation=impact-1;a.render();CHECK(a.combat_health(0).hp==hp_before);
  if(model==2&&move==0)CHECK(a.combat_status_amount(slot+1,ritual)==0);
  if(model<2&&move==1)CHECK(a.combat_pile_counts().discard==piles_before.discard);
  a.animation=impact;a.render();
  if(attack){CHECK(intent.damage>0&&a.combat_health(0).hp==hp_before-intent.damage);CHECK(a.hit_start[0]==int(impact));}
  else if(model==2)CHECK(a.combat_status_amount(slot+1,ritual)==enemy_power_value(a.game,slot,EnemyPower::Ritual));
  else{
   // The generated batch first occupies NCardPreviewContainer. Its pile
   // counter increases only as each preview finishes its later flight.
   const auto piles=a.combat_pile_counts();CHECK(piles.discard==piles_before.discard);
   CHECK(piles.draw==piles_before.draw&&piles.exhaust==piles_before.exhaust);
   unsigned previews=0;for(const auto&v:a.combat_cards.sprites)if(v.mode==CombatCardMotion::Preview&&v.card.kind==Slimed)++previews;
   CHECK(previews==unsigned(model==0?1:2));
  }
  if(attack){a.animation=end;a.render();CHECK(actor.until>end&&!actor.loop);}
  unchanged();
 }
 a.fast_mode=false;
}
static void advance_combat_ms(unsigned ms){while(ms){const unsigned step=ms>250?250:ms;a.advance(step);ms-=step;}}
static void original_combat_clock(){
 // Exercise the host update path, not only direct tick assignments. Original
 // Fast shortens waits while banners, Spine clips and HP trails use real time.
 for(unsigned fast=0;fast<2;++fast){
  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.tick_remainder=0;a.game.enemy_world.models[0].node=2;
  const uint32_t start=a.animation;a.input(Key::EndTurn);const uint32_t cue=a.enemy_presentation[0].queue[0].earliest;
  snapshot();advance_combat_ms((cue-start)*33);a.render();int clip=-1;uint32_t elapsed=0;bool loop=true;
  CHECK(a.animation==cue&&a.enemy_frame(0,clip,elapsed,loop)&&elapsed==0&&!loop);
  CHECK(!std::strcmp(world_enemy_clip_directory(unsigned(clip))->native_clip,"hiss"));
  advance_combat_ms(198);a.render();int next_clip=-1;CHECK(a.animation==cue+6&&a.enemy_frame(0,next_clip,elapsed,loop)&&next_clip==clip&&elapsed==198&&!loop);
  CHECK(a.enemy_presentation[0].until==cue+50); // Original 1,633 ms skeleton tail.
  capture(fast?"combat-real-clock-fast":"combat-real-clock");
  advance_combat_ms((start+69-a.animation)*33);a.render();CHECK(a.turn_banner.sample_enemy(a.animation).kind==TurnBannerPresentation::Enemy);
  advance_combat_ms(33);a.render();CHECK(a.animation==start+70&&a.turn_banner.sample_enemy(a.animation).kind==TurnBannerPresentation::None);
  CHECK(a.enemy_frame(0,next_clip,elapsed,loop)&&next_clip==clip&&elapsed==(70-(cue-start))*33&&!loop);unchanged();

  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.tick_remainder=0;a.game.enemy_world.models[0].node=0;a.game.block=0;a.render();
  const uint32_t hit_start=a.animation;a.input(Key::EndTurn);
  const uint32_t hit=a.enemy_presentation[0].queue[0].earliest+TurnBannerPresentation::wait_ticks(150,75,fast!=0);
  snapshot();advance_combat_ms((hit-hit_start)*33);a.render();const auto&bar=a.health_bars[0];
  CHECK(a.animation==hit&&bar.start_ms==hit*33&&bar.delayed&&bar.from_q8>bar.to_q8);
  const int held=bar.sample_q8(a.animation*33);
  advance_combat_ms(990);a.render();CHECK(a.animation==hit+30&&bar.sample_q8(a.animation*33)==held);
  advance_combat_ms(33);a.render();CHECK(a.animation==hit+31&&bar.sample_q8(a.animation*33)<held);
  unchanged();
 }
 a.fast_mode=false;
}
static void deferred_enemy_values(){
 static State direct;static SaveData expected;
 CHECK(sizeof(CombatFeedback)<=8192);
 for(unsigned fast=0;fast<2;++fast){
  enemy_visual_fight(EncounterId::NibbitsNormal);a.fast_mode=fast!=0;
  a.game.enemy_world.models[0].node=0;a.game.enemy_world.models[1].node=1;
  a.game.energy=0;a.game.block=3;
  const int hp=a.game.hp;const auto first_intent=enemy_intent(a.game,0),second_intent=enemy_intent(a.game,1);
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));CHECK(save(direct,expected));
  a.input(Key::EndTurn);CHECK(a.combat_feedback.active&&a.combat_feedback.finished&&!a.combat_feedback.overflow);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  CHECK(a.game.hp<hp&&a.game.energy>0&&a.game.enemies[1].block>0);
  CHECK(a.combat_health(0).hp==hp&&a.combat_health(0).block==3&&a.combat_energy()==0);
  CHECK(a.combat_health(2).block==0&&a.combat_intent(0).damage==first_intent.damage&&a.combat_intent(1).damage==second_intent.damage);
  const uint32_t first=a.enemy_presentation[0].queue[0].earliest,second=a.enemy_presentation[1].queue[0].earliest;
  const uint32_t lead=TurnBannerPresentation::intent_lead_ticks(fast!=0),impact=TurnBannerPresentation::wait_ticks(150,75,fast!=0);
  const uint32_t block=second+TurnBannerPresentation::wait_ticks(350,175,fast!=0),done=a.combat_feedback.end_at;
  snapshot();a.input(Key::Confirm);a.input(Key::EndTurn);unchanged();
  a.animation=first-lead-1;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent(1).flags);
  a.animation=first-lead;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)==255&&a.combat_intent(1).flags);
  a.animation=first+impact-1;a.render();CHECK(a.combat_health(0).hp==hp&&a.combat_health(0).block==3);
  a.animation=first+impact;a.render();CHECK(a.combat_health(0).hp==hp-(first_intent.damage-3)&&a.combat_health(0).block==0);
  CHECK(a.hit_start[0]==int(first+impact)&&a.combat_energy()==0);capture(fast?"enemy-values-first-fast":"enemy-values-first");
  a.animation=second+impact-1;a.render();CHECK(a.combat_health(0).hp==hp-(first_intent.damage-3)&&a.combat_intent(1).flags&&a.combat_intent_alpha(1)>0);
  a.animation=second+impact;a.render();CHECK(a.combat_health(0).hp==a.game.hp&&a.combat_health(2).block==0);
  a.animation=block-1;a.render();CHECK(a.combat_health(2).block==0);
  a.animation=block;a.render();CHECK(a.combat_health(2).block==a.game.enemies[1].block&&a.combat_energy()==0);
  a.animation=done;a.render();CHECK(!a.combat_feedback.active&&a.combat_energy()==a.game.energy);
  CHECK(a.combat_intent(0).flags==enemy_intent(a.game,0).flags&&a.combat_intent(1).damage==enemy_intent(a.game,1).damage);unchanged();

  // The pet absorbs at the same visible impact; its life is not read from
  // the synchronously advanced canonical turn before that boundary.
  enemy_visual_fight(EncounterId::NibbitsWeak,Character::Necrobinder);a.fast_mode=fast!=0;
  a.game.enemy_world.models[0].node=0;a.game.block=0;a.game.expansion.osty_hp=a.game.expansion.osty_max_hp=5;
  const int player=a.game.hp;a.last_osty_hp=5;a.input(Key::EndTurn);CHECK(a.combat_feedback.active);
  const uint32_t pet_hit=a.enemy_presentation[0].queue[0].earliest+impact;
  CHECK(a.combat_health(MaxEnemies+1).hp==5&&a.combat_health(0).hp==player);
  CHECK(a.combat_status_amount(0,45)==5);
  a.animation=pet_hit-1;a.render();CHECK(a.combat_health(MaxEnemies+1).hp==5);
  a.animation=pet_hit;a.render();CHECK(a.combat_health(MaxEnemies+1).hp==0&&a.combat_health(0).hp<player);
  CHECK(a.combat_status_amount(0,45)==0);
  snapshot();a.reset_presentation();CHECK(!a.combat_feedback.active&&a.combat_health(0).hp==a.game.hp);unchanged();

  for(int lethal=0;lethal<2;++lethal){
   enemy_visual_fight(EncounterId::NibbitsNormal);a.fast_mode=fast!=0;
   for(int i=0;i<2;++i)a.game.enemy_world.models[i].node=0;
   a.game.block=0;a.game.hp=lethal?enemy_intent(a.game,0).damage+1:1;a.input(Key::EndTurn);
   CHECK(a.game.phase==Phase::Dead&&a.combat_feedback.active);
   const uint32_t hit=a.enemy_presentation[lethal].queue[0].earliest+impact,finish=a.combat_feedback.end_at;
   snapshot();a.animation=hit-1;a.render();CHECK(a.combat_health(0).hp>0&&has(HitKind::Battlefield,0));
   CHECK(a.turn_banner.sample_enemy(a.animation).kind==TurnBannerPresentation::Enemy);
   a.animation=hit;a.render();CHECK(a.combat_health(0).hp==0&&has(HitKind::Battlefield,0));
   a.animation=finish+1;a.render();CHECK(!a.combat_feedback.active&&!has(HitKind::Battlefield,0));unchanged();
  }

  enemy_visual_fight(EncounterId::NibbitsNormal);a.fast_mode=fast!=0;
  for(int i=0;i<2;++i)a.game.enemy_world.models[i].node=0;
  a.game.enemies[0].hp=1;a.game.thorns=3;a.input(Key::EndTurn);
  CHECK(a.game.enemies[0].hp==0&&(a.game.enemy_world.models[0].flags&EnemyRemoved));
  CHECK(a.combat_health(1).hp==1&&!a.enemy_presentation[0].dead&&!a.enemy_presentation[0].removing&&a.enemy_opacity(0)==255);
  const uint32_t reflected=a.enemy_presentation[0].queue[0].earliest+impact;
  snapshot();a.animation=reflected-1;a.render();CHECK(a.combat_health(1).hp==1&&!a.enemy_presentation[0].removing);
  a.animation=reflected;a.render();CHECK(a.combat_health(1).hp==0&&a.enemy_presentation[0].dead&&a.enemy_presentation[0].removing);
  CHECK(a.hit_start[1]==int(reflected));unchanged();
 }
 a.fast_mode=false;

 for(unsigned fast=0;fast<2;++fast){
  enemy_visual_fight(EncounterId::FogmogNormal);a.fast_mode=fast!=0;a.game.enemy_world.models[0].node=0;a.input(Key::EndTurn);
  CHECK(a.game.enemy_count==2&&a.game.enemies[1].hp>0&&a.combat_health(2).hp==0&&a.enemy_opacity(1)==0);
  const uint32_t summon=a.enemy_presentation[0].queue[0].earliest;
  snapshot();a.animation=summon-1;a.render();CHECK(a.combat_health(2).hp==0&&a.enemy_opacity(1)==0);
  a.animation=summon;a.render();CHECK(a.combat_health(2).hp==a.game.enemies[1].hp&&a.enemy_opacity(1)==255);
  CHECK(a.enemy_presentation[1].identity==a.game.enemy_world.models[1].combat_id);unchanged();
  a.animation=a.combat_feedback.end_at;a.render();CHECK(!a.combat_feedback.active);
  CHECK(a.game.enemy_world.models[1].kind==EnemyId::EyeWithTeeth);
  const auto old_intent=a.combat_intent(1);CHECK(old_intent.flags==EnemyIntentStatus&&old_intent.status_cards==3);
  bool found=false;for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::EnemyIntent&&a.hits[i].value==1){
   a.pointer.x=a.hits[i].x+a.hits[i].w/2;a.pointer.y=a.hits[i].y+1;found=true;break;
  }
  CHECK(found);a.pointer.visible=true;a.render();
  const int panel_x=a.pointer.x>=170?8:134,title_x=panel_x+6;
  const auto intent_panel=[&](){
   const uint16_t gold=uint16_t((223>>3)<<11|(186>>2)<<5|(112>>3));
   for(int dx=0;dx<178;++dx)if(a.pixels[48*ScreenW+panel_x+dx]!=gold)return false;
   for(int dy=0;dy<20;++dy)if(a.pixels[(48+dy)*ScreenW+panel_x]!=gold)return false;
   return true;
  };
  CHECK(intent_panel());std::vector<uint16_t> old_title;
  for(int y=53;y<67;++y)old_title.insert(old_title.end(),a.pixels+y*ScreenW+title_x,a.pixels+y*ScreenW+title_x+166);
  char old_text[1024]{},retained_text[1024]{};
  a.localized_text("intents","STATUS.description",old_text,sizeof old_text,1,2);
  const uint32_t old_identity=a.enemy_presentation[1].identity,old_bar_identity=a.health_bars[2].identity,replace=a.animation+10;
  const auto old_pose=a.combat_enemy_pose(1);
  const int old_hp=a.game.enemies[1].hp;
  // Real escape/spawn commands force reuse of Fogmog's summoned Eye slot.
  // The Eye has not reached an intent-hide cue; reusing its slot must not
  // give the Nibbit its retained status-card icon or tooltip.
  {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback_move(0,0,replace,replace,replace+10);
   queue_extension(a.game,XOp::EnemyEscape,1);queue_extension(a.game,XOp::EnemySpawn,int(EnemyId::Nibbit),0);resolve_pending(a.game);}
  CHECK(a.game.enemy_world.models[1].combat_id!=old_identity&&a.combat_health(2).hp==old_hp&&a.enemy_presentation[1].identity==old_identity);
  CHECK(a.combat_feedback.finished&&!(a.combat_feedback.hidden&(1u<<1))&&a.combat_feedback.end_at>replace);
  const auto new_intent=enemy_intent(a.game,1);CHECK(new_intent.flags&&new_intent.flags!=old_intent.flags);
  const uint32_t done=a.combat_feedback.end_at;
  snapshot();a.animation=replace-1;a.render();
  CHECK(a.enemy_presentation[1].identity==old_identity&&a.enemy_opacity(1)==255&&a.health_bars[2].identity==old_bar_identity);
  CHECK(a.combat_enemy_pose(1).x==old_pose.x&&a.combat_enemy_pose(1).feet==old_pose.feet&&a.combat_enemy_pose(1).scale==old_pose.scale&&a.combat_enemy_pose(1).width==old_pose.width);
  CHECK(a.combat_intent(1).flags==old_intent.flags&&a.combat_intent(1).status_cards==old_intent.status_cards);
  CHECK(has(HitKind::EnemyIntent,1)&&intent_panel());
  a.localized_text("intents","STATUS.description",retained_text,sizeof retained_text,1,2);CHECK(!std::strcmp(old_text,retained_text));
  for(int y=53;y<67;++y)CHECK(!std::memcmp(a.pixels+y*ScreenW+title_x,old_title.data()+(y-53)*166,166*sizeof(uint16_t)));
  unchanged();
  a.animation=replace;a.render();
  CHECK(a.combat_feedback.active&&a.combat_health(2).hp==a.game.enemies[1].hp&&a.enemy_presentation[1].identity==a.game.enemy_world.models[1].combat_id&&a.health_bars[2].identity==a.combat_feedback.identity[2]);
  CHECK(a.combat_enemy_pose(1).scale==enemy_pose(a.game,1,a.creature_layout).scale&&a.combat_enemy_pose(1).width==enemy_pose(a.game,1,a.creature_layout).width);
  CHECK(!a.combat_intent(1).flags&&!has(HitKind::EnemyIntent,1)&&!intent_panel());unchanged();
  a.animation=done-1;a.render();CHECK(a.combat_feedback.active&&!a.combat_intent(1).flags&&!has(HitKind::EnemyIntent,1)&&!intent_panel());unchanged();
  a.animation=done;a.render();const auto revealed=a.combat_intent(1);
  CHECK(!a.combat_feedback.active&&revealed.flags==new_intent.flags&&revealed.damage==new_intent.damage&&revealed.hits==new_intent.hits&&revealed.status_cards==new_intent.status_cards&&!std::strcmp(revealed.move_key,new_intent.move_key));
  // The replacement's authored Bounds can move its intent. Hover the new
  // actual region after reveal, rather than the removed Eye's old geometry.
  CHECK(has(HitKind::EnemyIntent,1));found=false;
  for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::EnemyIntent&&a.hits[i].value==1){a.pointer.x=a.hits[i].x+a.hits[i].w/2;a.pointer.y=a.hits[i].y+1;found=true;break;}
  CHECK(found);a.render();CHECK(intent_panel());bool renamed=false;
  for(int y=53;y<67;++y)renamed|=std::memcmp(a.pixels+y*ScreenW+title_x,old_title.data()+(y-53)*166,166*sizeof(uint16_t))!=0;
  // synthetic_profile has numeric glyphs only. Its blank alphabetic
  // title cannot prove a name change; keep every state/intent/hit/panel check
  // there and require the rendered rename for every actual resource pack.
  if(actual_pack)CHECK(renamed);
  else CHECK(!a.content.glyph('E',12).pixels&&!a.content.glyph('N',12).pixels);
  unchanged();a.pointer.visible=false;
 }
 a.fast_mode=false;

 enemy_visual_fight(EncounterId::NibbitsNormal);CHECK(potion_set(a.game,0,Potion::Strength));a.input(Key::EndTurn);
 a.input(Key::Potions);CHECK(a.hud_focus==2&&!a.panel);a.input(Key::Confirm);CHECK(a.panel==4&&a.combat_actions_locked());snapshot();
 a.input(Key::Confirm);CHECK(a.panel==4&&potion_at(a.game,0)==Potion::Strength);unchanged();
 // Browser '5' and the calculator's numeric confirmation normalize to the
 // same Key::Confirm value (4); neither can bypass the panel's action guard.
 a.input(static_cast<Key>(4));unchanged();a.ui_action(UiAction::DiscardPotion);unchanged();
 a.render();HitRegion use{},discard{};bool found_use=false,found_discard=false;
 for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
  if(h.kind==HitKind::Potion&&h.value==0){use=h;found_use=true;}
  if(h.kind==HitKind::UiAction&&h.value==int(UiAction::DiscardPotion)){discard=h;found_discard=true;}}
 CHECK(found_use&&!use.enabled);CHECK(found_discard&&!discard.enabled);
 a.pointer_move(use.x+use.w/2,use.y+use.h/2);a.pointer_button(true);a.pointer_button(false);unchanged();
 a.pointer_move(discard.x+discard.w/2,discard.y+discard.h/2);a.pointer_button(true);a.pointer_button(false);unchanged();
 // A path which clears the generic busy counter still cannot consume the
 // already calculated next turn before the feedback endpoint.
 a.busy=0;CHECK(a.combat_actions_locked());a.input(Key::Confirm);a.ui_action(UiAction::DiscardPotion);unchanged();
 a.animation=a.combat_feedback.end_at;a.busy=0;a.render();CHECK(!a.combat_actions_locked());
 a.potion_menu_item=0;const int strength_before=a.game.strength;
 a.input(Key::Confirm);CHECK(potion_at(a.game,0)==Potion::Empty&&a.panel==0&&a.game.strength>strength_before);

 // A bounded queue may drop intermediate samples, but must retain the
 // complete canonical result until its scheduled convergence boundary.
 enemy_visual_fight(EncounterId::NibbitsWeak);a.begin_combat_feedback();auto&f=a.combat_feedback;
 const uint32_t base=a.animation;auto v=a.combat_health(0);
 for(unsigned i=0;i<CombatFeedback::Capacity+20;++i){v.block=int(i+1);f.append(0,v,f.identity[0],base+i+1);}
 CHECK(f.overflow);f.end_at=base+CombatFeedback::Capacity+30;
 a.finish_combat_feedback();snapshot();a.animation=f.end_at-1;a.sync_combat_feedback();CHECK(a.combat_feedback.active);
 a.animation=f.end_at;a.sync_combat_feedback();CHECK(!a.combat_feedback.active&&a.combat_health(0).block==a.game.block);unchanged();
}
static void deferred_combat_hand(){
 static State direct;static SaveData expected;
 CHECK(sizeof(CombatHand)<=1024&&sizeof(CombatFeedback)<=8192);
 const auto shown_only=[&](const CombatHand::Cards&expected_hand){
  const auto shown=a.combat_hand();CHECK(CombatHand::same(shown,expected_hand));
  for(unsigned i=0;i<shown.count;++i)CHECK(has(HitKind::Hand,shown.ids[i]));
  for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::Hand){
   bool arrived=false;for(unsigned j=0;j<shown.count;++j)arrived|=a.hits[i].value==shown.ids[j];
   CHECK(arrived);if(a.combat_actions_locked())CHECK(!a.hits[i].enabled);
  }
 };
 for(unsigned fast=0;fast<2;++fast)for(unsigned retain=0;retain<2;++retain){
  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.tick_remainder=0;
  a.game.enemy_world.models[0].node=0;
  int retained=-1;
  if(retain){
   retained=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,Purity,1+1024);resolve_pending(a.game);
   CHECK(a.game.cards[retained].kind==Purity&&card_retains(a.game,a.game.cards[retained],retained));
  }
  const auto old=CombatHand::capture(a.game.hand);CHECK(old.count==5+retain);
  a.cursor=old.count-1;a.hand_focus=true;a.render();shown_only(old);
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));CHECK(save(direct,expected));
  a.input(Key::EndTurn);CHECK(a.combat_feedback.active&&a.combat_feedback.finished&&!a.combat_feedback.hand.overflow);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  const auto next=CombatHand::capture(a.game.hand);CHECK(next.count==5+retain);
  CombatHand::Cards kept;if(retain){kept.count=1;kept.ids[0]=uint16_t(retained);}
  for(unsigned i=0;i<old.count;++i)if(old.ids[i]!=retained){
   CHECK(a.game.zones[old.ids[i]]==Zone::Discard);
   for(unsigned j=0;j<next.count;++j)CHECK(next.ids[j]!=old.ids[i]);
  }
  const uint32_t first=a.enemy_presentation[0].queue[0].earliest;
  const uint32_t impact=first+TurnBannerPresentation::wait_ticks(150,75,fast!=0);
  const uint32_t player=a.turn_banner.player_start,done=a.combat_feedback.end_at;
  CHECK(player>impact&&done==player+TurnBannerPresentation::wait_ticks(1250,500,fast!=0));
  snapshot();a.render();shown_only(kept);CHECK(a.hand_can_focus()==bool(retain));
  a.input(Key::Right);CHECK(a.cursor==0);a.input(Key::Detail);
  if(retain){CHECK(a.detail_instance==retained);a.input(Key::Back);}
  else CHECK(a.detail_card<0);
  // No future card can be focused, inspected or played through its old slot.
  a.input(Key::Confirm);a.input(Key::EndTurn);unchanged();a.render();shown_only(kept);
  advance_combat_ms((first-a.animation-1)*33);a.render();shown_only(kept);
  CHECK(a.turn_banner.sample_enemy(a.animation).kind==TurnBannerPresentation::Enemy);
  advance_combat_ms((impact-a.animation)*33);a.render();shown_only(kept);
  if(retain){
   a.input(Key::Right);a.render();CardHoverTarget target;CHECK(card_hover_target(a,target)&&target.instance==retained);
   capture(fast?"hand-retain-enemy-fast":"hand-retain-enemy");
  }else capture(fast?"hand-empty-enemy-fast":"hand-empty-enemy");
  advance_combat_ms((player-a.animation-1)*33);a.render();shown_only(kept);
  CHECK(a.turn_banner.sample_player(a.animation).kind==TurnBannerPresentation::None);
  // DrawInternal awaits Add for each actual draw. The new holder appears
  // at the draw-pile position; later IDs have neither pixels nor hit regions.
  advance_combat_ms(33);a.render();
  for(unsigned draws=1;draws<=5;++draws){
   CombatHand::Cards arriving=next;arriving.count=uint16_t(kept.count+draws);shown_only(arriving);
   const auto pose=a.combat_hand_pose(arriving.count-1,false);CHECK(pose.cx==20&&pose.cy==226);
   for(unsigned j=arriving.count;j<next.count;++j)CHECK(!has(HitKind::Hand,next.ids[j]));
   CHECK(a.combat_feedback.active&&a.combat_actions_locked());
   if(draws<5){const uint32_t next_add=player+TurnBannerPresentation::wait_ticks(draws*250,draws*100,fast!=0);
    advance_combat_ms((next_add-a.animation-1)*33);a.render();shown_only(arriving);
    advance_combat_ms(33);a.render();}
  }
  advance_combat_ms((done-a.animation)*33);a.render();shown_only(next);
  CHECK(!a.combat_feedback.active&&!a.combat_actions_locked());
  CHECK(a.turn_banner.sample_player(a.animation).kind==TurnBannerPresentation::Player);
  if(retain)capture(fast?"hand-player-draw-fast":"hand-player-draw");unchanged();
 }
 // A load restores the saved hand directly, without replaying discarded UI
 // samples. This is the same reset used by the ordinary Continue/Load path.
 enemy_visual_fight(EncounterId::NibbitsWeak);a.input(Key::EndTurn);CHECK(a.combat_feedback.active);
 snapshot();CHECK(load(a.game,before.bytes,before.size));a.reset_presentation();a.render();
 CHECK(!a.combat_feedback.active);shown_only(CombatHand::capture(a.game.hand));unchanged();

 // Real generated-card/discard commands at distinct cue times exercise the
 // bounded queue. An unusually long chain must never reveal its final hand
 // early and still converges at the existing side endpoint.
 enemy_visual_fight(EncounterId::NibbitsWeak);
 const uint32_t start=a.animation;
 {UiCoreObserver observer(a);a.begin_combat_feedback();
  for(unsigned i=0;i<=CombatHand::Capacity;++i){
   a.combat_feedback.effect_at=start+i+1;
   if(i%2)queue_extension(a.game,XOp::SDiscardHand);
   else queue_extension(a.game,XOp::EnemyGenerateCard,Wound,1+1024);
   resolve_pending(a.game);
  }
 }
 CHECK(a.combat_feedback.hand.overflow&&a.game.hand.count==1);
 const auto final_hand=CombatHand::capture(a.game.hand);const uint32_t end=a.combat_feedback.end_at;
 snapshot();advance_combat_ms((end-a.animation-1)*33);a.render();shown_only({});
 advance_combat_ms(33);a.render();shown_only(final_hand);CHECK(!a.combat_feedback.active);unchanged();
 a.fast_mode=false;
}
static void deferred_enemy_statuses(){
 static State direct;static SaveData expected;
 CHECK(sizeof(CombatStatus)<=64*1024);
 for(unsigned fast=0;fast<2;++fast){
  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.game.enemy_world.models[0].node=2;
  CHECK(a.combat_status_amount(1,0)==0);snapshot();CHECK(load(direct,before.bytes,before.size));
  CHECK(end_turn(direct));CHECK(save(direct,expected));a.input(Key::EndTurn);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  CHECK(a.game.enemies[0].strength>0&&a.combat_status_amount(1,0)==0);
  const uint32_t start=a.enemy_presentation[0].queue[0].earliest;
  const uint32_t power=start+TurnBannerPresentation::wait_ticks(600,250,fast!=0),done=a.combat_feedback.end_at;
  bool popup=false;for(unsigned i=0;i<a.combat_status.popup_count;++i){const auto&p=a.combat_status.popups[i];if(p.actor==1&&p.id==0){CHECK(p.at==power&&p.buff);popup=true;}}
  CHECK(popup);
  snapshot();a.animation=power-1;a.render();CHECK(a.combat_status_amount(1,0)==0&&!has(HitKind::BattleStatus,StatusKinds));
  a.animation=power;a.render();CHECK(a.combat_status_amount(1,0)==a.game.enemies[0].strength&&has(HitKind::BattleStatus,StatusKinds));
  a.open_status_icon(StatusKinds);CHECK(a.status_actor==1);int ids[StatusKinds];
  CHECK(a.combat_statuses(1,ids)>0&&ids[a.status_selected]==0);a.render();unchanged();a.input(Key::Statuses);
  if(!fast){a.animation=power+6;a.render();capture("power-applied-buff");unchanged();}
  a.animation=done;a.render();CHECK(!a.combat_feedback.active&&a.combat_status_amount(1,0)==a.game.enemies[0].strength);

  enemy_visual_fight(EncounterId::LouseProgenitorNormal);a.fast_mode=fast!=0;a.game.enemy_world.models[0].node=0;
  const int frail=EnemyStatusStart+int(EnemyPower::Frail);a.input(Key::EndTurn);
  CHECK(status_amount(a.game,0,frail)>0&&a.combat_status_amount(0,frail)==0);
  const uint32_t web=a.enemy_presentation[0].queue[0].earliest+TurnBannerPresentation::wait_ticks(400,200,fast!=0);
  snapshot();a.animation=web-1;a.render();CHECK(a.combat_status_amount(0,frail)==0&&!has(HitKind::BattleStatus,frail));
  a.animation=web;a.render();CHECK(a.combat_status_amount(0,frail)==status_amount(a.game,0,frail)&&has(HitKind::BattleStatus,frail));
  popup=false;for(unsigned i=0;i<a.combat_status.popup_count;++i){const auto&p=a.combat_status.popups[i];if(p.actor==0&&p.id==frail){CHECK(p.at==web&&!p.buff);popup=true;}}
  CHECK(popup);
  CHECK(a.combat_status_source(0,int(EnemyPower::Frail))==EnemyId::LouseProgenitor);unchanged();
  if(!fast){a.animation=web+6;a.render();capture("power-applied-debuff");unchanged();}
 }
 a.fast_mode=false;
 // Auxiliary counts and per-instance details follow the same presentation
 // boundary; changing canonical state must not leak a later tooltip value.
 enemy_visual_fight(EncounterId::NibbitsWeak);const int slow=EnemyStatusStart+int(EnemyPower::Slow);
 auto&m=a.game.enemy_world.models[0];m.power[int(EnemyPower::Slow)]=1;m.power_aux[int(EnemyPower::Slow)]=2;
 a.game.expansion.bomb_turns[0]=2;a.game.expansion.bomb_damage[0]=17;
 a.begin_combat_feedback();const uint32_t cue=a.animation+10;
 a.combat_feedback.effect_at=cue;m.power_aux[int(EnemyPower::Slow)]=5;
 a.game.expansion.bomb_turns[0]=1;a.game.expansion.bomb_damage[0]=23;
 a.observe_combat_effect(a.game,{});a.combat_feedback.end_at=cue+10;a.finish_combat_feedback();
 CHECK(a.combat_status_amount(1,slow,true)==20&&a.combat_status_auxiliary(1,int(EnemyPower::Slow))==2);
 CHECK(a.combat_status_fact(CombatStatus::BombTurns,0)==2&&a.combat_status_fact(CombatStatus::BombDamage,0)==17);
 a.animation=cue;a.sync_combat_feedback();CHECK(a.combat_status_amount(1,slow,true)==50);
 CHECK(a.combat_status_fact(CombatStatus::BombTurns,0)==1&&a.combat_status_fact(CombatStatus::BombDamage,0)==23);
 a.reset_presentation();CHECK(a.combat_status_amount(1,slow,true)==50);
 // Bounded playback retains the final values even if a long chain exceeds
 // the intermediate cue capacity, without widening any game/save fields.
 a.begin_combat_feedback();auto&view=a.combat_status;
 for(unsigned i=0;i<CombatStatus::Capacity+20;++i)view.append(0,{int(i+1),0,0,1},a.animation+i+1);
 CHECK(view.overflow);view.sync(a.animation+CombatStatus::Capacity+30);view.finish();
 CHECK(view.shown[0].amount==int(CombatStatus::Capacity+20));a.reset_presentation();
 // InvokePowerModified reports Counter/AllowNegative decreases through
 // PowerIncreased too, even while Strength remains positive. Ordinary
 // duration losses and removal are not application popups.
 enemy_visual_fight(EncounterId::NibbitsWeak);a.game.strength=3;a.game.enemies[0].weak=3;
 a.begin_combat_feedback();a.game.strength=1;a.game.enemies[0].weak=2;
 a.observe_combat_effect(a.game,{});CHECK(a.combat_status.popup_count==1);
 CHECK(a.combat_status.popups[0].id==0&&a.combat_status.popups[0].buff);
 a.combat_feedback.effect_at+=5;a.game.strength=-1;a.observe_combat_effect(a.game,{});
 CHECK(a.combat_status.popup_count==2&&!a.combat_status.popups[1].buff);
 a.combat_feedback.effect_at+=5;a.game.strength=0;a.observe_combat_effect(a.game,{});
 CHECK(a.combat_status.popup_count==3&&a.combat_status.popups[2].removed);a.reset_presentation();
}
static void ui6_combat_fidelity(){
 enemy_visual_fight(EncounterId::NibbitsWeak);a.begin_combat_feedback();
 a.game.enemies[0].strength=2;a.game.enemies[0].weak=2;a.game.enemies[0].vulnerable=2;a.observe_combat_effect(a.game,{});
 CHECK(a.combat_status.popup_count==3);unsigned lanes=0;for(unsigned i=0;i<3;++i)lanes|=1u<<a.combat_status.popups[i].lane;CHECK(lanes==7);
 a.game.enemies[0].weak=0;a.observe_combat_effect(a.game,{});CHECK(a.combat_status.popup_count==4&&a.combat_status.popups[3].removed);
 CHECK(a.combat_status.popups[3].at>a.animation); // Independent row queue, no overlapped fourth label.
 CHECK(!a.combat_status_counter(1,EnemyStatusStart+int(EnemyPower::Minion)));
 CHECK(!a.combat_status_counter(0,9)&&a.combat_status_counter(0,4));

 // A real summon retains the original actor's named marker and scale.
 enemy_visual_fight(EncounterId::OvicopterNormal);const auto original=enemy_pose(a.game,0);
 CHECK(a.game.enemy_count==1&&a.game.enemy_world.models[0].slot==5);
 {UiCoreObserver observer(a);a.begin_combat_feedback();queue_extension(a.game,XOp::EnemySpawn,int(EnemyId::ToughEgg),8);resolve_pending(a.game);}
 CHECK(a.game.enemy_count==2);const auto after_spawn=enemy_pose(a.game,0),egg=enemy_pose(a.game,1);
 CHECK(original.x==after_spawn.x&&original.feet==after_spawn.feet&&original.scale==after_spawn.scale);
 CHECK(egg.x<original.x&&egg.scale<original.scale);a.animation+=100;a.render();capture("ui6-ovicopter-reserved-slots");

 // The same real core command produces a horizontal generated-card preview,
 // then its original delayed flight; all saved state equals direct execution.
 enemy_visual_fight(EncounterId::NibbitsWeak);static State direct;static SaveData expected;
 const unsigned first=a.game.card_count;const uint32_t at=a.animation;snapshot();CHECK(load(direct,before.bytes,before.size));
 queue_extension(direct,XOp::EnemyGenerateCard,Slimed,2+2048);resolve_pending(direct);CHECK(save(direct,expected));
 {UiCoreObserver observer(a);a.begin_combat_feedback();queue_extension(a.game,XOp::EnemyGenerateCard,Slimed,2+2048);resolve_pending(a.game);}
 CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));snapshot();
 a.render();CHECK(a.combat_cards.find(first)&&a.combat_cards.find(first+1));CHECK(a.combat_cards.find(first)->mode==CombatCardMotion::Preview);
 a.animation=at+8;a.render();CHECK(a.combat_cards.find(first)->scale==640);capture("ui6-enemy-status-card-preview");unchanged();
 a.animation=at+44;a.render();CHECK(a.combat_cards.find(first)->mode==CombatCardMotion::Fly);unchanged();

 for(unsigned fast=0;fast<2;++fast){enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;
  {UiCoreObserver observer(a);damage_unpowered(a.game,0,9999);resolve_pending(a.game);}
  CHECK(a.combat_status.reward_scheduled);const auto&m=a.enemy_presentation[0];
  CHECK(a.combat_status.reward_at==(fast?a.animation+16:m.until+31));
  CHECK(a.combat_status.reward_at<m.hide_at);const uint32_t deadline=a.combat_status.reward_at;
  a.animation+=2;a.sync_enemy_feedback();CHECK(a.combat_status.reward_at==deadline&&a.combat_linger_until==deadline);
 }
 // Retained core enemy models cannot re-open a stale battle in a new room.
 a.game.world.active=1;a.game.world.room_type=uint8_t(WorldRoom::Event);a.game.phase=Phase::Event;
 a.reset_presentation();a.sync_enemy_feedback();CHECK(a.combat_linger_until==a.animation&&a.enemy_presentation[0].identity==0);
 a.fast_mode=false;
}
static void sequential_hit_feedback(){
 static State direct;static SaveData expected;
 // Real card commands exercise repeated single targets, an all-target hit,
 // and random targets. Playback must preserve every canonical RNG/save byte.
 const CardKind kinds[]={TwinStrike,DaggerSpray,SwordBoomerang,RegentCelestialMight,DefectRefract};
 for(unsigned fast=0;fast<2;++fast)for(unsigned test=0;test<5;++test){
  enemy_visual_fight(EncounterId::NibbitsNormal,test==1?Character::Silent:test==3?Character::Regent:test==4?Character::Defect:Character::Ironclad);a.fast_mode=fast!=0;
  a.game.energy=9;if(test==3)a.game.expansion.stars=9;
  for(int slot=0;slot<a.game.enemy_count;++slot){a.game.enemies[slot].hp=a.game.enemies[slot].max_hp=999;a.game.enemies[slot].block=2;}
  const int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,kinds[test],1025);resolve_pending(a.game);
  CHECK(a.game.zones[id]==Zone::Hand);a.reset_presentation();a.render();
  for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);a.cursor=a.game.hand.count-1;a.target=0;
  const int hit=damage(a.game,a.game.cards[id],0,id),hits=test<2||test==4?2:3;
  CHECK(hit>2);snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(play(direct,a.cursor,0));CHECK(save(direct,expected));
  const uint32_t start=a.animation;a.input(Key::Confirm);a.input(Key::Confirm);CHECK(a.combat_feedback.active&&!a.combat_feedback.overflow);
  CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));snapshot();
  unsigned distinct=0;uint32_t times[8]{};
  for(unsigned i=0;i<a.combat_feedback.count;++i){const auto&q=a.combat_feedback.cues[i];if(q.actor<1||q.actor>unsigned(a.game.enemy_count)||q.hp==999)continue;
   bool found=false;for(unsigned j=0;j<distinct;++j)found|=times[j]==q.at;if(!found){CHECK(distinct<8);times[distinct++]=q.at;}}
  CHECK(distinct==unsigned(hits));
  for(unsigned n=0;n<distinct;++n){
   const unsigned gap=test==4?(fast?100u:200u):(fast?175u:350u);
   const uint32_t at=start+((fast?75u:150u)+n*gap+32u)/33u;CHECK(times[n]==at);
   a.animation=at-1;a.render();
   if(!n)CHECK(a.combat_health(1).hp==999&&a.combat_health(1).block==2);
   a.animation=at;a.render();
   if(test!=2){const int expected_hp=999-int(n+1)*hit+2;CHECK(a.combat_health(1).hp==expected_hp&&a.combat_health(1).block==0);
    CHECK(a.last_hit[1]==hit-(n?0:2)&&a.hit_start[1]==int(at));
    unsigned numbers=0;for(unsigned d=0;d<a.combat_status.damage_count;++d)if(a.combat_status.damages[d].actor==1)++numbers;
    CHECK(numbers==n+1); // Every hit remains an independent live VFX.
    if(test==1)CHECK(a.combat_health(2).hp==expected_hp&&a.hit_start[2]==int(at));
    else CHECK(a.combat_health(2).hp==999);
   }
   if(n==0&&test==1)capture(fast?"multi-hit-area-first-fast":"multi-hit-area-first");
   unchanged();
  }
  a.animation=a.combat_feedback.end_at;a.render();CHECK(!a.combat_feedback.active);
  for(int slot=0;slot<a.game.enemy_count;++slot)CHECK(a.combat_health(slot+1).hp==a.game.enemies[slot].hp);unchanged();
 }
 for(unsigned fast=0;fast<2;++fast){
  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.game.energy=3;a.game.enemies[0].hp=1;
  const int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,TwinStrike,1025);resolve_pending(a.game);
  a.reset_presentation();a.render();
  for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);a.cursor=a.game.hand.count-1;const uint32_t start=a.animation;a.input(Key::Confirm);a.input(Key::Confirm);
  CHECK(a.game.enemies[0].hp==0&&a.combat_feedback.active);
  unsigned impacts=0;for(unsigned i=0;i<a.combat_feedback.count;++i){const auto&q=a.combat_feedback.cues[i];if(q.actor==1&&q.hp==0)++impacts;}
  CHECK(impacts==1);CHECK(a.combat_feedback.end_at<start+TurnBannerPresentation::wait_ticks(700,350,fast!=0));
 }
 // Chomper animates each hit; Axebot's OneTwo explicitly animates once.
 // Both use the original 200/100ms post-Damage wait, independent of clips.
 for(unsigned fast=0;fast<2;++fast)for(unsigned once=0;once<2;++once){
  enemy_visual_fight(once?EncounterId::AxebotsNormal:EncounterId::ChompersNormal);a.fast_mode=fast!=0;
  for(int slot=0;slot<a.game.enemy_count;++slot)a.game.enemy_world.models[slot].node=uint8_t(once?1:0);
  a.game.block=2;const auto intent=enemy_intent(a.game,0);CHECK(intent.hits==2&&intent.damage>2);
  const int hp=a.game.hp;snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));CHECK(save(direct,expected));
  a.input(Key::EndTurn);CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));
  const uint32_t start=a.enemy_presentation[0].queue[0].earliest;const unsigned delay=once?350:300,first=fast?delay/2:delay;
  const unsigned gap=(fast?100:200)+(once?0:first);snapshot();
  for(unsigned hit=0;hit<2;++hit){const uint32_t at=start+(first+hit*gap+32)/33;
   a.animation=at-1;a.render();CHECK(a.combat_health(0).hp==hp-int(hit)*intent.damage+(hit?2:0));
   a.animation=at;a.render();CHECK(a.combat_health(0).hp==hp-int(hit+1)*intent.damage+2&&a.combat_health(0).block==0);CHECK(a.hit_start[0]==int(at));unchanged();}
 }
 a.fast_mode=false;
 // HideIntent retains the old label during its delay, fades independently,
 // and is cancelled when the next set of intents is revealed.
 enemy_visual_fight(EncounterId::NibbitsWeak);a.begin_combat_feedback();const uint32_t hide=a.animation;
 a.combat_feedback_move(0,0,hide,hide,hide+40);snapshot();
 a.animation=hide+12;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)==255);
 a.animation=hide+20;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)>0&&a.combat_intent_alpha(0)<255);
 a.animation=hide+28;a.render();CHECK(!a.combat_intent(0).flags&&a.combat_intent_alpha(0)==0);unchanged();a.reset_presentation();
 // Regent's amount is still in the display projection, but never in the
 // power strip/list. A queued gain cannot leak into its independent HUD.
 enemy_visual_fight(EncounterId::NibbitsWeak,Character::Regent);a.game.expansion.stars=0;a.render();
 CHECK(has(HitKind::UiAction,int(UiAction::ShowStars))&&!has(HitKind::BattleStatus,44));
 const uint32_t gain=a.animation+10;
 {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=gain;queue_extension(a.game,XOp::RStars,3);resolve_pending(a.game);}
 CHECK(a.game.expansion.stars==3&&a.combat_status_amount(0,44)==0);snapshot();
 a.animation=gain-1;a.render();CHECK(a.combat_status_amount(0,44)==0&&!has(HitKind::BattleStatus,44));
 a.animation=gain;a.render();CHECK(a.combat_status_amount(0,44)==3&&has(HitKind::UiAction,int(UiAction::ShowStars)));
 int ids[StatusKinds];const int count=a.combat_statuses(0,ids);for(int i=0;i<count;++i)CHECK(ids[i]!=44);
 for(unsigned i=0;i<a.combat_status.popup_count;++i)CHECK(a.combat_status.popups[i].id!=44);
 a.ui_action(UiAction::ShowStars);CHECK(a.status_actor<0);unchanged();capture("regent-star-resource");
 a.reset_presentation();CHECK(a.combat_status.popup_count==0);
}
static void synchronous_damage_hook_feedback(){
 static State direct;static SaveData expected;
 for(unsigned fast=0;fast<2;++fast)for(unsigned flutter=0;flutter<2;++flutter){
  enemy_visual_fight(flutter?EncounterId::ThievingHopperWeak:EncounterId::LagavulinMatriarchBoss);
  a.fast_mode=fast!=0;a.game.energy=3;a.game.enemies[0].block=0;
  if(flutter){queue_extension(a.game,XOp::EnemyApplyPower,1,int(EnemyPower::Flutter)+80+720);resolve_pending(a.game);a.game.enemy_world.models[0].data[8]=1;}
  const int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,TwinStrike,1025);resolve_pending(a.game);
  a.reset_presentation();a.render();
  for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);a.cursor=a.game.hand.count-1;a.target=0;
  const int previous=a.enemy_presentation[0].state,hp=a.game.enemies[0].hp;
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(play(direct,a.cursor,0));CHECK(save(direct,expected));
  const uint32_t impact=a.animation+TurnBannerPresentation::wait_ticks(150,75,fast!=0);
  a.input(Key::Confirm);a.input(Key::Confirm);CHECK(a.combat_feedback.active&&a.game.enemies[0].hp<hp);
  CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));
  // These notifications happen synchronously *inside* damage. They must
  // queue at the already-established hit boundary, not at the input tick.
  CHECK(a.combat_health(1).hp==hp&&a.enemy_presentation[0].state==previous);
  const char*trigger=flutter?"StunTrigger":"Wake";bool hook=false;
  for(unsigned i=0;i<a.enemy_presentation[0].count;++i){const auto&q=a.enemy_presentation[0].queue[i];
   if(!std::strcmp(q.action,trigger)){CHECK(q.earliest==impact);hook=true;}}
  CHECK(hook);snapshot();a.animation=impact-1;a.render();
  CHECK(a.combat_health(1).hp==hp&&a.enemy_presentation[0].state==previous);unchanged();
  a.animation=impact;a.render();CHECK(a.combat_health(1).hp<hp);
  CHECK(a.enemy_presentation[0].state!=previous&&a.enemy_presentation[0].start==impact);
  unchanged();if(!fast)capture(flutter?"flutter-damage-stun":"sleep-damage-wake");
 }
 a.fast_mode=false;
}
static void hidden_choice_input(int token){
 CHECK(!a.combat_choice_ready());a.render();CHECK(has(HitKind::Battlefield,0));
 CHECK(!has(HitKind::WorldChoice,token)&&!has(HitKind::CombatChoice,token));
 CHECK(!has(HitKind::UiAction,int(UiAction::ConfirmChoice))&&!has(HitKind::UiAction,int(UiAction::ConfirmWorldChoice)));
 const int cursor=a.cursor;const auto effects=a.game.effect_count;snapshot();
 for(Key key:{Key::Left,Key::Right,Key::Up,Key::Down,Key::Confirm,Key::EndTurn,Key::Detail})a.input(key);
 a.toggle_choice(token);a.toggle_world_choice(token);a.world_choice_detail(token);a.pointer_scroll(1);
 CHECK(a.cursor==cursor&&!a.choice_selected_count&&a.detail_card<0&&a.detail_item<0);
 // A stale/preselected token must not let a raw confirmation bypass the
 // display gate. Clearing/paging actions share the same barrier as keys.
 a.choice_revision=a.game.revision;a.choice_selected[0]=uint16_t(token);a.choice_selected_count=1;
 a.confirm_choice();a.confirm_world_choice();
 for(UiAction action:{UiAction::ConfirmChoice,UiAction::ConfirmWorldChoice,UiAction::ClearChoice,UiAction::ClearWorldChoice,
     UiAction::PreviousChoicePage,UiAction::NextChoicePage,UiAction::PreviousWorldPage,UiAction::NextWorldPage})a.ui_action(action);
 CHECK(a.cursor==cursor&&a.choice_selected_count==1&&a.game.effect_count==effects);unchanged();a.choice_selected_count=0;
}
static void deferred_combat_choices(){
 static State direct;static SaveData expected;
 for(unsigned fast=0;fast<2;++fast)for(unsigned delayed=0;delayed<2;++delayed){
  enemy_visual_fight(EncounterId::NibbitsWeak);a.fast_mode=fast!=0;a.tick_remainder=0;a.game.enemy_world.models[0].node=0;
  init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicToastyMittens));CHECK(potion_set(a.game,0,Potion::Strength));
  // Obtaining a relic during combat has its own canonical continuation.
  resolve_pending(a.game);CHECK(!a.game.effect_count&&a.game.playing<0&&!a.combat_actions_locked());
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));CHECK(save(direct,expected));
  const uint32_t start=a.animation;a.input(Key::EndTurn);
  CHECK(a.game.phase==Phase::WorldRelicChoice&&a.combat_feedback.active&&!a.combat_feedback.finished);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  uint16_t ids[MaxWorldUiChoices];CHECK(a.world_choice_options(ids,MaxWorldUiChoices)>0);
  // Nibbit's audited 800/500ms lead, 400/250ms intent and 350/175ms
  // action are separately quantized by the existing real-time scheduler.
  // SetupPlayerTurn awaits Draw before AfterPlayerTurnStart; Toasty chooses
  // there, after all five awaited 250/100 ms Add commands have completed.
  const uint32_t player=start+(fast?30u:49u);
  const uint32_t barrier=player+TurnBannerPresentation::wait_ticks(1250,500,fast!=0);
  CHECK(a.combat_feedback.choice_at==barrier);
  const uint32_t original_end=a.combat_feedback.end_at;
  hidden_choice_input(ids[0]);
  a.input(Key::Potions);CHECK(a.hud_focus==2&&!a.panel);a.input(Key::Confirm);CHECK(a.panel==4&&a.combat_actions_locked());snapshot();
  a.input(Key::Confirm);a.ui_action(UiAction::DiscardPotion);unchanged();a.input(Key::Potions);CHECK(!a.panel);
  advance_combat_ms((barrier-a.animation-1)*33);a.render();hidden_choice_input(ids[0]);
  CHECK(a.turn_banner.player_start==player&&a.turn_banner.sample_player(a.animation).kind==TurnBannerPresentation::Player);
  CHECK(a.combat_hand().count==5);
  if(!delayed)capture(fast?"choice-toasty-before-fast":"choice-toasty-before");
  // Pressing the background before a new overlay appears cannot select its
  // card on release. A fresh click after the same boundary can select it.
  a.pointer_move(CardGridScroll::Left+CardGridScroll::CardW/2,80);a.pointer_button(true);advance_combat_ms(33);a.render();
  CHECK(a.animation==barrier&&a.combat_choice_ready()&&has(HitKind::WorldChoice,ids[0]));
  CHECK(!has(HitKind::Battlefield,0));a.pointer_button(false);CHECK(!a.choice_selected_count);
  CHECK(a.turn_banner.player_start==player);snapshot();
  a.ui_action(UiAction::ConfirmWorldChoice);CHECK(a.combat_feedback.choice_at==barrier&&!a.choice_selected_count);unchanged();
  if(delayed){
   advance_combat_ms(3300);a.render();CHECK(a.combat_choice_ready()&&a.combat_feedback.active&&a.combat_actions_locked());
   // A rejected core selection leaves the barrier answerable. Waiting
   // between retries advances the continuation once, not twice.
   a.choice_revision=a.game.revision;a.choice_selected[0]=MaxCards-1;a.choice_selected_count=1;
   a.ui_action(UiAction::ConfirmWorldChoice);CHECK(a.combat_feedback.choice_pending&&a.combat_choice_ready());unchanged();
   CHECK(a.combat_feedback.end_at==original_end+(a.animation-barrier));
   a.ui_action(UiAction::ClearWorldChoice);CHECK(!a.choice_selected_count);advance_combat_ms(99);a.render();
  }
  if(!delayed)capture(fast?"choice-toasty-ready-fast":"choice-toasty-ready");
  a.busy=300;HitRegion option{};for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::WorldChoice&&a.hits[i].value==ids[0]){option=a.hits[i];break;}
  CHECK(option.kind==HitKind::WorldChoice);a.pointer_move(option.x+option.w/2,option.y+option.h/2);a.pointer_button(true);a.pointer_button(false);
  CHECK(a.choice_selected_count==1&&a.choice_selected[0]==ids[0]);
  CHECK(world_relic_select(direct,ids,1));CHECK(save(direct,expected));
  a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&!a.game.effect_count);
  CHECK(a.turn_banner.player_start==player); // A late answer must not restart it.
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  if(a.combat_feedback.active&&int32_t(a.combat_feedback.end_at-a.animation)>0)advance_combat_ms((a.combat_feedback.end_at-a.animation)*33);
  a.render();CHECK(!a.combat_feedback.active&&a.combat_health(0).hp==a.game.hp);
 }
 for(unsigned fast=0;fast<2;++fast){
  // A real enemy choice followed by Toasty's turn-start choice exercises two
  // barriers in one end-turn queue. Knowledge's selection cue is audited;
  // the unreviewed remainder of that move retains its existing fallback.
  enemy_visual_fight(EncounterId::KnowledgeDemonBoss);a.fast_mode=fast!=0;a.tick_remainder=0;
  init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicToastyMittens));resolve_pending(a.game);
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(end_turn(direct));const uint32_t start=a.animation;a.input(Key::EndTurn);
  CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::EnemyKnowledge);
  const uint32_t first=a.combat_feedback.choice_at,old_end=a.combat_feedback.end_at;
  // Independently rounded 800/500 + 400/250 + 1000/250ms waits.
  CHECK(first==start+(fast?32u:69u)&&old_end>first);hidden_choice_input(MaxCards+1);
  advance_combat_ms((first-a.animation-1)*33);a.render();CHECK(!a.combat_choice_ready());
  advance_combat_ms(33);a.render();CHECK(a.combat_choice_ready()&&has(HitKind::CombatChoice,MaxCards+1));
  capture(fast?"choice-knowledge-ready-fast":"choice-knowledge-ready");
  advance_combat_ms((old_end-a.animation+60)*33);a.render();CHECK(a.combat_choice_ready()&&has(HitKind::CombatChoice,MaxCards+1));
  snapshot();a.choice_revision=a.game.revision;a.choice_selected[0]=MaxCards+2;a.choice_selected_count=1;
  a.confirm_choice();CHECK(a.combat_feedback.choice_pending&&a.combat_choice_ready());unchanged();
  a.ui_action(UiAction::ClearChoice);advance_combat_ms(99);
  const uint32_t answer=a.animation;a.busy=300;a.toggle_choice(MaxCards+1);CHECK(a.choice_selected_count==1);
  uint16_t knowledge=MaxCards+1;CHECK(choose_extended(direct,&knowledge,1));a.ui_action(UiAction::ConfirmChoice);
  CHECK(a.game.phase==Phase::WorldRelicChoice&&a.combat_feedback.active&&a.combat_feedback.choice_pending);
  const uint32_t second=a.combat_feedback.choice_at;
  CHECK(second==old_end+(answer-first)+TurnBannerPresentation::wait_ticks(1250,500,fast!=0)&&second>answer);
  CHECK(save(direct,expected));CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  uint16_t ids[MaxWorldUiChoices];CHECK(a.world_choice_options(ids,MaxWorldUiChoices)>0);hidden_choice_input(ids[0]);
  advance_combat_ms((second-a.animation-1)*33);a.render();CHECK(!a.combat_choice_ready());
  advance_combat_ms(33);a.render();CHECK(a.combat_choice_ready()&&has(HitKind::WorldChoice,ids[0]));
  capture(fast?"choice-consecutive-fast":"choice-consecutive");advance_combat_ms(3300);a.busy=300;
  CHECK(world_relic_select(direct,ids,1));CHECK(save(direct,expected));
  a.input(Key::Confirm);CHECK(a.choice_selected_count==1);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat);
  CHECK(save(a.game,after));CHECK(expected.size==after.size&&!std::memcmp(expected.bytes,after.bytes,after.size));
  if(a.combat_feedback.active&&int32_t(a.combat_feedback.end_at-a.animation)>0)advance_combat_ms((a.combat_feedback.end_at-a.animation)*33);
  a.render();CHECK(!a.combat_feedback.active&&!a.combat_actions_locked());
 }
 // Loading a canonical suspended choice does not resurrect a display-only
 // delay. The same selected instance and saved continuation remain usable.
 enemy_visual_fight(EncounterId::NibbitsWeak);init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicToastyMittens));resolve_pending(a.game);
 a.input(Key::EndTurn);CHECK(!a.combat_choice_ready());snapshot();CHECK(load(a.game,before.bytes,before.size));a.reset_presentation();a.render();
 CHECK(a.combat_choice_ready()&&!a.combat_feedback.active);unchanged();a.input(Key::Confirm);CHECK(a.choice_selected_count==1);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&!a.game.effect_count);
 a.fast_mode=false;
}
static void original_card_movement(){
 static State direct;static SaveData expected;
 const auto setup=[](Character character=Character::Ironclad){
  enemy_visual_fight(EncounterId::NibbitsWeak,character);
  // The enemy-only fixture does not enter a player turn. Manual-play cases
  // start with the same available energy as an ordinary first combat turn.
  a.game.energy=int16_t(max_energy(a.game));
 };
 const auto add=[&](CardKind kind,unsigned upgraded=0){
  const int id=a.game.card_count;queue_extension(a.game,XOp::EnemyGenerateCard,kind,1025);resolve_pending(a.game);
  CHECK(a.game.card_count==id+1&&a.game.zones[id]==Zone::Hand);a.game.cards[id].upgraded=uint8_t(upgraded);
  a.reset_presentation();a.render();for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);return id;
 };
 const auto equivalent=[&](){CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));};
 for(unsigned fast=0;fast<2;++fast){
  setup();a.fast_mode=fast!=0;a.tick_remainder=0;
  const int played=add(Defend);a.cursor=a.game.hand.count-1;a.hand_focus=true;a.render();
  const auto start_pose=a.combat_hand_pose(unsigned(a.cursor),true);const auto old=a.combat_hand();const auto old_piles=a.combat_pile_counts();
  CHECK(!a.combat_hand_settled(unsigned(a.cursor),true));
  int focus_regions=0;for(int h=0;h<a.hit_count;++h)if(a.hits[h].kind==HitKind::Hand&&a.hits[h].value==played)++focus_regions;
  CHECK(focus_regions==1);
  const auto*upper=a.hit_at(start_pose.cx,start_pose.cy-HandFaceH*start_pose.scale/(2*1024)+8);CHECK(upper&&upper->kind==HitKind::Hand&&upper->value==played);
  // The lower visible face must keep focus while the authored X lerps.
  a.pointer_move(start_pose.cx,224);a.render();CHECK(a.hand_focus&&a.cursor==old.count-1);
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(play(direct,a.cursor,a.target));CHECK(save(direct,expected));
  a.input(Key::Confirm);CHECK(a.keyboard_card==played);unchanged();
  if(fast){advance_combat_ms(198);a.render();}
  const auto play_pose=a.combat_hand_pose(unsigned(a.cursor),false);
  if(fast)CHECK(play_pose.cy>start_pose.cy&&play_pose.cy<=210&&play_pose.scale<start_pose.scale);
  else CHECK(play_pose.cx==start_pose.cx&&play_pose.cy==start_pose.cy);
  const uint32_t start=a.animation;a.input(Key::Confirm);equivalent();
  const uint32_t end=start+TurnBannerPresentation::wait_ticks(300,150,fast!=0);
  CHECK(a.combat_feedback.finished&&a.combat_feedback.end_at==end&&a.game.zones[played]==Zone::Discard);
  a.render();CHECK(a.combat_hand().count==old.count-1&&!has(HitKind::Hand,played));
  const auto*visual=a.combat_cards.find(unsigned(played));CHECK(visual&&visual->mode==CombatCardMotion::Play&&visual->origin.cx==play_pose.cx&&visual->origin.cy==play_pose.cy);
  CHECK(a.combat_pile_counts().discard==old_piles.discard);snapshot();a.input(Key::Confirm);a.input(Key::EndTurn);unchanged();
  advance_combat_ms(66);a.render();visual=a.combat_cards.find(unsigned(played));CHECK(visual&&visual->y<play_pose.cy*1024);
  capture(fast?"card-play-fast":"card-play");
  advance_combat_ms((end-a.animation-1)*33);a.render();CHECK(a.combat_cards.find(unsigned(played))->mode==CombatCardMotion::Play&&a.combat_actions_locked());
  advance_combat_ms(33);a.render();CHECK(!a.combat_feedback.active&&!a.combat_actions_locked());
  CHECK(a.combat_cards.find(unsigned(played))->mode==CombatCardMotion::Fly&&a.combat_pile_counts().discard==old_piles.discard);
  advance_combat_ms(66);a.render();capture(fast?"card-discard-flight-fast":"card-discard-flight");
  bool arrived=false;
  for(unsigned tick=0;tick<96;++tick){
   const auto*v=a.combat_cards.find(unsigned(played));if(!v||v->mode==CombatCardMotion::Tail){arrived=true;break;}
   CHECK(v->mode==CombatCardMotion::Fly&&!a.combat_actions_locked());advance_combat_ms(33);a.render();
  }
  CHECK(arrived&&a.combat_pile_counts().discard==old_piles.discard+1);equivalent();

  // A real three-card draw supplies three distinct Add boundaries. Its
  // canonical result already exists, but later cards cannot be selected or
  // inspected through a future hit region while the first one is arriving.
  setup();a.fast_mode=fast!=0;a.tick_remainder=0;
  const int trance=add(BattleTrance);a.cursor=a.game.hand.count-1;const auto hand_before=a.combat_hand();
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(play(direct,a.cursor,a.target));CHECK(save(direct,expected));
  const uint32_t draw_start=a.animation;a.input(Key::Confirm);a.input(Key::Confirm);a.render();equivalent();
  const auto final_hand=CombatHand::capture(a.game.hand);CHECK(final_hand.count==hand_before.count+2&&a.game.zones[trance]==Zone::Discard);
  const unsigned base=hand_before.count-1;const uint32_t draw_end=draw_start+TurnBannerPresentation::wait_ticks(750,300,fast!=0);
  CHECK(a.combat_feedback.end_at==draw_end);
  for(unsigned n=1;n<=3;++n){
   const auto shown=a.combat_hand();CHECK(shown.count==base+n);
   for(unsigned i=0;i<shown.count;++i)CHECK(shown.ids[i]==final_hand.ids[i]&&has(HitKind::Hand,shown.ids[i]));
   for(unsigned i=shown.count;i<final_hand.count;++i)CHECK(!has(HitKind::Hand,final_hand.ids[i]));
   const auto pose=a.combat_hand_pose(shown.count-1,false);CHECK(pose.cx==20&&pose.cy==226&&!a.combat_hand_settled(shown.count-1,false));
   if(n==2)capture(fast?"card-draw-second-fast":"card-draw-second");
   if(n<3){const uint32_t next=draw_start+TurnBannerPresentation::wait_ticks(n*250,n*100,fast!=0);
    advance_combat_ms((next-a.animation-1)*33);a.render();CHECK(a.combat_hand().count==base+n);
    advance_combat_ms(33);a.render();}
  }
  advance_combat_ms((draw_end-a.animation)*33);a.render();CHECK(!a.combat_actions_locked()&&CombatHand::same(a.combat_hand(),final_hand));equivalent();

  // A full ten-card hand does not create a phantom Add, animation or wait.
  setup();a.fast_mode=fast!=0;a.tick_remainder=0;
  while(a.game.hand.count<MaxHand)add(Wound);
  snapshot();CHECK(load(direct,before.bytes,before.size));queue_extension(direct,XOp::Draw,3);resolve_pending(direct);CHECK(save(direct,expected));
  const uint32_t full=a.animation;{UiCoreObserver observer(a);a.begin_combat_feedback();queue_extension(a.game,XOp::Draw,3);resolve_pending(a.game);}
  a.render();CHECK(a.animation==full&&!a.combat_feedback.active&&a.combat_hand().count==MaxHand&&!a.combat_cards.count);equivalent();

  // Power waits for its play-pile tween before applying its effect. The
  // physical card leaves the hand at time zero rather than duplicating there.
  setup();a.fast_mode=fast!=0;a.tick_remainder=0;
  const int inflame=add(Inflame);a.cursor=a.game.hand.count-1;
  snapshot();CHECK(load(direct,before.bytes,before.size));CHECK(play(direct,a.cursor,a.target));CHECK(save(direct,expected));
  const uint32_t power_start=a.animation;a.input(Key::Confirm);a.input(Key::Confirm);a.render();
  const uint32_t apply=power_start+TurnBannerPresentation::wait_ticks(250,250,fast!=0),power_end=power_start+TurnBannerPresentation::wait_ticks(550,400,fast!=0);
  CHECK(a.combat_feedback.end_at==power_end&&a.combat_status_amount(0,0)==0&&!has(HitKind::Hand,inflame));
  CHECK(a.combat_cards.find(unsigned(inflame))&&a.combat_cards.find(unsigned(inflame))->mode==CombatCardMotion::Play);
  advance_combat_ms((apply-a.animation-1)*33);a.render();CHECK(a.combat_status_amount(0,0)==0);
  advance_combat_ms(33);a.render();CHECK(a.combat_status_amount(0,0)==a.game.strength);
  advance_combat_ms((power_end-a.animation)*33);a.render();CHECK(!a.combat_cards.find(unsigned(inflame))&&!a.combat_actions_locked());equivalent();

  // Prepared's real card-choice continuation must wait for both awaited
  // draws. Its future canonical selection cannot appear at the first draw.
  setup(Character::Silent);a.fast_mode=fast!=0;a.tick_remainder=0;
  add(Prepared,1);a.cursor=a.game.hand.count-1;const uint32_t selection_start=a.animation;a.input(Key::Confirm);a.input(Key::Confirm);a.render();
  CHECK(a.game.phase==Phase::DiscardChoice&&a.combat_feedback.choice_pending);
  const uint32_t ready=selection_start+TurnBannerPresentation::wait_ticks(500,200,fast!=0);CHECK(a.combat_feedback.choice_at==ready);
  hidden_choice_input(a.game.hand.ids[0]);advance_combat_ms((ready-a.animation-1)*33);a.render();CHECK(!a.combat_choice_ready());
  for(int i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind!=HitKind::CombatChoice);
  advance_combat_ms(33);a.render();CHECK(a.combat_choice_ready()&&has(HitKind::CombatChoice,a.game.hand.ids[0]));
 }
 a.fast_mode=false;
}
static void ui6_room_removal(){
 new_run(a.game,0x6016,false);a.game.map[0].room=Room::Shop;CHECK(choose_node(a.game,0));a.game.gold=999;
 CHECK(event_card_enchant(a.game,0,EventEnchantment::Sharp,7));
 a.title=a.paused=a.help=a.library=a.character_select=false;a.reset_presentation();a.shop_open=true;a.pointer.visible=false;
 CHECK(buy(a.game,ShopRemove)&&a.game.phase==Phase::Remove);
 const Card removed=a.game.deck[0],next=a.game.deck[1];const EventCardFacts facts=a.game.event_cards.deck[0];
 char description[1024]{};a.card_description(removed,-1,description,sizeof description,-1,0,&facts);
 const int expected_cost=a.card_offer_cost(removed,&facts),deck_count=a.game.deck_count,price=remove_price(a.game),gold=a.game.gold;
 snapshot();static State oracle;static SaveData expected;CHECK(load(oracle,before.bytes,before.size));CHECK(remove_card(oracle,0));CHECK(save(oracle,expected));
 a.deck_candidate=0;a.deck_choice_revision=a.game.revision;a.deck_choice_phase=a.game.phase;a.confirm_deck_choice();
 CHECK(a.game.phase==Phase::Shop&&a.shop_open&&a.game.deck_count==deck_count-1&&a.game.gold==gold-price);
 CHECK(card_token(a.game.deck[0])==card_token(next));CHECK(save(a.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,expected.size));
 const auto&visual=a.reward_presentation.removed;CHECK(visual.active&&card_token(visual.card)==card_token(removed));
 CHECK(visual.facts.enchantment==EventEnchantment::Sharp&&visual.facts.enchantment_amount==7&&visual.energy==expected_cost);
 CHECK(!std::strcmp(visual.description,description));CHECK(!visual.scale(visual.start)&&visual.scale(visual.start+125)==3584&&visual.scale(visual.start+250)==4096);
 snapshot();a.advance(125);a.render();capture("card-remove-growing");unchanged();
 a.advance(125);a.render();capture("card-remove-preview");unchanged();
 a.advance(250);a.advance(250);a.advance(100);a.render();capture("card-remove-slash");unchanged();
 CHECK(visual.alive(a.map_clock_ms));a.advance(250);a.advance(250);a.advance(250);a.advance(100);a.render();
 CHECK(!visual.alive(a.map_clock_ms));unchanged();
 a.reset_presentation();CHECK(!a.reward_presentation.removed.active);a.render();unchanged();
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
  if(a.content.version()>=FeedbackContentVersion){
   CHECK(a.content.clip(AnimMerchantIronclad+character).count>1);
   const auto*outside=a.hit_at(166,190);CHECK(!outside||outside->kind!=HitKind::UiAction||outside->value!=int(UiAction::OpenShop));
  }
  capture((std::string("merchant-")+names[character]).c_str());a.advance(250);a.render();unchanged();
  a.ui_action(UiAction::OpenShop);CHECK(a.shop_open);a.render();
  CHECK(a.room_presentation.merchant.y(a.map_clock_ms)==-222);
  a.advance(250);a.advance(250);a.advance(200);a.render();unchanged();
  // Source card-slot centers in the 1440x1080 viewport, projected once at
  // 2/9. The rug intentionally extends beyond the viewport; its items do not.
  constexpr int centers[]={44,102,160,218,277,58,119};
  for(int i=0;i<ShopCardCount;++i){const HitRegion*card=nullptr;for(int j=0;j<a.hit_count;++j)if(a.hits[j].kind==HitKind::Choice&&a.hits[j].value==i)card=&a.hits[j];
   CHECK(card&&card->x+card->w/2==centers[i]&&card->w==54&&card->h==85);
   CHECK(card->x>=0&&card->x+card->w<=320&&card->y>=22&&card->y+card->h<=235);}
  capture((std::string("merchant-inventory-")+names[character]).c_str());a.input(Key::Back);CHECK(!a.shop_open);unchanged();
 }
}
int main(int argc,char**argv){std::vector<uint8_t>b;
 if(argc>1){actual_pack=true;FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));b.resize(size);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));}else b=synthetic_profile();
 if(argc>2)screenshot_directory=argv[2];CHECK(a.init(b.data(),b.size(),{}));options_and_readers();crystal_and_large_rewards();potion_lock_and_capacity();offer_facts();special_reward_snapshots();original_enemy_presentation();original_aspect_layout();original_osty_and_orbs();spatial_card_targeting();original_enemy_transformations();original_enemy_command_waits();audited_leaf_and_cultist_timing();original_combat_clock();deferred_enemy_values();deferred_combat_hand();deferred_enemy_statuses();ui6_combat_fidelity();deferred_combat_choices();original_card_movement();sequential_hit_feedback();synchronous_damage_hook_feedback();ui6_room_removal();feedback_hud_and_merchant();std::printf("world UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");}
