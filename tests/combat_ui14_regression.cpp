// UI14 uses real commands and the ordinary save codec. Optional private art
// and exported controlled saves are for visual/device testing, not natural runs.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>
using namespace spire;
static App a;
static State before,direct;
static SaveData first,second;
static unsigned checks;
static const char*scenario="initial";
static std::string output;
static bool original_art=false;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat UI14 [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static void equal_save(){CHECK(save(direct,first)&&save(a.game,second));CHECK(first.size==second.size&&!std::memcmp(first.bytes,second.bytes,first.size));}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof before));}
static void tick(unsigned ms){for(unsigned n=0;n<ms;n+=33){a.advance(mn(33u,ms-n));a.sync_combat_feedback();a.sync_enemy_feedback();}}
static void write(const std::string&name,const void*bytes,size_t count){
 if(output.empty())return;FILE*f=std::fopen((output+"/"+name).c_str(),"wb");CHECK(f);CHECK(std::fwrite(bytes,1,count,f)==count);CHECK(!std::fclose(f));
}
static void capture(const char*name){
 if(output.empty()||!original_art)return;a.render();
 std::string image="P6\n320 240\n255\n";image.reserve(image.size()+320*240*3);
 for(const auto p:a.pixels){image.push_back(char((p>>11)*255/31));image.push_back(char(((p>>5)&63)*255/63));image.push_back(char((p&31)*255/31));}
 write(std::string(name)+".ppm",image.data(),image.size());CHECK(a.content.healthy());
}
static void export_save(const char*name){
 CHECK(validate(a.game)&&save(a.game,first));CHECK(load(direct,first.bytes,first.size)&&validate(direct)&&save(direct,second));
 CHECK(first.size==second.size&&!std::memcmp(first.bytes,second.bytes,first.size));write(std::string(name)+".sav.tns",first.bytes,first.size);
}
static void fresh(Character character=Character::Ironclad,EncounterId encounter=EncounterId::NibbitsNormal){
 new_run(a.game,0x41cef1ea,false,character);CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 // Supply only earlier routing outcomes. The room and encounter still enter
 // through their production setup and validate as an ordinary world save.
 for(unsigned step=0;step<32&&!world_room_is_combat(a.game);++step){
  if(world_can_travel(a.game)){uint8_t ids[reference::Columns];const auto count=world_available_points(a.game,ids,reference::Columns);CHECK(count);CHECK(world_choose_point(a.game,ids[0]));}
  else{a.game.original_events={};CHECK(world_finish_room(a.game));}
 }
 CHECK(a.game.phase==Phase::Combat&&a.game.world.active);CHECK(encounter_setup(a.game,encounter));
 a.game.energy=20;a.game.expansion.stars=20;a.game.hp=a.game.max_hp=200;
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.reset_presentation();a.pointer.visible=false;a.map_clock_ms=1000;a.animation=30;
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(validate(a.game));
}
static void hand(std::initializer_list<CardKind>cards){
 auto&s=a.game;s.card_count=s.hand.count=s.draw.count=s.discard.count=s.exhaust.count=0;s.expansion.last_drawn=-1;
 for(auto&origin:s.event_cards.deck_origin)origin=0;
 for(const auto kind:cards){const unsigned id=s.card_count++;s.cards[id]={kind,0};s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=uint16_t(id);
  s.enemy_world.affliction[id]=0;s.enemy_world.affliction_amount[id]=0;s.event_cards.cards[id]={};if(kind==SovereignBlade)s.expansion.blade_damage[id]=10;}
 ++s.revision;a.reset_presentation();a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(validate(s));
}
static void scale_equations(){
 scenario="source750ms sine shrink keeps endpoints and interruption continuous";CreatureScaleMotion m;m.observe(512,1000);
 CHECK(m.sample(1000)==1024&&m.sample(1375)==768&&m.sample(1750)==512&&m.sample(9999)==512);
 int previous=1024;for(unsigned now=1000;now<=1750;++now){const int current=m.sample(now);CHECK(current<=previous&&current>=512);previous=current;}
 const int midway=m.sample(1400);m.observe(1024,1400);CHECK(m.sample(1400)==midway&&m.sample(2150)==1024);
 for(unsigned n=0;n<=9999;++n)CHECK(std::abs(CreatureScaleMotion::enlarged(n)-int(std::lround((1+.1*std::log(double(n)+1))*1024)))<=1);
 m.observe(1536,2200,0);CHECK(m.sample(2200)==1536);m.observe(1126,3000,100);CHECK(m.sample(2999)==1536&&m.sample(3100)==1126);
}
static void clock_icon(){
 scenario="default combat clock is an icon; elapsed time belongs only to its tooltip";
 fresh();CHECK(UiPreferences{}.show_run_timer&&a.show_run_timer&&a.run_timer_visible());a.begin_progress();
 before=a.game;Canvas c{a};a.progress.current.seconds=11;char label[24]{};a.run_timer_label(label,sizeof label);CHECK(!std::strcmp(label,"00:11"));
 std::memset(a.pixels,0,sizeof a.pixels);a.hit_count=0;hud(c);bool found=false;
 for(unsigned i=0;i<unsigned(a.hit_count);++i)if(a.hits[i].kind==HitKind::HudInfo&&a.hits[i].value==HudTimer){const auto&h=a.hits[i];CHECK(h.enabled&&h.x==hud_room_geometry(a.game).timer&&h.w==8);found=true;}
 CHECK(found);const auto initial=std::vector<uint16_t>(a.pixels,a.pixels+ScreenW*ScreenH);
 a.progress.current.seconds=3723;a.run_timer_label(label,sizeof label);CHECK(!std::strcmp(label,"1:02:03"));
 std::memset(a.pixels,0,sizeof a.pixels);a.hit_count=0;hud(c);CHECK(!std::memcmp(initial.data(),a.pixels,sizeof a.pixels));
 if(original_art){
  const int slots=int(potion_slot_count(a.game));
  for(int focus:{slots+2,slots+3,slots+6}){
   a.hud_focus=focus;a.progress.current.seconds=11;std::memset(a.pixels,0,sizeof a.pixels);hud_hover_tip(c);
   const auto first_tip=std::vector<uint16_t>(a.pixels,a.pixels+ScreenW*ScreenH);
   a.progress.current.seconds=3723;std::memset(a.pixels,0,sizeof a.pixels);hud_hover_tip(c);
   CHECK((std::memcmp(first_tip.data(),a.pixels,sizeof a.pixels)!=0)==(focus==slots+6));
  }
 }
 a.show_run_timer=false;CHECK(!a.run_timer_visible());a.hit_count=0;hud(c);
 for(unsigned i=0;i<unsigned(a.hit_count);++i)CHECK(a.hits[i].kind!=HitKind::HudInfo||a.hits[i].value!=HudTimer);
 a.show_run_timer=true;a.hud_focus=-1;unchanged();
}
static void actual_default_growth(){
 scenario="Big Mushroom's real acquisition grows the player immediately";fresh();const auto normal=a.combat_player_pose();direct=a.game;
 CHECK(gain_world_relic(direct,WorldRelicBigMushroom));{UiCoreObserver observer(a);CHECK(gain_world_relic(a.game,WorldRelicBigMushroom));}equal_save();
 const auto large=a.combat_player_pose();CHECK(large.scale==normal.scale*1536/1024&&large.hp==normal.hp&&large.feet==normal.feet);capture("big-mushroom");
 scenario="Gardener Enlarge records its true completed count and queues growth";fresh(Character::Ironclad,EncounterId::PhantasmalGardenersElite);
 const auto garden=a.combat_enemy_pose(3);direct=a.game;CHECK(end_turn(direct));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(a.game));}equal_save();
 CHECK(a.game.enemy_world.models[3].data[0]==1&&a.game.enemy_world.models[3].data[1]==1);
 for(unsigned i=0;i<300;++i){tick(33);a.combat_enemy_pose(3);}
 const auto grown=a.combat_enemy_pose(3);const auto base=enemy_pose(a.game,3,a.creature_layout);const int initial=creature_instance_scale(a.game,3);
 CHECK(a.enemy_presentation[3].facts.default_scale==1095);CHECK(grown.scale==mx(1,base.scale*1095/initial)&&grown.hp==garden.hp&&grown.feet==garden.feet);capture("gardener-enlarge");equal_save();
 scenario="Test Subject's actual respawn keeps its one-tenth growth and100ms tween";fresh(Character::Ironclad,EncounterId::TestSubjectBoss);
 const auto small=a.combat_enemy_pose(0);direct=a.game;damage_unpowered(direct,0,2000);resolve_pending(direct);
 {UiCoreObserver observer(a);a.begin_combat_feedback();damage_unpowered(a.game,0,2000);resolve_pending(a.game);}equal_save();
 CHECK(a.game.enemy_world.models[0].flags&EnemyDown);CHECK(end_turn(direct));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(a.game));}equal_save();
 CHECK(a.game.enemy_world.models[0].data[0]==1);for(unsigned i=0;i<300;++i){tick(33);a.combat_enemy_pose(0);}
 const auto reborn=a.combat_enemy_pose(0);CHECK(a.enemy_presentation[0].facts.default_scale==1126&&reborn.scale==small.scale*1126/1024&&reborn.hp==small.hp);capture("test-subject-respawn");equal_save();
}
static void actual_shrink(){
 scenario="Beetle Juice really changes the rendered enemy scale";fresh(Character::Ironclad,EncounterId::ShrinkerBeetleWeak);
 hand({Defend,Defend,Strike});potion_set(a.game,0,Potion::BeetleJuice);export_save("ui14-shrink");
 const auto initial_save=std::vector<uint8_t>(first.bytes,first.bytes+first.size);
 const auto original_enemy=a.combat_enemy_pose(0),original_player=a.combat_player_pose();capture("shrink-before");
 direct=a.game;CHECK(use_potion(direct,0,0));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(use_potion(a.game,0,0));}equal_save();
 int shrunk=0;CHECK(enemy_visual_fact(a.game,0,"Shrink",shrunk)&&shrunk);
 bool intermediate=false;for(unsigned i=0;i<90;++i){tick(33);const auto p=a.combat_enemy_pose(0);intermediate|=p.scale<original_enemy.scale&&p.scale>original_enemy.scale/2;
  CHECK(p.feet==original_enemy.feet&&p.hp==original_enemy.hp&&p.intent==original_enemy.intent&&p.width==original_enemy.width);}
 CHECK(intermediate&&a.combat_enemy_pose(0).scale==mx(1,original_enemy.scale/2));capture("shrink-enemy-half");equal_save();
 scenario="Shrinker Beetle's real opening move shrinks the player visuals";
 direct=a.game;CHECK(end_turn(direct));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(a.game));}equal_save();
 intermediate=false;for(unsigned i=0;i<150;++i){tick(33);const auto p=a.combat_player_pose();intermediate|=p.scale<original_player.scale&&p.scale>original_player.scale/2;
  CHECK(p.feet==original_player.feet&&p.hp==original_player.hp&&p.width==original_player.width);}
 CHECK(enemy_power_value(a.game,-1,EnemyPower::Shrink)!=0);CHECK(intermediate&&a.combat_player_pose().scale==mx(1,original_player.scale/2));capture("shrink-player-half");equal_save();
 scenario="another run with the identical seed and combat does not inherit a shrink tween";
 CHECK(load(a.game,initial_save.data(),initial_save.size())&&validate(a.game));++a.run_number;
 a.reset_presentation();a.sync_combat_feedback();a.sync_enemy_feedback();before=a.game;
 CHECK(a.combat_player_pose().scale==original_player.scale&&a.combat_enemy_pose(0).scale==original_enemy.scale);unchanged();
}
static void actual_forge(){
 scenario="real Forge queues the changed swords and leaves canonical results exact";fresh(Character::Regent);hand({RegentTheSmith,SovereignBlade,RegentDefend});
 direct=a.game;CHECK(play(direct,0,0));{UiCoreObserver observer(a);a.begin_combat_feedback(0);CHECK(play(a.game,0,0));}equal_save();
 bool forged=false;for(unsigned i=0;i<60;++i){tick(33);const auto*blade=a.combat_cards.blades.find(1);forged|=blade&&blade->forging&&blade->damage==40;
  if(i==6)capture("forge-smith");}
 CHECK(forged&&a.combat_cards.blades.find(1)&&a.combat_cards.blades.find(1)->damage==40);CHECK(a.game.expansion.blade_damage[1]==40);equal_save();
 scenario="render clocks move a source orbit without mutating the run";before=a.game;
 const auto p=a.combat_cards.blades.orbit(0,a.animation*33u),q=a.combat_cards.blades.orbit(0,a.animation*33u+1000);CHECK(p.x!=q.x||p.y!=q.y);
 for(unsigned i=0;i<20;++i){tick(33);if(original_art)a.render();}unchanged();
}
static void actual_enchant(){
 scenario="a real pending enchantment selection survives save/load and commits once";fresh(Character::Regent);hand({RegentTheSmith,SovereignBlade,RegentDefend});
 CHECK(gain_world_relic(a.game,WorldRelicGnarledHammer));CHECK(a.game.phase==Phase::WorldRelicChoice);
 export_save("ui14-regent-enchant-forge");uint16_t eligible[MaxDeck];const unsigned count=world_relic_eligible(a.game,eligible,MaxDeck);CHECK(count);
 // Ordinary resume clears presentation state while preserving the pending
 // combat choice. Preparation must finish before that choice is usable.
 a.reset_presentation();before=a.game;a.sync_combat_preparation();
 if(original_art)CHECK(a.combat_preparation.initialized);
 a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();
 CHECK(!a.combat_preparation.active);unchanged();
 const auto preparation_updates=a.combat_preparation.updates;
 const uint16_t chosen=eligible[0];direct=a.game;CHECK(world_relic_select(direct,&chosen,1));
 {UiCoreObserver observer(a);CHECK(world_relic_select(a.game,&chosen,1));}equal_save();
 a.sync_combat_preparation();CHECK(!a.combat_preparation.active&&a.combat_preparation.updates==preparation_updates);
 if(original_art)CHECK(a.combat_preparation.initialized);
 CHECK(a.reward_presentation.enchants.count==1);const auto&view=a.reward_presentation.enchants.cards[0];
 CHECK(view.deck==chosen&&view.model.card.kind==a.game.deck[chosen].kind&&view.model.facts.enchantment==EventEnchantment::Sharp&&view.model.facts.enchantment_amount==3);
 const auto start=view.start;before=a.game;Canvas c{a};
 CHECK(card_enchant_progress(0)==0&&card_enchant_progress(250)==512&&card_enchant_progress(500)==2048&&card_enchant_progress(750)==3584&&card_enchant_progress(1000)==4096);
 for(unsigned age:{0u,250u,500u,1000u,1999u,2000u,2100u,2600u}){a.map_clock_ms=start+age;enchanted_card_animation(c);unchanged();
  if(age==500)capture("enchant-reveal");if(age==1000)capture("enchant-settled");}
 CHECK(a.reward_presentation.enchants.cards[0].launched);const auto queued=a.reward_presentation.serial;enchanted_card_animation(c);CHECK(a.reward_presentation.serial==queued);unchanged();
 {UiCoreObserver observer(a);CHECK(!event_card_enchant(a.game,chosen,EventEnchantment::Sharp,3));}CHECK(a.reward_presentation.enchants.count==1);unchanged();
 a.reset_presentation();CHECK(!a.reward_presentation.enchants.count);
}
static void borders_and_cache(){
 scenario="Ancient, infection and every source affliction use distinct live layers";fresh();hand({EventCardNeowsFury,EventCardInfection,Strike,Rage});
 a.game.enemy_world.affliction[2]=unsigned(EnemyAffliction::Bound);a.game.enemy_world.affliction_amount[2]=1;++a.game.revision;
 export_save("ui14-card-borders");CHECK(card_face_overlay(a,{EventCardNeowsFury,0},0)==0&&card_face_overlay(a,{EventCardInfection,0},1)==8&&card_face_overlay(a,{Strike,0},2)==1);
 for(unsigned kind=1;kind<=7;++kind){a.game.enemy_world.affliction[1]=uint8_t(kind);CHECK(card_face_overlay(a,{EventCardInfection,0},1)==kind);}
 a.game.enemy_world.affliction[1]=0;before=a.game;Canvas c{a};capture("borders");
 if(!original_art)return;
 CHECK(a.card_face_frame({EventCardNeowsFury,0})==a.content.ui_image(I14_AncientBorder));
 const auto portrait=a.content.image(a.card_face_image({EventCardNeowsFury,0}));CHECK(portrait.w==96&&portrait.h==136);
 for(unsigned i=I14_AncientBorder;i<CombatUi14AuxiliaryCount;++i){const auto p=a.content.image(a.content.ui_image(i));CHECK(p.pixels&&p.w&&p.h);}
 for(unsigned frame=0;frame<30;++frame){a.map_clock_ms=frame*2000u/30u+1;CHECK(card_overlay_image(a,8)==a.content.ui_image(I14_Infection+frame));}
 a.phobia_mode=true;a.map_clock_ms=1500;CHECK(card_overlay_image(a,8)==a.content.ui_image(I14_Infection));a.phobia_mode=false;
 scenario="generated previews retain their face across frames while the affliction clock advances";
 CombatCardMotion::Sprite visual;visual.card={EventCardInfection,0};visual.energy=-2;visual.stars=0;invalidate_hand_tiles();
 const auto build=hand_tile(c,1,&visual).built;for(unsigned i=0;i<30;++i){a.map_clock_ms+=33;begin_card_face_frame();CHECK(hand_tile(c,1,&visual).built==build);}unchanged();
 scenario="hovered long card descriptions scroll by one LCD pixel";
 a.hand_focus=true;a.cursor=3;a.keyboard_card=-1;a.map_clock_ms=10000;invalidate_hand_tiles();
 const auto&initial=selected_hand_tile(c,3);CHECK(initial.description_truncated&&selected_hand_reader.maximum>0);
 const auto started=selected_hand_reader.started;const auto first_pixels=std::vector<uint16_t>(initial.pixels,initial.pixels+HandTileW*HandTileH);
 a.map_clock_ms=started+1525;const auto&next=selected_hand_tile(c,3);CHECK(selected_hand_reader.skip==1&&std::memcmp(first_pixels.data(),next.pixels,sizeof next.pixels));unchanged();
 scenario="cached small cards keep their overflow text live";
 invalidate_card_face_cache();a.overflow_label_clocks.reset();a.map_clock_ms=20000;std::memset(a.pixels,0,sizeof a.pixels);begin_card_face_frame();draw_card_face(c,{Rage,0},120,60,65,91);
 const auto original=std::vector<uint16_t>(a.pixels,a.pixels+ScreenW*ScreenH);bool scrolling=false;for(const auto&e:small_card_faces)scrolling|=e.valid&&(e.overflow&2);CHECK(scrolling);
 a.map_clock_ms+=1800;std::memset(a.pixels,0,sizeof a.pixels);begin_card_face_frame();draw_card_face(c,{Rage,0},120,60,65,91);
 CHECK(std::memcmp(original.data(),a.pixels,sizeof a.pixels));unchanged();CHECK(a.content.healthy());
}
static std::vector<uint8_t>read(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>v(size_t(n),0);CHECK(std::fread(v.data(),1,v.size(),f)==v.size());CHECK(!std::fclose(f));return v;}
int main(int argc,char**argv){
 CHECK(argc==1||argc==2||argc==3);std::vector<uint8_t>common,language;SplitResourceSource split;
 if(argc>=2){const std::string root=argv[1];common=read(root+"/data/nSpire2.dat.tns");language=read(root+"/data/nSpire2.eng.dat.tns");
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));original_art=true;}
 if(argc==3){output=argv[2];std::filesystem::create_directories(output);}
 scale_equations();clock_icon();actual_shrink();actual_default_growth();actual_forge();actual_enchant();borders_and_cache();
 const char note[]="Controlled UI14 fixtures; earlier room outcomes, cards and resources are supplied. These saves validate and round-trip through the production NSR codec. They are not natural runs.\nui14-card-borders: Neow's Fury, Infection and Bound Strike in hand.\nui14-shrink: Shrinker Beetle before its opening move, Beetle Juice in potion slot1; drink on the enemy, then end turn to shrink the player.\nui14-regent-enchant-forge: choose an Attack for Gnarled Hammer's Sharp enchantment, confirm, then play The Smith in the resumed Regent combat.\n";
 write("README.txt",note,sizeof note-1);std::printf("combat UI14: %u checks passed%s\n",checks,original_art?"; original art decoded and rendered":"");
}
