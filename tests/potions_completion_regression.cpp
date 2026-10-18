#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

using namespace spire;
static unsigned checks,policy_models;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)

// These are deterministic engine-contract regressions. They are not original
// game differential traces, nor claims that the incomplete character pools are
// complete. Numeric contracts follow the retained v0.111.0 descriptors.
static State fight(Character character=Character::Ironclad){
    State s{};new_run(s,197,false,character);
    s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
    s.relics=s.relic_count=s.relic_flash=0;s.energy=20;s.hp=s.max_hp=80;
    s.deck_count=1;s.deck[0]={Defend,0};
    for(auto&e:s.enemies)e={46,46,0,0,0,2};
    return s;
}
static int append(State&s,Card c,Zone zone){
    CHECK(s.card_count<MaxCards);
    int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;
    Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
    p.ids[p.count++]=(uint16_t)id;return id;
}
static void fill(State&s,Zone zone,unsigned count,Card c={Defend,0}){
    for(unsigned i=0;i<count;i++)append(s,c,zone);
}
static void restore(State&s){
    SaveData before{},after{};CHECK(validate(s));CHECK(save(s,before));
    State copy{};CHECK(load(copy,before.bytes,before.size));CHECK(save(copy,after));
    CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));s=copy;
}
static void unchanged(const State&s,const State&before){CHECK(!std::memcmp(&s,&before,sizeof s));}
static void drink(State&s,Potion p,int target=0){
    CHECK(s.potions[0]==Potion::Empty);s.potions[0]=p;
    CHECK(can_use_potion(s,0,target));CHECK(use_potion(s,0,target));
    if(p!=Potion::EntropicBrew)CHECK(s.potions[0]==Potion::Empty);
}
static void select(State&s,std::initializer_list<uint16_t> ids){
    CHECK(s.phase==Phase::ExtendedChoice);restore(s);
    CHECK(choose_extended(s,ids.begin(),(unsigned)ids.size()));restore(s);
}
static void quiet_turn(State&s){
    for(auto&e:s.enemies)e.move=2;
    CHECK(end_turn(s));CHECK(s.phase==Phase::Combat);restore(s);
}
static bool ordinary_outside(Potion p){
    return p==Potion::Blood||p==Potion::Fruit||p==Potion::EntropicBrew||p==Potion::Ambergris;
}
static void scopes_and_inventory(){
    const Phase outside[]={Phase::Map,Phase::Reward,Phase::Camp,Phase::Shop,Phase::Event,Phase::Treasure};
    unsigned targeted=0,automatic=0;
    for(unsigned k=1;k<=PotionKinds;k++){
        Potion p=(Potion)k;const auto*d=potion_definition(p);CHECK(d&&*d->key&&*d->name&&*d->description);
        for(unsigned j=1;j<k;j++)CHECK(std::strcmp(d->key,potion_definition((Potion)j)->key));
        auto s=fight();s.potions[0]=p;
        CHECK(can_use_potion(s,0,0)==(p!=Potion::FairyInABottle));
        CHECK(d->automatic==(p==Potion::FairyInABottle));automatic+=d->automatic;targeted+=d->targeted;
        for(int slot:{-1,2}){auto before=s;CHECK(!use_potion(s,slot,0));unchanged(s,before);}
        for(Phase phase:{Phase::Dead,Phase::Complete,Phase::Error,Phase::ExtendedChoice}){
            auto blocked=s;blocked.phase=phase;auto before=blocked;
            CHECK(!use_potion(blocked,0,0));unchanged(blocked,before);
        }
        auto busy=s;busy.effect_count=1;auto before=busy;CHECK(!use_potion(busy,0,0));unchanged(busy,before);
        busy=s;busy.playing=0;before=busy;CHECK(!use_potion(busy,0,0));unchanged(busy,before);
        busy=s;busy.hp=0;before=busy;CHECK(!use_potion(busy,0,0));unchanged(busy,before);
        if(d->targeted){
            for(int target:{-1,MaxEnemies}){before=s;CHECK(!use_potion(s,0,target));unchanged(s,before);}
            s.enemies[0].hp=0;before=s;CHECK(!use_potion(s,0,0));unchanged(s,before);
            CHECK(can_use_potion(s,0,1));
        }
        for(Phase phase:outside){
            s=fight();s.phase=phase;s.potions[0]=p;
            bool expected=ordinary_outside(p)||(p==Potion::FoulPotion&&phase==Phase::Shop);
            CHECK(can_use_potion(s,0,0)==expected);
            if(!expected){before=s;CHECK(!use_potion(s,0,0));unchanged(s,before);}
        }
        // All 64 kinds can be explicitly obtained (including practice-only
        // models), without overwriting occupied bottles or mutating on failure.
        s=fight();CHECK(procure_potion(s,p));CHECK(s.potions[0]==p);
        CHECK(procure_potion(s,Potion::FairyInABottle));before=s;
        CHECK(!procure_potion(s,p));unchanged(s,before);
        CHECK(discard_potion(s,0));CHECK(s.potions[1]==Potion::FairyInABottle);
        CHECK(procure_potion(s,p));restore(s);++policy_models;
    }
    CHECK(policy_models==64&&automatic==1&&targeted==8);
    CHECK(!potion_definition(Potion::Empty)&&!potion_definition((Potion)255));
    auto s=fight();auto before=s;CHECK(!procure_potion(s,Potion::Empty));unchanged(s,before);
    CHECK(!procure_potion(s,(Potion)255));unchanged(s,before);
}
static void resources_and_damage(){
    auto s=fight();s.strength=99;s.vigor=8;s.enemies[0].vulnerable=3;s.enemies[0].block=7;
    drink(s,Potion::Fire);CHECK(s.enemies[0].hp==33&&s.enemies[0].block==0&&s.vigor==8&&s.strength==99);
    s=fight();s.enemies[0].block=3;s.enemies[1].block=8;drink(s,Potion::Explosive);
    CHECK(s.enemies[0].hp==39&&s.enemies[1].hp==44);
    s=fight();s.enemies[1].block=5;drink(s,Potion::PotionShapedRock,1);CHECK(s.enemies[1].hp==36);
    s=fight();s.hp=10;s.block=8;s.enemies[0].block=5;drink(s,Potion::FoulPotion);
    CHECK(s.hp==6&&s.block==0&&s.enemies[0].hp==39&&s.enemies[1].hp==34);restore(s);
    s=fight();s.dexterity=99;s.no_block=2;drink(s,Potion::Block);CHECK(s.block==12);
    drink(s,Potion::Fortifier);CHECK(s.block==36);
    s.block=3334;drink(s,Potion::Fortifier);CHECK(s.block==9999);
    s=fight();s.max_hp=81;s.hp=1;drink(s,Potion::Blood);CHECK(s.hp==17);
    drink(s,Potion::Fruit);CHECK(s.max_hp==86&&s.hp==22);
    s.max_hp=998;s.hp=500;drink(s,Potion::Fruit);CHECK(s.max_hp==999&&s.hp==501);
    s.energy=998;drink(s,Potion::Energy);CHECK(s.energy==999);
    s.strength=s.dexterity=9998;drink(s,Potion::Strength);drink(s,Potion::Dexterity);
    CHECK(s.strength==9999&&s.dexterity==9999);restore(s);
    s=fight();s.strength=s.dexterity=9998;drink(s,Potion::Flex);drink(s,Potion::Speed);
    CHECK(s.temporary_strength==1&&s.temporary_dexterity==1);
    quiet_turn(s);CHECK(s.strength==9998&&s.dexterity==9998&&!s.temporary_strength&&!s.temporary_dexterity);
    s=fight();drink(s,Potion::FyshOil);CHECK(s.strength==1&&s.dexterity==1);
    s.enemies[0].move=0;s.enemies[1].move=2;s.block=20;drink(s,Potion::Bronze);
    CHECK(end_turn(s));CHECK(s.hp==80&&s.enemies[0].hp==43);restore(s);
    s=fight();s.enemies[0].vulnerable=s.enemies[0].weak=9998;
    drink(s,Potion::Vulnerable);drink(s,Potion::Weak);CHECK(s.enemies[0].vulnerable==9999&&s.enemies[0].weak==9999);
    s.enemies[1].hp=0;drink(s,Potion::PotionOfBinding);CHECK(!s.enemies[1].weak&&!s.enemies[1].vulnerable);
    s=fight();s.no_draw=1;fill(s,Zone::Draw,10);drink(s,Potion::Swift);CHECK(!s.hand.count&&s.draw.count==10);
    drink(s,Potion::CureAll);CHECK(s.energy==21&&!s.hand.count);
    s=fight();int a=append(s,{Strike,0},Zone::Hand),b=append(s,{Defend,1},Zone::Hand);
    int c=append(s,{Strike,0},Zone::Discard);drink(s,Potion::Forge);
    CHECK(s.cards[a].upgraded&&s.cards[b].upgraded&&!s.cards[c].upgraded&&!s.deck[0].upgraded);restore(s);
}
static void choices_and_creation(){
    for(Potion p:{Potion::AttackPotion,Potion::SkillPotion,Potion::PowerPotion,Potion::ColorlessPotion}){
        auto s=fight();fill(s,Zone::Hand,2);drink(s,p);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active);
        CHECK(choice_pile(s).count==3&&choice_min(s)==1&&choice_max(s)==1);
        for(unsigned i=0;i<3;i++){
            Card c=choice_card(s,choice_pile(s).ids[i]);
            CHECK(!card_generated(c.kind));
            if(p==Potion::ColorlessPotion)CHECK(card_colorless(c.kind));
            else CHECK(definitions[c.kind].type==(p==Potion::AttackPotion?CardType::Attack:p==Potion::SkillPotion?CardType::Skill:CardType::Power));
            for(unsigned j=0;j<i;j++)CHECK(c.kind!=choice_card(s,choice_pile(s).ids[j]).kind);
        }
        auto before=s;uint16_t wrong=65535;
        CHECK(!choose_extended(s,&wrong,1));unchanged(s,before);
        CHECK(!choose_extended(s,nullptr,0));unchanged(s,before);
        CHECK(!use_potion(s,1,0));unchanged(s,before);
        uint16_t token=choice_pile(s).ids[1];Card chosen=choice_card(s,token);
        select(s,{token});CHECK(s.hand.count==3&&!s.expansion.potion_active);
        int id=s.hand.ids[2];CHECK(s.cards[id].kind==chosen.kind&&cost(s,id)==0);
    }
    auto s=fight();append(s,{Strike,0},Zone::Hand);
    int unchosen=append(s,{Defend,0},Zone::Draw),chosen=append(s,{Defend,1},Zone::Draw);
    drink(s,Potion::DropletOfPrecognition);select(s,{(uint16_t)chosen});
    CHECK(s.zones[chosen]==Zone::Hand&&s.zones[unchosen]==Zone::Draw&&s.cards[chosen].upgraded);
    chosen=append(s,{Bludgeon,0},Zone::Discard);drink(s,Potion::LiquidMemories);select(s,{(uint16_t)chosen});
    CHECK(s.zones[chosen]==Zone::Hand&&cost(s,chosen)==0);
    s=fight();fill(s,Zone::Hand,4);fill(s,Zone::Draw,10);
    drink(s,Potion::GamblersBrew);uint16_t first=s.hand.ids[0],last=s.hand.ids[3];
    select(s,{first,last});CHECK(s.hand.count==4&&s.discard.count==2&&s.draw.count==8);
    CHECK(s.zones[first]==Zone::Discard&&s.zones[last]==Zone::Discard);
    s=fight();fill(s,Zone::Hand,3);s.no_draw=1;fill(s,Zone::Draw,3);
    drink(s,Potion::GamblersBrew);select(s,{s.hand.ids[1]});CHECK(s.hand.count==2&&s.draw.count==3);
    s=fight();fill(s,Zone::Hand,4);fill(s,Zone::Draw,12);
    s.pain=3;s.embrace=1;s.power_count=2;s.power_order[0]=0;s.power_order[1]=1;
    drink(s,Potion::Ashwater);select(s,{0,2,3});
    CHECK(s.exhaust.count==3&&s.hand.count==4&&s.block==9&&s.zones[1]==Zone::Hand);
    for(unsigned i=0;i<s.exhaust.count;i++)CHECK(s.exhaust.ids[i]<4);
    s=fight();fill(s,Zone::Hand,2);fill(s,Zone::Draw,15);s.embrace=1;s.power_count=1;s.power_order[0]=1;
    drink(s,Potion::GlowwaterPotion);CHECK(s.exhaust.count==2&&s.hand.count==10&&s.draw.count==5);
    s=fight();fill(s,Zone::Hand,2);fill(s,Zone::Draw,15);s.no_draw=1;
    drink(s,Potion::GlowwaterPotion);CHECK(s.exhaust.count==2&&!s.hand.count&&s.draw.count==15);
    s=fight();chosen=append(s,{Bludgeon,0},Zone::Hand);s.expansion.turn_cost[chosen]=4;
    drink(s,Potion::TouchOfInsanity);select(s,{(uint16_t)chosen});
    CHECK(cost(s,chosen)==0&&s.expansion.combat_cost[chosen]==1);
    quiet_turn(s);CHECK(cost(s,chosen)==0);
    s=fight();fill(s,Zone::Hand,9);s.no_draw=1;drink(s,Potion::CunningPotion);
    CHECK(s.hand.count==10&&s.discard.count==2);
    for(unsigned i=9;i<s.card_count;i++)CHECK(s.cards[i].kind==Shiv&&s.cards[i].upgraded);
    s=fight();fill(s,Zone::Hand,9);drink(s,Potion::PotOfGhouls);CHECK(s.hand.count==10&&s.discard.count==1);
    CHECK(s.cards[s.hand.ids[9]].kind==Soul&&s.cards[s.discard.ids[0]].kind==Soul);
    s=fight();drink(s,Potion::OrobicAcid);CHECK(s.hand.count==3);
    for(int i=0;i<3;i++){int id=s.hand.ids[i];CHECK(definitions[s.cards[id].kind].type==(CardType)i&&cost(s,id)==0);}
    s=fight();drink(s,Potion::CosmicConcoction);CHECK(s.hand.count==3);
    for(unsigned i=0;i<3;i++)CHECK(card_colorless(s.cards[s.hand.ids[i]].kind)&&s.cards[s.hand.ids[i]].upgraded);
    restore(s);
}
static void timed_and_automatic(){
    auto s=fight();fill(s,Zone::Draw,30);drink(s,Potion::Clarity);drink(s,Potion::RadiantTincture);
    CHECK(s.hand.count==1&&s.energy==21);
    for(int turn=0;turn<4;turn++){
        quiet_turn(s);CHECK(s.hand.count==(turn<3?6:5));CHECK(s.energy==(turn<3?4:3));
        CHECK(power_value(s,XPower::Clarity)==(turn<3?2-turn:0));
        CHECK(power_value(s,XPower::Radiance)==(turn<3?2-turn:0));
    }
    s=fight();s.hp=20;drink(s,Potion::RegenPotion);drink(s,Potion::MazalethsGift);
    const int health[]={25,29,32,34,35,35};
    for(int turn=0;turn<6;turn++){quiet_turn(s);CHECK(s.hp==health[turn]&&s.strength==turn+1);}
    s=fight();drink(s,Potion::ShipInABottle);CHECK(s.block==10&&s.block_next_turn==10);
    quiet_turn(s);CHECK(s.block==10&&!s.block_next_turn);quiet_turn(s);CHECK(!s.block);
    s=fight();drink(s,Potion::HeartOfIron);quiet_turn(s);CHECK(s.block==7&&power_value(s,XPower::Plating)==6);
    quiet_turn(s);CHECK(s.block==6&&power_value(s,XPower::Plating)==5);
    s=fight();fill(s,Zone::Hand,3);fill(s,Zone::Draw,30);drink(s,Potion::StableSerum);
    quiet_turn(s);CHECK(s.hand.count==8&&s.zones[0]==Zone::Hand&&power_value(s,XPower::Retain)==1);
    quiet_turn(s);CHECK(s.hand.count==10&&s.zones[0]==Zone::Hand&&!power_value(s,XPower::Retain));
    // The draw pile still contains enough cards to avoid immediately drawing
    // the discarded originals again after the second retained turn.
    for(auto&e:s.enemies)e.move=2;CHECK(end_turn(s));
    CHECK(s.discard.count==10&&s.hand.count==5);restore(s);
    s=fight();s.enemies[0].move=0;drink(s,Potion::BeetleJuice);CHECK(enemy_damage(s,0)==8);
    for(int i=0;i<4;i++){quiet_turn(s);CHECK(s.expansion.beetle[0]==3-i);}
    s=fight();s.block=100;for(auto&e:s.enemies)e.move=0;
    drink(s,Potion::ShacklingPotion);CHECK(s.enemies[0].strength==-7&&s.enemies[1].strength==-7);
    CHECK(end_turn(s));CHECK(s.enemies[0].strength==0&&s.enemies[1].strength==0&&!s.expansion.restore_strength[0]);
    s=fight();drink(s,Potion::PowderedDemise);s.enemies[0].block=999;
    quiet_turn(s);CHECK(s.enemies[0].hp==37);quiet_turn(s);CHECK(s.enemies[0].hp==28);
    s=fight();s.enemies[0].hp=33;drink(s,Potion::PotionOfDoom);CHECK(s.enemies[0].hp==33);
    quiet_turn(s);CHECK(!s.enemies[0].hp&&s.enemies[1].hp==46);
    s=fight();drink(s,Potion::PoisonPotion);quiet_turn(s);CHECK(s.enemies[0].hp==40&&s.enemies[0].poison==5);
    quiet_turn(s);CHECK(s.enemies[0].hp==35&&s.enemies[0].poison==4);
    s=fight();drink(s,Potion::LuckyTonic);s.block=5;player_hp_loss(s,3,true,true);
    CHECK(s.hp==80&&s.block==2&&power_value(s,XPower::Buffer)==1);
    player_hp_loss(s,10,true,true);CHECK(s.hp==80&&!s.block&&!power_value(s,XPower::Buffer));
    player_hp_loss(s,7,true,true);CHECK(s.hp==73);
    s=fight();drink(s,Potion::GhostInAJar);player_hp_loss(s,30,true,true);CHECK(s.hp==79);
    quiet_turn(s);CHECK(!power_value(s,XPower::Intangible));player_hp_loss(s,30,true,true);CHECK(s.hp==49);
    s=fight();s.potions[0]=s.potions[1]=Potion::FairyInABottle;s.hp=1;s.max_hp=81;
    CHECK(!can_use_potion(s,0,0));player_hp_loss(s,20,true,true);
    CHECK(s.hp==24&&s.potions[0]==Potion::Empty&&s.potions[1]==Potion::FairyInABottle);restore(s);
    player_hp_loss(s,30,true,true);CHECK(s.hp==24&&s.potions[1]==Potion::Empty);
    player_hp_loss(s,30,true,true);CHECK(s.phase==Phase::Dead&&s.hp==0);restore(s);
    s=fight();s.hp=1;s.potions[1]=Potion::FairyInABottle;s.enemies[0].hp=s.enemies[1].hp=1;
    drink(s,Potion::FoulPotion);CHECK(s.hp==24&&s.phase==Phase::Reward&&s.potions[1]==Potion::Empty);restore(s);
    s=fight();s.hp=1;s.enemies[0].move=s.enemies[1].move=0;drink(s,Potion::Ambergris);
    CHECK(s.hp==41);CHECK(end_turn(s));CHECK(s.hp==41&&s.turn==2&&!s.expansion.skip_enemy);
    CHECK(end_turn(s));CHECK(s.hp==17&&s.turn==3);restore(s);
}
static void replay_autoplay_and_resources(){
    auto s=fight();append(s,{TwinStrike,0},Zone::Hand);drink(s,Potion::GigantificationPotion);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==16&&!power_value(s,XPower::Gigantic));
    s=fight();append(s,{Strike,0},Zone::Hand);drink(s,Potion::Duplicator);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==34&&s.expansion.cards_finished==2&&s.energy==19&&!power_value(s,XPower::Duplicator));
    s=fight();append(s,{Strike,0},Zone::Hand);drink(s,Potion::Duplicator);quiet_turn(s);
    CHECK(!power_value(s,XPower::Duplicator));
    s=fight();int strike=append(s,{TwinStrike,0},Zone::Hand);append(s,{Defend,0},Zone::Draw);
    drink(s,Potion::SoldiersStew);CHECK(s.expansion.replay[strike]==1&&!s.expansion.replay[1]);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==26&&s.expansion.cards_finished==2);
    s=fight();s.energy=0;append(s,{Strike,0},Zone::Draw);append(s,{Defend,0},Zone::Draw);append(s,{Discovery,0},Zone::Draw);
    drink(s,Potion::DistilledChaos);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active&&s.expansion.depth==1);
    uint16_t token=choice_pile(s).ids[0];select(s,{token});
    CHECK(s.phase==Phase::Combat&&s.expansion.cards_finished==3&&s.energy==0&&!s.expansion.depth&&!s.expansion.potion_active);
    CHECK(s.block==5&&s.enemies[0].hp+s.enemies[1].hp==86);restore(s);
    s=fight();fill(s,Zone::Hand,4);fill(s,Zone::Draw,8);fill(s,Zone::Discard,3);
    drink(s,Potion::BottledPotential);CHECK(s.hand.count==5&&s.draw.count==10&&!s.discard.count);restore(s);
    s=fight();fill(s,Zone::Hand,4,{Bludgeon,0});fill(s,Zone::Draw,10,{Defend,0});drink(s,Potion::SneckoOil);
    CHECK(s.hand.count==10);for(unsigned i=0;i<s.hand.count;i++)CHECK(cost(s,s.hand.ids[i])>=0&&cost(s,s.hand.ids[i])<=3);
    quiet_turn(s);for(unsigned i=0;i<s.card_count;i++)CHECK(!s.expansion.turn_cost[i]);
    s=fight();drink(s,Potion::BoneBrew);player_hp_loss(s,10,true,true);
    CHECK(s.hp==80&&s.expansion.osty_hp==5&&s.expansion.osty_max_hp==15);
    drink(s,Potion::BoneBrew);CHECK(s.expansion.osty_hp==20&&s.expansion.osty_max_hp==30);
    s=fight();drink(s,Potion::FocusPotion);drink(s,Potion::PotionOfCapacity);
    CHECK(s.expansion.orb_slots==2&&power_value(s,XPower::Focus)==2);
    drink(s,Potion::EssenceOfDarkness);CHECK(s.expansion.orb_count==2);quiet_turn(s);
    CHECK(s.expansion.dark[0]==14&&s.expansion.dark[1]==14);
    drink(s,Potion::EssenceOfDarkness);CHECK(s.expansion.dark[0]==6&&s.expansion.dark[1]==6);
    CHECK(s.enemies[0].hp==18&&s.enemies[1].hp==46);
    s.expansion.orb_slots=9;drink(s,Potion::PotionOfCapacity);CHECK(s.expansion.orb_slots==MaxOrbs);restore(s);
    s=fight();drink(s,Potion::StarPotion);drink(s,Potion::StarPotion);CHECK(s.expansion.stars==6);
    drink(s,Potion::KingsCourage);CHECK(s.hand.count==1&&s.cards[s.hand.ids[0]].kind==SovereignBlade);
    int blade=s.hand.ids[0];CHECK(s.expansion.blade_damage[blade]==25);
    drink(s,Potion::KingsCourage);CHECK(s.hand.count==1&&s.expansion.blade_damage[blade]==40);
    CHECK(play(s,0,1));CHECK(s.enemies[1].hp==6);restore(s);
}
static void acquisition(){
    for(Character c:{Character::Ironclad,Character::Silent}){
        auto s=fight(c);s.phase=Phase::Shop;unsigned allowed=0;
        for(unsigned k=1;k<=PotionKinds;k++)allowed+=potion_in_pool((Potion)k,c);
        CHECK(allowed==48);
        for(int i=0;i<500;i++)CHECK(potion_in_pool(random_potion(s),c));
        s.potions[1]=Potion::FairyInABottle;drink(s,Potion::EntropicBrew);
        CHECK(potion_in_pool(s.potions[0],c)&&s.potions[1]==Potion::FairyInABottle);restore(s);
    }
    auto s=fight();s.phase=Phase::Shop;s.gold=200;s.shop_potions[0]=(uint8_t)Potion::LiquidMemories;
    s.shop_prices[ShopPotionStart]=100;s.potions[0]=Potion::Fire;s.potions[1]=Potion::Block;auto before=s;
    CHECK(!buy(s,ShopPotionStart));unchanged(s,before);
    CHECK(discard_potion(s,1));CHECK(buy(s,ShopPotionStart));
    CHECK(s.potions[0]==Potion::Fire&&s.potions[1]==Potion::LiquidMemories&&s.gold==100);
    before=s;CHECK(!buy(s,ShopPotionStart));unchanged(s,before);restore(s);
    s=fight();s.phase=Phase::Reward;s.reward_potion=Potion::CunningPotion;s.potions[0]=Potion::Fire;s.potions[1]=Potion::Block;
    before=s;CHECK(!claim_reward_potion(s));unchanged(s,before);CHECK(discard_potion(s,0));
    CHECK(claim_reward_potion(s));CHECK(s.potions[0]==Potion::CunningPotion&&s.potions[1]==Potion::Block&&(s.reward_claimed&4));
    before=s;CHECK(!claim_reward_potion(s));unchanged(s,before);restore(s);
    s=fight();s.phase=Phase::Shop;s.hp=31;s.gold=9950;drink(s,Potion::FoulPotion);
    CHECK(s.hp==31&&s.gold==9999&&s.enemies[0].hp==46);restore(s);
}
int main(){
    scopes_and_inventory();resources_and_damage();choices_and_creation();timed_and_automatic();replay_autoplay_and_resources();acquisition();
    std::printf("potion completion: %u checks; %u model policies\n",checks,policy_models);
}
