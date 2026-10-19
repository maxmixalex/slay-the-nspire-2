#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static_assert(SaveVersion==13&&CardKinds==228&&CharacterKinds==2,"Use the unchanged 7f486bdf v13 engine");
static void check(bool x,const char*what){if(!x){std::fprintf(stderr,"fixture producer failed: %s\n",what);std::exit(1);}}
static int add(State&s,Card c,Zone zone){int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:s.discard;p.ids[p.count++]=(uint16_t)id;return id;}
static int hand(const State&s,int id){for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]==id)return (int)i;return -1;}
static void cast(State&s,Card c){int id=add(s,c,Zone::Hand);check(play(s,hand(s,id),0),"play");check(validate(s),"post-play validation");}
static void write_hex(const char*path,const SaveData&b){FILE*f=std::fopen(path,"w");check(f,"open hex");for(size_t i=0;i<b.size;i++)std::fprintf(f,"%02x%s",b.bytes[i],(i+1)%32?"":"\n");if(b.size%32)std::fputc('\n',f);check(!std::fclose(f),"close hex");}
int main(int argc,char**argv){
 check(argc==3,"output hex and summary paths");State s{};new_run(s,0x16a13,false,Character::Silent);
 s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;s.hp=s.max_hp=70;s.energy=30;
 s.relics=s.relic_count=s.relic_flash=0;s.deck_count=2;s.deck[0]={DefendSilent,0};s.deck[1]={UpMySleeve,1};
 for(auto&e:s.enemies)e={46,46,0,0,0,2};
 cast(s,{Afterimage,0});cast(s,{SerpentForm,0});
 int target=add(s,{UpMySleeve,1},Zone::Hand);s.silent.discount[target]=2;s.silent.retain[target]=1;s.silent.sly[target]=1;
 cast(s,{Nightmare,0});check(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::NightmareCard,"Nightmare pause");
 uint16_t choice=(uint16_t)target;check(choose_extended(s,&choice,1),"Nightmare snapshot");check(s.silent.nightmare_active[0],"active delayed clone");
 int returned=add(s,{Anger,0},Zone::Discard);for(int i=0;i<8;i++)add(s,{DefendSilent,0},Zone::Draw);
 int headbutt=add(s,{Headbutt,1},Zone::Draw);int havoc=add(s,{Havoc,0},Zone::Hand);check(play(s,hand(s,havoc),0),"nested Havoc");
 check(s.phase==Phase::ReturnChoice&&s.playing==headbutt&&s.expansion.depth==1&&s.expansion.frames[0].playing==havoc,"nested historical pause");
 check(validate(s),"paused validation");SaveData paused{};check(save(s,paused),"v13 write");write_hex(argv[1],paused);
 check(choose_return_to_draw(s,returned),"resume Headbutt");check(s.phase==Phase::Combat&&s.playing==-1&&!s.effect_count&&validate(s),"resumed state");
 FILE*f=std::fopen(argv[2],"w");check(f,"open summary");
 std::fprintf(f,"{\n  \"writer_commit\": \"7f486bdf757fb9af9dff89cdc8a785f049a404fc\",\n  \"save_version\": 13,\n  \"bytes\": %zu,\n  \"ids\": {\"target\": %d, \"returned\": %d, \"headbutt\": %d, \"havoc\": %d},\n  \"resumed\": {\"hp\": %d, \"energy\": %d, \"block\": %d, \"enemy_hp\": [%d, %d], \"hand\": %u, \"draw\": %u, \"discard\": %u, \"exhaust\": %u, \"cards_finished\": %u, \"attacks_finished\": %u, \"skills_finished\": %u, \"nightmare_active\": %u}\n}\n",paused.size,target,returned,headbutt,havoc,s.hp,s.energy,s.block,s.enemies[0].hp,s.enemies[1].hp,s.hand.count,s.draw.count,s.discard.count,s.exhaust.count,s.expansion.cards_finished,s.silent.attacks_finished,s.silent.skills_finished,s.silent.nightmare_active[0]);
 check(!std::fclose(f),"close summary");std::printf("unchanged v13 writer: %zu bytes, nested headbutt=%d havoc=%d, resumed hp=%d/%d block=%d energy=%d\n",paused.size,headbutt,havoc,s.enemies[0].hp,s.enemies[1].hp,s.block,s.energy);
}
