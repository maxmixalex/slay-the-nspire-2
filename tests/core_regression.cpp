#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static unsigned checks=0,transitions=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State combat(std::initializer_list<Card> hand, int opponents=1){
    State s{};new_run(s,7,false);s.phase=Phase::Combat;s.floor=1;s.turn=1;
    s.enemy_count=(uint8_t)opponents;s.energy=3;
    s.deck_count=s.card_count=0;s.hand.count=s.draw.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    for(Card card:hand){unsigned id=s.card_count++;s.cards[id]=card;s.deck[s.deck_count++]=card;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=(uint16_t)id;}
    for(int i=0;i<opponents;++i)s.enemies[i]={46,46,0,0,0,0};
    CHECK(validate(s));return s;
}
static void supply(State& s,int n){
    for(int i=0;i<n;i++){unsigned id=s.card_count++;s.cards[id]={Defend,0};s.zones[id]=Zone::Draw;s.draw.ids[s.draw.count++]=(uint16_t)id;}
    CHECK(validate(s));
}
static void exhaust_powers(State& s){s.pain=3;s.embrace=1;s.power_order[0]=0;s.power_order[1]=1;s.power_count=2;}
static int find(const State&s,unsigned kind){for(int i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==kind)return i;return -1;}
static void roundtrip(State& s){
    SaveData a{},b{};CHECK(validate(s));CHECK(save(s,a));CHECK(a.bytes[4]==4);
    State copy{};CHECK(load(copy,a.bytes,a.size));CHECK(save(copy,b));CHECK(a.size==b.size);
    CHECK(!std::memcmp(a.bytes,b.bytes,a.size));
    a.bytes[a.size-1]^=1;State before=copy;CHECK(!load(copy,a.bytes,a.size));CHECK(!std::memcmp(&before,&copy,sizeof(State)));
}
static void scalar_cards(){
    for(unsigned upgraded=0;upgraded<2;upgraded++){
        Card c{Bloodletting,(uint8_t)upgraded};auto s=combat({c});s.block=20;CHECK(play(s,0,0));
        CHECK(s.hp==77&&s.block==20&&s.energy==5+(int)upgraded);CHECK(s.discard.count==1&&s.exhaust.count==0);roundtrip(s);
        s=combat({{Hemokinesis,(uint8_t)upgraded}});s.block=19;s.strength=2;s.enemies[0].vulnerable=1;
        int expected=(15+5*(int)upgraded+2)*3/2;CHECK(damage(s,s.cards[0],0)==expected);CHECK(play(s,0,0));
        CHECK(s.hp==78&&s.block==19&&s.enemies[0].hp==46-expected&&s.energy==2);roundtrip(s);
        s=combat({{Impervious,(uint8_t)upgraded}});s.dexterity=2;exhaust_powers(s);supply(s,3);CHECK(play(s,0,0));
        CHECK(s.block==35+10*(int)upgraded&&s.energy==1&&s.exhaust.count==1&&s.hand.count==1);roundtrip(s);
        s=combat({{Offering,(uint8_t)upgraded},{Strike,0}});exhaust_powers(s);supply(s,10);CHECK(play(s,0,0));
        CHECK(s.hp==74&&s.energy==5&&s.hand.count==5+2*upgraded&&s.exhaust.count==1&&s.block==3);roundtrip(s);
        s=combat({{Bludgeon,(uint8_t)upgraded}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==14-10*(int)upgraded&&s.energy==0);roundtrip(s);
        s=combat({{Thunderclap,(uint8_t)upgraded}},2);s.enemies[1].vulnerable=1;CHECK(play(s,0,-1));
        CHECK(s.enemies[0].hp==42-3*(int)upgraded);CHECK(s.enemies[1].hp==46-(4+3*(int)upgraded)*3/2);
        CHECK(s.enemies[0].vulnerable==1&&s.enemies[1].vulnerable==2);roundtrip(s);
        s=combat({{Uppercut,(uint8_t)upgraded}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==33);
        CHECK(s.enemies[0].weak==1+(int)upgraded&&s.enemies[0].vulnerable==1+(int)upgraded);
        CHECK(enemy_damage(s.enemies[0])==9);CHECK(end_turn(s));CHECK(s.hp==71);CHECK(s.enemies[0].weak==(int)upgraded);roundtrip(s);
        s=combat({{BodySlam,(uint8_t)upgraded}});s.block=12;s.strength=2;s.enemies[0].vulnerable=1;
        CHECK(cost(s.cards[0])==1-(int)upgraded);CHECK(damage(s,s.cards[0],0)==21);CHECK(play(s,0,0));
        CHECK(s.enemies[0].hp==25&&s.block==12&&s.energy==2+(int)upgraded);roundtrip(s);
        s=combat({{BattleTrance,(uint8_t)upgraded},{ShrugItOff,0}});supply(s,12);CHECK(play(s,0,0));
        CHECK(s.no_draw==1&&s.hand.count==4+upgraded);unsigned before=s.draw.count;
        CHECK(play(s,find(s,ShrugItOff),0));CHECK(s.draw.count==before);roundtrip(s);
        CHECK(end_turn(s));CHECK(s.no_draw==0&&s.hand.count==5);roundtrip(s);
        s=combat({{FiendFire,(uint8_t)upgraded},{Defend,0},{Strike,0}});exhaust_powers(s);supply(s,8);CHECK(play(s,0,0));
        CHECK(s.enemies[0].hp==32-6*(int)upgraded);CHECK(s.exhaust.count==3&&s.hand.count==3&&s.block==9&&s.energy==1);roundtrip(s);
    }
    auto s=combat({{BodySlam,0}});s.strength=2;CHECK(damage(s,s.cards[0],0)==2);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==44);
    s=combat({{FiendFire,0},{Defend,0},{Strike,0}});CHECK(gain_relic(s,PenNib));s.relic_counters[9]=9;s.enemies[0].vulnerable=1;
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==4&&relic_counter(s,PenNib)==0&&s.double_attack==0);roundtrip(s);
    s=combat({{FiendFire,0}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==46&&s.exhaust.count==1);roundtrip(s);
    s=combat({{Offering,0}});s.hp=6;s.block=50;supply(s,10);CHECK(play(s,0,0));CHECK(s.phase==Phase::Dead&&s.hp==0&&s.energy==3&&s.hand.count==0&&s.draw.count==10);roundtrip(s);
    s=combat({{Bloodletting,0}});s.hp=3;CHECK(play(s,0,0));CHECK(s.phase==Phase::Dead&&s.energy==3);roundtrip(s);
    s=combat({{BattleTrance,0},{Impervious,0}});exhaust_powers(s);supply(s,10);CHECK(play(s,0,0));unsigned n=s.draw.count;
    CHECK(play(s,find(s,Impervious),0));CHECK(s.draw.count==n&&s.block==33&&s.no_draw);roundtrip(s);
    s=combat({{Whirlwind,0}},2);s.energy=999;CHECK(play(s,0,-1));CHECK(s.phase==Phase::Reward&&s.effect_count==0&&s.playing==-1);roundtrip(s);
}
static void old_save(const char* path){
    FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t> bytes;unsigned v;
    while(std::fscanf(f,"%2x",&v)==1)bytes.push_back((uint8_t)v);std::fclose(f);
    CHECK(bytes.size()>1000&&bytes[4]==3);State s{};CHECK(load(s,bytes.data(),bytes.size()));
    CHECK(s.phase==Phase::ExhaustChoice&&s.pain==3&&s.embrace==1&&s.effect_count==2);
    CHECK(s.no_draw==0&&s.enemies[0].weak==0);roundtrip(s);CHECK(choose_exhaust(s,0));roundtrip(s);
}
static void stress(){
    uint32_t r=0x93ca418u;
    for(unsigned seed=1;seed<=600;seed++){
        State s{};new_run(s,seed,false);
        s.deck_count=24;for(unsigned i=0;i<24;i++)s.deck[i]={(uint8_t)i,(uint8_t)(seed%2)};
        s.encounter=(uint8_t)(seed%2);enter_combat(s);s.hp=s.max_hp=999;
        for(unsigned j=0;j<180&&s.phase!=Phase::Reward&&s.phase!=Phase::Dead;j++){
            CHECK(validate(s));
            if(s.phase==Phase::ExhaustChoice){CHECK(choose_exhaust(s,random_below(r,s.hand.count)));}
            else if(s.hand.count&&random_below(r,5)){
                unsigned h=random_below(r,s.hand.count);int t=s.enemy_count==2&&s.enemies[1].hp>0?1:0;
                if(!play(s,h,t))CHECK(end_turn(s));
            }else CHECK(end_turn(s));
            ++transitions;roundtrip(s);
        }
        CHECK(validate(s));
    }
}
int main(int argc,char**argv){scalar_cards();if(argc>1)old_save(argv[1]);stress();std::printf("core: %u checks, %u generated transitions passed\n",checks,transitions);}
