#include "app.hpp"
#include "status.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static unsigned checks=0,transitions=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State fight(std::initializer_list<Card> cards,int enemies=1){
    State s{};new_run(s,31,false);s.phase=Phase::Combat;s.turn=1;s.floor=1;s.energy=9;s.enemy_count=(uint8_t)enemies;s.current_node=1;
    s.deck_count=s.card_count=0;
    for(auto card:cards){int id=s.card_count++;s.deck[s.deck_count++]=card;s.cards[id]=card;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=(uint16_t)id;}
    for(int i=0;i<enemies;i++)s.enemies[i]={46,46,0,0,0,0};CHECK(validate(s));return s;
}
static void add(State&s,int n,Zone zone=Zone::Draw,Card card={Defend,0}){
    Pile& pile=zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:s.exhaust;
    for(int i=0;i<n;i++){int id=s.card_count++;s.cards[id]=card;s.zones[id]=zone;pile.ids[pile.count++]=(uint16_t)id;}CHECK(validate(s));
}
static void rt(const State&s){SaveData b{},out{};State copy{};CHECK(validate(s));CHECK(save(s,b)&&b.bytes[4]==11);CHECK(load(copy,b.bytes,b.size));CHECK(save(copy,out));CHECK(b.size==out.size&&!std::memcmp(b.bytes,out.bytes,b.size));}
static void equal(const State&a,const State&b){SaveData x{},y{};CHECK(save(a,x)&&save(b,y));CHECK(x.size==y.size&&!std::memcmp(x.bytes,y.bytes,x.size));}
static void cards(){
    for(unsigned up=0;up<2;up++){
        uint8_t u=(uint8_t)up;
        auto s=fight({{Headbutt,u}});add(s,27,Zone::Discard);int id=s.discard.ids[21];uint32_t rng=s.rng_cards;
        CHECK(play(s,0,0));CHECK(s.phase==Phase::ReturnChoice&&s.hand.count==0&&s.enemies[0].hp==37-3*(int)up);rt(s);
        CHECK(&choice_pile(s)==&s.discard);State before=s;CHECK(!choose_return_to_draw(s,0));CHECK(!choose_return_to_draw(s,-1));CHECK(!end_turn(s));equal(s,before);
        CHECK(choose_return_to_draw(s,id));CHECK(s.phase==Phase::Combat&&s.draw.ids[s.draw.count-1]==id&&s.rng_cards==rng);
        CHECK(s.discard.count==27&&s.zones[0]==Zone::Discard);draw_cards(s,1);CHECK(s.hand.ids[0]==id);rt(s);
        s=fight({{Headbutt,u}});CHECK(play(s,0,0));CHECK(s.phase==Phase::Combat&&s.discard.count==1);rt(s);
        s=fight({{Headbutt,u}});add(s,3,Zone::Discard);s.enemies[0].hp=1;CHECK(play(s,0,0));CHECK(s.phase==Phase::Reward);rt(s);
        s=fight({{SwordBoomerang,u}});CHECK(play(s,0,-1));CHECK(s.enemies[0].hp==46-3*(3+(int)up));rt(s);
        s=fight({{SwordBoomerang,u}},2);s.strength=1;auto copy=s;CHECK(play(s,0,-1)&&play(copy,0,-1));CHECK(s.enemies[0].hp+s.enemies[1].hp==92-4*(3+(int)up));equal(s,copy);
        s=fight({{SwordBoomerang,u}});s.enemies[0].hp=1;rng=s.rng_cards;random_below(rng,1);CHECK(play(s,0,-1));CHECK(s.phase==Phase::Reward&&s.rng_cards==rng);rt(s);
        s=fight({{PerfectedStrike,u},{Strike,0},{TwinStrike,0},{Anger,0}});add(s,1,Zone::Discard,{PommelStrike,0});add(s,1,Zone::Exhaust,{StrikeSilent,0});
        CHECK(card_base_damage(s,s.cards[0])==6+5*(2+(int)up));int amount=damage(s,s.cards[0],0);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==46-amount);rt(s);
        s=fight({{FlameBarrier,u}},2);s.strength=20;s.enemies[0].vulnerable=1;CHECK(play(s,0,0));CHECK(s.block==12+4*(int)up&&s.flame_barrier==4+2*(int)up);rt(s);
        CHECK(end_turn(s));CHECK(s.flame_barrier==0&&s.enemies[0].hp==42-2*(int)up&&s.enemies[1].hp==42-2*(int)up);rt(s);
        s=fight({{DemonForm,u}});add(s,20);CHECK(play(s,0,0));CHECK(s.demon_form==3+(int)up&&s.strength==0&&s.powers.count==1);
        CHECK(end_turn(s));CHECK(s.strength==3+(int)up);CHECK(end_turn(s));CHECK(s.strength==6+2*(int)up);rt(s);
        s=fight({{DodgeAndRoll,u}});s.dexterity=5;add(s,10);CHECK(play(s,0,0));CHECK(s.block==9+2*(int)up&&s.block_next_turn==s.block);s.dexterity=9;s.no_block=2;rt(s);
        CHECK(end_turn(s));CHECK(s.block==9+2*(int)up&&s.block_next_turn==0&&s.no_block==1);rt(s);
        s=fight({{DodgeAndRoll,u}});s.no_block=2;CHECK(play(s,0,0));CHECK(s.block==0&&s.block_next_turn==0);rt(s);
        s=fight({{InfiniteBlades,u}});add(s,15);CHECK(play(s,0,0));CHECK(s.infinite_blades==1&&s.hand.count==0);CHECK(end_turn(s));CHECK(s.hand.count==6&&s.cards[s.hand.ids[0]].kind==Shiv);rt(s);
        s=fight({{Backstab,u}});CHECK(card_innate(s.cards[0]));CHECK(play(s,0,0));CHECK(s.enemies[0].hp==35-4*(int)up&&s.exhaust.count==1);rt(s);
        s=fight({{EscapePlan,u}});add(s,1);CHECK(play(s,0,0));CHECK(s.block==3+2*(int)up&&s.hand.count==1);rt(s);
        s=fight({{EscapePlan,u},{Defend,0}});add(s,1,Zone::Draw,{Strike,0});CHECK(play(s,0,0));CHECK(s.block==0&&s.hand.count==2);rt(s);
        s=fight({{EscapePlan,u},{Defend,0}});s.no_draw=1;add(s,1);CHECK(play(s,0,0));CHECK(s.block==0&&s.hand.count==1);rt(s);
        s=fight({{EscapePlan,u}});CHECK(play(s,0,0));CHECK(s.block==0&&s.hand.count==0);rt(s);
        s=fight({{Predator,u}});add(s,20);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==31-5*(int)up&&s.draw_next_turn==2&&s.hand.count==0);rt(s);
        CHECK(end_turn(s));CHECK(s.hand.count==7&&s.draw_next_turn==0);rt(s);
    }
    auto s=fight({{SwordBoomerang,0}});s.strength=1;s.vigor=2;s.enemies[0].vulnerable=1;CHECK(gain_relic(s,PenNib));s.relic_counters[9]=9;
    CHECK(play(s,0,-1));CHECK(s.phase==Phase::Reward&&s.vigor==0&&s.double_attack==0&&relic_counter(s,PenNib)==0);rt(s);
    s=fight({{InfiniteBlades,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0},{Purity,0}});add(s,10);
    CHECK(play(s,0,0));CHECK(end_turn(s));CHECK(s.hand.count==10&&s.draw.count==10&&s.cards[s.hand.ids[9]].kind==Shiv);rt(s);
    new_run(s,7,false,Character::Silent);s.deck_count=12;for(int i=0;i<12;i++)s.deck[i]={i<7?Backstab:DefendSilent,0};s.floor=2;enter_combat(s);
    CHECK(s.hand.count==7);for(int i=0;i<7;i++)CHECK(s.cards[s.hand.ids[i]].kind==Backstab);rt(s);
    for(int i=0;i<12;i++)s.deck[i]={Backstab,0};enter_combat(s);CHECK(s.hand.count==10&&s.draw.count==2);rt(s);
    s.deck[0]={InfiniteBlades,1};enter_combat(s);CHECK(s.hand.count==10);CHECK(card_innate({InfiniteBlades,1})&&!card_innate({InfiniteBlades,0}));
    s.demon_form=4;s.infinite_blades=3;s.block_next_turn=12;s.draw_next_turn=3;s.flame_barrier=4;enter_combat(s);
    CHECK(!s.demon_form&&!s.infinite_blades&&!s.block_next_turn&&!s.draw_next_turn&&!s.flame_barrier);rt(s);
    CHECK(character_pool(Character::Ironclad,nullptr)==29&&character_pool(Character::Silent,nullptr)==29);
    for(unsigned i=Headbutt;i<=DemonForm;i++)CHECK(card_in_pool(i,Character::Ironclad)&&!card_in_pool(i,Character::Silent));
    for(unsigned i=DodgeAndRoll;i<=Predator;i++)CHECK(card_in_pool(i,Character::Silent)&&!card_colorless(i));
}
static void migration(const char*path){
    FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t> bytes;unsigned v;while(std::fscanf(f,"%2x",&v)==1)bytes.push_back((uint8_t)v);std::fclose(f);
    CHECK(bytes[4]==7);State s{};CHECK(load(s,bytes.data(),bytes.size()));CHECK(s.phase==Phase::DiscardChoice&&!s.demon_form&&!s.infinite_blades&&!s.block_next_turn&&!s.draw_next_turn&&!s.flame_barrier);rt(s);
    uint16_t id=s.hand.ids[0];CHECK(choose_discard(s,&id,1));rt(s);
}
static void stress(){
    uint32_t rng=0x19511;
    for(unsigned seed=1;seed<=300;seed++){
        State s{};new_run(s,seed,true,seed%2?Character::Silent:Character::Ironclad);s.hp=s.max_hp=999;
        for(int turn=0;turn<180&&combat_phase(s.phase);turn++){
            if(s.phase==Phase::ExtendedChoice){uint16_t ids[MaxHand];unsigned count=choice_min(s)+random_below(rng,choice_max(s)-choice_min(s)+1);for(unsigned j=0;j<count;j++)ids[j]=choice_pile(s).ids[j];CHECK(choose_extended(s,ids,count));}
            else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[random_below(rng,s.discard.count)]));
            else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
            else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
            else if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned j=0;j<s.discard_required;j++)ids[j]=s.hand.ids[j];CHECK(choose_discard(s,ids,s.discard_required));}
            else if(s.hand.count&&random_below(rng,5)){if(!play(s,random_below(rng,s.hand.count),0))CHECK(end_turn(s));}
            else CHECK(end_turn(s));
            ++transitions;rt(s);
        }
    }
}
int main(int argc,char**argv){cards();if(argc>1)migration(argv[1]);stress();printf("tactics: %u checks, %u controlled transitions passed\n",checks,transitions);}
