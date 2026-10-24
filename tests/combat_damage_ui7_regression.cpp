#include "app.hpp"
#include "ui_core_observer.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static State direct;
static SaveData expected,actual;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"damage UI7:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Results {
 CoreDamageResult values[32]{};unsigned count=0,flashes=0;const char*last_power=nullptr;
 static void damage(void*context,const State&,const CoreDamageResult&r){auto&v=*static_cast<Results*>(context);CHECK(v.count<32);v.values[v.count++]=r;}
 static void power(void*context,const State&,int,const char*key){auto&v=*static_cast<Results*>(context);++v.flashes;v.last_power=key;}
};
static void setup(Character character=Character::Ironclad){
 new_run(app.game,0x41cef1ea,false,character);auto&s=app.game;s.phase=Phase::Combat;s.current_node=1;s.floor=s.turn=1;s.hp=s.max_hp=999;
 CHECK(encounter_setup(s,EncounterId::NibbitsNormal));CHECK(s.enemy_count>1);
 for(unsigned i=0;i<s.deck_count;++i){int id=s.card_count;queue_extension(s,XOp::EnemyGenerateCard,s.deck[i].kind,1);resolve_pending(s);CHECK(s.card_count==id+1);notify_deck_card_created(s,id,i);}
 draw_cards(s,5);app.title=app.paused=app.library=app.help=app.character_select=false;app.reset_presentation();app.render();
 for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
 CHECK(!app.combat_preparation.active&&validate(s));
}
static Results enemy_damage(int hp,int block,int amount){
 setup();auto&s=app.game;s.enemies[0].hp=hp;s.enemies[0].max_hp=99;s.enemies[0].block=block;
 Results out;const auto previous=set_core_observer({&out,nullptr,nullptr,nullptr,nullptr,Results::damage,Results::power});
 damage_unpowered(s,0,amount);resolve_pending(s);set_core_observer(previous);CHECK(out.count==1);CHECK(validate(s));return out;
}
static void exact_results(){
 auto r=enemy_damage(7,0,20).values[0];CHECK(r.receiver==0&&r.blocked==0&&r.unblocked==7&&r.overkill==13&&r.amount()==20);
 CHECK(!(r.flags&CoreDamageResult::FullyBlocked)&&r.shake_strength==20);
 r=enemy_damage(10,3,8).values[0];CHECK(r.blocked==3&&r.unblocked==5&&r.overkill==0&&r.amount()==5);
 CHECK((r.flags&CoreDamageResult::BlockBroken)&&!(r.flags&CoreDamageResult::FullyBlocked)&&r.shake_strength==5);
 r=enemy_damage(10,7,3).values[0];CHECK(r.blocked==3&&r.unblocked==0&&r.overkill==0&&r.amount()==0);
 CHECK((r.flags&CoreDamageResult::FullyBlocked)&&!(r.flags&CoreDamageResult::BlockBroken)&&r.shake_strength==5);
 r=enemy_damage(10,3,3).values[0];CHECK((r.flags&(CoreDamageResult::FullyBlocked|CoreDamageResult::BlockBroken))==(CoreDamageResult::FullyBlocked|CoreDamageResult::BlockBroken));
 r=enemy_damage(10,0,0).values[0];CHECK(!r.amount()&&(r.flags&CoreDamageResult::ShowZero)&&!(r.flags&CoreDamageResult::FullyBlocked)&&!r.shake_strength);
 r=enemy_damage(50,0,20).values[0];CHECK(r.shake_strength==40);
 // NDamageNumVfx is not emitted for Buffer's prevented nonzero hit.
 setup();app.game.expansion.powers[int(XPower::Buffer)]=1;Results out;
 auto old=set_core_observer({&out,nullptr,nullptr,nullptr,nullptr,Results::damage,Results::power});
 player_hp_loss(app.game,9,true,true,-1,0);resolve_pending(app.game);set_core_observer(old);
 CHECK(out.count==1&&!out.values[0].amount()&&!(out.values[0].flags&(CoreDamageResult::FullyBlocked|CoreDamageResult::ShowZero)));
 CHECK(app.game.hp==999&&validate(app.game));
 // Force-kill has no DamageResult and must not invent a 999 damage number.
 setup();out={};old=set_core_observer({&out,nullptr,nullptr,nullptr,nullptr,Results::damage,Results::power});
 player_kill(app.game,true);resolve_pending(app.game);set_core_observer(old);CHECK(!out.count&&app.game.phase==Phase::Dead);
}
static void pet_and_powers(){
 setup(Character::Necrobinder);auto&s=app.game;s.expansion.osty_hp=3;s.expansion.osty_max_hp=8;s.block=2;
 Results out;auto old=set_core_observer({&out,nullptr,nullptr,nullptr,nullptr,Results::damage,Results::power});
 player_hp_loss(s,10,true,true,-1,0);resolve_pending(s);set_core_observer(old);
 CHECK(out.count==2);const auto&pet=out.values[0];const auto&player=out.values[1];
 CHECK(pet.receiver==-2&&pet.blocked==0&&pet.unblocked==3&&pet.overkill==5&&pet.amount()==8);
 CHECK(player.receiver==-1&&player.blocked==2&&player.unblocked==5&&player.overkill==0&&!(player.flags&CoreDamageResult::FullyBlocked));
 CHECK(s.hp==994&&!s.expansion.osty_hp&&validate(s));
 setup();app.game.expansion.powers[int(XPower::Intangible)]=1;out={};
 old=set_core_observer({&out,nullptr,nullptr,nullptr,nullptr,Results::damage,Results::power});
 player_hp_loss(app.game,10,true,true,-1,0);resolve_pending(app.game);set_core_observer(old);
 CHECK(out.count==1&&out.values[0].amount()==1&&out.flashes==1&&!std::strcmp(out.last_power,"INTANGIBLE_POWER"));
}
static void save_equal(){CHECK(save(direct,expected)&&save(app.game,actual));CHECK(expected.size==actual.size&&!std::memcmp(expected.bytes,actual.bytes,expected.size));}
static void presentation(){
 // Actual core calls with and without the display observer produce the same
 // serialized run/RNG. Only the original result, not the HP delta, is queued.
 for(unsigned kind=0;kind<4;++kind){
  setup();auto&s=app.game;s.enemies[0].hp=7;s.enemies[0].max_hp=99;s.enemies[0].block=kind==1?9:kind==2?4:0;
  const int amount=kind==3?0:kind==0?20:4;app.reset_presentation();app.render();direct=s;
  damage_unpowered(direct,0,amount);resolve_pending(direct);
  {UiCoreObserver observer(app);app.begin_combat_feedback();damage_unpowered(s,0,amount);resolve_pending(s);}
  app.render();save_equal();CHECK(app.combat_status.damage_count==1);
  CHECK(app.combat_sparks.count==(kind==3?0:1));
  if(kind!=3)CHECK(bool(app.combat_sparks.bursts[0].blocked)==(kind==1||kind==2));
  const auto&d=app.combat_status.damages[0];CHECK(d.actor==1&&d.amount==(kind==0?20:0));
  CHECK(bool(d.flags&CoreDamageResult::FullyBlocked)==(kind==1||kind==2));
  CHECK(bool(d.flags&CoreDamageResult::ShowZero)==(kind==3));
  for(unsigned i=0;i<62;++i){app.advance(33);app.render();}CHECK(!app.combat_status.damage_count&&!app.combat_sparks.count);save_equal();
 }
 setup();app.game.expansion.powers[int(XPower::Intangible)]=1;app.reset_presentation();app.render();direct=app.game;
 player_hp_loss(direct,10,true,true,-1,0);resolve_pending(direct);
 {UiCoreObserver observer(app);app.begin_combat_feedback();player_hp_loss(app.game,10,true,true,-1,0);resolve_pending(app.game);}
 app.render();save_equal();CHECK(app.combat_status.flash_count==1&&app.combat_status.flashes[0].id==21);
 // Host updates deliberately clamp stalls to250ms; exercise the real tick
 // clock rather than asking one frame to fast-forward a full VFX lifetime.
 for(unsigned i=0;i<32;++i){app.advance(33);app.render();}
 CHECK(!app.combat_status.flash_count);save_equal();
}
static void screen_shake(){
 setup();app.game.enemies[0].hp=app.game.enemies[0].max_hp=99;app.reset_presentation();app.render();direct=app.game;
 damage_unpowered(direct,0,20);resolve_pending(direct);
 {UiCoreObserver observer(app);app.begin_combat_feedback();damage_unpowered(app.game,0,20);resolve_pending(app.game);}
 app.render();CHECK(app.combat_status.damage_count==1&&app.combat_status.damages[0].shake_strength==40);
 // Keep the cosmetic direction horizontal so the settings' projected
 // amplitudes can be compared without depending on gameplay random state.
 app.combat_status.damages[0].angle=0;int x=0,y=0;
 app.screen_shake=0;app.combat_scene_offset(x,y);CHECK(x==0&&y==0);
 int widths[5]{};for(unsigned i=1;i<5;++i){app.screen_shake=uint8_t(i);app.combat_scene_offset(x,y);CHECK(y==0);widths[i]=x<0?-x:x;}
 CHECK(widths[1]>0&&widths[2]>widths[1]&&widths[3]>widths[2]&&widths[4]>widths[3]);
 app.screen_shake=2;for(unsigned i=0;i<10;++i){app.advance(33);app.render();}
 app.combat_scene_offset(x,y);CHECK(x==0&&y==0);save_equal();
}
static void block_break(){
 HealthBarMotion m;m.observe(1,10,10,58,1000);m.observe_block(4,1000,true);CHECK(!m.block_breaking);
 m.observe_block(0,1033);CHECK(m.block_breaking&&m.block_break_ms==1033);m.observe_block(0,1100);CHECK(m.block_break_ms==1033);
 CHECK(HealthBarMotion::block_break_progress(0)==0&&HealthBarMotion::block_break_progress(200)==2048&&HealthBarMotion::block_break_progress(400)==4096);
 CHECK(HealthBarMotion::block_break_progress(300,true)==2048&&HealthBarMotion::block_break_progress(600,true)==4096);
 m.observe_block(0,1633);CHECK(!m.block_breaking);m.observe_block(3,1700,true);m.observe_block(0,1701,true);CHECK(!m.block_breaking);
}
static void equal_sparks(const CombatSparkMotion&a,const CombatSparkMotion&b){
 // Cosmetic structs contain alignment padding. Compare all live display
 // fields; their uninitialized padding has no renderer or save semantics.
 CHECK(a.count==b.count&&a.clock==b.clock);
 for(unsigned i=0;i<a.count;++i){const auto&x=a.bursts[i];const auto&y=b.bursts[i];
  CHECK(x.at==y.at&&x.seed==y.seed&&x.x==y.x&&x.y==y.y&&x.stepped==y.stepped&&x.steps==y.steps&&x.blocked==y.blocked);
  for(unsigned j=0;j<CombatSparkMotion::Particles;++j){const auto&p=x.particles[j];const auto&q=y.particles[j];
   CHECK(p.distance==q.distance&&p.speed==q.speed&&p.radial==q.radial&&p.damping==q.damping&&p.scale==q.scale&&p.lifetime==q.lifetime&&p.angle==q.angle&&p.variation==q.variation);
  }
 }
}
static void sparks(){
 CombatSparkMotion stepped,skipped;
 for(unsigned i=0;i<3;++i){stepped.emit(100,50+int(i)*40,90,i==1,0x41cef1eau+i);skipped.emit(100,50+int(i)*40,90,i==1,0x41cef1eau+i);}
 CHECK(stepped.count==3&&skipped.count==3);
 for(const auto&p:stepped.bursts[0].particles){CHECK(p.lifetime>=200&&p.lifetime<=1000);CHECK(p.scale>=307&&p.scale<=461);}
 for(unsigned now=100;now<600;now+=17)stepped.step(now);stepped.step(600);skipped.step(600);
 // The authored material is integrated on its own60Hz boundaries. Host
 // rendering cadence cannot alter its emitted paths or display random seed.
 equal_sparks(stepped,skipped);
 const auto before=stepped;stepped.step(600);equal_sparks(before,stepped);
 CHECK(stepped.bursts[0].particles[0].distance>0);stepped.step(1099);CHECK(stepped.count==3);stepped.step(1100);CHECK(!stepped.count);
 for(unsigned i=0;i<CombatSparkMotion::Capacity+3;++i)stepped.emit(i,20,30,false,i+1);
 CHECK(stepped.count==CombatSparkMotion::Capacity&&stepped.bursts[0].at==3);
 stepped.reset();CHECK(!stepped.count);
}
int main(int argc,char**argv){
 std::vector<uint8_t> data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);CHECK(app.init(data.data(),data.size(),{}));}
 exact_results();pet_and_powers();presentation();block_break();screen_shake();sparks();std::printf("damage UI7: %u checks passed\n",checks);
}
