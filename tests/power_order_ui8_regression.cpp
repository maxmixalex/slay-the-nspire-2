#include "app.hpp"
#include "world_run.hpp"
#include "power_strip_layout.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static App a;
static State restored,canonical;
static SaveData saved,again,bad;
static UiProgress decoded;
static uint8_t progress_bytes[UiProgressCapacity];
static unsigned checks;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"power_order_ui8:%d [%s] %s phase=%u\n",__LINE__,scenario,#x,unsigned(a.game.phase));std::exit(1);}}while(0)
static void effect(XOp op,int x=0,int y=0){queue_extension(a.game,op,x,y);resolve_pending(a.game);CHECK(a.game.phase!=Phase::Error);}
static void start(EncounterId encounter=EncounterId::CorpseSlugsNormal){
 new_run(a.game,0x41cef1ea,false,Character::Regent);a.game.phase=Phase::Combat;a.game.current_node=1;
 a.game.floor=a.game.turn=1;a.game.hp=a.game.max_hp=999;a.game.energy=99;
 CHECK(encounter_setup(a.game,encounter));a.title=a.library=a.paused=a.help=false;a.reset_presentation();
 CHECK(validate(a.game));
}
static int add(CardKind kind){int id=a.game.card_count;effect(XOp::EnemyGenerateCard,int(kind),1025);CHECK(a.game.card_count==id+1);return id;}
static void cast(int id){for(unsigned i=0;i<a.game.hand.count;++i)if(a.game.hand.ids[i]==id){CHECK(play(a.game,i,0));return;}CHECK(false);}
static int position(unsigned actor,unsigned id,unsigned slot=0){const auto token=power_token(actor,id,slot);
 for(unsigned i=0;i<a.game.power_sequence.count;++i)if(a.game.power_sequence.entries[i]==token)return int(i);return -1;}
static void complete_order(){
 CHECK(validate(a.game));canonical=a.game;power_order_reconcile(canonical);
 if(std::memcmp(&canonical.power_sequence,&a.game.power_sequence,sizeof canonical.power_sequence)){
  for(unsigned i=0;i<canonical.power_sequence.count;++i)if(position(power_actor(canonical.power_sequence.entries[i]),power_id(canonical.power_sequence.entries[i]),power_instance(canonical.power_sequence.entries[i]))<0)
   std::fprintf(stderr,"missing token actor=%u id=%u slot=%u\n",power_actor(canonical.power_sequence.entries[i]),power_id(canonical.power_sequence.entries[i]),power_instance(canonical.power_sequence.entries[i]));
 }
 CHECK(!std::memcmp(&canonical.power_sequence,&a.game.power_sequence,sizeof canonical.power_sequence));
}
static void roundtrip(){CHECK(save(a.game,saved));CHECK(saved.bytes[4]==SaveVersion);CHECK(load(restored,saved.bytes,saved.size));
 CHECK(save(restored,again));CHECK(saved.size==again.size&&!std::memcmp(saved.bytes,again.bytes,saved.size));
 CHECK(!std::memcmp(&restored.power_sequence,&a.game.power_sequence,sizeof restored.power_sequence));}
static void ordered(std::initializer_list<uint16_t> expected,unsigned actor=0){uint16_t tokens[MaxPlayerPowerEntries];unsigned n=a.combat_power_entries(int(actor),tokens,MaxPlayerPowerEntries);
 CHECK(n==expected.size());unsigned at=0;for(auto token:expected){CHECK(tokens[at++]==token);CHECK(position(power_actor(token),power_id(token),power_instance(token))>=0);}}
static bool hit(HitKind kind,int value){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return true;return false;}
static void actual_power_order(){
 scenario="cross-family applications, instanced models, removal and reapplication";start();
 const unsigned loop=DefectStatusStart+unsigned(DPower::Loop),barricade=IroncladStatusStart+unsigned(IPower::Barricade),
  afterimage=SilentStatusStart+unsigned(SPower::Afterimage),shroud=NecroStatusStart+unsigned(NPower::Shroud),
  panache=18+unsigned(XPower::Panache),automation=18+unsigned(XPower::Automation),orbit=RegentStatusStart+unsigned(RPower::Orbit),
  monologue=RegentStatusStart+unsigned(RPower::Monologue),focus=18+unsigned(XPower::Focus);
 effect(XOp::DPower,int(DPower::Loop),2);effect(XOp::Power,int(XPower::Panache),10);
 effect(XOp::IPower,int(IPower::Barricade),1);effect(XOp::SPower,int(SPower::Afterimage),1);
 effect(XOp::NPower,int(NPower::Shroud),3);effect(XOp::Power,int(XPower::Automation),1);
 effect(XOp::RPower,int(RPower::Orbit),1);effect(XOp::Power,int(XPower::Panache),14);
 effect(XOp::RPower,int(RPower::Monologue),1);CHECK(!a.combat_power_counter(power_token(0,monologue)));effect(XOp::Bomb,40,3);gain_power(a.game,XPower::Focus,-2);
 ordered({power_token(0,loop),power_token(0,panache),power_token(0,barricade),power_token(0,afterimage),power_token(0,shroud),
  power_token(0,automation),power_token(0,orbit),power_token(0,panache,1),power_token(0,monologue),power_token(0,43),power_token(0,focus)});
 complete_order();roundtrip();
 // Amount changes retain the exact source position. An actual model removal
 // and reapplication must append, including a negative-capable power.
 const int old=position(0,loop);effect(XOp::DPower,int(DPower::Loop),1);CHECK(position(0,loop)==old);
 gain_power(a.game,XPower::Focus,2);CHECK(position(0,focus)<0);effect(XOp::Power,int(XPower::Plating),3);
 gain_power(a.game,XPower::Focus,-1);CHECK(position(0,focus)>position(0,18+unsigned(XPower::Plating)));
 // Nightmare adds an independent original snapshot through its real chooser.
 const int defend=add(DefendSilent),nightmare=add(Nightmare);cast(nightmare);CHECK(a.game.expansion.choice==XChoice::NightmareCard);
 const uint16_t selected=uint16_t(defend);CHECK(choose_extended(a.game,&selected,1));CHECK(position(0,CharacterExtraStatusStart)>=0);
 CHECK(a.combat_power_amount(power_token(0,CharacterExtraStatusStart),true)==3);
 effect(XOp::Power,int(XPower::RollingBoulder),5);CHECK(position(0,18+unsigned(XPower::RollingBoulder))>=0);
 complete_order();roundtrip();
 // Each instance keeps its own progress/display value rather than using the
 // aggregate Panache damage, Monologue rate, or active-instance count.
 a.game.expansion.panache_count[0]=1;a.game.expansion.panache_count[1]=4;
 a.game.expansion.automation_count[0]=3;a.game.regent.orbit_remainder[0]=2;
 CHECK(a.combat_power_amount(power_token(0,panache),true)==4&&a.combat_power_amount(power_token(0,panache,1),true)==1);
 CHECK(a.combat_power_amount(power_token(0,automation),true)==7&&a.combat_power_amount(power_token(0,orbit),true)==2);
 const int applied=a.combat_power_amount(power_token(0,monologue),true);effect(XOp::RMonologueStrength,0,3);
 CHECK(a.combat_power_counter(power_token(0,monologue))&&a.combat_power_amount(power_token(0,monologue),true)==applied+3);
 // Real invisible wrapper, independent orb and pet controls do not create
 // duplicate on-creature power icons. Every visible instance gets a hit.
 effect(XOp::DPower,int(DPower::HyperbeamFocus),2);effect(XOp::Summon,5);a.render();
 for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}CHECK(!a.combat_preparation.active);
 CHECK(hit(HitKind::PowerInstance,int(power_token(0,panache)))&&hit(HitKind::PowerInstance,int(power_token(0,panache,1))));
 CHECK(!hit(HitKind::BattleStatus,44)&&!hit(HitKind::BattleStatus,45)&&!hit(HitKind::BattleStatus,46));
 CHECK(!hit(HitKind::BattleStatus,DefectStatusStart+int(DPower::HyperbeamFocus)));
 complete_order();roundtrip();
 scenario="enemy interleaving, zero removal and artifact consumption";
 auto enemy_power=[&](EnemyPower p,int amount){effect(XOp::EnemyApplyPower,amount,int(p)+80);};
 enemy_power(EnemyPower::Strength,2);enemy_power(EnemyPower::Weak,2);const int strength=position(1,0),weak=position(1,4);CHECK(strength>=0&&weak>strength);
 enemy_power(EnemyPower::Strength,1);CHECK(position(1,0)==strength);enemy_power(EnemyPower::Weak,-2);CHECK(position(1,4)<0);
 enemy_power(EnemyPower::Vulnerable,1);enemy_power(EnemyPower::Weak,1);CHECK(position(1,4)>position(1,5));
 enemy_power(EnemyPower::Artifact,1);CHECK(position(1,CharacterExtraStatusStart+2)>=0);
 effect(XOp::NApplyCopiedDebuff,int(NDebuff::Poison)*MaxEnemies,2);CHECK(position(1,CharacterExtraStatusStart+2)<0);
 complete_order();roundtrip();
}
static void delayed_order_and_layout(){
 scenario="presentation waits for its effect and keeps source two-row geometry";
 CombatStatus view;PowerOrderState initial{},next{};initial.entries[0]=power_token(0,0);initial.entries[1]=power_token(0,39);initial.count=2;
 view.capture_order(initial,0,true);next=initial;next.entries[0]=initial.entries[1];next.entries[1]=initial.entries[0];
 view.capture_order(next,20,false);CHECK(view.count==2);view.sync(19);CHECK(view.shown_order.entries[0]==initial.entries[0]);
 view.sync(20);CHECK(view.shown_order.entries[0]==next.entries[0]&&view.shown_order.entries[1]==next.entries[1]&&!view.overflow);
 PowerOrderState crowded{},removed{};for(unsigned i=0;i<80;++i)crowded.entries[crowded.count++]=power_token(1+i/EnemyPowerCount,EnemyStatusStart+i%EnemyPowerCount);
 removed.count=uint16_t(crowded.count-1);for(unsigned i=0;i<removed.count;++i)removed.entries[i]=crowded.entries[i+1];
 view.capture_order(crowded,30,true);view.capture_order(removed,40,false);CHECK(view.count==1&&!view.overflow);view.sync(40);CHECK(view.shown_order.count==removed.count);
 const auto small=PowerStripLayout::make(10,140,47,3,12);CHECK(small.columns==4&&small.rows==1&&small.left==10&&small.width==36);
 const auto large=PowerStripLayout::make(10,140,47,11,12);CHECK(large.columns==6&&large.rows==2&&large.left==-2&&large.width==72);
 CHECK(large.x(5)==58&&large.y(5)==140&&large.x(6)==-2&&large.y(6)==152&&large.y(10)==152);
}
static void word(uint8_t*b,size_t at,unsigned value){for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(value>>(8*i));}
static void migration(){
 scenario="v17 compact sequence and frozen v15/v16 migration";complete_order();roundtrip();
 const size_t tail=(4+2*a.game.power_sequence.count+3)&~size_t(3),at=saved.size-4-tail;
 CHECK(saved.bytes[at+2]==(a.game.power_sequence.count&255));
 bad=saved;bad.bytes[at+4]=255;bad.bytes[at+5]=255;CHECK(!load(restored,bad.bytes,bad.size));
 bad=saved;bad.bytes[at+2]=255;bad.bytes[at+3]=255;CHECK(!load(restored,bad.bytes,bad.size));
 bad=saved;bad.bytes[at+6]=bad.bytes[at+4];bad.bytes[at+7]=bad.bytes[at+5];CHECK(!load(restored,bad.bytes,bad.size));
 bad=saved;bad.size=at-16+4;word(bad.bytes,4,15);word(bad.bytes,bad.size-4,0);
 CHECK(load(restored,bad.bytes,bad.size)&&restored.test_subject_kills==0&&restored.run_options.mode==RunMode::Standard);
 CHECK(valid_power_order(restored));canonical=restored;power_order_reconcile(canonical);CHECK(!std::memcmp(&canonical.power_sequence,&restored.power_sequence,sizeof canonical.power_sequence));
 CHECK(save(restored,again)&&again.bytes[4]==17&&!std::memcmp(bad.bytes+8,again.bytes+8,bad.size-12));
 a.game.run_options.mode=RunMode::Custom;a.game.run_options.modifiers=run_modifier_bit(RunModifier::Flight);CHECK(save(a.game,saved));
 bad=saved;bad.size=at+4;word(bad.bytes,4,16);word(bad.bytes,bad.size-4,0);
 CHECK(load(restored,bad.bytes,bad.size)&&restored.run_options.mode==RunMode::Custom&&restored.run_options.modifiers==run_modifier_bit(RunModifier::Flight));
 CHECK(save(restored,again)&&again.bytes[4]==17&&!std::memcmp(bad.bytes+8,again.bytes+8,bad.size-12));
}
static void test_subject(){
 scenario="TestSubject knockout count survives each saved transformation";start(EncounterId::TestSubjectBoss);
 for(unsigned form=1;form<=2;++form){damage_unpowered(a.game,0,9999);resolve_pending(a.game);CHECK(a.game.test_subject_kills==form&&!enemy_allow_target(a.game,0));
  roundtrip();a.game=restored;resolve_pending(a.game);CHECK(a.game.test_subject_kills==form);
  effect(XOp::EnemyMove,0,0);effect(XOp::EnemyMoveFinish,0,0);CHECK(enemy_allow_target(a.game,0)&&a.game.test_subject_kills==form);
 }
 damage_unpowered(a.game,0,9999);resolve_pending(a.game);CHECK(a.game.phase==Phase::Reward&&a.game.test_subject_kills==2);roundtrip();
 scenario="run-end profile total, custom/daily accounting, NSH5 and title";
 clear_ui_progress(a.progress);a.progress_loaded=true;a.progress.test_subject_kills=23;
 CHECK(std::strstr(a.monster_title(EnemyId::TestSubject),"31")&&!std::strchr(a.monster_title(EnemyId::TestSubject),'{'));
 for(unsigned mode=0;mode<3;++mode){new_run(a.game,0x7100+mode,false);a.game.run_options.mode=RunMode(mode);if(mode==2)a.game.run_options.daily_day=20750;
  CHECK(world_begin_run(a.game));a.title=false;a.begin_progress();a.game.test_subject_kills=2;
  const unsigned previous=a.progress.test_subject_kills;CHECK(previous==23+mode*2);
  // An unfinished current run never changes the original title's profile
  // baseline. Losing or abandoning before a respawn still retains the count.
  char expected[32];std::snprintf(expected,sizeof expected,"%u",previous+8);CHECK(std::strstr(a.monster_title(EnemyId::TestSubject),expected));
  if(mode==0){player_kill(a.game,true);resolve_pending(a.game);a.finish_progress();}else a.finish_progress(true);
  CHECK(a.progress.test_subject_kills==previous+2);a.finish_progress(true);CHECK(a.progress.test_subject_kills==previous+2);
 }
 a.progress.map_drawing.seed=1;a.progress.map_drawing.serial=1;a.progress.map_drawing.act=0;a.progress.map_drawing.count=1;a.progress.map_drawing.points[0].x=10;a.progress.map_drawing.points[0].y_flags=20|0x8000;
 const size_t n=encode_ui_progress(a.progress,progress_bytes,sizeof progress_bytes);CHECK(n&&progress_bytes[3]=='5');
 CHECK(decode_ui_progress(progress_bytes,n,decoded)&&decoded.test_subject_kills==29&&decoded.map_drawing.count==1);
 UiProgressSummary summary;CHECK(decode_ui_progress_summary(progress_bytes,n,summary)&&summary.exists);
 // NSH4's entire map/history prefix is identical; its missing count starts at
 // zero. The original ring and profile facts remain valid across migration.
 progress_bytes[3]='4';word(progress_bytes,4,unsigned(n-4));CHECK(decode_ui_progress(progress_bytes,n-4,decoded)&&decoded.test_subject_kills==0&&decoded.map_drawing.count==1);
 progress_bytes[3]='5';word(progress_bytes,4,unsigned(n-4));CHECK(!decode_ui_progress(progress_bytes,n-4,decoded));
}
int main(int argc,char**argv){std::vector<uint8_t> bytes;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));CHECK(a.init(bytes.data(),bytes.size(),{}));}
 actual_power_order();delayed_order_and_layout();migration();test_subject();
 std::printf("Power order: %u checks; actual multi-family applications, seven instance types, visibility, effect-time order, v15/v16/v17 migration, TestSubject run/profile persistence. State=%zu order=%zu CombatStatus=%zu\n",checks,sizeof(State),sizeof(PowerOrderState),sizeof(CombatStatus));
}
