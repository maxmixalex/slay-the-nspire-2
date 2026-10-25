// Source equations, actual command observers and original private masks.
// These are correctness checks; calculator frame time is measured in Firebird.
#include "../src/app.cpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Shake=ScreenShakePresentation;
static App a;
static State direct;
static SaveData one,two;
static unsigned checks;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"screen effects UI12 [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static void equal_save(){CHECK(save(direct,one)&&save(a.game,two));CHECK(one.size==two.size&&!std::memcmp(one.bytes,two.bytes,one.size));}
static void source_equations(){
 scenario="independent source punch and multiplier equations";
 constexpr double Pi=3.14159265358979323846;
 const double multipliers[]={0,.5,1,2,4};
 for(unsigned option=0;option<5;++option){CHECK(Shake::multiplier(option)==unsigned(multipliers[option]*1024));
  for(unsigned strength:{2u,5u,20u,40u,80u})for(unsigned duration:{300u,800u,1200u})for(unsigned angle:{0u,512u,1024u,2048u,3000u}){
   Shake m;m.bind(71,100);m.add(100,100,Shake::Punch,strength,duration,angle);
   for(unsigned age:{0u,33u,99u,250u,300u,799u,1200u}){int x,y;m.sample(100+age,option,x,y);
    const double t=double(age)/duration,amplitude=age<duration?strength*multipliers[option]*(1-t*t*t)*std::cos(double(duration-age)*.060):0;
    const auto limit=[](int v,int n){return v< -n?-n:v>n?n:v;};
    const int expected_x=limit(int(std::lround(amplitude*std::cos(angle*2*Pi/4096)*4/21)),64);
    const int expected_y=limit(int(std::lround(amplitude*std::sin(angle*2*Pi/4096)*4/21)),48);
    CHECK(std::abs(x-expected_x)<=2&&std::abs(y-expected_y)<=2);
   }
  }
 }
 scenario="noise continuity across positive and negative lattice cells";
 for(unsigned seed:{1u,7u,29u,0xffffffffu})for(int p=-4096;p<4096;++p){
  const int n=Shake::noise(p,seed),next=Shake::noise(p+1,seed);
  CHECK(std::abs(n)<=1050);CHECK(std::abs(next-n)<=20);
 }
 scenario="latest punch overrides rumble, trauma adds and decays";
 Shake m;m.bind(7,0);m.add(0,0,Shake::Rumble,40,1200);unsigned visible=0;
 for(unsigned now=0;now<800;now+=33){int x,y;m.advance(now);m.sample(now,2,x,y);visible+=x!=0||y!=0;}
 CHECK(visible>5);m.add(800,800,Shake::Punch,20,300,1024);
 Shake punch;punch.bind(7,800);punch.add(800,800,Shake::Punch,20,300,1024);
 for(unsigned now=800;now<1100;now+=33){int x,y,px,py;m.sample(now,2,x,y);punch.sample(now,2,px,py);CHECK(x==px&&y==py);}
 m.bind(8,0);m.add(0,0,Shake::Trauma,40);CHECK(m.trauma_now(0)==614&&m.trauma_now(200)==205);
 m.add(200,200,Shake::Trauma,40);CHECK(m.trauma==819&&m.trauma_now(400)==410&&m.trauma_now(600)==0);
 m.add(200,200,Shake::Trauma,80);CHECK(m.trauma==1024);
 for(unsigned option=0;option<5;++option)for(unsigned now=200;now<700;now+=11){int x,y;m.sample(now,option,x,y);CHECK(x*x+y*y<=121);if(!option)CHECK(!x&&!y);}
 m.bind(9,0);m.add(0,0,Shake::Drunk,80,1200);visible=0;
 for(unsigned now=0;now<1200;now+=17){int x,y;m.advance(now);m.sample(now,4,x,y);CHECK(std::abs(x)<=64&&std::abs(y)<=48);visible+=x||y;}
 CHECK(visible>5);int x,y;m.sample(1200,4,x,y);CHECK(!x&&!y);
 scenario="future source cues preserve ordering and additive trauma";
 m.bind(10,0);for(unsigned at:{750u,250u,500u})m.add(0,at,Shake::Punch,5,300,2048);
 CHECK(m.count==3&&m.cues[0].at==250&&m.cues[1].at==500&&m.cues[2].at==750);
 m.add(0,500,Shake::Punch,20);CHECK(m.count==3&&m.cues[1].strength==20);
 m.advance(499);CHECK(m.count==2&&m.punch.at==250);m.advance(500);CHECK(m.count==1&&m.punch.strength==20);
 m.add(500,750,Shake::Trauma,5);m.add(500,750,Shake::Trauma,5);m.advance(750);CHECK(!m.count&&m.trauma==410);
 m.bind(11,0);for(unsigned i=0;i<Shake::Capacity+1;++i)m.add(0,i+1,Shake::Trauma,2);CHECK(m.count==48&&m.overflow);
 CHECK(sizeof(Shake)<768&&sizeof(CreatureSleepPresentation)==12);
}
static void sleeping_lifetime(){
 scenario="sleep emission stops while existing particles finish";
 CreatureSleepPresentation s;s.observe(false,0);CHECK(!s.alive(0));s.observe(true,100);
 CHECK(s.start==100&&s.active&&s.sample(0,100,1).visible&&!s.sample(1,100,1).visible);
 for(unsigned i=0;i<4;++i){const auto first=s.sample(i,100+i*1000,7),later=s.sample(i,600+i*1000,7);
  CHECK(first.visible&&later.visible&&first.alpha==255&&later.y<first.y&&later.x>=first.x&&later.size>=first.size);
 }
 s.observe(false,2600);CHECK(!s.active&&s.alive(2600));CHECK(!s.sample(3,3100,7).visible&&s.sample(2,3100,7).visible);
 CHECK(!s.alive(6600));for(unsigned i=0;i<4;++i)CHECK(!s.sample(i,6600,7).visible);
 s.observe(true,9000);CHECK(s.start==9000&&s.active&&s.sample(0,9000,7).visible&&!s.sample(1,9000,7).visible);
}
static void fresh(){
 new_run(a.game,0x41cef1ea,false,Character::Ironclad);
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.reset_presentation();a.pointer.visible=false;a.map_clock_ms=1000;a.animation=30;a.screen_shake=2;
}
static void pixels_and_hits(){
 scenario="in-place translation matches independent immutable raster and hit targets";
 fresh();direct=a.game;Canvas c{a};std::vector<uint16_t> original(320*240);
 for(int y=0;y<240;++y)for(int x=0;x<320;++x)original[y*320+x]=uint16_t(x+321*y);
 for(int dx:{-64,-3,0,5,64})for(int dy:{-48,-4,0,7,48}){
  std::memcpy(a.pixels,original.data(),sizeof a.pixels);a.hit_count=0;a.add_hit(2,3,4,5,HitKind::Menu,0,true);a.add_hit(100,120,20,30,HitKind::Enemy,0,true);
  screen_translate(c,dx,dy,1,22);
  for(int y=0;y<240;++y)for(int x=0;x<320;++x){const int sx=y<22?x:std::max(0,std::min(319,x-dx)),sy=y<22?y:std::max(22,std::min(239,y-dy));CHECK(a.pixels[y*320+x]==original[sy*320+sx]);}
  CHECK(a.hits[0].x==2&&a.hits[0].y==3&&a.hits[1].x==100+dx&&a.hits[1].y==120+dy);equal_save();
 }
}
static void settings_and_smith(){
 scenario="actual settings change previews trauma without modifying a run";
 fresh();direct=a.game;a.title=a.settings_open=true;a.ui_action(UiAction::SettingsShakeUp);
 CHECK(a.screen_shake==3&&a.screen_motion.trauma==410);equal_save();
 a.ui_action(UiAction::SettingsShakeUp);CHECK(a.screen_shake==4);a.screen_motion.reset();a.ui_action(UiAction::SettingsShakeUp);CHECK(!a.screen_motion.bound);equal_save();
 scenario="ordinary rest Smith commits once and queues the three source hammer shakes";
 fresh();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events={};auto&w=a.game.world;
 w.room_type=uint8_t(WorldRoom::Rest);w.room_open=1;w.rest_count=1;w.rest_options[0]=uint8_t(WorldRestOption::Smith);w.rest_used=0;w.rest_pending=0;
 a.game.phase=Phase::Camp;a.cursor=0;CHECK(a.camp_input(Key::Confirm));CHECK(a.game.phase==Phase::Upgrade);
 a.deck_candidate=0;a.deck_choice_revision=a.game.revision;a.deck_choice_phase=a.game.phase;direct=a.game;CHECK(upgrade_card(direct,0));
 a.confirm_deck_choice();equal_save();CHECK(a.reward_presentation.upgrades.count==1&&a.screen_motion.count==3);
 for(unsigned i=0;i<3;++i){const auto&q=a.screen_motion.cues[i];CHECK(q.at==a.map_clock_ms+250u+i*250u&&q.duration==300&&q.strength==5&&q.angle==2048);}
 const auto pending=a.screen_motion.count;a.confirm_deck_choice();CHECK(a.screen_motion.count==pending);equal_save();
 scenario="actual Girya selection uses the completed rest trigger";
 fresh();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events={};CHECK(gain_world_relic(a.game,WorldRelicGirya));
 auto&lift=a.game.world;lift.room_type=uint8_t(WorldRoom::Rest);lift.room_open=1;lift.rest_count=1;lift.rest_options[0]=uint8_t(WorldRestOption::Lift);lift.rest_used=0;lift.rest_pending=0;
 a.game.phase=Phase::Camp;a.cursor=0;direct=a.game;CHECK(world_rest_choose(direct,0));CHECK(a.camp_input(Key::Confirm));equal_save();CHECK(a.screen_motion.punch.strength==40&&a.screen_motion.punch.duration==300);
}
static int encounter(EncounterId id,EnemyId wanted){
 fresh();auto&s=a.game;s.phase=Phase::Combat;s.current_node=1;s.floor=s.turn=1;s.hp=s.max_hp=999;s.energy=99;
 CHECK(encounter_setup(s,id));int slot=-1;for(int i=0;i<s.enemy_count;++i)if(s.enemy_world.models[i].kind==wanted)slot=i;
 CHECK(slot>=0);a.reset_presentation();a.sync_enemy_feedback();a.render();
 for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}CHECK(!a.combat_preparation.active&&validate(s));return slot;
}
static void actual_sleep(){
 scenario="original encounter initialization and real damage wake share presented facts";
 for(const auto id:{EncounterId::SlumberingBeetleNormal,EncounterId::LagavulinMatriarchBoss}){
  const bool beetle=id==EncounterId::SlumberingBeetleNormal;const int slot=encounter(id,beetle?EnemyId::SlumberingBeetle:EnemyId::LagavulinMatriarch);
  scenario=beetle?"Slumbering Beetle actual stunned wake":"Lagavulin actual unblocked-damage wake";
  CHECK(a.enemy_presentation[slot].sleep.active);direct=a.game;for(unsigned i=0;i<35;++i){a.advance(33);a.render();}equal_save();
  CHECK(a.enemy_presentation[slot].sleep.active);a.game.enemies[slot].block=0;direct=a.game;
  const unsigned hits=beetle?3u:1u;for(unsigned i=0;i<hits;++i){damage_unpowered(direct,slot,1);resolve_pending(direct);}
  {UiCoreObserver observer(a);a.begin_combat_feedback();for(unsigned i=0;i<hits;++i){damage_unpowered(a.game,slot,1);resolve_pending(a.game);}}
  if(beetle){
   // Slumber schedules WakeUp as the next STUNNED move; unlike Asleep,
   // its third hit does not invoke that move during the player's action.
   CHECK(a.game.enemy_world.models[slot].node==31&&a.enemy_presentation[slot].sleep.active);equal_save();
   CHECK(end_turn(direct));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(a.game));}
  }
  // The beetle is third in its authored encounter. Its WakeUp waits behind
  // both Bowlbugs' real move cues rather than revealing canonical future
  // facts during their presentation; allow that bounded command sequence.
  for(unsigned i=0;i<600&&a.enemy_presentation[slot].sleep.active;++i){a.advance(33);a.render();}
  if(a.enemy_presentation[slot].sleep.active){int awake=-1;ui_enemy_fact(&a.enemy_presentation[slot].facts,"IsAwake",awake);std::fprintf(stderr,"wake trace: tick=%u end=%u visual=%u phase=%u node=%u saved=%d shown=%d state=%d clip=%d queued=%u\n",a.animation,a.combat_feedback.end_at,a.enemy_visual_clock,unsigned(a.game.phase),a.game.enemy_world.models[slot].node,a.game.enemy_world.models[slot].data[8],awake,a.enemy_presentation[slot].state,a.enemy_presentation[slot].clip,a.enemy_presentation[slot].count);}
  CHECK(!a.enemy_presentation[slot].sleep.active);equal_save();
  const uint32_t stopped=a.enemy_presentation[slot].sleep.stopped;CHECK(a.enemy_presentation[slot].sleep.alive(stopped));
  for(unsigned i=0;i<122;++i){a.advance(33);a.render();}CHECK(!a.enemy_presentation[slot].sleep.alive(a.animation*33u));equal_save();
 }
}
static void actual_cards(){
 for(const CardKind kind:{Inflame,Bludgeon}){
  scenario=kind==Inflame?"actual Power arrival triggers its punch":"blocked Bludgeon retains authored delayed heavy impact";
  encounter(EncounterId::NibbitsNormal,EnemyId::Nibbit);auto&s=a.game;
  s.card_count=s.hand.count=1;s.draw.count=s.discard.count=s.exhaust.count=0;s.cards[0]={kind,0};s.hand.ids[0]=0;s.zones[0]=Zone::Hand;s.enemies[0].block=999;
  a.reset_presentation();a.render();for(unsigned i=0;i<160;++i){a.advance(33);a.render();}
  a.cursor=a.target=0;direct=s;CHECK(play(direct,0,0));a.input(Key::Confirm);a.input(Key::Confirm);a.render();equal_save();
  const unsigned expected=kind==Inflame?20u:40u;bool queued=false,fired=false;
  for(unsigned i=0;i<150;++i){
   for(unsigned n=0;n<a.screen_motion.count;++n)queued|=a.screen_motion.cues[n].strength==expected;
   fired|=a.screen_motion.punch.strength==expected;
   a.advance(33);a.render();
  }
  CHECK(queued&&fired);equal_save();
 }
}
static bool doom_camera(){
 const auto match=[](const Shake::Cue&q){return q.kind==Shake::Punch&&q.strength==5&&q.duration==300&&q.angle==2048;};
 if(match(a.screen_motion.punch))return true;
 for(unsigned i=0;i<a.screen_motion.count;++i)if(match(a.screen_motion.cues[i]))return true;
 return false;
}
static void first_world_combat(){
 fresh();a.forget_run_checkpoint();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 // Prior noncombat outcomes are a routing fixture; enter the real first
 // authored encounter through world_choose_point, not a debug command.
 for(unsigned step=0;step<32&&!world_room_is_combat(a.game);++step){
  if(world_can_travel(a.game)){uint8_t points[reference::Columns];const unsigned count=world_available_points(a.game,points,reference::Columns);CHECK(count);CHECK(world_choose_point(a.game,points[0]));}
  else{if(a.game.original_events.active)a.game.original_events={};CHECK(world_finish_room(a.game));}
 }
 CHECK(a.game.phase==Phase::Combat&&a.game.world.active&&a.game.enemy_world.active);a.reset_presentation();a.render();
 for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}CHECK(!a.combat_preparation.active&&validate(a.game));
}
static void actual_doom(){
 scenario="Doom's real immediate check emits only on execution";
 for(unsigned lethal=0;lethal<2;++lethal){
  encounter(EncounterId::NibbitsNormal,EnemyId::Nibbit);auto&s=a.game;
  s.card_count=s.hand.count=1;s.draw.count=s.discard.count=s.exhaust.count=0;s.cards[0]={NecroEndOfDays,0};s.hand.ids[0]=0;s.zones[0]=Zone::Hand;
  for(int i=0;i<s.enemy_count;++i)s.enemies[i].hp=s.enemies[i].max_hp=500;
  s.enemies[0].hp=lethal?29:100;a.reset_presentation();a.render();
  for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}CHECK(!a.combat_preparation.active);a.screen_motion.reset();direct=s;
  CHECK(play(direct,0,0));{UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(play(s,0,0));}
  equal_save();CHECK(bool(!s.enemies[0].hp)==bool(lethal));CHECK(doom_camera()==bool(lethal));
  if(lethal){bool fired=false;for(unsigned i=0;i<200;++i){a.advance(33);a.render();fired|=a.screen_motion.punch.angle==2048&&a.screen_motion.punch.strength==5;}CHECK(fired);equal_save();}
 }
 scenario="Doom's own-side hooks cover the enemy and player without changing save bytes";
 for(unsigned player=0;player<2;++player){
  scenario=player?"player Doom actual death camera":"enemy Doom actual side-end camera";
  if(player)first_world_combat();else encounter(EncounterId::NibbitsNormal,EnemyId::Nibbit);auto&s=a.game;
  if(player){s.hp=3;s.necro.player_doom=3;}else{s.enemies[0].hp=1;s.expansion.doom[0]=1;}
  a.reset_presentation();a.render();for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}CHECK(!a.combat_preparation.active);
  a.screen_motion.reset();direct=s;CHECK(end_turn(direct));
  {UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(s));}
  equal_save();CHECK(player?s.phase==Phase::Dead:!s.enemies[0].hp);CHECK(doom_camera());
  const auto queued=a.screen_motion;const unsigned at=a.animation,linger=a.combat_linger_until;
  uint32_t doom_at=queued.punch.at;for(unsigned i=0;i<queued.count;++i)if(queued.cues[i].angle==2048&&queued.cues[i].strength==5)doom_at=queued.cues[i].at;
  unsigned retained=0,displaced=0;bool fired=false;for(unsigned i=0;i<250;++i){
   a.advance(33);a.render();fired|=a.screen_motion.punch.angle==2048&&a.screen_motion.punch.strength==5;
   if(player&&a.animation*33u-doom_at<300u){
    CHECK(screen_combat_target(a)&&a.screen_motion.target==queued.target&&a.screen_motion.punch.at==doom_at);
    CHECK(a.game_over_presentation.bound&&a.game.phase==Phase::Dead&&a.game.world.finished);
    int x=0,y=0;a.screen_scene_offset(x,y);displaced+=x||y;++retained;
   }
  }
  if(!fired){std::fprintf(stderr,"Doom trace: player=%u phase=%u tick=%u linger=%u queued=%u punch=%u/%u at=%u target=%u -> %u\n",player,unsigned(s.phase),at,linger,queued.count,queued.punch.strength,queued.punch.angle,queued.punch.at,queued.target,a.screen_motion.target);for(unsigned i=0;i<queued.count;++i){const auto&q=queued.cues[i];std::fprintf(stderr," cue at=%u kind=%u strength=%u angle=%u\n",q.at,q.kind,q.strength,q.angle);}}
  CHECK(fired);if(player)CHECK(retained>=9&&displaced>0&&!screen_combat_target(a)&&a.game_over_presentation.bound);equal_save();
 }
}
static void advance_room(unsigned ms){while(ms){const unsigned step=std::min(ms,33u);a.advance(step);a.render();ms-=step;}}
static void architect_equations(){
 scenario="Architect independent normal and fast command times";
 for(unsigned fast=0;fast<2;++fast)for(unsigned character=0;character<5;++character)for(unsigned dialogue:{0u,2u}){
  const auto c=Character(character);const unsigned count=character==1||character==4?4u:5u;
  for(unsigned start=0;start<2;++start){
   const unsigned hits=start?(character==1?4u:0u):character==1&&dialogue==2?0u:count;
   const auto p=source_architect_shake(c,dialogue,start,fast);CHECK(p.hits==hits);
   if(start&&!hits){CHECK(p.complete==2250&&!p.architect_at);continue;}
   const unsigned normal[]={100,300,500,700,900},quick[]={50,200,350,500,650};
   for(unsigned i=0;i<hits;++i)CHECK(p.hit_at(i)==(start?750u:0u)+(fast?quick[i]:normal[i]));
   const unsigned tail=hits?(start?750u:0u)+(fast?quick[hits-1]:normal[hits-1])+2000u:0u;
   CHECK(p.architect_at==(start?0u:tail+(fast?250u:500u)));
   CHECK(p.complete==(start?tail+500u:tail+(fast?750u:1000u)));
  }
 }
 scenario="future terminal presentation times clamp before origin and cross uint32 wrap";
 for(uint32_t now:{1000u,0xfffffff0u}){
  GameOverPresentation g;g.bound=g.event_room=true;g.opened=now+3900u;
  CHECK(!g.initial_age(now)&&!g.black_alpha(now)&&!g.ui_alpha(now)&&!g.ready(now));
  CHECK(!g.initial_age(now+3899u)&&!g.ready(now+3899u));
  CHECK(!g.initial_age(now+3900u)&&g.initial_age(now+4000u)==100u);
  CHECK(g.black_alpha(now+4100u)==255u&&!g.ready(now+5849u)&&g.ready(now+5850u));
  Shake s;s.bind(123,now);s.add(now,now+3900u,Shake::SequenceEnd,0,0);
  CHECK(s.sequence_active(now)&&s.sequence_end(now)==now+3900u);s.advance(now+3899u);CHECK(s.sequence_active(now+3899u));
  s.advance(now+3900u);CHECK(!s.sequence_active(now+3900u)&&!s.count);
 }
}
static void architect_room(Character character,bool fast,unsigned dialogue,unsigned shake){
 fresh();a.forget_run_checkpoint();new_run(a.game,0x7195c3,false,character);CHECK(world_set_seed(a.game,"NSPIRE-GAME-OVER-017"));CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,3));
 a.game.event_run.wins=a.game.event_run.character_wins[unsigned(character)]=dialogue;
 // Supply prior room outcomes to the normal world controller, exactly as
 // its routing gate does. These fixtures do not claim combat victories.
 for(unsigned step=0;step<192;++step){
  if(world_can_travel(a.game)){uint8_t points[reference::Columns];const unsigned count=world_available_points(a.game,points,reference::Columns);CHECK(count);CHECK(world_choose_point(a.game,points[0]));}
  else{
   if(a.game.original_events.active)a.game.original_events={};if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;
   const auto*m=world_current_map(a.game);CHECK(m);const unsigned end=m->second_boss!=reference::NoPoint?m->second_boss:m->boss;
   if(a.game.world.act_index==2&&a.game.world.current_point==end){
    a.reset_presentation();a.fast_mode=fast;a.screen_shake=uint8_t(shake);direct=a.game;CHECK(world_finish_room(direct));
    {UiCoreObserver observer(a);CHECK(world_finish_room(a.game));}equal_save();
    CHECK(a.game.phase==Phase::Event&&a.game.original_events.active&&a.game.original_events.id==OriginalEventId::TheArchitect);
    CHECK(a.game.original_events.values[0]==int(dialogue));return;
   }
   CHECK(world_finish_room(a.game));
  }
 }
 CHECK(false);
}
static void architect_queue(uint32_t origin,bool start,bool fast,unsigned dialogue){
 const unsigned character=unsigned(a.game.character),count=character==1||character==4?4u:5u;
 const unsigned hits=start?(character==1?4u:0u):character==1&&dialogue>=2?0u:count;
 const unsigned normal[]={100,300,500,700,900},quick[]={50,200,350,500,650};
 CHECK(a.screen_motion.count==hits+unsigned(!start)+1u);
 for(unsigned i=0;i<hits;++i){const auto&q=a.screen_motion.cues[i];CHECK(q.at==origin+(start?750u:0u)+(fast?quick[i]:normal[i]));CHECK(q.kind==Shake::Punch&&q.strength==(i+1==hits?40:5)&&q.duration==(i+1==hits?800:300));}
 const unsigned tail=hits?(start?750u:0u)+(fast?quick[hits-1]:normal[hits-1])+2000u:0u;
 if(!start){const auto&q=a.screen_motion.cues[hits];CHECK(q.at==origin+tail+(fast?250u:500u)&&q.strength==5&&q.duration==800&&q.kind==Shake::Punch);}
 {const auto&q=a.screen_motion.cues[a.screen_motion.count-1];const unsigned duration=start?(hits?tail+500u:2250u):tail+(fast?750u:1000u);CHECK(q.kind==Shake::SequenceEnd&&q.at==origin+duration);}
}
static void actual_architect(Character character,bool fast,unsigned dialogue,unsigned shake){
 scenario="Architect actual final-room entry, Proceed and terminal lifecycle";
 architect_room(character,fast,dialogue,shake);const uint32_t started=a.map_clock_ms;architect_queue(started,true,fast,dialogue);
 {
  const uint32_t end=a.screen_motion.sequence_end(a.map_clock_ms);CHECK(end>started);a.render();equal_save();
  a.input(Key::Confirm);equal_save();
  // A cached Choice region from the previous frame cannot dispatch through
  // pointer_button: that path rebuilds hits and also shares room_input.
  a.hit_count=0;a.add_hit(96,150,216,40,HitKind::Choice,0,true);a.pointer_move(180,170);a.pointer_button(true);a.pointer_button(false);equal_save();
  CHECK(a.game.phase==Phase::Event&&a.screen_motion.sequence_active(a.map_clock_ms));
  advance_room(unsigned(end-a.map_clock_ms)-1u);CHECK(a.screen_motion.sequence_active(a.map_clock_ms));a.input(Key::Confirm);equal_save();
  advance_room(1);CHECK(!a.screen_motion.sequence_active(a.map_clock_ms));equal_save();
 }
 a.pointer.visible=false;
 // Use the real keyboard path for every authored dialogue page and final
 // Proceed, waiting only for its actual original option tween.
 uint32_t ending_at=0;
 for(unsigned page=0;page<5&&a.game.phase==Phase::Event;++page){
  a.render();for(unsigned i=0;i<200&&!a.room_presentation.event.option_ready(0,a.map_clock_ms);++i)advance_room(33);
  CHECK(a.room_presentation.event.option_ready(0,a.map_clock_ms));const bool ending=a.game.original_events.finished;
  const uint32_t at=a.map_clock_ms;direct=a.game;CHECK(original_event_choose(direct,0));a.cursor=0;a.input(Key::Confirm);equal_save();
  if(ending){ending_at=at;CHECK(a.game.phase==Phase::Complete&&!a.game.original_events.active&&a.game.world.finished);architect_queue(at,false,fast,dialogue);break;}
  CHECK(a.game.phase==Phase::Event&&!a.screen_motion.sequence_active(a.map_clock_ms));
 }
 CHECK(a.game.phase==Phase::Complete);const uint32_t terminal=a.game_over_presentation.opened;
 CHECK(terminal==a.screen_motion.sequence_end(a.map_clock_ms)&&!a.game_over_presentation.initial_age(a.map_clock_ms));
 const unsigned queued=a.screen_motion.count;{UiCoreObserver observer(a);CHECK(!original_event_leave(a.game));}CHECK(a.screen_motion.count==queued);equal_save();
 a.input(Key::Confirm);CHECK(!a.game_over_summary);equal_save();
 unsigned strong=0;bool visible_frame_checked=false;for(unsigned i=0;i<200&&int32_t(terminal-a.map_clock_ms)>0;++i){
  CHECK(!a.game_over_presentation.black_alpha(a.map_clock_ms)&&!a.game_over_presentation.ready(a.map_clock_ms));
  advance_room(std::min(33u,unsigned(terminal-a.map_clock_ms)));const auto&p=a.screen_motion.punch;
  strong+=p.strength==40&&int32_t(p.at-ending_at)>=0&&a.map_clock_ms-p.at<p.duration;
  int x=0,y=0;a.screen_scene_offset(x,y);if(!shake)CHECK(!x&&!y);
  if(!visible_frame_checked&&(x||y)&&a.content.current_profile()){
   // The completed canonical event has active=false. Render the real
   // terminal room both ways at one instant, then independently translate
   // its original background to prove the effect survives that boundary.
   const auto previous=a.screen_shake;a.screen_shake=0;a.render();std::vector<uint16_t> still(a.pixels,a.pixels+320*240);
   a.screen_shake=previous;a.render();unsigned changed=0;
   for(int py=40;py<240;++py)for(int px=0;px<320;++px){
    const int sx=std::max(0,std::min(319,px-x)),sy=std::max(22,std::min(239,py-y));
    CHECK(a.pixels[py*320+px]==still[sy*320+sx]);changed+=a.pixels[py*320+px]!=still[py*320+px];
   }
   CHECK(changed>1000);visible_frame_checked=true;
  }
 }
 CHECK(a.map_clock_ms==terminal&&!a.screen_motion.sequence_active(a.map_clock_ms));
 if(shake&&a.content.current_profile())CHECK(visible_frame_checked);
 CHECK(bool(strong)==!(character==Character::Silent&&dialogue>=2));equal_save();
 advance_room(1949);CHECK(!a.game_over_button_ready());a.input(Key::Confirm);CHECK(!a.game_over_summary);equal_save();
 advance_room(1);CHECK(a.game_over_button_ready());a.input(Key::Confirm);CHECK(a.game_over_summary);equal_save();
 const auto&m=a.game_over_presentation;advance_room(m.menu_start()+1);a.ui_action(UiAction::GameOverMainMenu);CHECK(a.title&&!a.screen_motion.count);equal_save();
}
static SaveData architect_saved;
static bool load_architect(State&s){return load(s,architect_saved.bytes,architect_saved.size);}
static void architect_reload(){
 scenario="Architect actual Continue does not replay a transient source attack";
 architect_room(Character::Silent,false,0,2);CHECK(a.screen_motion.sequence_active(a.map_clock_ms));CHECK(save(a.game,architect_saved));direct=a.game;
 const auto callbacks=a.callbacks;a.callbacks.load=load_architect;a.return_to_title();CHECK(!a.screen_motion.count&&a.has_save);
 CHECK(a.load_saved_run());a.title=false;a.render();CHECK(!a.screen_motion.sequence_active(a.map_clock_ms)&&!a.screen_motion.count);equal_save();
 for(unsigned i=0;i<200&&!a.room_presentation.event.option_ready(0,a.map_clock_ms);++i)advance_room(33);
 direct=a.game;CHECK(original_event_choose(direct,0));a.input(Key::Confirm);equal_save();CHECK(a.game.phase==Phase::Complete);
 CHECK(save(a.game,architect_saved));CHECK(load(a.game,architect_saved.bytes,architect_saved.size));a.reset_presentation();a.render();CHECK(!a.screen_motion.sequence_active(a.map_clock_ms)&&!a.screen_motion.count);
 CHECK(a.game_over_presentation.opened==a.map_clock_ms&&!a.game_over_presentation.initial_age(a.map_clock_ms));equal_save();a.callbacks=callbacks;
}
static std::vector<uint8_t> read(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>b(size_t(n),0);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;}
static void actual_masks(const char*root){
 const auto common=read(std::string(root)+"/nSpire2.common.tns");
 for(const char*locale:{"eng","jpn"}){const auto language=read(std::string(root)+"/nSpire2."+locale+".tns");SplitResourceSource split;
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),!std::strcmp(locale,"jpn")?ResourceLanguage::Japanese:ResourceLanguage::English));CHECK(a.init(split.source(),{}));
  scenario="original localized Z masks retain visible centered ink and source anchor";CHECK(a.content.has_sleeping_vfx_assets());
  const int slot=encounter(EncounterId::LagavulinMatriarchBoss,EnemyId::LagavulinMatriarch);direct=a.game;Canvas c{a};
  a.animation=a.enemy_presentation[slot].sleep.start/33u+31u;std::memset(a.pixels,0,sizeof a.pixels);creature_sleep_particles(c,slot);
  unsigned ink=0;int left=320,right=0,top=240,bottom=0;for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(a.pixels[y*320+x]){++ink;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}
  CHECK(ink>5&&ink<1000&&left>80&&right<320&&top>=22&&bottom<180);equal_save();CHECK(a.content.healthy());
  actual_sleep();actual_cards();actual_doom();actual_architect(Character::Silent,false,2,2);CHECK(a.content.healthy());
 }
}
int main(int argc,char**argv){
 CHECK(argc==1||argc==2);source_equations();sleeping_lifetime();pixels_and_hits();settings_and_smith();actual_sleep();actual_cards();actual_doom();architect_equations();
 for(unsigned fast=0;fast<2;++fast)for(unsigned character=0;character<5;++character)actual_architect(Character(character),fast,0,2);
 actual_architect(Character::Silent,false,2,0);actual_architect(Character::Silent,true,2,0);architect_reload();if(argc==2)actual_masks(argv[1]);
 std::printf("screen effects UI12: %u checks passed%s\n",checks,argc==2?" with original English/Japanese masks":" (asset-independent)");
}
