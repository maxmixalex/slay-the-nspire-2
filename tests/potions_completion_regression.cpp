#include "core.hpp"
#include "inplace_reset.hpp"
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
// Reset the caller's fixture instead of returning a large State temporary
// for every potion scenario. new_run deliberately preserves event history,
// so clear it here to retain the original fresh-fixture semantics.
static void fight(State&s,Character character=Character::Ironclad){
    reset_in_place(s);new_run(s,197,false,character);
    s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
    s.relics=s.relic_count=s.relic_flash=0;s.energy=20;s.hp=s.max_hp=80;
    s.deck_count=1;s.deck[0]={Defend,0};
    for(auto&e:s.enemies)e={46,46,0,0,0,2};
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
    // The test calls this serially, never recursively. Reuse its independent
    // round-trip workspaces rather than stacking them below a scenario State.
    static SaveData before{},after{};static State copy{};
    CHECK(validate(s));CHECK(save(s,before));
    CHECK(load(copy,before.bytes,before.size));CHECK(save(copy,after));
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
        State s{};fight(s);s.potions[0]=p;
        CHECK(can_use_potion(s,0,0)==(p!=Potion::FairyInABottle));
        CHECK(can_discard_potion(s,0));
        CHECK(d->automatic==(p==Potion::FairyInABottle));automatic+=d->automatic;targeted+=d->targeted;
        for(int slot:{-1,2}){auto before=s;CHECK(!use_potion(s,slot,0));unchanged(s,before);}
        for(Phase phase:{Phase::Dead,Phase::Complete,Phase::Error,Phase::ExtendedChoice,Phase::WorldRelicChoice,Phase::OriginalEventChoice}){
            auto blocked=s;blocked.phase=phase;auto before=blocked;
            CHECK(!use_potion(blocked,0,0));unchanged(blocked,before);
            CHECK(!can_discard_potion(blocked,0)&&!discard_potion(blocked,0));unchanged(blocked,before);
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
            fight(s);s.phase=phase;s.potions[0]=p;
            bool expected=ordinary_outside(p)||(p==Potion::FoulPotion&&phase==Phase::Shop);
            CHECK(can_use_potion(s,0,0)==expected);
            CHECK(can_discard_potion(s,0));
            if(!expected){before=s;CHECK(!use_potion(s,0,0));unchanged(s,before);}
        }
        for(Phase phase:{Phase::WorldRelicChoice,Phase::OriginalEventChoice}){
            fight(s);s.phase=phase;s.potions[0]=p;
            s.world_relics.choice=WorldRelicChoice::OfferReward;s.original_events.selection=OriginalEventSelection::Rewards;
            CHECK(can_use_potion(s,0,0)==ordinary_outside(p));CHECK(can_discard_potion(s,0));
            before=s;
            if(!ordinary_outside(p)){CHECK(!use_potion(s,0,0));unchanged(s,before);}
            s.world_relics.choice=WorldRelicChoice::OfferCard;s.original_events.selection=OriginalEventSelection::OfferCard;before=s;
            CHECK(!can_use_potion(s,0,0)&&!can_discard_potion(s,0));CHECK(!discard_potion(s,0));unchanged(s,before);
        }
        // All 64 kinds can be explicitly obtained (including practice-only
        // models), without overwriting occupied bottles or mutating on failure.
        fight(s);CHECK(procure_potion(s,p));CHECK(s.potions[0]==p);
        CHECK(procure_potion(s,Potion::FairyInABottle));before=s;
        CHECK(!procure_potion(s,p));unchanged(s,before);
        CHECK(discard_potion(s,0));CHECK(s.potions[1]==Potion::FairyInABottle);
        CHECK(procure_potion(s,p));restore(s);++policy_models;
    }
    CHECK(policy_models==64&&automatic==1&&targeted==8);
    CHECK(!potion_definition(Potion::Empty)&&!potion_definition((Potion)255));
    State s{};fight(s);auto before=s;CHECK(!procure_potion(s,Potion::Empty));unchanged(s,before);
    CHECK(!procure_potion(s,(Potion)255));unchanged(s,before);
    CHECK(!can_discard_potion(s,-1)&&!can_discard_potion(s,0)&&!can_discard_potion(s,int(potion_slot_count(s))));
}
static void resources_and_damage(){
    State s{};fight(s);s.strength=99;s.vigor=8;s.enemies[0].vulnerable=3;s.enemies[0].block=7;
    drink(s,Potion::Fire);CHECK(s.enemies[0].hp==33&&s.enemies[0].block==0&&s.vigor==8&&s.strength==99);
    fight(s);s.enemies[0].block=3;s.enemies[1].block=8;drink(s,Potion::Explosive);
    CHECK(s.enemies[0].hp==39&&s.enemies[1].hp==44);
    fight(s);s.enemies[1].block=5;drink(s,Potion::PotionShapedRock,1);CHECK(s.enemies[1].hp==36);
    fight(s);s.hp=10;s.block=8;s.enemies[0].block=5;drink(s,Potion::FoulPotion);
    CHECK(s.hp==6&&s.block==0&&s.enemies[0].hp==39&&s.enemies[1].hp==34);restore(s);
    fight(s);s.dexterity=99;s.no_block=2;drink(s,Potion::Block);CHECK(s.block==12);
    drink(s,Potion::Fortifier);CHECK(s.block==36);
    s.block=3334;drink(s,Potion::Fortifier);CHECK(s.block==9999);
    fight(s);s.max_hp=81;s.hp=1;drink(s,Potion::Blood);CHECK(s.hp==17);
    drink(s,Potion::Fruit);CHECK(s.max_hp==86&&s.hp==22);
    s.max_hp=998;s.hp=500;drink(s,Potion::Fruit);CHECK(s.max_hp==999&&s.hp==501);
    s.energy=998;drink(s,Potion::Energy);CHECK(s.energy==999);
    s.strength=s.dexterity=9998;drink(s,Potion::Strength);drink(s,Potion::Dexterity);
    CHECK(s.strength==9999&&s.dexterity==9999);restore(s);
    fight(s);s.strength=s.dexterity=9998;drink(s,Potion::Flex);drink(s,Potion::Speed);
    CHECK(s.temporary_strength==1&&s.temporary_dexterity==1);
    quiet_turn(s);CHECK(s.strength==9998&&s.dexterity==9998&&!s.temporary_strength&&!s.temporary_dexterity);
    fight(s);drink(s,Potion::FyshOil);CHECK(s.strength==1&&s.dexterity==1);
    s.enemies[0].move=0;s.enemies[1].move=2;s.block=20;drink(s,Potion::Bronze);
    CHECK(end_turn(s));CHECK(s.hp==80&&s.enemies[0].hp==43);restore(s);
    fight(s);s.enemies[0].vulnerable=s.enemies[0].weak=9998;
    drink(s,Potion::Vulnerable);drink(s,Potion::Weak);CHECK(s.enemies[0].vulnerable==9999&&s.enemies[0].weak==9999);
    s.enemies[1].hp=0;drink(s,Potion::PotionOfBinding);CHECK(!s.enemies[1].weak&&!s.enemies[1].vulnerable);
    fight(s);s.no_draw=1;fill(s,Zone::Draw,10);drink(s,Potion::Swift);CHECK(!s.hand.count&&s.draw.count==10);
    drink(s,Potion::CureAll);CHECK(s.energy==21&&!s.hand.count);
    fight(s);int a=append(s,{Strike,0},Zone::Hand),b=append(s,{Defend,1},Zone::Hand);
    int c=append(s,{Strike,0},Zone::Discard);drink(s,Potion::Forge);
    CHECK(s.cards[a].upgraded&&s.cards[b].upgraded&&!s.cards[c].upgraded&&!s.deck[0].upgraded);restore(s);
}
static void choices_and_creation(){
    for(Potion p:{Potion::AttackPotion,Potion::SkillPotion,Potion::PowerPotion,Potion::ColorlessPotion}){
        State s{};fight(s);fill(s,Zone::Hand,2);drink(s,p);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active);
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
        int id=s.hand.ids[2];CHECK(s.cards[id].kind==chosen.kind&&cost(s,id)==(cost(chosen)<0?cost(chosen):0));
    }
    State s{};fight(s);append(s,{Strike,0},Zone::Hand);
    int unchosen=append(s,{Defend,0},Zone::Draw),chosen=append(s,{Defend,1},Zone::Draw);
    drink(s,Potion::DropletOfPrecognition);select(s,{(uint16_t)chosen});
    CHECK(s.zones[chosen]==Zone::Hand&&s.zones[unchosen]==Zone::Draw&&s.cards[chosen].upgraded);
    chosen=append(s,{Bludgeon,0},Zone::Discard);drink(s,Potion::LiquidMemories);select(s,{(uint16_t)chosen});
    CHECK(s.zones[chosen]==Zone::Hand&&cost(s,chosen)==0);
    fight(s);fill(s,Zone::Hand,4);fill(s,Zone::Draw,10);
    drink(s,Potion::GamblersBrew);uint16_t first=s.hand.ids[0],last=s.hand.ids[3];
    select(s,{first,last});CHECK(s.hand.count==4&&s.discard.count==2&&s.draw.count==8);
    CHECK(s.zones[first]==Zone::Discard&&s.zones[last]==Zone::Discard);
    fight(s);fill(s,Zone::Hand,3);s.no_draw=1;fill(s,Zone::Draw,3);
    drink(s,Potion::GamblersBrew);select(s,{s.hand.ids[1]});CHECK(s.hand.count==2&&s.draw.count==3);
    fight(s);fill(s,Zone::Hand,4);fill(s,Zone::Draw,12);
    s.pain=3;s.embrace=1;s.power_count=2;s.power_order[0]=0;s.power_order[1]=1;
    drink(s,Potion::Ashwater);select(s,{0,2,3});
    CHECK(s.exhaust.count==3&&s.hand.count==4&&s.block==9&&s.zones[1]==Zone::Hand);
    for(unsigned i=0;i<s.exhaust.count;i++)CHECK(s.exhaust.ids[i]<4);
    fight(s);fill(s,Zone::Hand,2);fill(s,Zone::Draw,15);s.embrace=1;s.power_count=1;s.power_order[0]=1;
    drink(s,Potion::GlowwaterPotion);CHECK(s.exhaust.count==2&&s.hand.count==10&&s.draw.count==5);
    fight(s);fill(s,Zone::Hand,2);fill(s,Zone::Draw,15);s.no_draw=1;
    drink(s,Potion::GlowwaterPotion);CHECK(s.exhaust.count==2&&!s.hand.count&&s.draw.count==15);
    fight(s);chosen=append(s,{Bludgeon,0},Zone::Hand);s.expansion.turn_cost[chosen]=4;
    drink(s,Potion::TouchOfInsanity);select(s,{(uint16_t)chosen});
    CHECK(cost(s,chosen)==0&&s.expansion.combat_cost[chosen]==1);
    quiet_turn(s);CHECK(cost(s,chosen)==0);
    fight(s);fill(s,Zone::Hand,9);s.no_draw=1;drink(s,Potion::CunningPotion);
    CHECK(s.hand.count==10&&s.discard.count==2);
    for(unsigned i=9;i<s.card_count;i++)CHECK(s.cards[i].kind==Shiv&&s.cards[i].upgraded);
    fight(s);fill(s,Zone::Hand,9);drink(s,Potion::PotOfGhouls);CHECK(s.hand.count==10&&s.discard.count==1);
    CHECK(s.cards[s.hand.ids[9]].kind==Soul&&s.cards[s.discard.ids[0]].kind==Soul);
    fight(s);drink(s,Potion::OrobicAcid);CHECK(s.hand.count==3);
    for(int i=0;i<3;i++){int id=s.hand.ids[i];CHECK(definitions[s.cards[id].kind].type==(CardType)i&&cost(s,id)==(cost(s.cards[id])<0?-1:0));}
    fight(s);drink(s,Potion::CosmicConcoction);CHECK(s.hand.count==3);
    for(unsigned i=0;i<3;i++){
        CHECK(card_colorless(s.cards[s.hand.ids[i]].kind)&&s.cards[s.hand.ids[i]].upgraded);
        for(unsigned j=0;j<i;j++)CHECK(s.cards[s.hand.ids[i]].kind!=s.cards[s.hand.ids[j]].kind);
    }
    restore(s);
}
static void timed_and_automatic(){
    State s{};fight(s);fill(s,Zone::Draw,30);drink(s,Potion::Clarity);drink(s,Potion::RadiantTincture);
    CHECK(s.hand.count==1&&s.energy==21);
    for(int turn=0;turn<4;turn++){
        quiet_turn(s);CHECK(s.hand.count==(turn<3?6:5));CHECK(s.energy==(turn<3?4:3));
        CHECK(power_value(s,XPower::Clarity)==(turn<3?2-turn:0));
        CHECK(power_value(s,XPower::Radiance)==(turn<3?2-turn:0));
    }
    fight(s);s.hp=20;drink(s,Potion::RegenPotion);drink(s,Potion::MazalethsGift);
    const int health[]={25,29,32,34,35,35};
    for(int turn=0;turn<6;turn++){quiet_turn(s);CHECK(s.hp==health[turn]&&s.strength==turn+1);}
    fight(s);drink(s,Potion::ShipInABottle);CHECK(s.block==10&&s.block_next_turn==10);
    quiet_turn(s);CHECK(s.block==10&&!s.block_next_turn);quiet_turn(s);CHECK(!s.block);
    fight(s);drink(s,Potion::HeartOfIron);s.enemies[0].move=0;
    CHECK(end_turn(s));CHECK(s.hp==75&&!s.block&&power_value(s,XPower::Plating)==6);
    quiet_turn(s);CHECK(s.hp==75&&!s.block&&power_value(s,XPower::Plating)==5);
    fight(s);fill(s,Zone::Hand,3);fill(s,Zone::Draw,30);drink(s,Potion::StableSerum);
    quiet_turn(s);CHECK(s.hand.count==8&&s.zones[0]==Zone::Hand&&power_value(s,XPower::Retain)==1);
    quiet_turn(s);CHECK(s.hand.count==10&&s.zones[0]==Zone::Hand&&!power_value(s,XPower::Retain));
    // The draw pile still contains enough cards to avoid immediately drawing
    // the discarded originals again after the second retained turn.
    for(auto&e:s.enemies)e.move=2;
    CHECK(end_turn(s));
    CHECK(s.discard.count==10&&s.hand.count==5);restore(s);
    fight(s);s.enemies[0].move=0;drink(s,Potion::BeetleJuice);CHECK(enemy_damage(s,0)==8);
    for(int i=0;i<4;i++){quiet_turn(s);CHECK(s.expansion.beetle[0]==3-i);}
    fight(s);s.block=100;for(auto&e:s.enemies)e.move=0;
    drink(s,Potion::ShacklingPotion);CHECK(s.enemies[0].strength==-7&&s.enemies[1].strength==-7);
    CHECK(end_turn(s));CHECK(s.enemies[0].strength==0&&s.enemies[1].strength==0&&!s.expansion.restore_strength[0]);
    fight(s);drink(s,Potion::PowderedDemise);s.enemies[0].block=999;
    quiet_turn(s);CHECK(s.enemies[0].hp==37);quiet_turn(s);CHECK(s.enemies[0].hp==28);
    fight(s);s.enemies[0].hp=33;drink(s,Potion::PotionOfDoom);CHECK(s.enemies[0].hp==33);
    quiet_turn(s);CHECK(!s.enemies[0].hp&&s.enemies[1].hp==46);
    fight(s);s.enemies[0].hp=40;drink(s,Potion::PotionOfDoom);drink(s,Potion::PowderedDemise);
    quiet_turn(s);CHECK(s.enemies[0].hp==31&&s.expansion.doom[0]==33);
    quiet_turn(s);CHECK(!s.enemies[0].hp&&s.enemies[1].hp==46);
    fight(s);drink(s,Potion::PoisonPotion);quiet_turn(s);CHECK(s.enemies[0].hp==40&&s.enemies[0].poison==5);
    quiet_turn(s);CHECK(s.enemies[0].hp==35&&s.enemies[0].poison==4);
    fight(s);drink(s,Potion::LuckyTonic);s.block=5;player_hp_loss(s,3,true,true);
    CHECK(s.hp==80&&s.block==2&&power_value(s,XPower::Buffer)==1);
    player_hp_loss(s,10,true,true);CHECK(s.hp==80&&!s.block&&!power_value(s,XPower::Buffer));
    player_hp_loss(s,7,true,true);CHECK(s.hp==73);
    fight(s);drink(s,Potion::GhostInAJar);player_hp_loss(s,30,true,true);CHECK(s.hp==79);
    quiet_turn(s);CHECK(!power_value(s,XPower::Intangible));player_hp_loss(s,30,true,true);CHECK(s.hp==49);
    fight(s);s.potions[0]=s.potions[1]=Potion::FairyInABottle;s.hp=1;s.max_hp=81;
    CHECK(!can_use_potion(s,0,0));player_hp_loss(s,20,true,true);
    CHECK(s.hp==24&&s.potions[0]==Potion::Empty&&s.potions[1]==Potion::FairyInABottle);restore(s);
    player_hp_loss(s,30,true,true);CHECK(s.hp==24&&s.potions[1]==Potion::Empty);
    player_hp_loss(s,30,true,true);CHECK(s.phase==Phase::Dead&&s.hp==0);restore(s);
    fight(s);s.hp=1;s.potions[1]=Potion::FairyInABottle;s.enemies[0].hp=s.enemies[1].hp=1;
    drink(s,Potion::FoulPotion);CHECK(s.hp==24&&s.phase==Phase::Reward&&s.potions[1]==Potion::Empty);restore(s);
    fight(s);s.hp=1;s.enemies[0].move=s.enemies[1].move=0;drink(s,Potion::Ambergris);
    CHECK(s.hp==41);CHECK(end_turn(s));CHECK(s.hp==41&&s.turn==2&&!s.expansion.skip_enemy);
    CHECK(end_turn(s));CHECK(s.hp==17&&s.turn==3);restore(s);
}
static void replay_autoplay_and_resources(){
    State s{};fight(s);append(s,{TwinStrike,0},Zone::Hand);drink(s,Potion::GigantificationPotion);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==16&&!power_value(s,XPower::Gigantic));
    fight(s);append(s,{Strike,0},Zone::Hand);drink(s,Potion::Duplicator);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==34&&s.expansion.cards_finished==2&&s.energy==19&&!power_value(s,XPower::Duplicator));
    fight(s);append(s,{Strike,0},Zone::Hand);drink(s,Potion::Duplicator);quiet_turn(s);
    CHECK(!power_value(s,XPower::Duplicator));
    fight(s);int strike=append(s,{TwinStrike,0},Zone::Hand);append(s,{Defend,0},Zone::Draw);
    drink(s,Potion::SoldiersStew);CHECK(s.expansion.replay[strike]==1&&!s.expansion.replay[1]);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==26&&s.expansion.cards_finished==2);
    fight(s);s.energy=0;append(s,{Strike,0},Zone::Draw);append(s,{Defend,0},Zone::Draw);append(s,{Discovery,0},Zone::Draw);
    drink(s,Potion::DistilledChaos);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active&&s.expansion.depth==1);
    uint16_t token=choice_pile(s).ids[0];select(s,{token});
    CHECK(s.phase==Phase::Combat&&s.expansion.cards_finished==3&&s.energy==0&&!s.expansion.depth&&!s.expansion.potion_active);
    CHECK(s.block==5&&s.enemies[0].hp+s.enemies[1].hp==86);restore(s);
    fight(s);fill(s,Zone::Hand,4);fill(s,Zone::Draw,8);fill(s,Zone::Discard,3);
    drink(s,Potion::BottledPotential);CHECK(s.hand.count==5&&s.draw.count==10&&!s.discard.count);restore(s);
    fight(s);fill(s,Zone::Hand,4,{Bludgeon,0});fill(s,Zone::Draw,10,{Defend,0});drink(s,Potion::SneckoOil);
    CHECK(s.hand.count==10);for(unsigned i=0;i<s.hand.count;i++)CHECK(cost(s,s.hand.ids[i])>=0&&cost(s,s.hand.ids[i])<=3);
    quiet_turn(s);for(unsigned i=0;i<s.card_count;i++)CHECK(!s.expansion.turn_cost[i]);
    fight(s);drink(s,Potion::BoneBrew);player_hp_loss(s,10,true,true);
    CHECK(s.hp==80&&s.expansion.osty_hp==5&&s.expansion.osty_max_hp==15);
    drink(s,Potion::BoneBrew);CHECK(s.expansion.osty_hp==20&&s.expansion.osty_max_hp==30);
    fight(s);drink(s,Potion::FocusPotion);drink(s,Potion::PotionOfCapacity);
    CHECK(s.expansion.orb_slots==2&&power_value(s,XPower::Focus)==2);
    drink(s,Potion::EssenceOfDarkness);CHECK(s.expansion.orb_count==2);quiet_turn(s);
    CHECK(s.expansion.dark[0]==14&&s.expansion.dark[1]==14);
    drink(s,Potion::EssenceOfDarkness);CHECK(s.expansion.dark[0]==6&&s.expansion.dark[1]==6);
    CHECK(s.enemies[0].hp==18&&s.enemies[1].hp==46);
    s.expansion.orb_slots=9;drink(s,Potion::PotionOfCapacity);CHECK(s.expansion.orb_slots==MaxOrbs);restore(s);
    fight(s);drink(s,Potion::StarPotion);drink(s,Potion::StarPotion);CHECK(s.expansion.stars==6);
    drink(s,Potion::KingsCourage);CHECK(s.hand.count==1&&s.cards[s.hand.ids[0]].kind==SovereignBlade);
    int blade=s.hand.ids[0];CHECK(s.expansion.blade_damage[blade]==25);
    drink(s,Potion::KingsCourage);CHECK(s.hand.count==1&&s.expansion.blade_damage[blade]==40);
    CHECK(play(s,0,1));CHECK(s.enemies[1].hp==6);restore(s);
}
static void acquisition(){
    for(Character c:{Character::Ironclad,Character::Silent}){
        State s{};fight(s,c);s.phase=Phase::Shop;unsigned allowed=0;
        for(unsigned k=1;k<=PotionKinds;k++)allowed+=potion_in_pool((Potion)k,c);
        CHECK(allowed==48);
        for(int i=0;i<500;i++)CHECK(potion_in_pool(random_potion(s),c));
        s.potions[1]=Potion::FairyInABottle;drink(s,Potion::EntropicBrew);
        CHECK(potion_in_pool(s.potions[0],c)&&s.potions[1]==Potion::FairyInABottle);restore(s);
    }
    State s{};fight(s);s.phase=Phase::Shop;s.gold=200;s.shop_potions[0]=(uint8_t)Potion::LiquidMemories;
    s.shop_prices[ShopPotionStart]=100;s.potions[0]=Potion::Fire;s.potions[1]=Potion::Block;auto before=s;
    CHECK(!buy(s,ShopPotionStart));unchanged(s,before);
    CHECK(discard_potion(s,1));CHECK(buy(s,ShopPotionStart));
    CHECK(s.potions[0]==Potion::Fire&&s.potions[1]==Potion::LiquidMemories&&s.gold==100);
    before=s;CHECK(!buy(s,ShopPotionStart));unchanged(s,before);restore(s);
    fight(s);s.phase=Phase::Reward;s.reward_potion=Potion::CunningPotion;s.potions[0]=Potion::Fire;s.potions[1]=Potion::Block;
    before=s;CHECK(!claim_reward_potion(s));unchanged(s,before);CHECK(discard_potion(s,0));
    CHECK(claim_reward_potion(s));CHECK(s.potions[0]==Potion::CunningPotion&&s.potions[1]==Potion::Block&&(s.reward_claimed&4));
    before=s;CHECK(!claim_reward_potion(s));unchanged(s,before);restore(s);
    fight(s);s.phase=Phase::Shop;s.hp=31;s.gold=9950;drink(s,Potion::FoulPotion);
    CHECK(s.hp==31&&s.gold==9999&&s.enemies[0].hp==46);restore(s);
}
// Static inspection of the locked DLL establishes these lifetime and stacking
// rules. Unlike the earlier descriptor-only tests, the expected boundaries are
// supported by CardEnergyCost, the matching power hooks and PotionFactory.
static void pinned_cost_lifetimes(){
    State s{};fight(s);int id=append(s,{Strike,0},Zone::Hand);s.rng_cards=2;
    gain_power(s,XPower::Nostalgia,2);drink(s,Potion::SneckoOil);
    CHECK(cost(s,id)==2);restore(s);CHECK(play(s,0,0));
    CHECK(s.energy==18&&!s.expansion.turn_cost[id]&&s.zones[id]==Zone::Draw);
    drink(s,Potion::Swift);CHECK(s.hand.count==1&&s.hand.ids[0]==id&&cost(s,id)==1);
    CHECK(play(s,0,0));CHECK(s.energy==17&&s.enemies[0].hp==34);restore(s);

    // Generated/returned "free this turn" cards use the same expiry as Snecko:
    // their first play removes the override, including a same-turn return.
    fight(s);id=append(s,{Strike,0},Zone::Discard);gain_power(s,XPower::Nostalgia,2);
    drink(s,Potion::LiquidMemories);select(s,{(uint16_t)id});CHECK(cost(s,id)==0);
    CHECK(play(s,0,0));CHECK(s.energy==20&&!s.expansion.turn_cost[id]);
    drink(s,Potion::Swift);CHECK(s.hand.ids[0]==id&&cost(s,id)==1);
    CHECK(play(s,0,0));CHECK(s.energy==19);restore(s);

    // A later Snecko override expires back to Touch of Insanity's combat-wide
    // zero cost. Applying Touch after Snecko makes the first play free too.
    for(bool touch_first:{false,true}){
        fight(s);id=append(s,{Strike,0},Zone::Hand);gain_power(s,XPower::Nostalgia,2);s.rng_cards=2;
        if(touch_first){drink(s,Potion::TouchOfInsanity);select(s,{(uint16_t)id});}
        drink(s,Potion::SneckoOil);CHECK(cost(s,id)==2);
        if(!touch_first){drink(s,Potion::TouchOfInsanity);select(s,{(uint16_t)id});}
        CHECK(cost(s,id)==(touch_first?2:0));CHECK(play(s,0,0));
        int energy=touch_first?18:20;CHECK(s.energy==energy);
        drink(s,Potion::Swift);CHECK(cost(s,id)==0&&s.expansion.combat_cost[id]==1);
        CHECK(play(s,0,0));CHECK(s.energy==energy);restore(s);
    }

    // X-cost is excluded before sampling, and free-cost setters do not replace
    // the X payment/captured value with a zero-energy play.
    State ordinary{};fight(ordinary);append(ordinary,{Strike,0},Zone::Hand);ordinary.rng_cards=2;
    drink(ordinary,Potion::SneckoOil);
    fight(s);append(s,{Strike,0},Zone::Hand);id=append(s,{Whirlwind,0},Zone::Hand);s.rng_cards=2;
    drink(s,Potion::SneckoOil);CHECK(s.rng_cards==ordinary.rng_cards&&cost(s,0)==cost(ordinary,0));
    CHECK(cost(s,id)==-1&&!s.expansion.turn_cost[id]);
    drink(s,Potion::TouchOfInsanity);CHECK(choice_pile(s).count==1&&choice_pile(s).ids[0]==0);
    auto before=s;uint16_t excluded=(uint16_t)id;
    CHECK(!choose_extended(s,&excluded,1));unchanged(s,before);select(s,{0});CHECK(cost(s,id)==-1);
    s.energy=2;CHECK(play(s,1,0));CHECK(s.energy==0&&s.enemies[0].hp==36&&s.enemies[1].hp==36);restore(s);

    fight(s);id=append(s,{Whirlwind,0},Zone::Discard);drink(s,Potion::LiquidMemories);
    select(s,{(uint16_t)id});CHECK(cost(s,id)==-1);s.energy=2;
    CHECK(play(s,0,0));CHECK(s.energy==0&&s.enemies[0].hp==36&&s.enemies[1].hp==36);restore(s);

    // Touch's original hand predicate excludes cards with no payable cost,
    // including existing temporary/combat-free instances and X cards.
    fight(s);append(s,{Finesse,0},Zone::Hand);int temporarily_free=append(s,{Strike,0},Zone::Hand);
    s.expansion.turn_cost[temporarily_free]=1;int permanently_free=append(s,{Defend,0},Zone::Hand);
    s.expansion.combat_cost[permanently_free]=1;append(s,{Whirlwind,0},Zone::Hand);
    int eligible=append(s,{Bludgeon,0},Zone::Hand);drink(s,Potion::TouchOfInsanity);
    CHECK(choice_pile(s).count==1&&choice_pile(s).ids[0]==eligible);
    select(s,{(uint16_t)eligible});CHECK(cost(s,eligible)==0);restore(s);
    fight(s);append(s,{Finesse,0},Zone::Hand);append(s,{Whirlwind,0},Zone::Hand);
    drink(s,Potion::TouchOfInsanity);CHECK(s.phase==Phase::Combat&&!s.expansion.potion_active);restore(s);
    fight(s);eligible=append(s,{Finesse,0},Zone::Hand);s.expansion.turn_cost[eligible]=3;
    drink(s,Potion::TouchOfInsanity);CHECK(choice_pile(s).count==1&&choice_pile(s).ids[0]==eligible);
    select(s,{(uint16_t)eligible});CHECK(cost(s,eligible)==0&&s.expansion.combat_cost[eligible]==1&&!s.expansion.turn_cost[eligible]);
}
static void pinned_stacking_and_piles(){
    State s{};fight(s);fill(s,Zone::Draw,60);
    for(int i=0;i<2;i++){drink(s,Potion::Clarity);drink(s,Potion::RadiantTincture);}
    CHECK(s.hand.count==2&&s.energy==22);
    for(int turn=0;turn<7;turn++){
        quiet_turn(s);CHECK(s.hand.count==(turn<6?6:5)&&s.energy==(turn<6?4:3));
        CHECK(power_value(s,XPower::Clarity)==(turn<6?5-turn:0));
        CHECK(power_value(s,XPower::Radiance)==(turn<6?5-turn:0));
    }
    fight(s);fill(s,Zone::Hand,3,{Strike,0});
    drink(s,Potion::Duplicator);drink(s,Potion::Duplicator);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==34&&power_value(s,XPower::Duplicator)==1);
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==22&&!power_value(s,XPower::Duplicator));
    CHECK(play(s,0,0));CHECK(s.enemies[0].hp==16&&s.energy==17&&s.expansion.cards_finished==5);restore(s);

    fight(s);append(s,{TwinStrike,0},Zone::Hand);drink(s,Potion::SoldiersStew);
    queue_extension(s,XOp::Create,1,Strike);resolve_pending(s);
    CHECK(s.hand.count==2&&s.expansion.replay[0]==1&&!s.expansion.replay[1]);
    CHECK(play(s,0,0));CHECK(play(s,0,0));
    CHECK(s.enemies[0].hp==20&&s.expansion.cards_finished==3);restore(s);

    // Bottled Potential affects hand/draw/discard, preserving exhausted cards
    // and installed powers. Its shuffle can suspend for Stratagem before draw.
    fight(s);fill(s,Zone::Hand,2);fill(s,Zone::Draw,6);fill(s,Zone::Discard,3);
    int exhausted=append(s,{Strike,1},Zone::Exhaust),installed=append(s,{Inflame,0},Zone::Power);
    gain_power(s,XPower::Stratagem,1);drink(s,Potion::BottledPotential);
    CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active);
    CHECK(!s.hand.count&&s.draw.count==11&&!s.discard.count&&choice_pile(s).count==11);
    CHECK(choice_min(s)==1&&choice_max(s)==1);uint16_t chosen=choice_pile(s).ids[0];select(s,{chosen});
    CHECK(s.hand.count==6&&s.draw.count==5&&s.zones[chosen]==Zone::Hand);
    CHECK(s.exhaust.count==1&&s.exhaust.ids[0]==exhausted&&s.powers.count==1&&s.powers.ids[0]==installed);
    CHECK(!s.expansion.potion_active);restore(s);
}
static void pinned_generation(){
    // Repeated seeds expose duplicates from independent sampling. The original
    // Cosmic Concoction factory samples three distinct, upgraded models.
    for(uint32_t seed=1;seed<=80;seed++){
        State s{};fight(s);s.rng_cards=seed;drink(s,Potion::CosmicConcoction);CHECK(s.hand.count==3);
        for(unsigned i=0;i<3;i++){
            const auto&c=s.cards[s.hand.ids[i]];CHECK(card_colorless(c.kind)&&c.upgraded);
            for(unsigned j=0;j<i;j++)CHECK(c.kind!=s.cards[s.hand.ids[j]].kind);
        }
    }
    for(Character c:{Character::Ironclad,Character::Silent})for(bool combat:{false,true}){
        State s{};fight(s,c);s.rng_world=197;unsigned rarity[4]{},seen[PotionKinds+1]{};
        for(unsigned i=0;i<4096;i++){
            Potion p=random_potion(s,combat);CHECK(potion_in_pool(p,c));
            if(combat)CHECK(p!=Potion::FairyInABottle&&p!=Potion::Fruit&&p!=Potion::RegenPotion);
            ++rarity[potion_definition(p)->rarity];++seen[(unsigned)p];
        }
        // Broad deterministic distribution bounds detect model-uniform
        // sampling without depending on original PC random seed equivalence.
        CHECK(rarity[1]>2450&&rarity[1]<2850);
        CHECK(rarity[2]>850&&rarity[2]<1200);
        CHECK(rarity[3]>300&&rarity[3]<520);
        for(unsigned k=1;k<=PotionKinds;k++){
            Potion p=(Potion)k;bool allowed=potion_in_pool(p,c)&&
                (!combat||(p!=Potion::FairyInABottle&&p!=Potion::Fruit&&p!=Potion::RegenPotion));
            CHECK(bool(seen[k])==allowed);
        }
    }
    struct Expected{uint32_t seed;Potion first,second;};
    const Expected cases[]={
        {12,Potion::RegenPotion,Potion::FyshOil},
        {53,Potion::Fruit,Potion::Dexterity},
        {382,Potion::FairyInABottle,Potion::RegenPotion},
        {279,Potion::EntropicBrew,Potion::AttackPotion},
        {15,Potion::SkillPotion,Potion::SkillPotion}
    };
    for(const auto&expected:cases){
        State s{};fight(s);s.rng_world=expected.seed;drink(s,Potion::EntropicBrew);
        CHECK(s.phase==Phase::Combat&&s.potions[0]==expected.first&&s.potions[1]==expected.second);restore(s);
        fight(s);s.phase=Phase::Shop;s.rng_world=expected.seed;drink(s,Potion::EntropicBrew);
        CHECK(s.potions[0]==expected.first&&s.potions[1]==expected.second);restore(s);
    }
    // The combat-generation opcode used by Alchemize has its stricter filter.
    State s{};fight(s);s.rng_world=382;append(s,{Alchemize,0},Zone::Hand);
    CHECK(play(s,0,0));CHECK(s.potions[0]!=Potion::Empty&&s.potions[0]!=Potion::FairyInABottle&&
        s.potions[0]!=Potion::Fruit&&s.potions[0]!=Potion::RegenPotion);restore(s);
}
static void fairy_in_events(){
    for(EventId event:{EventId::Baths,EventId::Vegetation}){
        State s{};fight(s);s.phase=Phase::Event;s.event_id=event;s.event_value=event==EventId::Vegetation?75:0;
        s.max_hp=event==EventId::Baths?78:80;s.hp=1;s.potions[0]=s.potions[1]=Potion::FairyInABottle;
        int gold=s.gold;CHECK(event_choice(s,0));
        CHECK(s.phase==Phase::Event&&s.hp==24&&s.max_hp==80);
        CHECK(s.potions[0]==Potion::Empty&&s.potions[1]==Potion::FairyInABottle);
        if(event==EventId::Baths)CHECK(s.event_count==1&&s.event_page==1&&s.gold==gold);
        else CHECK(s.event_page==2&&s.gold==gold+75);
        restore(s);

        fight(s);s.phase=Phase::Event;s.event_id=event;s.event_value=event==EventId::Vegetation?75:0;
        s.max_hp=event==EventId::Baths?78:80;s.hp=1;gold=s.gold;
        CHECK(event_choice(s,0));CHECK(s.phase==Phase::Dead&&s.hp==0&&s.gold==gold&&s.event_page==0);restore(s);

        fight(s);s.phase=Phase::Event;s.event_id=event;s.event_value=event==EventId::Vegetation?75:0;
        s.max_hp=event==EventId::Baths?78:80;s.hp=10;s.potions[1]=Potion::FairyInABottle;
        CHECK(event_choice(s,0));CHECK(s.phase==Phase::Event&&s.hp==(event==EventId::Baths?9:2));
        CHECK(s.potions[1]==Potion::FairyInABottle);restore(s);
    }
}
static void pinned_full_hand_choices(){
    for(Potion potion:{Potion::DropletOfPrecognition,Potion::LiquidMemories}){
        State s{};fight(s);fill(s,Zone::Hand,MaxHand);s.no_draw=1;
        Zone source=potion==Potion::DropletOfPrecognition?Zone::Draw:Zone::Discard;
        int unchosen=append(s,{Strike,0},source),chosen=append(s,{Bludgeon,1},source);
        s.expansion.turn_cost[chosen]=4;
        drink(s,potion);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active);
        CHECK(choice_pile(s).count==2&&choice_min(s)==1&&choice_max(s)==1);
        auto before=s;CHECK(!choose_extended(s,nullptr,0));unchanged(s,before);
        select(s,{(uint16_t)chosen});
        CHECK(s.hand.count==MaxHand&&s.zones[unchosen]==source&&s.zones[chosen]==Zone::Discard);
        CHECK(s.cards[chosen].kind==Bludgeon&&s.cards[chosen].upgraded&&s.no_draw);
        for(unsigned i=0;i<MaxHand;i++)CHECK(s.hand.ids[i]==i);
        CHECK(s.expansion.last_drawn==-1&&!s.expansion.potion_active);
        if(potion==Potion::DropletOfPrecognition){
            CHECK(s.draw.count==1&&s.discard.count==1&&s.discard.ids[0]==chosen&&cost(s,chosen)==3);
        }else{
            CHECK(!s.draw.count&&s.discard.count==2&&s.discard.ids[1]==chosen&&cost(s,chosen)==0);
        }
        restore(s);
        fight(s);fill(s,Zone::Hand,MaxHand);drink(s,potion);
        CHECK(s.phase==Phase::Combat&&s.hand.count==MaxHand&&!s.expansion.potion_active);restore(s);
    }
}
static void distilled_batch(){
    State s{};fight(s);s.energy=0;
    int drawn=append(s,{Defend,1},Zone::Draw);
    append(s,{TwinStrike,0},Zone::Draw);append(s,{Defend,0},Zone::Draw);append(s,{FlashOfSteel,0},Zone::Draw);
    drink(s,Potion::DistilledChaos);
    CHECK(s.phase==Phase::Combat&&s.energy==0&&s.expansion.cards_finished==3);
    CHECK(s.hand.count==1&&s.hand.ids[0]==drawn&&s.draw.count==0&&s.discard.count==3);
    CHECK(s.block==5);
    // In the pinned release Flash of Steel deals 5, then Twin Strike 5 twice.
    CHECK(s.enemies[0].hp+s.enemies[1].hp==77);
    CHECK(!s.expansion.potion_active);restore(s);

    // The first reserved card remains in Limbo while fetching the rest of the
    // batch triggers a shuffle choice. Save/load must preserve its identity.
    fight(s);s.energy=0;int reserved=append(s,{FlashOfSteel,0},Zone::Draw);
    int chosen=append(s,{Strike,0},Zone::Discard);append(s,{Defend,0},Zone::Discard);
    gain_power(s,XPower::Stratagem,1);drink(s,Potion::DistilledChaos);
    CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.potion_active);
    CHECK(s.zones[reserved]==Zone::Limbo&&s.playing==-1&&s.expansion.cards_finished==0);
    select(s,{(uint16_t)chosen});
    CHECK(s.phase==Phase::Combat&&s.expansion.cards_finished==2&&s.energy==0);
    CHECK(s.hand.count==1&&s.hand.ids[0]==chosen&&s.discard.count==2&&!s.draw.count);
    CHECK(s.block==5&&s.enemies[0].hp+s.enemies[1].hp==87&&!s.expansion.potion_active);restore(s);

    // Early victory must retire all reserved instances as well as the playing
    // card. The original tears down its CombatState at this point, so this
    // checks the expedition's serializable boundary without inventing its pile.
    fight(s);s.energy=3;append(s,{Strike,0},Zone::Draw);append(s,{Defend,0},Zone::Draw);
    append(s,{Whirlwind,0},Zone::Draw);s.enemies[0].hp=s.enemies[1].hp=1;
    drink(s,Potion::DistilledChaos);CHECK(s.phase==Phase::Reward&&!s.expansion.potion_active);
    for(unsigned id=0;id<s.card_count;id++)CHECK(s.zones[id]!=Zone::Limbo);
    restore(s);
}
static void costs_before_enemy_choice(){
    State s{};fight(s);int recalled=append(s,{Bludgeon,0},Zone::Discard);
    drink(s,Potion::LiquidMemories);select(s,{(uint16_t)recalled});CHECK(cost(s,recalled)==0);
    drink(s,Potion::StableSerum);CHECK(gain_relic(s,CentennialPuzzle));
    int drawn=append(s,{Strike,0},Zone::Discard);s.expansion.turn_cost[drawn]=4;
    int combat_free=append(s,{Defend,0},Zone::Discard);
    s.expansion.combat_cost[combat_free]=1;s.expansion.turn_cost[combat_free]=3;
    int exhausted=append(s,{Defend,0},Zone::Exhaust);s.expansion.turn_cost[exhausted]=1;
    gain_power(s,XPower::Stratagem,1);s.enemies[0].move=0;s.enemies[1].move=2;
    CHECK(end_turn(s));
    // Player flush is complete, but the enemy's first HP hit has suspended
    // Centennial Puzzle's draw at a shuffle/Stratagem choice. This is still
    // turn 1: checking only after the next player turn misses the old bug.
    CHECK(s.phase==Phase::ExtendedChoice&&s.turn==1&&s.expansion.enemy_side&&s.hp==68);
    CHECK(relic_counter(s,CentennialPuzzle)==1&&s.hand.count==1&&s.hand.ids[0]==recalled);
    CHECK(cost(s,recalled)==3&&cost(s,drawn)==1&&cost(s,combat_free)==0&&cost(s,exhausted)==1);
    for(unsigned id=0;id<s.card_count;id++)CHECK(!s.expansion.turn_cost[id]);
    CHECK(choice_pile(s).count==2);select(s,{(uint16_t)drawn});
    CHECK(s.phase==Phase::Combat&&s.turn==2&&!s.expansion.enemy_side&&s.hp==68);
    CHECK(s.zones[recalled]==Zone::Hand&&s.zones[drawn]==Zone::Hand&&s.zones[combat_free]==Zone::Hand);
    CHECK(s.zones[exhausted]==Zone::Exhaust&&cost(s,recalled)==3&&cost(s,drawn)==1&&cost(s,combat_free)==0);
    restore(s);
}
int main(){
    scopes_and_inventory();resources_and_damage();choices_and_creation();timed_and_automatic();replay_autoplay_and_resources();acquisition();
    pinned_cost_lifetimes();pinned_stacking_and_piles();pinned_generation();fairy_in_events();pinned_full_hand_choices();distilled_batch();costs_before_enemy_choice();
    std::printf("potion completion: %u checks; %u model policies\n",checks,policy_models);
}
