#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static unsigned checks;
static const char* scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL [%s] %d: %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static int add(State&s,CardKind kind,Zone zone=Zone::Hand,unsigned upgraded=0){
 int id=s.card_count++;CHECK(id<MaxCards);s.cards[id]={kind,uint8_t(upgraded)};s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:s.exhaust;
 p.ids[p.count++]=uint16_t(id);return id;
}
static State fight(std::initializer_list<Card> cards,Character character=Character::Ironclad){
 State s{};new_run(s,0x41cef1ea,false,character);s.phase=Phase::Combat;s.current_node=0;s.turn=1;s.floor=1;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=99;s.expansion.stars=99;s.enemy_count=1;s.enemies[0]={999,999,0,0,0,2};
 s.deck_count=0;for(auto c:cards){add(s,c.kind,Zone::Hand,c.upgraded);s.deck[s.deck_count++]=c;}
 CHECK(validate(s));return s;
}
static void roundtrip(const State&s){
 CHECK(validate(s));SaveData before{},after{};CHECK(save(s,before));State loaded{};CHECK(load(loaded,before.bytes,before.size));
 CHECK(save(loaded,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
static void ready(const State&s){CHECK(s.phase==Phase::Combat&&s.playing<0&&!s.effect_count&&!s.expansion.depth);roundtrip(s);}
struct Trace {
 bool offered=false;unsigned effects=0;CoreObserver old{};
 Trace(){CoreObserver observer{};observer.context=this;observer.on_effect=[](void*context,const State&s,Effect){auto&t=*static_cast<Trace*>(context);++t.effects;t.offered|=choice_phase(s.phase)||s.phase==Phase::WorldRelicChoice;};old=set_core_observer(observer);}
 ~Trace(){set_core_observer(old);}
};
static void act(State&s){CHECK(play(s,0,0));}
static void command(State&s,XOp op,int a=0,int b=0){
 CHECK(s.phase==Phase::Combat&&!s.effect_count);s.effects[s.effect_count++]={EffectKind::Extended,int16_t(a),int16_t(op),int16_t(b)};resolve_pending(s);
}
static void legacy_phases(){
 scenario="Burning Pact captures one card before its exhaust/draw hooks";
 auto s=fight({{BurningPact,0},{Wound,0}});for(unsigned i=0;i<5;i++)add(s,Defend,Zone::Draw);
 s.pain=3;s.embrace=1;s.power_count=2;s.power_order[0]=0;s.power_order[1]=1;
 const uint32_t rng=s.rng_cards;{Trace trace;act(s);CHECK(!trace.offered&&trace.effects>0);}
 CHECK(s.zones[1]==Zone::Exhaust&&s.hand.count==3&&s.draw.count==2&&s.block==3&&s.rng_cards==rng);ready(s);
 scenario="True Grit+ one card automatically exhausts; two remain a real choice";
 s=fight({{TrueGrit,1},{Defend,0}});act(s);CHECK(s.zones[1]==Zone::Exhaust&&s.block==9);ready(s);
 s=fight({{TrueGrit,1},{Defend,0},{Strike,0}});act(s);CHECK(s.phase==Phase::ExhaustChoice);roundtrip(s);
 CHECK(choose_exhaust(s,0));CHECK(s.zones[1]==Zone::Exhaust&&s.zones[2]==Zone::Hand);ready(s);
 scenario="Prepared+ chooses its complete two-card hand without a modal";
 s=fight({{Prepared,1}},Character::Silent);add(s,Defend,Zone::Draw);add(s,Strike,Zone::Draw);
 {Trace trace;act(s);CHECK(!trace.offered);}CHECK(!s.hand.count&&s.discard.count==3&&!s.discard_required);ready(s);
 scenario="Survivor discard hooks resolve after the fixed selection";
 s=fight({{Survivor,0},{Tactician,0}},Character::Silent);act(s);CHECK(!s.hand.count&&s.energy==99&&s.block==8);ready(s);
 scenario="Headbutt automatically returns the sole discard card";
 s=fight({{Headbutt,0}});int id=add(s,Defend,Zone::Discard);{Trace trace;act(s);CHECK(!trace.offered);}
 CHECK(s.draw.count==1&&s.draw.ids[0]==id&&s.zones[0]==Zone::Discard);ready(s);
 scenario="Purity is optional even with one eligible card";
 s=fight({{Purity,0},{Defend,0}});act(s);CHECK(s.phase==Phase::ExhaustManyChoice&&s.hand.count==1);
 CHECK(choose_exhaust_many(s,nullptr,0));CHECK(s.zones[1]==Zone::Hand);ready(s);
}
static void extended_families(){
 scenario="Armaments upgrades one eligible card and ignores unupgradeable cards";
 auto s=fight({{Armaments,0},{Strike,0},{Wound,0}});act(s);CHECK(s.cards[1].upgraded&&!s.cards[2].upgraded);ready(s);
 scenario="Hand Trick and Nightmare use the shared low-level result path";
 s=fight({{HandTrick,0},{Defend,0}},Character::Silent);act(s);CHECK(s.silent.sly[1]&2);ready(s);
 s=fight({{Nightmare,0},{Strike,0}},Character::Silent);act(s);CHECK(s.silent.nightmare_active[0]&&s.silent.nightmares[0].card.kind==Strike);ready(s);
 scenario="Begone and Charge automatically transform all fixed candidates";
 s=fight({{RegentBegone,0},{Defend,0}},Character::Regent);act(s);CHECK(s.cards[1].kind==MinionStrike);ready(s);
 s=fight({{RegentCharge,0}},Character::Regent);int bottom=add(s,Strike,Zone::Draw),top=add(s,Defend,Zone::Draw);act(s);
 CHECK(s.cards[bottom].kind==MinionDiveBomb&&s.cards[top].kind==MinionDiveBomb&&s.draw.ids[0]==bottom&&s.draw.ids[1]==top);ready(s);
 scenario="Decisions queues three automatic plays without recursive selection";
 s=fight({{RegentDecisionsDecisions,0},{Defend,0}},Character::Regent);{Trace trace;act(s);CHECK(!trace.offered);}
 CHECK(s.block==15&&s.zones[1]==Zone::Discard&&s.zones[0]==Zone::Exhaust);ready(s);
 scenario="Guards remains optional with one candidate";
 s=fight({{RegentGuards,0},{Defend,0}},Character::Regent);act(s);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::RTransformGuards);
 CHECK(choose_extended(s,nullptr,0));CHECK(s.cards[1].kind==Defend);ready(s);
 scenario="Necrobinder pile/keyword selectors automatically apply one result";
 s=fight({{NecroCleanse,0}},Character::Necrobinder);int clean=add(s,Dazed,Zone::Draw);act(s);CHECK(s.zones[clean]==Zone::Exhaust);ready(s);
 s=fight({{NecroSculptingStrike,0},{Strike,0}},Character::Necrobinder);act(s);CHECK(s.necro.ethereal[1]);ready(s);
 s=fight({{NecroTransfigure,0},{Strike,0}},Character::Necrobinder);act(s);CHECK(s.expansion.replay[1]==1&&cost(s,1)==2);ready(s);
 scenario="Dual Wield continuation metadata is initialized before auto-return";
 s=fight({{EventCardDualWield,0},{Strike,0}});act(s);CHECK(s.card_count==3&&s.cards[2].kind==Strike&&s.hand.count==2&&!s.event_cards.choice_copies);ready(s);
 scenario="Neow's Fury retains optional confirmation with one discard candidate";
 s=fight({{EventCardNeowsFury,0}});add(s,Defend,Zone::Discard);act(s);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::EventNeowsFury);
 CHECK(choose_extended(s,nullptr,0));CHECK(s.hand.count==0);ready(s);
}
static void pile_order_and_potions(){
 scenario="Foregone returns all draw candidates top-first without rarity sorting";
 auto s=fight({{Strike,0}});int bottom=add(s,Defend,Zone::Draw),top=add(s,Bash,Zone::Draw);const uint32_t rng=s.rng_cards;
 command(s,XOp::RForegone,4);CHECK(s.hand.count==3&&s.hand.ids[1]==top&&s.hand.ids[2]==bottom&&!s.draw.count&&s.rng_cards==rng);ready(s);
 scenario="Ordinary draw-pile result order is preserved for a multi-card request";
 s=fight({{Strike,0}});bottom=add(s,Defend,Zone::Draw);top=add(s,Bash,Zone::Draw);command(s,XOp::ChooseDraw,0,-2);
 CHECK(s.hand.count==3&&s.hand.ids[1]==top&&s.hand.ids[2]==bottom);ready(s);
 scenario="Fixed requests above Hand capacity still auto-return every candidate";
 for(XOp op:{XOp::RForegone,XOp::ChooseDraw}){
  s=fight({{Strike,0}});for(unsigned i=0;i<12;i++)add(s,i&1?Defend:Bash,Zone::Draw);
  command(s,op,op==XOp::RForegone?12:0,op==XOp::ChooseDraw?-12:0);
  CHECK(!s.draw.count&&s.hand.count==MaxHand&&s.discard.count==3);
  for(unsigned i=1;i<MaxHand;i++)CHECK(s.hand.ids[i]==13-i);
  CHECK(s.discard.ids[0]==3&&s.discard.ids[1]==2&&s.discard.ids[2]==1);ready(s);
 }
 scenario="Nontrivial Foregone keeps the user's explicit selection order";
 s=fight({{Strike,0}});bottom=add(s,Defend,Zone::Draw);add(s,Strike,Zone::Draw);top=add(s,Bash,Zone::Draw);
 command(s,XOp::RForegone,2);CHECK(s.phase==Phase::ExtendedChoice);uint16_t selection[]={uint16_t(top),uint16_t(bottom)};
 CHECK(choose_extended(s,selection,2));CHECK(s.hand.ids[1]==top&&s.hand.ids[2]==bottom);ready(s);
 scenario="Mandatory potion selection finishes its captured bottle continuation";
 s=fight({{Strike,0}});int discard=add(s,Defend,Zone::Discard);s.potions[0]=Potion::LiquidMemories;CHECK(use_potion(s,0,-1));
 CHECK(s.zones[discard]==Zone::Hand&&cost(s,discard)==0&&s.potions[0]==Potion::Empty&&!s.expansion.potion_active);ready(s);
 s=fight({{Strike,0}});s.potions[0]=Potion::TouchOfInsanity;CHECK(use_potion(s,0,-1));CHECK(cost(s,0)==0);ready(s);
 scenario="Optional one-card potion selection is still presented";
 for(Potion potion:{Potion::Ashwater,Potion::GamblersBrew}){
  s=fight({{Strike,0}});s.potions[0]=potion;CHECK(use_potion(s,0,-1));CHECK(s.phase==Phase::ExtendedChoice&&choice_min(s)==0&&choice_max(s)==1);
  CHECK(choose_extended(s,nullptr,0));CHECK(s.hand.count==1);ready(s);
 }
 scenario="Generated offer still requires choosing or explicitly skipping";
 s=fight({{Discovery,0}});act(s);CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Offer&&choice_min(s)==0);
 CHECK(choose_extended(s,nullptr,0));CHECK(!s.hand.count);ready(s);
}
static void relic_and_legacy_continuations(){
 scenario="Toasty's one-card shortcut retains exhaust-before-strength hooks";
 auto s=fight({{Defend,0}});CHECK(gain_world_relic(s,WorldRelicToastyMittens));resolve_pending(s);
 {Trace trace;world_relic_hook(s,WorldRelicHook::AfterHandDraw);resolve_pending(s);CHECK(!trace.offered);}
 CHECK(s.strength==1&&s.zones[0]==Zone::Exhaust&&!world_relic_pending(s));ready(s);
 scenario="Gambling Chip remains an optional hand selector";
 s=fight({{Defend,0}});CHECK(gain_world_relic(s,WorldRelicGamblingChip));resolve_pending(s);world_relic_hook(s,WorldRelicHook::AfterHandDraw);resolve_pending(s);
 CHECK(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Hand&&s.world_relics.choice_min==0);
 roundtrip(s);CHECK(world_relic_select(s,nullptr,0));CHECK(s.hand.count==1);ready(s);
 scenario="Old suspended singleton saves remain unchanged until user action";
 s=fight({{Headbutt,0}});int first=add(s,Defend,Zone::Discard),second=add(s,Strike,Zone::Discard);act(s);CHECK(s.phase==Phase::ReturnChoice);
 // A pre-existing serialized singleton remains a valid pending command.
 s.discard.count=1;s.exhaust.ids[s.exhaust.count++]=uint16_t(second);s.zones[second]=Zone::Exhaust;
 SaveData bytes{},after{};CHECK(save(s,bytes));State loaded{};CHECK(load(loaded,bytes.bytes,bytes.size));CHECK(loaded.phase==Phase::ReturnChoice);
 CHECK(save(loaded,after)&&bytes.size==after.size&&!std::memcmp(bytes.bytes,after.bytes,bytes.size));
 CHECK(choose_return_to_draw(loaded,first));ready(loaded);
}
int main(){legacy_phases();extended_families();pile_order_and_potions();relic_and_legacy_continuations();std::printf("Combat source selection shortcuts: %u checks passed\n",checks);}
