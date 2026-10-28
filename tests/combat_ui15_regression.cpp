// Real commands, retained presentation boundaries, and production save bytes.
// Optional private packs exercise the renderer; no game art is in this fixture.
#include "../src/app.cpp"
#include "vantom_vfx_ui15.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
using namespace spire;
static App a;
static State direct,before;
static SaveData saved,again;
static unsigned checks=0;
static const char*scenario="initial";
static std::string output;
static bool actual_art=false;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat UI15 [%s]:%d %s fast=%u animation=%u enemy_clock=%u effect=%u\n",scenario,__LINE__,#x,unsigned(a.fast_mode),a.animation,a.enemy_visual_clock,a.combat_feedback.effect_at);std::exit(1);}}while(0)
static unsigned ticks(unsigned ms){return (ms+32u)/33u;}
static void same(){CHECK(save(direct,saved)&&save(a.game,again));CHECK(saved.size==again.size&&!std::memcmp(saved.bytes,again.bytes,saved.size));}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof before));}
static std::vector<uint8_t> read(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0&&!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>b(size_t(n),0);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;}
static void capture(const char*name){
 if(!actual_art||output.empty())return;a.render();CHECK(a.content.healthy());
 FILE*f=std::fopen((output+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(const auto pixel:a.pixels){const uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));
}
static void setup(){
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 // Rebase the isolated fixture before binding presentation clocks. Leaving
 // a prior scenario's future enemy clock across a backwards time jump makes
 // the next damage hook inherit an unrelated await.
 a.map_clock_ms=1000;a.animation=30;a.reset_presentation();a.pointer.visible=false;
}
static void event_room(){
 new_run(a.game,0x41cef1ea,false);CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 // The room fixture isolates the following event's real begin/choose APIs.
 // world_begin_run already entered Neow; discard only that fixture event
 // before beginning Darv/Tablet, as the ordinary event UI suite does.
 a.game.original_events=OriginalEventState{};setup();
 CHECK(a.game.world.room_type==unsigned(WorldRoom::Event)&&validate(a.game));
}
static void fight(EncounterId encounter){
 new_run(a.game,0x41cef1ea,false);a.game.phase=Phase::Combat;a.game.current_node=1;a.game.floor=a.game.turn=1;
 enter_combat(a.game);CHECK(a.game.phase==Phase::Combat&&encounter_setup(a.game,encounter));
 a.game.hp=a.game.max_hp=999;a.game.block=0;setup();
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(validate(a.game));
}
static int slot(EnemyId model){for(int i=0;i<a.game.enemy_count;++i)if(a.game.enemy_world.models[i].kind==model)return i;CHECK(false);return -1;}
static void perform(State&s,int actor,unsigned move){
 s.enemy_world.actor=uint8_t(actor);s.enemy_world.models[actor].node=uint8_t(move);record(s,LogKind::Enemy,actor,int(move));
 queue_extension(s,XOp::EnemyMove,actor,int(move));queue_extension(s,XOp::EnemyMoveFinish,actor,int(move));resolve_pending(s);CHECK(s.phase!=Phase::Error);
}
static void finish_choices(State&s){
 for(unsigned tries=0;tries<4&&choice_phase(s.phase);++tries){
  CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::EnemyKnowledge);
  const uint16_t token=MaxCards+1;CHECK(choose_extended(s,&token,1));
 }
 CHECK(!choice_phase(s.phase)&&s.phase!=Phase::Error);
}
static void make_fixture(State&s,EncounterId encounter,unsigned seed,unsigned ascension){
 new_run(s,0x41cef1ea+seed,false);s.phase=Phase::Combat;s.current_node=1;s.floor=s.turn=1;
 enter_combat(s);CHECK(s.phase==Phase::Combat&&encounter_setup(s,encounter,ascension));
 s.hp=s.max_hp=999;s.block=0;
}
static void all_original_moves(){
 // Create every actor through actual encounters, summons and replacements.
 // Forcing a legal move here isolates its command body; the ordinary AI/full
 // turn driver is separately covered by enemies_regression's174 encounters.
 std::vector<State> fixtures(EnemyKinds);int actors[EnemyKinds];
 for(auto&actor:actors)actor=-1;unsigned executions=0;
 for(unsigned ascension:{0u,9u}){
  for(auto&actor:actors)actor=-1;
  const auto remember=[&](const State&s){
   for(unsigned t=0;t<s.enemy_count;++t){const unsigned model=unsigned(s.enemy_world.models[t].kind);
    if(!model||model>=EnemyKinds||actors[model]>=0||s.enemies[t].hp<=0)continue;
    fixtures[model]=s;actors[model]=int(t);
   }
  };
  for(unsigned seed=0;seed<8;++seed)for(unsigned id=1;id<EncounterKinds;++id){
   const auto encounter=EncounterId(id);scenario=encounter_definition(encounter)->key;
   make_fixture(direct,encounter,seed,ascension);remember(direct);
   if(encounter==EncounterId::FabricatorNormal||encounter==EncounterId::FogmogNormal||
      encounter==EncounterId::OvicopterNormal||encounter==EncounterId::TheObscuraNormal){
    perform(direct,0,0);remember(direct);
   }else if(encounter==EncounterId::LivingFogNormal){perform(direct,0,1);remember(direct);}
   else if(encounter==EncounterId::GremlinMercNormal){
    damage_unpowered(direct,0,9999);resolve_pending(direct);CHECK(direct.phase==Phase::Combat);remember(direct);
   }
  }
  CHECK(actors[unsigned(EnemyId::LegacyNibbit)]<0);
  for(unsigned model=1;model<EnemyKinds;++model){
   scenario=enemy_definition(EnemyId(model))->model;CHECK(actors[model]>=0);
   const auto*visual=world_enemy_visual(model);CHECK(visual);
   for(unsigned node=0;node<32;++node){const auto*move=world_enemy_move(model,int(node));if(!move)continue;
    const auto*program=enemy_command_program::find(model,int(node));CHECK(program);
    for(unsigned fast=0;fast<2;++fast){
     char name[200];std::snprintf(name,sizeof name,"%s.%s asc%u fast%u",visual->name,move->key,ascension,fast);scenario=name;
     a.game=fixtures[model];const int actor=actors[model];const auto kind=EnemyId(model);
     // Downed/third-form actions need the real preceding transformation.
     if(kind==EnemyId::TestSubject&&(node==0||node>=4)){
      damage_unpowered(a.game,actor,9999);resolve_pending(a.game);
      if(node>=4){perform(a.game,actor,0);damage_unpowered(a.game,actor,9999);resolve_pending(a.game);perform(a.game,actor,0);}
     }else if(kind>=EnemyId::DecimillipedeSegmentBack&&kind<=EnemyId::DecimillipedeSegmentMiddle&&node>=3){
      damage_unpowered(a.game,actor,9999);resolve_pending(a.game);
     }else if(kind==EnemyId::WaterfallGiant&&node>=6){
      queue_extension(a.game,XOp::EnemyApplyPower,12,int(EnemyPower::SteamEruption)+80*(actor+1)+720*(actor+1));resolve_pending(a.game);
      damage_unpowered(a.game,actor,9999);resolve_pending(a.game);if(node==7)perform(a.game,actor,6);
     }else if(kind==EnemyId::ToughEgg&&node==1)perform(a.game,actor,0);
     CHECK(a.game.phase==Phase::Combat&&validate(a.game));setup();a.fast_mode=fast!=0;a.sync_enemy_feedback();
     direct=a.game;perform(direct,actor,node);finish_choices(direct);
     {UiCoreObserver observer(a);a.begin_combat_feedback();perform(a.game,actor,node);finish_choices(a.game);}
     same();CHECK(validate(a.game));
     CHECK(!a.combat_feedback.commands.active&&!a.combat_feedback.overflow&&!a.combat_feedback.damage_overflow);
     CHECK(!a.combat_cards.overflow&&!a.combat_cards.count_overflow);
     CHECK(a.combat_feedback.commands.cursor==a.combat_feedback.commands.end);
     before=a.game;for(unsigned i=0;i<3;++i){a.advance(33);a.sync_combat_feedback();a.sync_enemy_feedback();}unchanged();
     ++executions;
    }
   }
  }
 }
 //324 named source moves plus4 authored Task.CompletedTask placeholders;
 // no duplicate old Nibbit adapter is counted or supported by this program.
 CHECK(executions==328u*2u*2u);a.fast_mode=false;
 std::printf("combat UI15: %u real move executions at ascension0/9 and normal/fast\n",executions);
}
static uint32_t observed_move(int actor,unsigned move){
 a.game.enemy_world.models[actor].node=uint8_t(move);direct=a.game;perform(direct,actor,move);
 {UiCoreObserver observer(a);a.begin_combat_feedback();perform(a.game,actor,move);}same();
 CHECK(a.combat_feedback.active&&!a.combat_feedback.commands.active);return a.combat_feedback.commands.base;
}
static void command_boundaries(){
 // Hand-authored milliseconds from the pinned command bodies, independently
 // of the extracted numeric table. Both modes execute the ordinary core.
 struct Sample{EncounterId encounter;EnemyId model;unsigned move,normal,fast;};
 const Sample samples[]={
  // An original, unrevived Axebot applies0 Strength. PowerCmd returns
  // before its visible-power wait; the revived branch is covered below.
  {EncounterId::AxebotsNormal,EnemyId::Axebot,0,800,500},
  {EncounterId::BygoneEffigyElite,EnemyId::BygoneEffigy,0,500,500},
  {EncounterId::BygoneEffigyElite,EnemyId::BygoneEffigy,1,750,600},
  {EncounterId::CultistsNormal,EnemyId::DampCultist,0,1200,575},
  {EncounterId::FlyconidNormal,EnemyId::Flyconid,0,750,350},
  {EncounterId::SlimesNormal,EnemyId::LeafSlimeS,1,1500,1250},
  {EncounterId::TerrorEelElite,EnemyId::TerrorEel,3,1250,900},
  {EncounterId::TwoTailedRatsNormal,EnemyId::TwoTailedRat,2,550,250},
 };
 for(unsigned fast=0;fast<2;++fast)for(const auto&s:samples){
  scenario=enemy_definition(s.model)->model;fight(s.encounter);a.fast_mode=fast!=0;
  const int actor=slot(s.model);const uint32_t start=observed_move(actor,s.move);
  CHECK(a.combat_feedback.effect_at==start+ticks(fast?s.fast:s.normal));CHECK(a.enemy_visual_clock==a.combat_feedback.effect_at);
  before=a.game;for(unsigned i=0;i<100;++i){a.advance(33);a.sync_combat_feedback();a.sync_enemy_feedback();}unchanged();
 }
 a.fast_mode=false;
}
static void axebot_revived_bootup(){
 for(unsigned fast=0;fast<2;++fast){
  scenario="revived Axebot waits for its actual positive Strength application";
  fight(EncounterId::AxebotsNormal);const int defeated=slot(EnemyId::Axebot);
  damage_unpowered(a.game,defeated,9999);resolve_pending(a.game);
  int replacement=-1;
  for(int i=0;i<a.game.enemy_count;++i)if(a.game.enemy_world.models[i].kind==EnemyId::Axebot&&
      a.game.enemies[i].hp>0&&a.game.enemy_world.models[i].data[0]==1)replacement=i;
  CHECK(replacement>=0&&validate(a.game));a.reset_presentation();a.sync_enemy_feedback();a.fast_mode=fast!=0;
  const uint32_t start=observed_move(replacement,0);
  CHECK(a.game.enemies[replacement].strength==3);
  CHECK(a.combat_feedback.effect_at==start+ticks(fast?600u:1050u));
 }
 a.fast_mode=false;
}
static void conditional_backup_wait(){
 for(unsigned full=0;full<2;++full)for(unsigned fast=0;fast<2;++fast){
  scenario=full?"CallForBackup full slots has no500ms summon wait":"CallForBackup available slot retains500ms summon wait";
  fight(EncounterId::TwoTailedRatsNormal);const int actor=slot(EnemyId::TwoTailedRat);
  if(full){
   for(int name=0;name<2;++name)queue_extension(a.game,XOp::EnemySpawn,int(EnemyId::TwoTailedRat),name+8*(actor+1));
   resolve_pending(a.game);CHECK(a.game.enemy_count==5&&validate(a.game));a.reset_presentation();a.sync_enemy_feedback();
  }
  const unsigned old=a.game.enemy_count;a.fast_mode=fast!=0;const uint32_t start=observed_move(actor,3);
  CHECK(a.game.enemy_count==old+unsigned(!full));
  CHECK(a.combat_feedback.effect_at==start+ticks((fast?150u:300u)+(full?0u:500u)));
 }
 a.fast_mode=false;
}
static void first_speech_after_stun(){
 for(unsigned fast=0;fast<2;++fast){
  scenario="stunned first Gimme retains its actual first-speech wait";fight(EncounterId::GremlinMercNormal);
  const int actor=slot(EnemyId::GremlinMerc);enemy_stun(a.game,actor);CHECK(a.game.enemy_world.models[actor].node==31);
  CHECK(end_turn(a.game));CHECK(a.game.enemy_world.models[actor].node==0&&a.game.enemy_world.models[actor].seen_moves[0]>=2);
  CHECK(!a.game.enemy_world.models[actor].data[0]);a.reset_presentation();a.sync_enemy_feedback();a.fast_mode=fast!=0;
  const uint32_t first=observed_move(actor,0);CHECK(a.game.enemy_world.models[actor].data[0]==1);
  CHECK(a.combat_feedback.effect_at==first+ticks(fast?525u:1050u));
  scenario="later Gimme has no repeated first-speech wait";a.reset_presentation();a.sync_enemy_feedback();
  const uint32_t again=observed_move(actor,0);CHECK(a.combat_feedback.effect_at==again+ticks(fast?275u:550u));
 }
 a.fast_mode=false;
}
static void stunned_hook_boundaries(){
 for(unsigned fast=0;fast<2;++fast)for(unsigned plow=0;plow<2;++plow){
  scenario=plow?"real Plow interruption retains SetStunned and StunnedMove awaits":
   "real Burrowed block break retains GetStunned and StillDizzyMove awaits";
  const EnemyId model=plow?EnemyId::CeremonialBeast:EnemyId::Tunneler;
  const EnemyPower power=plow?EnemyPower::Plow:EnemyPower::Burrowed;
  fight(plow?EncounterId::CeremonialBeastBoss:EncounterId::TunnelerWeak);const int actor=slot(model);
  // Reach the real power through Stamp/Burrow, then interrupt through an
  // ordinary damage command. No synthetic stunned node or fake hook call.
  perform(a.game,actor,plow?0u:1u);CHECK(enemy_power_value(a.game,actor,power)>0);
  const int hp=a.game.enemies[actor].hp,block=a.game.enemies[actor].block;
  const int damage=plow?hp-enemy_power_value(a.game,actor,power):block+1;
  CHECK(damage>0&&hp>1);a.reset_presentation();a.sync_enemy_feedback();a.fast_mode=fast!=0;
  const uint32_t impact=a.animation+10;
  direct=a.game;damage_unpowered(direct,actor,damage);resolve_pending(direct);
  {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=impact;
   damage_unpowered(a.game,actor,damage);resolve_pending(a.game);}
  same();CHECK(a.game.enemies[actor].hp>0&&a.game.enemy_world.models[actor].node==31);
  CHECK(!enemy_power_value(a.game,actor,power));
  const unsigned normal=plow?600u:250u,quick=plow?250u:125u;
  const uint32_t complete=impact+ticks(fast?quick:normal)+ticks(fast?200u:400u);
  CHECK(a.combat_feedback.effect_at==complete&&a.combat_feedback.end_at==complete);
  unsigned cues=0;for(unsigned i=0;i<a.enemy_presentation[actor].count;++i){const auto&q=a.enemy_presentation[actor].queue[i];
   if(!std::strcmp(q.action,"Stun")){CHECK(q.earliest==impact);++cues;}}
  CHECK(cues==1);before=a.game;a.animation=impact-1;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.combat_health(actor+1).hp==hp&&a.combat_health(actor+1).block==block);unchanged();
  a.animation=impact;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.combat_health(actor+1).hp<hp&&a.enemy_presentation[actor].start==impact);unchanged();
  a.animation=complete;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(!a.combat_feedback.active);unchanged();
  // Execute the actual suspended continuation through EnemyMove31. Its
  // source Unstun/WakeUp await is separate from the preceding hurt hook.
  a.reset_presentation();a.sync_enemy_feedback();const uint32_t resume=observed_move(actor,31);
  CHECK(a.combat_feedback.effect_at==resume+ticks(fast?quick:normal));
  const char*trigger=plow?"Unstun":"WakeUp";cues=0;
  for(unsigned i=0;i<a.enemy_presentation[actor].count;++i){const auto&q=a.enemy_presentation[actor].queue[i];
   if(!std::strcmp(q.action,trigger)){CHECK(q.earliest==resume);++cues;}}
  CHECK(cues==1);before=a.game;a.animation=resume-1;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.enemy_presentation[actor].start!=resume);unchanged();
  a.animation=resume;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.enemy_presentation[actor].start==resume);unchanged();
 }
 a.fast_mode=false;
}
static void test_subject_respawn(){
 for(unsigned fast=0;fast<2;++fast){
  scenario="real Test Subject respawn keeps growth at800ms and healing at1950ms";fight(EncounterId::TestSubjectBoss);a.fast_mode=fast!=0;
  damage_unpowered(a.game,0,9999);resolve_pending(a.game);CHECK(a.game.enemies[0].hp==0&&(a.game.enemy_world.models[0].flags&EnemyDown));
  a.reset_presentation();a.sync_enemy_feedback();const uint32_t start=observed_move(0,0);
  CHECK(a.game.enemy_world.models[0].data[0]==1&&a.game.enemies[0].hp>0);
  bool scale=false,heal=false;
  for(unsigned i=0;i<a.enemy_presentation[0].count;++i){const auto&q=a.enemy_presentation[0].queue[i];if(!std::strcmp(q.action,"DefaultScaleChanged")){CHECK(q.earliest==start+ticks(800));scale=true;}}
  for(unsigned i=0;i<a.combat_feedback.count;++i){const auto&q=a.combat_feedback.cues[i];if(q.actor==1&&q.hp>0){CHECK(q.at==start+ticks(1950));heal=true;}}
  CHECK(scale&&heal&&a.combat_feedback.effect_at==start+ticks(fast?2150:2450));same();
 }
 a.fast_mode=false;
}
static void ravenous_hook_boundaries(){
 for(unsigned fast=0;fast<2;++fast){
  scenario="real CorpseSlug death awaits each Ravenous devour before its Strength";
  fight(EncounterId::CorpseSlugsNormal);a.fast_mode=fast!=0;CHECK(a.game.enemy_count==3);
  const int victim=slot(EnemyId::CorpseSlug);const uint32_t impact=a.animation+10;
  direct=a.game;damage_unpowered(direct,victim,9999);resolve_pending(direct);
  {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=impact;
   damage_unpowered(a.game,victim,9999);resolve_pending(a.game);}
  same();CHECK(a.game.enemies[victim].hp==0&&a.game.phase==Phase::Combat);
  const unsigned devour=ticks(fast?250u:500u),power=ticks(fast?100u:250u);
  const uint32_t complete=impact+2u*(devour+power);CHECK(a.combat_feedback.effect_at==complete);
  unsigned ordered=0;int survivor=-1;
  for(int actor=0;actor<a.game.enemy_count;++actor)if(actor!=victim){
   CHECK(a.game.enemy_world.models[actor].node==31&&a.game.enemies[actor].strength==enemy_power_value(a.game,actor,EnemyPower::Ravenous));
   unsigned count=0;for(unsigned i=0;i<a.enemy_presentation[actor].count;++i){const auto&q=a.enemy_presentation[actor].queue[i];
    if(std::strcmp(q.action,"DevourStartTrigger"))continue;
    CHECK(q.earliest==impact||q.earliest==impact+devour+power);
    const unsigned index=q.earliest==impact?0u:1u;CHECK(!(ordered&(1u<<index)));ordered|=1u<<index;++count;
   }
   CHECK(count==1);survivor=actor;
   unsigned flashes=0;for(unsigned i=0;i<a.combat_status.flash_count;++i){const auto&flash=a.combat_status.flashes[i];
    const char*key=ui_status_key(flash.id);if(flash.actor!=actor+1||!key||std::strcmp(key,"RAVENOUS_POWER"))continue;
    CHECK(flash.at==impact||flash.at==impact+devour+power);++flashes;
   }CHECK(flashes==1);
  }
  CHECK(ordered==3&&survivor>=0);before=a.game;a.animation=impact-1;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.combat_health(victim+1).hp>0);unchanged();
  a.animation=impact;a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(!a.combat_health(victim+1).hp);unchanged();
  a.animation=complete;a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(!a.combat_feedback.active);unchanged();
  scenario="real Ravenous suspended move awaits its500/250ms DevourEnd";
  a.reset_presentation();a.sync_enemy_feedback();const uint32_t resume=observed_move(survivor,31);
  CHECK(a.combat_feedback.effect_at==resume+devour);unsigned count=0;
  for(unsigned i=0;i<a.enemy_presentation[survivor].count;++i){const auto&q=a.enemy_presentation[survivor].queue[i];
   if(!std::strcmp(q.action,"DevourEndkTrigger")){CHECK(q.earliest==resume);++count;}}
  CHECK(count==1);before=a.game;a.animation=resume-1;a.sync_combat_feedback();a.sync_enemy_feedback();
  CHECK(a.enemy_presentation[survivor].start!=resume);unchanged();
  a.animation=resume;a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(a.enemy_presentation[survivor].start==resume);unchanged();
 }
 a.fast_mode=false;
}
static void crab_rage_flash(){
 scenario="real partner death flashes Crab Rage before Strength and block";
 fight(EncounterId::KaiserCrabBoss);const int victim=slot(EnemyId::Crusher),survivor=slot(EnemyId::Rocket);
 const uint32_t impact=a.animation+10;const int strength=a.game.enemies[survivor].strength;
 direct=a.game;damage_unpowered(direct,victim,9999);resolve_pending(direct);
 {UiCoreObserver observer(a);a.begin_combat_feedback();a.combat_feedback.effect_at=impact;
  damage_unpowered(a.game,victim,9999);resolve_pending(a.game);}
 same();CHECK(a.game.enemies[victim].hp==0&&a.game.enemies[survivor].hp>0&&a.game.enemies[survivor].strength==strength+6&&a.game.enemies[survivor].block==99);
 unsigned flashes=0;for(unsigned i=0;i<a.combat_status.flash_count;++i){const auto&flash=a.combat_status.flashes[i];
  const char*key=ui_status_key(flash.id);if(flash.actor!=survivor+1||!key||std::strcmp(key,"CRAB_RAGE_POWER"))continue;
  CHECK(flash.at==impact);++flashes;
 }CHECK(flashes==1&&!enemy_power_value(a.game,survivor,EnemyPower::CrabRage));
}
static void generated_entry(){
 scenario="real enemy-created cards grow from zero before their preview flight";fight(EncounterId::SlimesNormal);
 const unsigned old=a.game.card_count;const uint32_t start=observed_move(slot(EnemyId::LeafSlimeS),1);CHECK(a.game.card_count>old);
 const uint32_t appear=start+ticks(500);
 if(actual_art){
  scenario="queued generated cards prepare without advancing their appearance or changing pixels";
  invalidate_hand_tiles();Canvas c{a};before=a.game;
  const auto count=a.combat_cards.count,head=a.combat_cards.head;const auto clock=a.animation;
  std::memset(a.pixels,0,sizeof a.pixels);bool prepared=false;
  for(unsigned i=0;i<CombatCardMotion::Capacity;++i){const auto built=hand_build_clock;
   const bool warmed=prepare_generated_card_face(c);CHECK(hand_build_clock-built<=1);prepared|=warmed;if(!warmed)break;}
  CHECK(prepared&&a.animation==clock&&a.combat_cards.count==count&&a.combat_cards.head==head&&!a.combat_cards.find(old));
  for(const auto pixel:a.pixels)CHECK(pixel==0);unchanged();
  a.animation=appear;a.sync_combat_feedback();const auto*v=a.combat_cards.find(old);CHECK(v&&v->scale==0);
  const auto built=hand_build_clock;hand_tile(c,int(old),v);CHECK(hand_build_clock==built);unchanged();
 }
 a.animation=appear;a.sync_combat_feedback();
 const auto*first=a.combat_cards.find(old);CHECK(first&&first->mode==CombatCardMotion::Preview&&first->scale==0);
 before=a.game;a.animation=appear+1;a.sync_combat_feedback();first=a.combat_cards.find(old);
 CHECK(first&&first->scale>0&&first->scale<HandFocusScale);unchanged();
 a.animation=appear+8;a.sync_combat_feedback();first=a.combat_cards.find(old);CHECK(first&&first->scale==HandFocusScale);
 capture("generated-status-entry");unchanged();
}
static void generated_card_creator(){
 scenario="enemy status cards do not trigger the player's creation listeners";fight(EncounterId::SlimesNormal);
 const int rocket=a.game.card_count;
 queue_extension(a.game,XOp::EnemyGenerateCard,DefectRocketPunch,1025);
 queue_extension(a.game,XOp::EnemyGenerateCard,DefectTurbo,1025);resolve_pending(a.game);
 const int turbo=rocket+1;CHECK(a.game.cards[rocket].kind==DefectRocketPunch&&a.game.cards[turbo].kind==DefectTurbo);
 CHECK(gain_world_relic(a.game,WorldRelicRegalite));resolve_pending(a.game);
 queue_extension(a.game,XOp::RPower,int(RPower::Arsenal),1);
 queue_extension(a.game,XOp::RPower,int(RPower::PillarOfCreation),3);
 queue_extension(a.game,XOp::DPower,int(DPower::Smokestack),5);
 queue_extension(a.game,XOp::DPower,int(DPower::TrashToTreasure),1);resolve_pending(a.game);
 CHECK(validate(a.game));const unsigned created=a.game.regent.cards_created,orbs=a.game.expansion.orb_count;
 const int strength=a.game.strength,block=a.game.block,discount=a.game.defect.cost_delta[rocket];
 const auto flashed=a.game.world_relics.flash_revision;int hp[MaxEnemies]{};for(int i=0;i<a.game.enemy_count;++i)hp[i]=a.game.enemies[i].hp;
 const unsigned count=a.game.card_count;observed_move(slot(EnemyId::LeafSlimeS),1);
 CHECK(a.game.card_count==count+1&&a.game.regent.cards_created==created&&a.game.expansion.orb_count==orbs);
 CHECK(a.game.strength==strength&&a.game.block==block&&a.game.defect.cost_delta[rocket]==discount);
 CHECK(a.game.world_relics.flash_revision==flashed);for(int i=0;i<a.game.enemy_count;++i)CHECK(a.game.enemies[i].hp==hp[i]);
 scenario="actual player Turbo still triggers every eligible creation listener";
 a.reset_presentation();a.sync_enemy_feedback();int hand=-1;
 for(unsigned i=0;i<a.game.hand.count;++i)if(a.game.hand.ids[i]==turbo)hand=int(i);CHECK(hand>=0);
 direct=a.game;CHECK(play(direct,hand,0));
 {UiCoreObserver observer(a);a.begin_combat_feedback(turbo);CHECK(play(a.game,hand,0));}same();CHECK(validate(a.game));
 CHECK(a.game.card_count==count+2&&a.game.regent.cards_created==created+1&&a.game.expansion.orb_count==orbs+1);
 CHECK(a.game.strength==strength+1&&a.game.block==block+7&&a.game.defect.cost_delta[rocket]==discount-1);
 CHECK(a.game.world_relics.flash_revision>flashed);for(int i=0;i<a.game.enemy_count;++i)CHECK(a.game.enemies[i].hp==hp[i]-5);
}
static void ancient_heal(){
 scenario="actual Ancient entry retains fullscreen healing and canonical RNG";event_room();a.game.hp=9;
 direct=a.game;CHECK(original_event_begin(direct,OriginalEventId::Darv,42,1));
 {UiCoreObserver observer(a);CHECK(original_event_begin(a.game,OriginalEventId::Darv,42,1));}same();
 CHECK(a.game.hp==a.game.max_hp&&a.reward_presentation.heal.active);
 CHECK(a.reward_presentation.heal.start==a.map_clock_ms+200u&&!a.reward_presentation.heal.alive(a.map_clock_ms+199u));
 const uint32_t start=a.reward_presentation.heal.start;CHECK(a.reward_presentation.heal.ratio>0);
 before=a.game;Canvas c{a};
 for(unsigned age:{0u,33u,100u,500u,1100u,1700u,2099u,2100u}){
  a.map_clock_ms=start+age;std::memset(a.pixels,0,sizeof a.pixels);fullscreen_heal_screen(c);unchanged();
  if(actual_art&&age==500){bool painted=false;for(const auto pixel:a.pixels)painted|=pixel!=0;CHECK(painted);capture("ancient-heal");}
 }
 CHECK(!a.reward_presentation.heal.alive(start+2100));
 scenario="a full-health Ancient has no positive HealedAmount and no invented fullscreen effect";
 event_room();direct=a.game;CHECK(a.game.hp==a.game.max_hp);
 const unsigned previous_logs=a.game.log_count;CHECK(previous_logs<23);
 CHECK(original_event_begin(direct,OriginalEventId::Darv,42,1));
 {UiCoreObserver observer(a);CHECK(original_event_begin(a.game,OriginalEventId::Darv,42,1));}same();
 CHECK(!a.reward_presentation.heal.active);
 bool zero_healed=false;for(unsigned i=previous_logs;i<a.game.log_count;++i)if(a.game.logs[i].kind==LogKind::Heal){CHECK(a.game.logs[i].a==0);zero_healed=true;}CHECK(zero_healed);
 scenario="ordinary positive heal requests still show at full health without inflating actual history";
 direct=a.game;heal(direct,direct.max_hp);
 {UiCoreObserver observer(a);heal(a.game,a.game.max_hp);}same();
 CHECK(a.reward_presentation.heal.alive(a.map_clock_ms)&&a.reward_presentation.heal.ratio==4096);
 CHECK(a.game.logs[a.game.log_count-1].kind==LogKind::Heal&&!a.game.logs[a.game.log_count-1].a);
 scenario="Neow healing survives the actual character departure reset";
 a.title=true;a.character_select=true;a.practice_entry=false;a.run_settings=false;a.selected_character=Character::Ironclad;
 a.selected_ascension=0;a.selected_region=reference::ActKind::Overgrowth;a.selected_seed[0]=0;
 a.ui_action(UiAction::StartCharacter);CHECK(a.character_departure.active());
 for(unsigned frame=0;frame<90&&a.title;++frame)a.advance(33);
 CHECK(!a.title&&a.game.original_events.id==OriginalEventId::Neow&&a.reward_presentation.heal.active);
 CHECK(a.reward_presentation.heal.start==a.map_clock_ms+200u&&!a.reward_presentation.heal.alive(a.map_clock_ms));
 a.advance(200);CHECK(a.reward_presentation.heal.alive(a.map_clock_ms));
}
static void tablet_upgrade(){
 for(unsigned fast=0;fast<2;++fast){
  scenario="all real Tablet upgrades retain their own actual card snapshots";event_room();a.fast_mode=fast!=0;a.game.hp=a.game.max_hp=80;
  CHECK(original_event_begin(a.game,OriginalEventId::TabletOfTruth,19,0));
  for(unsigned page=0;page<5;++page){
   a.map_clock_ms+=4000;direct=a.game;const unsigned old=a.reward_presentation.upgrades.count;
   CHECK(original_event_choose(direct,0));{UiCoreObserver observer(a);CHECK(original_event_choose(a.game,0));}same();
   const auto&m=a.reward_presentation.upgrades;CHECK(m.count>0&&m.alive(a.map_clock_ms));
   (void)old;for(unsigned i=0;i<m.count;++i){const auto&v=m.cards[i];CHECK(v.deck>=0&&v.deck<a.game.deck_count&&v.card.upgraded);
    CHECK(v.card.kind==a.game.deck[v.deck].kind&&v.energy==a.card_offer_cost(v.card,&v.facts));
    CHECK(v.style==(page==4?RewardPresentation::Upgrades::Messy:RewardPresentation::Upgrades::Event));
    CHECK(v.start==a.map_clock_ms+(page==4?i*(fast?100u:200u):0u));}
   if(page==4)CHECK(m.count==a.game.deck_count-4);
   if(actual_art){before=a.game;a.map_clock_ms+=300;Canvas c{a};upgraded_card_animation(c);unchanged();capture(page==4?"tablet-mass-upgrade":"tablet-upgrade");}
  }
  for(unsigned i=0;i<a.game.deck_count;++i)CHECK(a.game.deck[i].upgraded);CHECK(a.game.hp==1&&a.game.max_hp==1);
 }
 a.fast_mode=false;
}
static void relic_flashes(){
 scenario="Large Capsule keeps each obtained serial and every real relic flash";event_room();
 const unsigned old=a.game.world_relics.count;direct=a.game;
 CHECK(gain_world_relic(direct,WorldRelicLargeCapsule));{UiCoreObserver observer(a);CHECK(gain_world_relic(a.game,WorldRelicLargeCapsule));}same();
 CHECK(a.game.world_relics.count==old+3&&a.hud_relic_motion.flash_count>=3);
 for(unsigned i=old;i<a.game.world_relics.count;++i){const auto serial=a.game.world_relics.entries[i].serial;
  CHECK(a.hud_relic_motion.holders[i].identity==serial&&a.hud_relic_motion.holders[i].entering);
  CHECK(!a.hud_relic_motion.flashing(i,serial,a.map_clock_ms)&&a.hud_relic_motion.flashing(i,serial,a.map_clock_ms+350));}
 // Obtain is not an all-inventory flash; the starter remains quiet until
 // its own gameplay hook actually fires.
 CHECK(!a.hud_relic_motion.flashing(0,a.game.world_relics.entries[0].serial,a.map_clock_ms+350));
 a.map_clock_ms+=400;before=a.game;capture("capsule-three-flashes");unchanged();
 const unsigned initial=a.hud_relic_motion.flash_count;const auto id=a.game.world_relics.entries[0].id;
 direct=a.game;world_relic_flash(direct,id);{UiCoreObserver observer(a);world_relic_flash(a.game,id);}same();
 CHECK(a.hud_relic_motion.flash_count==initial+1&&a.hud_relic_motion.flashing(0,a.game.world_relics.entries[0].serial,a.map_clock_ms));
 before=a.game;Canvas c{a};for(unsigned age=0;age<1050;age+=33){a.map_clock_ms+=33;reward_pickup_screen(c);unchanged();}
 a.hud_relic_motion.expire(a.map_clock_ms+2000);CHECK(!a.hud_relic_motion.flash_count);
}
static void vantom_authored_events(){
 using namespace vantom_vfx;
 scenario="Vantom authored attack event preserves its actual death_spray_off spelling";
 Clock clock;unsigned births[EmitterCount]{},restarts=0;uint32_t restart_at=0;
 auto emit=[&](Emitter kind,uint32_t,unsigned){++births[kind];};
 auto restart=[&](uint32_t at){++restarts;restart_at=at;};
 clock.observe(Attack,10000,10932,false,emit,restart);CHECK(!births[Spray]);
 clock.observe(Attack,10000,10933,false,emit,restart);CHECK(births[Spray]==1&&clock.streams[Spray].active);
 clock.observe(Attack,10000,11033,false,emit,restart);CHECK(clock.streams[Spray].active);
 clock.observe(Idle,11267,11300,true,emit,restart);CHECK(!clock.streams[Spray].active&&births[Spray]==45);
 clock.observe(Idle,11267,15300,true,emit,restart);CHECK(births[Spray]==45&&!restarts);
 scenario="Vantom death owns one-second quadratic tail dissolve and independent emitter clocks";
 clock.reset();for(auto&n:births)n=0;
 clock.observe(Die,20000,20032,false,emit,restart);CHECK(!clock.dissolving);
 clock.observe(Die,20000,20533,false,emit,restart);CHECK(clock.dissolve(20533)==1024);
 clock.observe(Die,20000,21032,false,emit,restart);CHECK(!restarts);
 clock.observe(Die,20000,21033,false,emit,restart);CHECK(restarts==1&&restart_at==21033&&clock.dissolve(21033)==4096);
 clock.observe(Die,20000,22133,false,emit,restart);CHECK(births[Explosion]==30);
 CHECK(!clock.streams[DeathFront].active&&!clock.streams[DeathBack].active&&births[DeathFront]==births[DeathBack]);
 const auto exploded=births[Explosion];clock.observe(Die,20000,24000,false,emit,restart);CHECK(births[Explosion]==exploded&&restarts==1);
 clock.observe(Idle,24000,24033,true,emit,restart);CHECK(!clock.dissolving);
 scenario="Vantom delayed frame skips only already-expired births";
 clock.reset();for(auto&n:births)n=0;
 clock.observe(Debuff,30000,40000,false,emit,restart);CHECK(!births[Spray]&&!clock.streams[Spray].active);
 CHECK(lifetime(Spray,0)==666&&lifetime(DeathBack,65535)==493&&lifetime(Explosion,65535)==600);
}
int main(int argc,char**argv){
 CHECK(argc>=1&&argc<=3);std::vector<uint8_t>common,language;SplitResourceSource split;
 if(argc>1){const std::string root=argv[1];common=read(root+"/data/nSpire2.dat.tns");language=read(root+"/data/nSpire2.eng.dat.tns");
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));actual_art=true;}
 if(argc>2){output=argv[2];std::filesystem::create_directories(output);}
 all_original_moves();command_boundaries();axebot_revived_bootup();conditional_backup_wait();first_speech_after_stun();stunned_hook_boundaries();test_subject_respawn();ravenous_hook_boundaries();crab_rage_flash();generated_entry();generated_card_creator();ancient_heal();tablet_upgrade();relic_flashes();vantom_authored_events();
 std::printf("combat UI15: %u checks passed%s\n",checks,actual_art?"; actual resources decoded and rendered":"");
}
