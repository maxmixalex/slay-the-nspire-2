#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

using namespace spire;
static unsigned checks,variants;
static unsigned active_kind,active_upgrade;
static bool covered[CardKinds][2]{};
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %s:%d %s [%s%s]\n",__FILE__,__LINE__,#x,definitions[active_kind].key,active_upgrade?"+":""); std::exit(1); } } while (0)

static int add(State&s,unsigned kind,unsigned up=0,Zone zone=Zone::Hand){
    CHECK(s.card_count<MaxCards);
    int id=s.card_count++;
    s.cards[id]={(CardKind)kind,(uint8_t)up};s.zones[id]=zone;
    Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
    p.ids[p.count++]=(uint16_t)id;
    return id;
}
static State fight(unsigned kind,unsigned up=0){
    State s{};new_run(s,0x41cef1ea,false);
    s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;s.energy=100;
    s.relics=s.relic_count=s.relic_flash=0;s.deck_count=1;s.deck[0]={(CardKind)kind,(uint8_t)up};
    for(auto&e:s.enemies)e={46,46,0,0,0,2};
    add(s,kind,up);
    return s;
}
static void supply(State&s,int count,unsigned kind=Defend,Zone zone=Zone::Draw){
    for(int i=0;i<count;i++)add(s,kind,0,zone);
}
static bool contains(const Pile&p,int id){for(unsigned i=0;i<p.count;i++)if(p.ids[i]==id)return true;return false;}
static int total_hp(const State&s){int n=0;for(int i=0;i<s.enemy_count;i++)n+=s.enemies[i].hp;return n;}
static int kind_count(const State&s,const Pile&p,unsigned kind){int n=0;for(unsigned i=0;i<p.count;i++)n+=s.cards[p.ids[i]].kind==kind;return n;}
static void act(State&s,unsigned kind,int target=0){
    int index=-1;for(unsigned i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==kind){index=(int)i;break;}
    CHECK(index>=0);CHECK(play(s,index,target));
}
static SaveData snapshot(const State&s){SaveData b{};CHECK(validate(s));CHECK(save(s,b));return b;}
static void same(const State&s,const SaveData&before){auto after=snapshot(s);CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,after.size));}
static void roundtrip(const State&s){auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));same(copy,b);}
static void ready(const State&s){CHECK(s.phase==Phase::Combat&&s.playing==-1&&!s.effect_count&&!s.expansion.depth);roundtrip(s);}
static void next_turn(State&s){for(auto&e:s.enemies){e.move=2;e.strength=0;}CHECK(end_turn(s));}
static void pick(State&s,unsigned n,unsigned offset=0){
    CHECK(s.phase==Phase::ExtendedChoice);
    auto before=snapshot(s);State copy{};CHECK(load(copy,before.bytes,before.size));
    uint16_t ids[MaxHand]{};CHECK(n<=MaxHand&&offset+n<=s.expansion.choices.count);
    for(unsigned i=0;i<n;i++)ids[i]=s.expansion.choices.ids[offset+i];
    const uint16_t wrong=65535;
    CHECK(!choose_extended(s,&wrong,1));same(s,before);
    if(n>1){uint16_t duplicate[MaxHand]{};for(unsigned i=0;i<n;i++)duplicate[i]=ids[0];CHECK(!choose_extended(s,duplicate,n));same(s,before);}
    CHECK(choose_extended(s,ids,n));CHECK(choose_extended(copy,ids,n));same(copy,snapshot(s));
}
static void drain_choices(State&s){
    for(unsigned step=0;choice_phase(s.phase)&&step<32;step++){
        if(s.phase==Phase::ExtendedChoice)pick(s,choice_min(s));
        else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
        else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
        else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
        else {uint16_t ids[MaxHand]{};unsigned n=choice_min(s);for(unsigned i=0;i<n;i++)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,n));}
    }
    CHECK(!choice_phase(s.phase));
}

// These contracts exercise effects and their continuations, with explicit
// numbers from the retained v0.111.0 source audit. They are implementation
// regressions, not a claim of running a differential original-game oracle.
static void card_contract(unsigned kind,unsigned up){
    active_kind=kind;active_upgrade=up;
    State s=fight(kind,up);
    switch(kind){
    case Finesse:
        supply(s,3);act(s,kind);CHECK(s.block==4+3*(int)up&&s.hand.count==1&&s.draw.count==2);break;
    case FlashOfSteel:
        supply(s,3);act(s,kind);CHECK(s.enemies[0].hp==46-5-3*(int)up&&s.hand.count==1);break;
    case Impatience:{
        supply(s,8);act(s,kind);CHECK(s.hand.count==2+up);
        auto blocked=fight(kind,up);supply(blocked,8);add(blocked,Strike);act(blocked,kind);CHECK(blocked.hand.count==1&&blocked.draw.count==8);roundtrip(blocked);break;}
    case MasterOfStrategy:
        supply(s,8);act(s,kind);CHECK(s.hand.count==3+up&&s.zones[0]==Zone::Exhaust);break;
    case Shockwave:
        act(s,kind);for(const auto&e:s.enemies)CHECK(e.weak==3+2*(int)up&&e.vulnerable==3+2*(int)up);CHECK(s.zones[0]==Zone::Exhaust);break;
    case Scrawl:
        supply(s,14);add(s,Defend);CHECK(card_retains(s.cards[0])==(up!=0));act(s,kind);CHECK(s.hand.count==MaxHand&&s.draw.count==5&&s.zones[0]==Zone::Exhaust&&s.energy==99);break;
    case Purity:{
        int a=add(s,Defend),b=add(s,Strike);supply(s,2);
        act(s,kind);CHECK(s.phase==Phase::ExhaustManyChoice&&choice_max(s)==2);
        uint16_t ids[]={(uint16_t)b,(uint16_t)a};CHECK(choose_exhaust_many(s,ids,2));
        CHECK(s.exhaust.count==3&&!s.hand.count&&s.draw.count==2);break;}
    case HandOfGreed:{
        act(s,kind);CHECK(s.enemies[0].hp==46-20-5*(int)up&&s.gold==99);
        auto lethal=fight(kind,up);lethal.enemies[0].hp=1;act(lethal,kind);CHECK(!lethal.enemies[0].hp&&lethal.gold==119+5*(int)up);ready(lethal);break;}
    case PanicButton:
        act(s,kind);CHECK(s.block==30+10*(int)up&&s.no_block==2);add(s,UltimateDefend);act(s,UltimateDefend);CHECK(s.block==30+10*(int)up);break;
    case Alchemize:
        act(s,kind);CHECK(s.potions[0]!=Potion::Empty&&potion_in_pool(s.potions[0],s.character)&&s.zones[0]==Zone::Exhaust&&s.energy==99+(int)up);break;
    case Anointed:{
        int common=add(s,Inflame,0,Zone::Draw),rare1=add(s,Offering,0,Zone::Draw),rare2=add(s,DemonForm,1,Zone::Draw);
        s.no_draw=1;act(s,kind);CHECK(s.hand.count==2&&contains(s.hand,rare1)&&contains(s.hand,rare2)&&contains(s.draw,common)&&s.zones[0]==Zone::Exhaust);
        CHECK(card_retains({(CardKind)kind,(uint8_t)up})==(up!=0));break;}
    case Automation:{
        act(s,kind);CHECK(power_value(s,XPower::Automation)==1&&s.energy==99+(int)up);supply(s,12);int energy=s.energy;
        draw_cards(s,9);CHECK(s.energy==energy&&s.expansion.automation_count[0]==9);
        draw_cards(s,1);CHECK(s.energy==energy+1&&s.expansion.automation_count[0]==0);break;}
    case BeatDown:
        supply(s,3+(int)up,Shiv,Zone::Discard);act(s,kind);
        CHECK(s.expansion.cards_finished==4+up&&s.exhaust.count==3+up&&total_hp(s)==92-4*(3+(int)up)&&s.energy==97);break;
    case Bolas:case ThrummingHatchet:{
        const int hit=kind==Bolas?3+(int)up:11+3*(int)up;
        act(s,kind);CHECK(s.enemies[0].hp==46-hit&&s.expansion.return_turn[0]==2);
        supply(s,8);next_turn(s);CHECK(contains(s.hand,0)&&!s.expansion.return_turn[0]&&s.hand.count==6);break;}
    case Calamity:{
        act(s,kind);int strike=add(s,Strike);CHECK(power_value(s,XPower::Calamity)==1&&s.energy==97+(int)up);act(s,Strike);
        CHECK(s.hand.count==1&&s.card_count==3&&definitions[s.cards[s.hand.ids[0]].kind].type==CardType::Attack&&s.zones[strike]==Zone::Discard);break;}
    case Catastrophe:
        supply(s,2+(int)up,Shiv);act(s,kind);
        CHECK(s.expansion.cards_finished==3+up&&s.exhaust.count==2+up&&total_hp(s)==92-4*(2+(int)up)&&s.energy==98);break;
    case DarkShackles:
        s.enemies[0].strength=3;act(s,kind);CHECK(s.enemies[0].strength==-6-6*(int)up&&s.expansion.restore_strength[0]==9+6*(int)up);
        s.enemies[0].move=0;s.enemies[1].move=2;CHECK(end_turn(s));CHECK(s.enemies[0].strength==3&&!s.expansion.restore_strength[0]);break;
    case Discovery:{
        act(s,kind);CHECK(s.expansion.choice==XChoice::Offer&&s.expansion.choices.count==3);
        Card selected=choice_card(s,s.expansion.choices.ids[1]);pick(s,1,1);
        // Expanded character pools can offer an X card. The original local
        // free-cost modifier leaves that canonical X cost unchanged.
        CHECK(s.hand.count==1&&s.cards[s.hand.ids[0]].kind==selected.kind&&!s.cards[s.hand.ids[0]].upgraded&&cost(s,s.hand.ids[0])==(cost(selected)<0?cost(selected):0));
        CHECK(s.zones[0]==(up?Zone::Discard:Zone::Exhaust));break;}
    case DramaticEntrance:
        CHECK(card_innate(s.cards[0]));act(s,kind);CHECK(total_hp(s)==92-2*(11+4*(int)up)&&s.zones[0]==Zone::Exhaust);break;
    case Entropy:{
        act(s,kind);CHECK(power_value(s,XPower::Entropy)==1&&card_innate({(CardKind)kind,(uint8_t)up})==(up!=0));supply(s,8);
        next_turn(s);CHECK(s.expansion.choice==XChoice::Transform&&choice_min(s)==1&&choice_max(s)==1);
        int id=s.expansion.choices.ids[0];unsigned old=s.cards[id].kind;pick(s,1);
        CHECK(s.cards[id].kind!=old&&s.hand.count==5&&s.draw.count==3);break;}
    case Equilibrium:case Salvo:{
        int retained=add(s,Defend);supply(s,8);act(s,kind);CHECK(power_value(s,XPower::Retain)==1);
        if(kind==Equilibrium)CHECK(s.block==13+3*(int)up);else CHECK(s.enemies[0].hp==46-12-4*(int)up);
        next_turn(s);CHECK(contains(s.hand,retained)&&!power_value(s,XPower::Retain)&&s.hand.count==6);break;}
    case EternalArmor:
        act(s,kind);CHECK(power_value(s,XPower::Plating)==9+3*(int)up);next_turn(s);CHECK(s.block==0&&power_value(s,XPower::Plating)==8+3*(int)up);break;
    case Fasten:
        act(s,kind);add(s,Defend);add(s,UltimateDefend);add(s,Finesse);
        act(s,Defend);CHECK(s.block==9+2*(int)up);act(s,UltimateDefend);CHECK(s.block==24+4*(int)up);act(s,Finesse);CHECK(s.block==28+4*(int)up);break;
    case Fisticuffs:
        s.enemies[0].block=3;act(s,kind);CHECK(s.enemies[0].hp==42-2*(int)up&&s.block==7+2*(int)up);break;
    case GoldAxe:
        s.expansion.cards_finished=7;s.gold=1;act(s,kind);CHECK(s.enemies[0].hp==39&&s.expansion.cards_finished==8&&s.gold==1);CHECK(card_retains({(CardKind)kind,(uint8_t)up})==(up!=0));break;
    case HiddenGem:{
        int id=add(s,Strike,0,Zone::Draw);act(s,kind);CHECK(s.expansion.replay[id]==2+up);draw_cards(s,1);act(s,Strike);
        CHECK(s.expansion.cards_finished==4+up&&s.enemies[0].hp==46-6*(3+(int)up)&&s.zones[id]==Zone::Discard&&s.energy==98);break;}
    case JackOfAllTrades:
        s.no_draw=1;act(s,kind);CHECK(s.hand.count==1+up&&s.zones[0]==Zone::Exhaust);
        for(unsigned i=0;i<s.hand.count;i++)CHECK(card_colorless(s.cards[s.hand.ids[i]].kind)&&s.cards[s.hand.ids[i]].kind!=JackOfAllTrades&&card_can_generate(s.cards[s.hand.ids[i]].kind));
        if(up)CHECK(s.cards[s.hand.ids[0]].kind!=s.cards[s.hand.ids[1]].kind);break;
    case Jackpot:
        act(s,kind);CHECK(s.enemies[0].hp==21-5*(int)up&&s.hand.count==3);
        for(unsigned i=0;i<s.hand.count;i++){Card c=s.cards[s.hand.ids[i]];CHECK(cost({c.kind,0})==0&&c.upgraded==up);}break;
    case Mayhem:
        act(s,kind);CHECK(s.energy==98+(int)up);add(s,Shiv,0,Zone::Draw);supply(s,5);next_turn(s);
        CHECK(total_hp(s)==88&&s.exhaust.count==1&&s.expansion.cards_finished==2&&s.hand.count==5&&s.energy==3);break;
    case MindBlast:
        supply(s,7);CHECK(card_innate(s.cards[0]));act(s,kind);CHECK(s.enemies[0].hp==39&&s.energy==99+(int)up);break;
    case Nostalgia:{
        act(s,kind);CHECK(s.energy==99+(int)up);int first=add(s,Strike),second=add(s,Defend);act(s,Strike);CHECK(s.zones[first]==Zone::Draw&&s.draw.ids[s.draw.count-1]==first);
        act(s,Defend);CHECK(s.zones[second]==Zone::Discard&&s.expansion.nonpower_started==2);break;}
    case Omnislice:
        act(s,kind);CHECK(total_hp(s)==92-2*(8+3*(int)up));break;
    case Panache:
        act(s,kind);CHECK(s.expansion.panache_count[0]==0);supply(s,5,Production,Zone::Hand);
        for(int i=0;i<4;i++)act(s,Production);CHECK(total_hp(s)==92&&s.expansion.panache_count[0]==4);
        act(s,Production);CHECK(total_hp(s)==92-2*(10+4*(int)up)&&s.expansion.panache_count[0]==0);break;
    case PrepTime:
        act(s,kind);next_turn(s);CHECK(s.vigor==4+2*(int)up);next_turn(s);CHECK(s.vigor==8+4*(int)up);
        s.energy=100;add(s,TwinStrike);act(s,TwinStrike);CHECK(s.enemies[0].hp==46-2*(13+4*(int)up)&&!s.vigor&&!s.attack_vigor);break;
    case Production:
        act(s,kind);CHECK(s.energy==102+(int)up&&s.zones[0]==Zone::Exhaust);break;
    case Prolong:
        s.block=13;act(s,kind);CHECK(s.block_next_turn==13&&s.block==13&&s.zones[0]==(up?Zone::Discard:Zone::Exhaust));
        next_turn(s);CHECK(s.block==13&&!s.block_next_turn);break;
    case Prowess:
        act(s,kind);CHECK(s.strength==1+(int)up&&s.dexterity==1+(int)up);add(s,Defend);act(s,Defend);CHECK(s.block==6+(int)up);break;
    case Rend:{
        s.enemies[0].strength=-2;s.enemies[0].weak=1;s.enemies[0].vulnerable=1;
        const int amount=(10+2*(int)up+3*(5+3*(int)up))*3/2;
        CHECK(damage(s,s.cards[0],0)==amount);act(s,kind);CHECK(s.enemies[0].hp==(amount<46?46-amount:0));break;}
    case Restlessness:{
        supply(s,6);act(s,kind);CHECK(s.hand.count==2+up&&s.energy==102+(int)up&&card_retains({(CardKind)kind,(uint8_t)up}));
        auto blocked=fight(kind,up);supply(blocked,6);add(blocked,Defend);act(blocked,kind);CHECK(blocked.hand.count==1&&blocked.energy==100&&blocked.draw.count==6);ready(blocked);break;}
    case RollingBoulder:
        act(s,kind);CHECK(total_hp(s)==92);next_turn(s);CHECK(total_hp(s)==92-2*(5+5*(int)up));
        next_turn(s);CHECK(total_hp(s)==92-2*(15+10*(int)up)&&power_value(s,XPower::RollingBoulder)==15+5*(int)up);break;
    case SecretTechnique:case SecretWeapon:{
        int attack=add(s,Strike,0,Zone::Draw),skill=add(s,Defend,0,Zone::Draw),power=add(s,Inflame,0,Zone::Draw);
        act(s,kind);CHECK(s.expansion.choice==XChoice::Draw&&s.expansion.choices.count==1);
        CHECK(s.expansion.choices.ids[0]==(kind==SecretTechnique?skill:attack));pick(s,1);
        CHECK(s.hand.count==1&&s.draw.count==2&&contains(s.draw,power)&&s.zones[0]==(up?Zone::Discard:Zone::Exhaust));break;}
    case SeekerStrike:
        supply(s,8);act(s,kind);CHECK(s.enemies[0].hp==37-3*(int)up&&s.expansion.choice==XChoice::Draw&&s.expansion.choices.count==3);
        pick(s,1,2);CHECK(s.hand.count==1&&s.draw.count==7);break;
    case Splash:
        act(s,kind);CHECK(s.expansion.choice==XChoice::Offer&&s.expansion.choices.count==3);
        for(unsigned i=0;i<3;i++){Card c=choice_card(s,s.expansion.choices.ids[i]);CHECK(definitions[c.kind].type==CardType::Attack&&!card_colorless(c.kind)&&!card_in_pool(c.kind,s.character)&&card_rarity(c.kind)>=2&&card_rarity(c.kind)<=4&&c.upgraded==up);}
        pick(s,1);{int id=s.hand.ids[0];Card c=s.cards[id];CHECK(cost(s,id)==0||cost(c)==-1);CHECK(card_star_cost(s,c,id)==0||card_star_cost(s,c)==-1);}break;
    case Stratagem:{
        act(s,kind);CHECK(s.energy==99+(int)up);supply(s,4,Defend,Zone::Discard);draw_cards(s,1);
        CHECK(s.expansion.choice==XChoice::Draw&&choice_min(s)==1&&s.expansion.choices.count==4);pick(s,1);
        CHECK(s.hand.count==2&&s.draw.count==2);break;}
    case TheBomb:
        act(s,kind);CHECK(s.expansion.bomb_turns[0]==3);next_turn(s);CHECK(total_hp(s)==92&&s.expansion.bomb_turns[0]==2);
        next_turn(s);CHECK(total_hp(s)==92&&s.expansion.bomb_turns[0]==1);for(auto&e:s.enemies)e.block=10;next_turn(s);
        CHECK(total_hp(s)==92-2*(30+10*(int)up)&&!s.expansion.bomb_turns[0]&&!s.expansion.bomb_damage[0]);break;
    case TheGambit:{
        act(s,kind);CHECK(s.block==50+25*(int)up&&power_value(s,XPower::Gambit)==1);
        player_hp_loss(s,1,false,false);CHECK(s.hp==79&&s.phase==Phase::Combat);player_hp_loss(s,1,true,true);CHECK(s.hp==79&&s.phase==Phase::Combat);
        auto lethal=s;player_hp_loss(lethal,lethal.block+1,true,true);CHECK(lethal.phase==Phase::Dead&&!lethal.hp);roundtrip(lethal);break;}
    case ThinkingAhead:{
        supply(s,6);act(s,kind);CHECK(s.expansion.choice==XChoice::Top&&s.hand.count==2);
        int selected=s.expansion.choices.ids[1];pick(s,1,1);CHECK(s.hand.count==1&&s.draw.count==5&&s.draw.ids[s.draw.count-1]==selected&&s.zones[0]==(up?Zone::Discard:Zone::Exhaust));break;}
    case UltimateDefend:
        act(s,kind);CHECK(s.block==11+4*(int)up&&s.energy==99);break;
    case UltimateStrike:
        act(s,kind);CHECK(s.enemies[0].hp==32-6*(int)up&&s.energy==99&&card_strike_tag(kind));break;
    case Volley:{
        s.energy=3;act(s,kind);CHECK(total_hp(s)==92-3*(10+4*(int)up)&&s.energy==0&&s.expansion.cards_finished==1);
        auto zero=fight(kind,up);zero.energy=0;act(zero,kind);CHECK(total_hp(zero)==92&&zero.expansion.cards_finished==1&&zero.attacked_this_turn);ready(zero);break;}
    default:CHECK(false);
    }
    drain_choices(s);ready(s);covered[kind][up]=true;++variants;
}

static void nested_choices(){
    active_kind=Catastrophe;active_upgrade=0;
    auto s=fight(Catastrophe);add(s,Discovery,0,Zone::Draw);add(s,ThinkingAhead,0,Zone::Draw);
    act(s,Catastrophe);CHECK(choice_phase(s.phase)&&s.expansion.depth==1);roundtrip(s);drain_choices(s);
    CHECK(s.expansion.depth==0&&s.expansion.cards_finished>=2&&s.zones[0]==Zone::Discard);ready(s);

    // A full-card preview selection cannot masquerade as an actual instance.
    s=fight(Discovery);act(s,Discovery);auto before=snapshot(s);uint16_t real_id=0;
    CHECK(!choose_extended(s,&real_id,1));same(s,before);pick(s,1);ready(s);

    // Exhaust choice must reserve its queued continuations atomically.
    s=fight(Defend);add(s,Strike);queue_extension(s,XOp::ExhaustAny);resolve_pending(s);
    CHECK(s.phase==Phase::ExtendedChoice);
    while(s.effect_count<MaxEffects)queue_extension(s,XOp::Noop);
    before=snapshot(s);uint16_t selected=s.hand.ids[0];CHECK(!choose_extended(s,&selected,1));same(s,before);
    CHECK(choose_extended(s,nullptr,0));ready(s);
}

static void token_contracts(){
    for(unsigned up=0;up<2;up++){
        active_kind=Soul;active_upgrade=up;auto s=fight(Soul,up);supply(s,5);act(s,Soul);
        CHECK(s.hand.count==2+up&&s.zones[0]==Zone::Exhaust&&s.energy==100);ready(s);
        active_kind=SovereignBlade;s=fight(SovereignBlade,up);s.expansion.blade_damage[0]=37;
        CHECK(card_retains(s.cards[0])&&damage(s,s.cards[0],0,0)==37);act(s,SovereignBlade);
        CHECK(s.enemies[0].hp==9&&s.energy==98+(int)up&&s.expansion.blade_damage[0]==37);ready(s);
    }
}

static void modifier_boundaries(){
    for(unsigned kind:{(unsigned)Fisticuffs,(unsigned)Omnislice})for(unsigned up=0;up<2;up++){
        active_kind=kind;active_upgrade=up;auto s=fight(kind,up);s.strength=10;s.enemies[0].block=2;s.enemies[0].hp=3;
        act(s,kind);CHECK(!s.enemies[0].hp);if(kind==Fisticuffs)CHECK(s.block==17+2*(int)up);else CHECK(s.enemies[1].hp==28-3*(int)up);ready(s);
    }
    active_kind=Fisticuffs;active_upgrade=0;auto s=fight(Fisticuffs);s.dexterity=3;s.no_block=1;act(s,Fisticuffs);CHECK(!s.block);ready(s);
    active_kind=Automation;s=fight(Automation);act(s,Automation);supply(s,10);s.no_draw=1;draw_cards(s,10);
    CHECK(s.expansion.automation_count[0]==0&&s.energy==99&&!s.hand.count);ready(s);
}

static void generation_and_transform_pools(){
    for(unsigned character=0;character<CharacterKinds;character++)for(unsigned seed=1;seed<=64;seed++){
        active_kind=Discovery;active_upgrade=0;auto s=fight(Discovery);s.character=(Character)character;s.rng_cards=seed;
        act(s,Discovery);
        for(unsigned i=0;i<3;i++){
            unsigned k=choice_card(s,s.expansion.choices.ids[i]).kind;
            CHECK(card_in_pool(k,s.character)&&card_rarity(k)>=2&&card_rarity(k)<=4);
            for(unsigned j=0;j<i;j++)CHECK(k!=choice_card(s,s.expansion.choices.ids[j]).kind);
        }
        pick(s,1);ready(s);
        active_kind=JackOfAllTrades;active_upgrade=1;s=fight(JackOfAllTrades,1);s.rng_cards=seed;
        act(s,JackOfAllTrades);CHECK(s.hand.count==2);
        for(unsigned i=0;i<2;i++){
            unsigned k=s.cards[s.hand.ids[i]].kind;
            CHECK(card_colorless(k)&&k!=JackOfAllTrades&&k!=Alchemize&&k!=HandOfGreed&&k!=HiddenGem);
        }
        CHECK(s.cards[s.hand.ids[0]].kind!=s.cards[s.hand.ids[1]].kind);ready(s);
    }
    for(unsigned old:{(unsigned)Strike,(unsigned)StrikeSilent,(unsigned)Finesse,(unsigned)Shiv,(unsigned)Soul,(unsigned)SovereignBlade}){
        active_kind=Entropy;active_upgrade=0;auto s=fight(old,1);
        s.expansion.turn_cost[0]=4;s.expansion.combat_cost[0]=1;s.expansion.replay[0]=2;
        queue_extension(s,XOp::Transform,1);resolve_pending(s);pick(s,1);
        unsigned k=s.cards[0].kind;
        CHECK(k!=old&&!s.cards[0].upgraded&&!s.expansion.turn_cost[0]&&!s.expansion.combat_cost[0]&&!s.expansion.replay[0]);
        if(old==Strike)CHECK(card_in_pool(k,Character::Ironclad));
        else if(old==StrikeSilent)CHECK(card_in_pool(k,Character::Silent));
        else CHECK(card_colorless(k)&&k!=Alchemize&&k!=HandOfGreed&&k!=HiddenGem);
        CHECK(card_rarity(k)>=2&&card_rarity(k)<=4);ready(s);
    }
}

static void original_continuation_boundaries(){
    active_kind=Anointed;active_upgrade=0;auto full=fight(Anointed);supply(full,8,Defend,Zone::Hand);
    supply(full,4,Offering);int common=add(full,Anger,0,Zone::Draw);full.no_draw=1;
    act(full,Anointed);CHECK(full.hand.count==MaxHand&&kind_count(full,full.hand,Offering)==2);
    CHECK(full.draw.count==3&&kind_count(full,full.draw,Offering)==2&&contains(full.draw,common)&&full.discard.count==0);ready(full);
    // An automatic Anointed played into a full hand leaves every rare in draw.
    full=fight(Defend);supply(full,9,Defend,Zone::Hand);int auto_id=add(full,Anointed,0,Zone::Draw);supply(full,3,Offering);
    uint32_t rng=full.rng_cards;queue_extension(full,XOp::AutoInstance,auto_id);resolve_pending(full);
    CHECK(full.hand.count==MaxHand&&full.draw.count==3&&full.rng_cards==rng&&full.discard.count==0);ready(full);

    // General pile selection still resolves at the hand cap; its selected
    // card goes to discard, as opposed to Anointed's explicit capacity limit.
    active_kind=SecretWeapon;full=fight(Defend);supply(full,9,Defend,Zone::Hand);
    auto_id=add(full,SecretWeapon,0,Zone::Discard);int chosen=add(full,Strike,0,Zone::Draw);
    queue_extension(full,XOp::AutoInstance,auto_id);resolve_pending(full);
    CHECK(full.expansion.choice==XChoice::Draw&&full.expansion.choices.ids[0]==chosen);pick(full,1);
    CHECK(full.hand.count==MaxHand&&full.draw.count==0&&full.zones[chosen]==Zone::Discard&&full.zones[auto_id]==Zone::Exhaust);ready(full);

    for(unsigned up=0;up<2;up++){
        active_kind=BeatDown;active_upgrade=up;auto s=fight(BeatDown,up);int id=add(s,Strike,0,Zone::Discard);
        act(s,BeatDown);CHECK(s.expansion.cards_finished==2&&total_hp(s)==86&&s.zones[id]==Zone::Discard);ready(s);
    }
    active_kind=Mayhem;active_upgrade=0;auto s=fight(Mayhem);act(s,Mayhem);add(s,Entropy);act(s,Entropy);
    add(s,Shiv,0,Zone::Draw);supply(s,5);next_turn(s);
    CHECK(s.expansion.choice==XChoice::Transform&&s.hand.count==5&&total_hp(s)==92);
    pick(s,1);CHECK(total_hp(s)==88&&s.hand.count==5&&s.exhaust.count==1);ready(s);

    for(unsigned kind:{(unsigned)Bolas,(unsigned)ThrummingHatchet}){
        active_kind=kind;s=fight(kind);act(s,kind);draw_cards(s,1);CHECK(contains(s.hand,0));
        add(s,Purity);act(s,Purity);uint16_t id=0;CHECK(choose_exhaust_many(s,&id,1));CHECK(s.zones[0]==Zone::Exhaust);
        supply(s,5);next_turn(s);CHECK(s.zones[0]==Zone::Hand&&s.hand.count==6&&!s.expansion.return_turn[0]);ready(s);
    }
}

static void reserved_autoplay_contracts(){
    active_kind=Mayhem;active_upgrade=0;auto s=fight(Mayhem);act(s,Mayhem);add(s,Mayhem);act(s,Mayhem);
    add(s,Strike,0,Zone::Draw);add(s,FlashOfSteel,0,Zone::Draw);supply(s,5);
    next_turn(s);CHECK(total_hp(s)==81&&s.hand.count==5&&s.expansion.cards_finished==4&&s.discard.count==2);ready(s);

    // The first reserved card is already outside draw when the next card
    // requires a shuffle, whose Stratagem selection suspends the whole batch.
    s=fight(Mayhem);act(s,Mayhem);add(s,Mayhem);act(s,Mayhem);add(s,Stratagem);act(s,Stratagem);
    int reserved=add(s,FlashOfSteel,0,Zone::Draw);supply(s,5);
    add(s,Strike,0,Zone::Discard);int selected=add(s,Defend,0,Zone::Discard);next_turn(s);
    CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Draw&&s.zones[reserved]==Zone::Limbo&&s.playing==-1);
    CHECK(total_hp(s)==92&&s.expansion.cards_finished==3);roundtrip(s);
    int queued=-1;for(unsigned i=0;i<s.effect_count;i++)if(s.effects[i].kind==EffectKind::Extended&&s.effects[i].target==(int)XOp::AutoInstance&&s.effects[i].value==reserved)queued=(int)i;
    CHECK(queued>=0&&s.effect_count<MaxEffects);
    auto duplicate=s;duplicate.effects[duplicate.effect_count++]=s.effects[queued];CHECK(!validate(duplicate));SaveData refused{};CHECK(!save(duplicate,refused));
    auto orphan=s;orphan.effects[queued].value=s.hand.ids[0];CHECK(!validate(orphan));CHECK(!save(orphan,refused));
    unsigned offset=0;while(offset<s.expansion.choices.count&&s.expansion.choices.ids[offset]!=selected)++offset;
    CHECK(offset<s.expansion.choices.count);pick(s,1,offset);
    CHECK(total_hp(s)==81&&s.hand.count==6&&s.expansion.cards_finished==5&&s.zones[reserved]==Zone::Discard);ready(s);

    // Queue exhaustion is detected before any source is moved to Limbo.
    s=fight(Defend);supply(s,3,FlashOfSteel);queue_extension(s,XOp::AutoTop,3);
    while(s.effect_count<MaxEffects)queue_extension(s,XOp::Noop);
    resolve_pending(s);CHECK(s.phase==Phase::Error&&s.draw.count==3);
    for(unsigned i=0;i<s.draw.count;i++)CHECK(s.zones[s.draw.ids[i]]==Zone::Draw);
}

static void ordered_turn_start_contracts(){
    // Entropy was applied before Boulder. Its choice precedes that damage,
    // Mercury, and Mayhem; ordinary draw and early relic benefits are ready.
    active_kind=Entropy;active_upgrade=0;auto s=fight(Entropy);act(s,Entropy);
    add(s,RollingBoulder);act(s,RollingBoulder);add(s,Mayhem);act(s,Mayhem);
    CHECK(gain_relic(s,MercuryHourglass));CHECK(gain_relic(s,ArtOfWar));CHECK(gain_relic(s,HornCleat));
    CHECK(s.expansion.entropy_boulder_prefix==0);add(s,Strike,0,Zone::Draw);supply(s,5);next_turn(s);
    CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Transform&&s.hand.count==5);
    CHECK(total_hp(s)==92&&s.energy==4&&s.block==14&&s.expansion.boulder[0]==5&&s.expansion.cards_finished==3);
    pick(s,1);CHECK(total_hp(s)==70&&s.expansion.boulder[0]==10&&s.expansion.cards_finished==4&&s.energy==4);ready(s);

    // Reversing application order changes the observable state at the choice.
    s=fight(RollingBoulder);act(s,RollingBoulder);add(s,Entropy);act(s,Entropy);
    CHECK(s.expansion.entropy_boulder_prefix==1);supply(s,8);next_turn(s);
    CHECK(s.expansion.choice==XChoice::Transform&&total_hp(s)==82&&s.expansion.boulder[0]==10);
    pick(s,1);CHECK(total_hp(s)==82);ready(s);

    // A duplicated power applies two independent Boulders before Entropy.
    // Later Boulders follow it; additional Entropy stacks preserve its place.
    s=fight(RollingBoulder);gain_power(s,XPower::Duplicator,1);act(s,RollingBoulder);
    add(s,Entropy);act(s,Entropy);add(s,RollingBoulder,1);act(s,RollingBoulder);
    add(s,Entropy);act(s,Entropy);CHECK(s.expansion.entropy_boulder_prefix==2&&power_value(s,XPower::Entropy)==2);
    supply(s,8);next_turn(s);
    CHECK(s.expansion.choice==XChoice::Transform&&choice_min(s)==2&&total_hp(s)==72);
    CHECK(s.expansion.boulder[0]==10&&s.expansion.boulder[1]==10&&s.expansion.boulder[2]==10);
    auto invalid=s;invalid.expansion.entropy_boulder_prefix=4;CHECK(!validate(invalid));SaveData refused{};CHECK(!save(invalid,refused));
    pick(s,2);CHECK(total_hp(s)==52&&s.expansion.boulder[2]==15&&power_value(s,XPower::RollingBoulder)==35);ready(s);
}

int main(){
    for(unsigned kind=0;kind<CardKinds;kind++)if(card_colorless(kind))for(unsigned up=0;up<2;up++)card_contract(kind,up);
    CHECK(variants==106);for(unsigned kind=0;kind<CardKinds;kind++)if(card_colorless(kind))CHECK(covered[kind][0]&&covered[kind][1]);
    nested_choices();token_contracts();modifier_boundaries();generation_and_transform_pools();original_continuation_boundaries();reserved_autoplay_contracts();ordered_turn_start_contracts();
    std::printf("colorless completion: %u checks, %u explicit card-effect variants, token and continuation contracts\n",checks,variants);
}
