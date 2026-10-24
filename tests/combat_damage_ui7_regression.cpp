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
 CHECK(!(r.flags&CoreDamageResult::FullyBlocked));
 r=enemy_damage(10,3,8).values[0];CHECK(r.blocked==3&&r.unblocked==5&&r.overkill==0&&r.amount()==5);
 CHECK((r.flags&CoreDamageResult::BlockBroken)&&!(r.flags&CoreDamageResult::FullyBlocked));
 r=enemy_damage(10,7,3).values[0];CHECK(r.blocked==3&&r.unblocked==0&&r.overkill==0&&r.amount()==0);
 CHECK((r.flags&CoreDamageResult::FullyBlocked)&&!(r.flags&CoreDamageResult::BlockBroken));
 r=enemy_damage(10,3,3).values[0];CHECK((r.flags&(CoreDamageResult::FullyBlocked|CoreDamageResult::BlockBroken))==(CoreDamageResult::FullyBlocked|CoreDamageResult::BlockBroken));
 r=enemy_damage(10,0,0).values[0];CHECK(!r.amount()&&(r.flags&CoreDamageResult::ShowZero)&&!(r.flags&CoreDamageResult::FullyBlocked));
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
  const auto&d=app.combat_status.damages[0];CHECK(d.actor==1&&d.amount==(kind==0?20:0));
  CHECK(bool(d.flags&CoreDamageResult::FullyBlocked)==(kind==1||kind==2));
  CHECK(bool(d.flags&CoreDamageResult::ShowZero)==(kind==3));
  for(unsigned i=0;i<62;++i){app.advance(33);app.render();}CHECK(!app.combat_status.damage_count);save_equal();
 }
 setup();app.game.expansion.powers[int(XPower::Intangible)]=1;app.reset_presentation();app.render();direct=app.game;
 player_hp_loss(direct,10,true,true,-1,0);resolve_pending(direct);
 {UiCoreObserver observer(app);app.begin_combat_feedback();player_hp_loss(app.game,10,true,true,-1,0);resolve_pending(app.game);}
 app.render();save_equal();CHECK(app.combat_status.flash_count==1&&app.combat_status.flashes[0].id==21);
 app.advance(1056);app.render();CHECK(!app.combat_status.flash_count);save_equal();
}
static void block_break(){
 HealthBarMotion m;m.observe(1,10,10,58,1000);m.observe_block(4,1000,true);CHECK(!m.block_breaking);
 m.observe_block(0,1033);CHECK(m.block_breaking&&m.block_break_ms==1033);m.observe_block(0,1100);CHECK(m.block_break_ms==1033);
 CHECK(HealthBarMotion::block_break_progress(0)==0&&HealthBarMotion::block_break_progress(200)==2048&&HealthBarMotion::block_break_progress(400)==4096);
 CHECK(HealthBarMotion::block_break_progress(300,true)==2048&&HealthBarMotion::block_break_progress(600,true)==4096);
 m.observe_block(0,1633);CHECK(!m.block_breaking);m.observe_block(3,1700,true);m.observe_block(0,1701,true);CHECK(!m.block_breaking);
}
int main(int argc,char**argv){
 std::vector<uint8_t> data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);CHECK(app.init(data.data(),data.size(),{}));}
 exact_results();pet_and_powers();presentation();block_break();std::printf("damage UI7: %u checks passed\n",checks);
}
