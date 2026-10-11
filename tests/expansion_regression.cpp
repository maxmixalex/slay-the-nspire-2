#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State combat(){State s{};new_run(s,17,true);s.practice=0;s.encounter=1;enter_combat(s);s.relics=0;s.relic_count=0;s.relic_flash=0;for(auto&e:s.enemies)e={46,46,0,0,0,0};CHECK(validate(s));return s;}
static void hand(State&s,std::initializer_list<Card> cards){s.card_count=0;s.draw.count=s.hand.count=s.discard.count=s.exhaust.count=s.powers.count=0;for(auto c:cards){auto id=s.card_count++;s.cards[id]=c;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=id;}for(int i=0;i<18;i++){auto id=s.card_count++;s.cards[id]={Defend,0};s.zones[id]=Zone::Draw;s.draw.ids[s.draw.count++]=id;}}
static void rt(const State&s){SaveData a{},b{};State t{};CHECK(validate(s));CHECK(save(s,a));CHECK(a.bytes[4]==9);CHECK(load(t,a.bytes,a.size));CHECK(save(t,b));CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));}
static bool drink(State&s,Potion p,int target=0){s.potions[0]=p;return use_potion(s,0,target);}
static void potions(){
    for(int kind=1;kind<=PotionKinds;kind++){
        State s=combat();s.hp=60;s.block=6;hand(s,{{Strike,0},{Defend,0}});s.potions[0]=(Potion)kind;
        CHECK(potion_definition((Potion)kind));CHECK(can_use_potion(s,0,0));CHECK(use_potion(s,0,0));CHECK(s.potions[0]==Potion::Empty);CHECK(!use_potion(s,0,0));rt(s);
        switch((Potion)kind){
        case Potion::Fire:CHECK(s.enemies[0].hp==26);break;
        case Potion::Block:CHECK(s.block==18);break;
        case Potion::Blood:CHECK(s.hp==76);break;
        case Potion::Energy:CHECK(s.energy==5);break;
        case Potion::Strength:CHECK(s.strength==2);break;
        case Potion::Dexterity:CHECK(s.dexterity==2);break;
        case Potion::Swift:CHECK(s.hand.count==5);break;
        case Potion::Explosive:CHECK(s.enemies[0].hp==36&&s.enemies[1].hp==36);break;
        case Potion::Vulnerable:CHECK(s.enemies[0].vulnerable==3);break;
        case Potion::Weak:CHECK(s.enemies[0].weak==3&&enemy_damage(s.enemies[0])==9);break;
        case Potion::Fruit:CHECK(s.hp==65&&s.max_hp==85);break;
        case Potion::Forge:CHECK(s.cards[0].upgraded&&s.cards[1].upgraded&&!s.deck[0].upgraded);break;
        case Potion::Flex:CHECK(s.strength==5&&s.temporary_strength==5);CHECK(end_turn(s));CHECK(!s.strength&&!s.temporary_strength);break;
        case Potion::Speed:CHECK(s.dexterity==5&&s.temporary_dexterity==5);CHECK(end_turn(s));CHECK(!s.dexterity&&!s.temporary_dexterity);break;
        case Potion::Bronze:CHECK(s.thorns==3);break;
        case Potion::Fortifier:CHECK(s.block==18);break;
        default:CHECK(false);
        }rt(s);
        s=combat();s.phase=Phase::Camp;s.potions[0]=(Potion)kind;
        bool outside=kind==3||kind==11;CHECK(can_use_potion(s,0,0)==outside);auto original=s;
        CHECK(use_potion(s,0,0)==outside);if(!outside)CHECK(!std::memcmp(&s,&original,sizeof s));rt(s);
    }
    auto s=combat();s.strength=50;s.dexterity=50;s.vigor=8;s.enemies[0].vulnerable=1;s.enemies[0].block=5;
    CHECK(drink(s,Potion::Fire));CHECK(s.enemies[0].hp==31&&s.vigor==8);CHECK(drink(s,Potion::Block));CHECK(s.block==12);rt(s);
    s=combat();s.no_draw=1;unsigned old=s.hand.count;CHECK(drink(s,Potion::Swift));CHECK(s.hand.count==old);rt(s);
    s=combat();s.strength=9998;CHECK(drink(s,Potion::Flex));CHECK(s.strength==9999&&s.temporary_strength==1);CHECK(end_turn(s));CHECK(s.strength==9998);rt(s);
    s=combat();s.potions[0]=Potion::Weak;State oldstate=s;CHECK(!use_potion(s,0,2));CHECK(!std::memcmp(&s,&oldstate,sizeof s));
    CHECK(discard_potion(s,0));CHECK(!discard_potion(s,0));rt(s);
    s=combat();s.potions[0]=Potion::Fire;s.phase=Phase::Dead;s.hp=0;CHECK(!use_potion(s,0,0));CHECK(!discard_potion(s,0));
    s=combat();s.enemies[0].hp=s.enemies[1].hp=10;CHECK(drink(s,Potion::Explosive));CHECK(s.phase==Phase::Reward);rt(s);
}
static void relics(){
    auto s=combat();CHECK(gain_relic(s,Strawberry));CHECK(s.max_hp==87&&s.hp==87);CHECK(!gain_relic(s,Strawberry));
    CHECK(gain_relic(s,Pear));CHECK(gain_relic(s,Mango));CHECK(s.max_hp==111&&s.hp==111);rt(s);
    for(const auto&d:relic_definitions)gain_relic(s,d.id);CHECK(s.relic_count==RelicKinds&&s.relics==((1u<<RelicKinds)-1));rt(s);
    enter_combat(s);CHECK(s.thorns==3&&s.vigor==8&&s.enemies[0].vulnerable==1&&s.enemies[1].vulnerable==1);rt(s);
    s=combat();CHECK(gain_relic(s,Akabeko));enter_combat(s);hand(s,{{TwinStrike,0},{Strike,0}});s.enemies[0]={46,46,0,0,0,0};
    CHECK(damage(s,{TwinStrike,0},0)==13);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==20&&!s.vigor&&!s.attack_vigor);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==14);rt(s);
    s=combat();s.vigor=8;s.energy=0;hand(s,{{Whirlwind,0}});CHECK(play(s,0,-1));CHECK(s.vigor==8);rt(s);
    s=combat();s.vigor=8;hand(s,{{FiendFire,0}});CHECK(play(s,0,0));CHECK(s.vigor==8);rt(s);
    s=combat();s.thorns=3;s.block=100;CHECK(end_turn(s));CHECK(s.hp==80&&s.enemies[0].hp==43&&s.enemies[1].hp==43);rt(s);
    s=combat();s.thorns=3;s.enemies[0].move=s.enemies[1].move=2;CHECK(end_turn(s));CHECK(s.enemies[0].hp==46&&s.enemies[1].hp==46);rt(s);
    s=combat();s.thorns=3;s.enemies[0].hp=s.enemies[1].hp=3;CHECK(end_turn(s));CHECK(s.phase==Phase::Reward&&s.hp==56);rt(s);
    s=combat();s.thorns=3;s.hp=1;s.enemies[0].hp=3;s.enemies[1].hp=0;CHECK(end_turn(s));CHECK(s.phase==Phase::Dead&&s.hp==0);rt(s);
}
static void colorless(){
    for(int upgraded=0;upgraded<2;upgraded++){
        auto s=combat();hand(s,{{Finesse,(uint8_t)upgraded}});CHECK(play(s,0,0));CHECK(s.block==4+3*upgraded&&s.hand.count==1&&s.energy==3);rt(s);
        s=combat();hand(s,{{FlashOfSteel,(uint8_t)upgraded}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==41-3*upgraded&&s.hand.count==1&&s.energy==3);rt(s);
    }
}
static void rooms(){
    auto s=combat();s.current_node=1;s.next_row=1;s.phase=Phase::Map;s.gold=9999;CHECK(choose_node(s,1));CHECK(s.phase==Phase::Shop);
    for(int i=0;i<5;i++){CHECK(card_in_pool(s.shop_cards[i],Character::Ironclad)&&card_rarity(s.shop_cards[i])>=2);for(int j=0;j<i;j++)CHECK(s.shop_cards[i]!=s.shop_cards[j]);}
    CHECK(card_colorless(s.shop_cards[5])&&card_colorless(s.shop_cards[6]));
    CHECK(s.shop_relics[0]!=s.shop_relics[1]&&s.shop_relics[1]!=s.shop_relics[2]);rt(s);
    for(int i=0;i<ShopRemove;i++){
        if(i>=ShopPotionStart){discard_potion(s,0);discard_potion(s,1);}
        int gold=s.gold;CHECK(buy(s,i));int expected=gold-s.shop_prices[i];
        if(i>=ShopRelicStart&&i<ShopPotionStart&&s.shop_relics[i-ShopRelicStart]==OldCoin){expected+=300;if(expected>9999)expected=9999;}
        CHECK(s.gold==expected);CHECK(!buy(s,i));rt(s);
    }
    CHECK(buy(s,ShopRemove));CHECK(remove_card(s,0));CHECK(!buy(s,ShopRemove));rt(s);
    s=combat();hand(s,{{Bludgeon,1}});s.enemies[0].hp=1;s.enemies[1].hp=0;int gold=s.gold;CHECK(play(s,0,0));CHECK(s.phase==Phase::Reward&&s.gold==gold);
    s.reward_potion=Potion::Fruit;s.potions[0]=s.potions[1]=Potion::Fire;CHECK(!claim_reward_potion(s));CHECK(!(s.reward_claimed&4));
    CHECK(discard_potion(s,0));CHECK(claim_reward_potion(s));CHECK(s.potions[0]==Potion::Fruit);CHECK(!claim_reward_potion(s));rt(s);
    CHECK(claim_reward_gold(s));CHECK(s.gold==gold+s.last_gold);CHECK(!claim_reward_gold(s));rt(s);
    int count=s.deck_count;CHECK(choose_reward(s,1));CHECK(s.deck_count==count+1&&s.phase==Phase::Reward);CHECK(!choose_reward(s,2));rt(s);
    CHECK(leave_rewards(s));CHECK(s.phase==Phase::Map);rt(s);
}
static void migration(const char*path){
    FILE*f=fopen(path,"r");CHECK(f);std::vector<uint8_t>b;unsigned v;while(fscanf(f,"%2x",&v)==1)b.push_back((uint8_t)v);fclose(f);
    CHECK(b[4]==4);State s{};CHECK(load(s,b.data(),b.size()));CHECK(s.phase==Phase::Shop);
    CHECK(s.shop_sold&(1u<<ShopRelicStart));CHECK(s.shop_sold&(1u<<4));CHECK(s.shop_sold&(1u<<6));
    CHECK(s.reward_claimed==1&&s.potion_chance==40);CHECK(s.shop_prices[7]==150);CHECK(!s.thorns&&!s.vigor);rt(s);
    CHECK(buy(s,0));CHECK(buy(s,ShopPotionStart));rt(s);
}
int main(int argc,char**argv){potions();relics();colorless();rooms();if(argc>1)migration(argv[1]);std::printf("expansion: %u checks passed\n",checks);}
