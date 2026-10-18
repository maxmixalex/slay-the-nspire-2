#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x); std::exit(1); } } while (0)
static SaveData snapshot(const State&s){SaveData b{};CHECK(validate(s));CHECK(save(s,b));CHECK(b.bytes[4]==SaveVersion);return b;}
static void same(const State&s,const SaveData&b){auto a=snapshot(s);CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,b.size));}
static void roundtrip(const State&s){auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));same(copy,b);}
static int total_hp(const State&s){int n=0;for(unsigned i=0;i<s.enemy_count;i++)n+=s.enemies[i].hp;return n;}

static void genuine_v12(const char*path){
 // Produced by the unchanged b4b67e71 v12 writer, not by the v13 codec.
 // Entropy has suspended before a later Rolling Boulder and TurnRelics.
 FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t> bytes;unsigned value;
 while(std::fscanf(f,"%2x",&value)==1)bytes.push_back((uint8_t)value);std::fclose(f);
 CHECK(bytes.size()==12756&&bytes[4]==12);
 State s{};CHECK(load(s,bytes.data(),bytes.size()));
 CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Transform);
 CHECK(s.expansion.entropy_boulder_prefix==0&&s.expansion.boulder[0]==5&&total_hp(s)==92);
 CHECK(s.effect_count==2&&s.effects[0].target==(int)XOp::BoulderTick&&s.effects[1].target==(int)XOp::TurnRelics);
 CHECK(!s.ironclad.power_count&&!s.ironclad.attacks_started&&!s.silent.power_count&&!s.silent.drawn_combat);
 auto migrated=snapshot(s);CHECK(migrated.size>bytes.size());
 CHECK(!std::memcmp(migrated.bytes+8,bytes.data()+8,bytes.size()-12));
 State copy{};CHECK(load(copy,migrated.bytes,migrated.size));
 uint16_t selected=s.expansion.choices.ids[0];
 CHECK(choose_extended(s,&selected,1)&&choose_extended(copy,&selected,1));
 CHECK(s.phase==Phase::Combat&&s.playing==-1&&!s.effect_count&&total_hp(s)==82&&s.expansion.boulder[0]==10);
 same(copy,snapshot(s));roundtrip(s);
 // Invalid checksums and truncated historical payloads must not replace a run.
 State protected_state=s;auto before=snapshot(protected_state);bytes[20]^=1;
 CHECK(!load(protected_state,bytes.data(),bytes.size()));same(protected_state,before);
 bytes[20]^=1;CHECK(!load(protected_state,bytes.data(),bytes.size()-4));same(protected_state,before);
}

static void capacity_bound(){
 // Exercise every variable-length allocation together, including suspended
 // nested plays, a full queue, maximum permanent deck and all 512 instances.
 State s{};new_run(s,0x41cef1ea,false);s.phase=Phase::ExtendedChoice;s.floor=s.turn=1;s.enemy_count=2;s.current_node=0;
 s.relics=s.relic_count=s.relic_flash=0;s.deck_count=MaxDeck;s.card_count=MaxCards;
 for(unsigned i=0;i<s.deck_count;i++)s.deck[i]={(uint8_t)(i%CardKinds),0};
 for(auto&e:s.enemies)e={46,46,0,0,0,2};
 s.playing=MaxCards-1;s.cards[s.playing]={SecretWeapon,0};s.zones[s.playing]=Zone::Limbo;
 auto&x=s.expansion;x.depth=MaxAutoFrames;x.choice=XChoice::Draw;x.choice_min=x.choice_max=1;
 const unsigned remaining=MaxCards-MaxAutoFrames-1;
 for(unsigned i=0;i<remaining;i++){
  s.cards[i]={(uint8_t)(i%CardKinds),(uint8_t)(i%2)};s.zones[i]=Zone::Draw;
  s.draw.ids[s.draw.count++]=(uint16_t)i;x.choices.ids[x.choices.count++]=(uint16_t)i;
 }
 for(unsigned i=0;i<MaxAutoFrames;i++){
  unsigned id=remaining+i;s.cards[id]={Havoc,0};s.zones[id]=Zone::Limbo;x.frames[i].playing=(int16_t)id;
 }
 s.effect_count=MaxEffects;for(auto&e:s.effects)e={EffectKind::Extended,0,(int16_t)XOp::Noop,0};
 s.log_count=24;for(auto&l:s.logs)l={LogKind::Turn,1,0};
 auto b=snapshot(s);roundtrip(s);
 // The only unsaturated variable field is the mutually exclusive choice
 // pile: at most the 13 Limbo instances could add 4 bytes apiece.
 CHECK(b.size+4*(MaxAutoFrames+1)<=sizeof(b.bytes));
 std::printf("v13 bounded save: %zu bytes; conservative ceiling %zu / %zu\n",b.size,b.size+4*(MaxAutoFrames+1),sizeof(b.bytes));
 auto invalid=s;invalid.expansion.depth=MaxAutoFrames+1;CHECK(!validate(invalid));
 invalid=s;invalid.silent.inky[0]=1;CHECK(!validate(invalid));
 invalid=s;invalid.effects[0]={EffectKind::Extended,MaxCards,(int16_t)XOp::SKnifeShiv,0};CHECK(!validate(invalid));
 invalid=s;invalid.effects[0]={EffectKind::Extended,MaxCards,(int16_t)XOp::IIncreaseDamage,1};CHECK(!validate(invalid));
}

int main(int argc,char**argv){CHECK(argc==2);genuine_v12(argv[1]);capacity_bound();std::printf("character integration: %u checks\n",checks);}
