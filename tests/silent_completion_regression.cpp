#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

using namespace spire;
static unsigned checks,variants;
static const char* scenario="initial";
static unsigned upgrade;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d [%s +%u] %s\n",__FILE__,__LINE__,scenario,upgrade,#x);std::exit(1);}}while(0)

// Values in this table are the pinned assembly's canonical facts, rather than
// values obtained from the runtime Definition array being tested.
struct Canonical {uint8_t id;const char*name;int8_t base_cost,up_cost;uint8_t base_keywords,up_keywords;CardType type;};
static const Canonical canonical[]={
 {Abrasive,"Abrasive",3,3,16,16,CardType::Power},
 {Accelerant,"Accelerant",1,1,0,0,CardType::Power},
 {Accuracy,"Accuracy",1,1,0,0,CardType::Power},
 {Acrobatics,"Acrobatics",1,1,0,0,CardType::Skill},
 {Adrenaline,"Adrenaline",0,0,1,1,CardType::Skill},
 {Afterimage,"Afterimage",1,1,0,2,CardType::Power},
 {Anticipate,"Anticipate",0,0,0,0,CardType::Skill},
 {Assassinate,"Assassinate",0,0,3,3,CardType::Attack},
 {Backflip,"Backflip",1,1,0,0,CardType::Skill},
 {Backstab,"Backstab",0,0,3,3,CardType::Attack},
 {BladeOfInk,"BladeOfInk",1,1,0,0,CardType::Skill},
 {BladeDance,"BladeDance",1,1,1,1,CardType::Skill},
 {Blur,"Blur",1,1,0,0,CardType::Skill},
 {BouncingFlask,"BouncingFlask",2,2,0,0,CardType::Skill},
 {BubbleBubble,"BubbleBubble",1,1,0,0,CardType::Skill},
 {BulletTime,"BulletTime",3,2,0,0,CardType::Skill},
 {Burst,"Burst",1,1,0,0,CardType::Skill},
 {CalculatedGamble,"CalculatedGamble",0,0,1,5,CardType::Skill},
 {CloakAndDagger,"CloakAndDagger",1,1,0,0,CardType::Skill},
 {CorrosiveWave,"CorrosiveWave",1,1,0,0,CardType::Skill},
 {DaggerSpray,"DaggerSpray",1,1,0,0,CardType::Attack},
 {DaggerThrow,"DaggerThrow",1,1,0,0,CardType::Attack},
 {Dash,"Dash",2,2,0,0,CardType::Attack},
 {DeadlyPoison,"DeadlyPoison",1,1,0,0,CardType::Skill},
 {DefendSilent,"DefendSilent",1,1,0,0,CardType::Skill},
 {Deflect,"Deflect",0,0,0,0,CardType::Skill},
 {DodgeAndRoll,"DodgeAndRoll",1,1,0,0,CardType::Skill},
 {EchoingSlash,"EchoingSlash",1,1,0,0,CardType::Attack},
 {Envenom,"Envenom",2,2,0,0,CardType::Power},
 {EscapePlan,"EscapePlan",0,0,0,0,CardType::Skill},
 {Expertise,"Expertise",1,1,0,0,CardType::Skill},
 {Expose,"Expose",0,0,1,1,CardType::Skill},
 {FanOfKnives,"FanOfKnives",2,2,0,0,CardType::Power},
 {Finisher,"Finisher",1,1,0,0,CardType::Attack},
 {Flechettes,"Flechettes",1,1,0,0,CardType::Attack},
 {FlickFlack,"FlickFlack",1,1,16,16,CardType::Attack},
 {Sidestep,"Sidestep",0,0,0,0,CardType::Skill},
 {Footwork,"Footwork",1,1,0,0,CardType::Power},
 {GrandFinale,"GrandFinale",0,0,0,0,CardType::Attack},
 {HandTrick,"HandTrick",1,1,0,0,CardType::Skill},
 {Haze,"Haze",2,2,0,0,CardType::Skill},
 {HiddenDaggers,"HiddenDaggers",0,0,0,0,CardType::Skill},
 {InfiniteBlades,"InfiniteBlades",1,1,0,2,CardType::Power},
 {KnifeTrap,"KnifeTrap",2,2,0,0,CardType::Skill},
 {LeadingStrike,"LeadingStrike",1,1,0,0,CardType::Attack},
 {LegSweep,"LegSweep",2,2,0,0,CardType::Skill},
 {Malaise,"Malaise",-1,-1,1,1,CardType::Skill},
 {MasterPlanner,"MasterPlanner",2,1,0,0,CardType::Power},
 {MementoMori,"MementoMori",1,1,0,0,CardType::Attack},
 {Mirage,"Mirage",1,1,1,0,CardType::Skill},
 {Murder,"Murder",3,2,0,0,CardType::Attack},
 {Neutralize,"Neutralize",0,0,0,0,CardType::Attack},
 {Nightmare,"Nightmare",3,2,1,1,CardType::Skill},
 {NoxiousFumes,"NoxiousFumes",1,1,0,0,CardType::Power},
 {Outbreak,"Outbreak",3,3,0,0,CardType::Skill},
 {PhantomBlades,"PhantomBlades",1,1,0,0,CardType::Power},
 {PiercingWail,"PiercingWail",1,1,1,1,CardType::Skill},
 {Pinpoint,"Pinpoint",3,3,0,0,CardType::Attack},
 {PoisonedStab,"PoisonedStab",1,1,0,0,CardType::Attack},
 {Pounce,"Pounce",2,2,0,0,CardType::Attack},
 {PreciseCut,"PreciseCut",0,0,0,0,CardType::Attack},
 {Predator,"Predator",2,2,0,0,CardType::Attack},
 {Prepared,"Prepared",0,0,0,0,CardType::Skill},
 {Reflex,"Reflex",3,3,16,16,CardType::Skill},
 {Ricochet,"Ricochet",2,2,16,16,CardType::Attack},
 {SerpentForm,"SerpentForm",3,3,0,0,CardType::Power},
 {ShadowStep,"ShadowStep",1,0,0,0,CardType::Skill},
 {Shadowmeld,"Shadowmeld",1,0,0,0,CardType::Skill},
 {Skewer,"Skewer",-1,-1,0,0,CardType::Attack},
 {Slice,"Slice",0,0,0,0,CardType::Attack},
 {Snakebite,"Snakebite",2,2,4,4,CardType::Skill},
 {Speedster,"Speedster",2,2,0,2,CardType::Power},
 {StormOfSteel,"StormOfSteel",1,1,0,0,CardType::Skill},
 {Strangle,"Strangle",1,1,0,0,CardType::Attack},
 {StrikeSilent,"StrikeSilent",1,1,0,0,CardType::Attack},
 {SuckerPunch,"SuckerPunch",1,1,0,0,CardType::Attack},
 {Suppress,"Suppress",0,0,2,2,CardType::Attack},
 {Survivor,"Survivor",1,1,0,0,CardType::Skill},
 {Tactician,"Tactician",3,3,16,16,CardType::Skill},
 {TheHunt,"TheHunt",1,1,1,1,CardType::Attack},
 {ToolsOfTheTrade,"ToolsOfTheTrade",1,0,0,0,CardType::Power},
 {Tracking,"Tracking",2,1,0,0,CardType::Power},
 {Untouchable,"Untouchable",2,2,16,16,CardType::Skill},
 {UpMySleeve,"UpMySleeve",2,2,0,0,CardType::Skill},
 {WellLaidPlans,"WellLaidPlans",2,1,0,0,CardType::Power},
 {WraithForm,"WraithForm",3,3,0,0,CardType::Power},
 {Shiv,"Shiv",0,0,1,1,CardType::Attack},
};

static State fight(){
 State s{};new_run(s,197,false,Character::Silent);
 s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=30;s.hp=s.max_hp=80;
 s.deck_count=1;s.deck[0]={DefendSilent,0};
 for(auto&e:s.enemies)e={46,46,0,0,0,2};
 return s;
}
static int append(State&s,Card c,Zone zone){
 CHECK(s.card_count<MaxCards);int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||p.count<MaxHand);p.ids[p.count++]=(uint16_t)id;return id;
}
static void fill(State&s,Zone z,int n,Card c={DefendSilent,0}){for(int i=0;i<n;i++)append(s,c,z);}
static int hand_index(const State&s,int id){for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]==id)return (int)i;return -1;}
static int count_kind(const State&s,unsigned kind,Zone z){int n=0;for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==kind&&s.zones[id]==z)++n;return n;}
static int power(const State&s,SPower p){return s.silent.powers[(unsigned)p];}
static void restore(State&s){
 CHECK(validate(s));SaveData a{},b{};CHECK(save(s,a));State t{};CHECK(load(t,a.bytes,a.size));CHECK(save(t,b));
 CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));s=t;
}
static void settle(State&s){
 for(int step=0;step<100;step++){
  CHECK(s.phase!=Phase::Error&&s.phase!=Phase::Dead);restore(s);
  if(s.phase==Phase::Combat||s.phase==Phase::Reward)return;
  if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned i=0;i<s.discard_required;i++)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,s.discard_required));}
  else if(s.phase==Phase::ExtendedChoice){uint16_t ids[MaxHand];unsigned n=choice_min(s);CHECK(n<=MaxHand);for(unsigned i=0;i<n;i++)ids[i]=choice_pile(s).ids[i];CHECK(choose_extended(s,ids,n));}
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else CHECK(false);
 }
 CHECK(false);
}
static int cast(State&s,unsigned kind,unsigned up=0,int target=0){int id=append(s,{(CardKind)kind,(uint8_t)up},Zone::Hand);CHECK(play(s,hand_index(s,id),target));settle(s);return id;}
static void quiet_turn(State&s){for(auto&e:s.enemies)e.move=2;CHECK(end_turn(s));settle(s);CHECK(s.phase==Phase::Combat);}
static void choose(State&s,int id){CHECK(s.phase==Phase::ExtendedChoice);restore(s);uint16_t selected=(uint16_t)id;CHECK(choose_extended(s,&selected,1));settle(s);}
static void recall(State&s,int id){
 CHECK(s.zones[id]==Zone::Discard);s.potions[0]=Potion::LiquidMemories;CHECK(use_potion(s,0,0));choose(s,id);CHECK(s.zones[id]==Zone::Hand);
}

static void complete_model_matrix(){
 CardKind ids[CardKinds];CHECK(character_pool(Character::Silent,ids)==86);CHECK(sizeof canonical/sizeof canonical[0]==87);
 bool seen[CardKinds]{};
 for(const auto&m:canonical){
  CHECK(!seen[m.id]);seen[m.id]=true;CHECK(card_in_pool(m.id,Character::Silent)==(m.id!=Shiv));
  for(upgrade=0;upgrade<2;upgrade++){
   scenario=m.name;Card c{m.id,(uint8_t)upgrade};int kw=upgrade?m.up_keywords:m.base_keywords;
   CHECK(cost(c)==(upgrade?m.up_cost:m.base_cost));CHECK(definitions[m.id].type==m.type);
   CHECK(card_exhausts(c)==((kw&1)!=0));CHECK(card_innate(c)==((kw&2)!=0));CHECK(card_retains(c)==((kw&4)!=0));
   CHECK(((upgrade?definitions[m.id].upgraded_keywords:definitions[m.id].keywords)&16)==(kw&16));
   auto s=fight();int id=append(s,c,Zone::Hand);fill(s,Zone::Hand,3);
   if(m.id!=GrandFinale)fill(s,Zone::Draw,20);
   fill(s,Zone::Exhaust,3,{Shiv,0});s.enemies[0].poison=3;s.enemies[1].poison=2;
   if(m.id==Finisher)s.silent.attacks_finished=2;
   if(m.id==MementoMori)s.silent.discarded_turn=2;
   if(m.id==Murder)s.silent.drawn_combat=4;
   if(m.id==Expose){s.enemies[0].block=7;s.silent.enemy_artifact[0]=2;}
   if(cost(c)==-1)s.energy=3;
   restore(s);CHECK(can_play_card(s,id));CHECK(play(s,0,0));settle(s);
   CHECK(s.playing==-1&&!s.effect_count);CHECK(s.zones[id]==(m.type==CardType::Power?Zone::Power:(kw&1)?Zone::Exhaust:Zone::Discard));
   int hit=0,all=0,random=0,block=0,draw=0,shivs=0;
   switch(m.id){
   case StrikeSilent:case Slice:hit=upgrade?9:6;break;
   case Backstab:hit=upgrade?15:11;break;
   case DefendSilent:block=upgrade?8:5;break;
   case Deflect:block=upgrade?7:4;break;
   case Neutralize:hit=upgrade?4:3;CHECK(s.enemies[0].weak==(upgrade?2:1));break;
   case Survivor:block=upgrade?11:8;CHECK(s.hand.count==2&&s.silent.discarded_turn==1);break;
   case Acrobatics:draw=upgrade?4:3;CHECK(s.hand.count==(upgrade?6:5)&&s.silent.discarded_turn==1);break;
   case Backflip:block=upgrade?8:5;draw=2;break;
   case DaggerThrow:hit=upgrade?12:9;draw=1;CHECK(s.hand.count==3&&s.silent.discarded_turn==1);break;
   case DeadlyPoison:CHECK(s.enemies[0].poison==(upgrade?10:8));break;
   case PoisonedStab:hit=upgrade?8:6;CHECK(s.enemies[0].poison==(upgrade?7:6));break;
   case CloakAndDagger:block=6;shivs=upgrade?2:1;break;
   case BladeDance:shivs=upgrade?4:3;break;
   case DaggerSpray:all=upgrade?12:8;break;
   case Dash:hit=block=upgrade?13:10;break;
   case LegSweep:block=upgrade?14:11;CHECK(s.enemies[0].weak==(upgrade?3:2));break;
   case Footwork:CHECK(s.dexterity==(upgrade?3:2));break;
   case NoxiousFumes:CHECK(s.noxious_fumes==(upgrade?3:2)&&power(s,SPower::NoxiousFumes)==s.noxious_fumes);break;
   case BouncingFlask:CHECK(s.enemies[0].poison+s.enemies[1].poison==5+(upgrade?12:9));break;
   case Prepared:draw=upgrade?2:1;CHECK(s.hand.count==3&&s.silent.discarded_turn==(upgrade?2:1));break;
   case SuckerPunch:hit=upgrade?10:8;CHECK(s.enemies[0].weak==(upgrade?2:1));break;
   case Adrenaline:draw=2;CHECK(s.energy==(upgrade?32:31));break;
   case Accuracy:CHECK(s.accuracy==(upgrade?6:4));break;
   case LeadingStrike:hit=upgrade?6:3;shivs=2;break;
   case DodgeAndRoll:block=upgrade?6:4;CHECK(s.block_next_turn==block);break;
   case InfiniteBlades:CHECK(s.infinite_blades==1&&power(s,SPower::InfiniteBlades)==1);break;
   case EscapePlan:block=upgrade?5:3;draw=1;break;
   case Predator:hit=upgrade?20:15;CHECK(s.draw_next_turn==2);break;
   case Shiv:hit=upgrade?6:4;break;
   case Abrasive:CHECK(s.dexterity==1&&s.thorns==(upgrade?6:4));break;
   case Accelerant:CHECK(power(s,SPower::Accelerant)==(upgrade?2:1));break;
   case Afterimage:CHECK(power(s,SPower::Afterimage)==1);break;
   case Anticipate:CHECK(s.dexterity==(upgrade?4:2)&&s.temporary_dexterity==s.dexterity);break;
   case Assassinate:hit=upgrade?13:10;CHECK(s.enemies[0].vulnerable==(upgrade?2:1));break;
   case BladeOfInk:shivs=upgrade?3:2;for(unsigned i=24;i<s.card_count;i++)if(s.zones[i]==Zone::Hand)CHECK(s.silent.inky[i]);break;
   case Blur:block=upgrade?8:5;CHECK(power(s,SPower::Blur)==1);break;
   case BubbleBubble:CHECK(s.enemies[0].poison==(upgrade?15:12));break;
   case BulletTime:CHECK(s.no_draw);for(unsigned i=0;i<s.hand.count;i++)CHECK(cost(s,s.hand.ids[i])==0&&s.expansion.turn_cost[s.hand.ids[i]]==1);break;
   case Burst:CHECK(power(s,SPower::Burst)==(upgrade?2:1));break;
   case CalculatedGamble:draw=3;CHECK(s.hand.count==3&&s.silent.discarded_turn==3);break;
   case CorrosiveWave:CHECK(power(s,SPower::CorrosiveWave)==(upgrade?3:2));break;
   case EchoingSlash:all=upgrade?13:10;break;
   case Envenom:CHECK(power(s,SPower::Envenom)==(upgrade?2:1));break;
   case Expertise:draw=upgrade?3:2;for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]>=4)CHECK(s.silent.retain[s.hand.ids[i]]==2);break;
   case Expose:CHECK(!s.silent.enemy_artifact[0]&&!s.enemies[0].block&&s.enemies[0].vulnerable==(upgrade?3:2));break;
   case FanOfKnives:shivs=upgrade?5:4;CHECK(power(s,SPower::FanOfKnives)==1);break;
   case Finisher:hit=upgrade?16:12;break;
   case Flechettes:hit=upgrade?21:15;break;
   case FlickFlack:all=upgrade?9:7;break;
   case Sidestep:CHECK(power(s,SPower::EnergyNextTurn)==(upgrade?2:1));break;
   case GrandFinale:all=upgrade?75:60;CHECK(s.phase==Phase::Reward);break;
   case HandTrick:block=upgrade?10:7;CHECK(s.silent.sly[1]==2);break;
   case Haze:CHECK(s.enemies[0].poison==(upgrade?9:7)&&s.enemies[1].poison==(upgrade?8:6));CHECK(s.enemies[0].weak==(upgrade?2:1)&&s.enemies[1].weak==s.enemies[0].weak);break;
   case HiddenDaggers:shivs=2;CHECK(s.silent.discarded_turn==2);break;
   case KnifeTrap:hit=upgrade?18:12;CHECK(s.exhaust.count==3&&s.silent.shiv_finished);for(unsigned i=24;i<27;i++)CHECK(s.cards[i].upgraded==upgrade);break;
   case Malaise:CHECK(!s.energy&&s.enemies[0].strength==-(3+(int)upgrade)&&s.enemies[0].weak==3+(int)upgrade);break;
   case MasterPlanner:CHECK(power(s,SPower::MasterPlanner)==1);break;
   case MementoMori:hit=upgrade?21:17;break;
   case Mirage:block=5;break;
   case Murder:hit=5;break;
   case Nightmare:CHECK(s.silent.nightmare_active[0]&&s.silent.nightmares[0].card.kind==DefendSilent);break;
   case Outbreak:CHECK(s.enemies[0].hp==(upgrade?31:34)&&s.enemies[1].hp==(upgrade?32:35));CHECK(s.enemies[0].poison==(upgrade?14:11)&&s.enemies[1].poison==(upgrade?13:10));break;
   case PhantomBlades:CHECK(power(s,SPower::PhantomBlades)==(upgrade?12:9));for(unsigned i=24;i<27;i++)CHECK(s.silent.retain[i]==1);break;
   case PiercingWail:for(int t=0;t<2;t++)CHECK(s.enemies[t].strength==-(upgrade?8:6)&&s.expansion.restore_strength[t]==(upgrade?8:6));break;
   case Pinpoint:hit=upgrade?19:15;break;
   case Pounce:hit=upgrade?20:14;CHECK(power(s,SPower::FreeSkill)==1);break;
   case PreciseCut:hit=upgrade?10:7;break;
   case Reflex:draw=upgrade?3:2;CHECK(s.energy==27);break;
   case Ricochet:random=upgrade?15:12;break;
   case SerpentForm:CHECK(power(s,SPower::SerpentForm)==(upgrade?6:4));break;
   case ShadowStep:CHECK(!s.hand.count&&s.silent.discarded_turn==3&&power(s,SPower::ShadowStep)==1);break;
   case Shadowmeld:CHECK(power(s,SPower::Shadowmeld)==1);break;
   case Skewer:hit=upgrade?33:24;CHECK(!s.energy);break;
   case Snakebite:CHECK(s.enemies[0].poison==(upgrade?13:10));break;
   case Speedster:CHECK(power(s,SPower::Speedster)==2);break;
   case StormOfSteel:shivs=3;CHECK(s.hand.count==3&&s.silent.discarded_turn==3);break;
   case Strangle:hit=upgrade?10:8;CHECK(s.silent.strangle[0]==(upgrade?3:2));break;
   case Suppress:hit=upgrade?17:11;CHECK(s.enemies[0].weak==(upgrade?5:3));break;
   case Tactician:CHECK(s.energy==(upgrade?29:28));break;
   case TheHunt:hit=upgrade?15:10;CHECK(!s.silent.extra_rewards);break;
   case ToolsOfTheTrade:CHECK(power(s,SPower::ToolsOfTheTrade)==1);break;
   case Tracking:CHECK(power(s,SPower::Tracking)==50);break;
   case Untouchable:block=upgrade?9:6;break;
   case UpMySleeve:shivs=upgrade?4:3;CHECK(s.silent.discount[id]==1&&cost(s,id)==1);break;
   case WellLaidPlans:CHECK(power(s,SPower::WellLaidPlans)==1);break;
   case WraithForm:CHECK(power_value(s,XPower::Intangible)==(upgrade?3:2)&&power(s,SPower::WraithForm)==1);break;
   default:CHECK(false);
   }
   CHECK(s.block==block);
   if(m.id!=Outbreak){
    if(random)CHECK(s.enemies[0].hp+s.enemies[1].hp==92-random);
    else{CHECK(s.enemies[0].hp==(46-hit-all>0?46-hit-all:0));CHECK(s.enemies[1].hp==(46-all>0?46-all:0));}
   }
   if(m.id!=GrandFinale)CHECK(s.draw.count==20-draw);
   CHECK(count_kind(s,Shiv,Zone::Hand)==shivs);
   if(m.id==HiddenDaggers||m.id==StormOfSteel)for(unsigned i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==Shiv)CHECK(s.cards[s.hand.ids[i]].upgraded==upgrade);
   restore(s);++variants;
  }
 }
 for(unsigned id=0;id<CardKinds;id++)CHECK(seen[id]==(card_in_pool(id,Character::Silent)||id==Shiv));
 CHECK(variants==174);CHECK(card_rarity(Suppress)==5&&card_rarity(WraithForm)==5);
 unsigned rewards=character_pool(Character::Silent,ids,true);for(unsigned i=0;i<rewards;i++)CHECK(ids[i]!=Suppress&&ids[i]!=WraithForm);
}

static void sly_choices_and_discard(){
 scenario="Sly batch and active discard";upgrade=0;
 auto s=fight();int gamble=append(s,{CalculatedGamble,0},Zone::Hand);
 int reflex=append(s,{Reflex,0},Zone::Hand),tactician=append(s,{Tactician,1},Zone::Hand);
 fill(s,Zone::Draw,10);CHECK(play(s,hand_index(s,gamble),0));settle(s);
 CHECK(s.hand.count==4&&s.draw.count==6&&s.energy==32&&s.silent.discarded_turn==2);
 CHECK(s.zones[reflex]==Zone::Discard&&s.zones[tactician]==Zone::Discard&&s.expansion.cards_finished==3);
 // Ordinary end-of-turn flush never calls active-discard/Sly hooks.
 s=fight();reflex=append(s,{Reflex,0},Zone::Hand);tactician=append(s,{Tactician,0},Zone::Hand);fill(s,Zone::Draw,20);
 quiet_turn(s);CHECK(s.energy==3&&s.draw.count==15&&s.zones[reflex]==Zone::Discard&&s.zones[tactician]==Zone::Discard&&!s.expansion.cards_finished);
 s=fight();int trick=append(s,{HandTrick,0},Zone::Hand),normal=append(s,{DefendSilent,0},Zone::Hand);
 append(s,{Reflex,0},Zone::Hand);append(s,{StrikeSilent,0},Zone::Hand);fill(s,Zone::Draw,20);
 CHECK(play(s,0,0));CHECK(s.phase==Phase::ExtendedChoice&&choice_pile(s).count==1&&choice_pile(s).ids[0]==normal);
 auto before=s;uint16_t bad=65535;CHECK(!choose_extended(s,&bad,1));CHECK(!std::memcmp(&before,&s,sizeof s));choose(s,normal);
 CHECK(s.silent.sly[normal]==2&&s.zones[trick]==Zone::Discard);quiet_turn(s);CHECK(!s.silent.sly[normal]);
 s=fight();cast(s,MasterPlanner);int defender=cast(s,DefendSilent);CHECK(s.silent.sly[defender]==1);
 recall(s,defender);int survivor=append(s,{Survivor,0},Zone::Hand);CHECK(play(s,hand_index(s,survivor),0));
 CHECK(s.phase==Phase::DiscardChoice);uint16_t selected=(uint16_t)defender;restore(s);CHECK(choose_discard(s,&selected,1));settle(s);
 CHECK(s.block==18&&s.silent.discarded_turn==1&&s.silent.sly[defender]==1);
}

static void shivs_and_power_triggers(){
 scenario="Phantom, Fan of Knives and Inky";upgrade=0;
 auto s=fight();cast(s,Accuracy);cast(s,PhantomBlades);cast(s,FanOfKnives);cast(s,BladeOfInk);
 int inky=-1;for(unsigned i=0;i<s.card_count;i++)if(s.silent.inky[i]){inky=(int)i;break;}
 CHECK(inky>=0&&!card_targets_enemy(s,s.cards[inky])&&s.silent.retain[inky]);
 CHECK(damage(s,s.cards[inky],0,inky)==17);CHECK(play(s,hand_index(s,inky),-1));settle(s);
 CHECK(s.enemies[0].hp==29&&s.enemies[1].hp==29&&s.enemies[0].weak==1&&s.enemies[1].weak==1);
 int next=-1;for(unsigned i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==Shiv){next=s.hand.ids[i];break;}
 CHECK(next>=0&&damage(s,s.cards[next],0,next)==8);CHECK(play(s,hand_index(s,next),-1));settle(s);CHECK(s.enemies[0].hp==21&&s.enemies[1].hp==21);
 scenario="Captured power amounts and replay";s=fight();s.potions[0]=Potion::Duplicator;CHECK(use_potion(s,0,0));
 cast(s,Afterimage);CHECK(power(s,SPower::Afterimage)==2&&s.block==1);cast(s,DefendSilent);CHECK(s.block==8);
 s=fight();cast(s,SerpentForm);CHECK(s.enemies[0].hp+s.enemies[1].hp==92);cast(s,DefendSilent);CHECK(s.enemies[0].hp+s.enemies[1].hp==88&&s.block==5);
 s=fight();cast(s,Tracking);s.enemies[0].weak=s.enemies[0].vulnerable=1;
 CHECK(damage(s,{FlashOfSteel,0},0)==11);cast(s,FlashOfSteel);CHECK(s.enemies[0].hp==35);
 s=fight();cast(s,Strangle);CHECK(s.enemies[0].hp==38);cast(s,DefendSilent);CHECK(s.enemies[0].hp==36);quiet_turn(s);CHECK(!s.silent.strangle[0]);
}

static void temporal_resources(){
 scenario="Block and duration";upgrade=0;
 auto s=fight();fill(s,Zone::Draw,30);cast(s,Blur);CHECK(s.block==5);quiet_turn(s);CHECK(s.block==5&&!power(s,SPower::Blur));quiet_turn(s);CHECK(!s.block);
 s=fight();fill(s,Zone::Draw,30);cast(s,Barricade);cast(s,Blur);quiet_turn(s);CHECK(s.block==5&&!power(s,SPower::Blur));
 s=fight();cast(s,Shadowmeld);cast(s,DefendSilent);CHECK(s.block==10);s.potions[0]=Potion::Block;CHECK(use_potion(s,0,0));CHECK(s.block==34);
 s=fight();fill(s,Zone::Draw,30);cast(s,ShadowStep);cast(s,ShadowStep);quiet_turn(s);
 CHECK(power(s,SPower::DoubleDamage)==2&&damage(s,{StrikeSilent,0},0)==12);quiet_turn(s);
 CHECK(power(s,SPower::DoubleDamage)==1&&damage(s,{StrikeSilent,0},0)==12);quiet_turn(s);CHECK(!power(s,SPower::DoubleDamage));
 s=fight();fill(s,Zone::Draw,30);cast(s,WraithForm);quiet_turn(s);CHECK(s.dexterity==-1&&power_value(s,XPower::Intangible)==1);
 quiet_turn(s);CHECK(s.dexterity==-2&&!power_value(s,XPower::Intangible));cast(s,DefendSilent);CHECK(s.block==3);restore(s);
 scenario="Tools hand draw and selection save";s=fight();fill(s,Zone::Draw,20);cast(s,ToolsOfTheTrade);
 CHECK(end_turn(s));CHECK(s.phase==Phase::DiscardChoice&&s.playing==-1&&s.hand.count==6&&s.discard_required==1);restore(s);settle(s);
 CHECK(s.hand.count==5&&s.draw.count==14&&s.silent.discarded_turn==1);
 s=fight();int retained=append(s,{DefendSilent,0},Zone::Hand);fill(s,Zone::Draw,30);cast(s,WellLaidPlans);quiet_turn(s);CHECK(s.zones[retained]==Zone::Hand&&s.hand.count==6);
 scenario="Poison trigger and draw hooks";s=fight();cast(s,Accelerant);cast(s,Outbreak);
 CHECK(s.enemies[0].hp==29&&s.enemies[1].hp==29&&s.enemies[0].poison==7&&s.enemies[1].poison==7);
 s=fight();cast(s,Envenom);s.enemies[0].block=4;cast(s,Shiv);CHECK(!s.enemies[0].poison);cast(s,Shiv);CHECK(s.enemies[0].poison==1);
 s=fight();fill(s,Zone::Draw,20);cast(s,CorrosiveWave);cast(s,Speedster);cast(s,Reflex);
 CHECK(s.enemies[0].poison==4&&s.enemies[1].poison==4&&s.enemies[0].hp==42&&s.enemies[1].hp==42);
 int total=s.silent.drawn_combat;cast(s,BladeDance);CHECK(s.silent.drawn_combat==total&&s.enemies[0].hp==42);
}

static void costs_clones_and_limits(){
 scenario="BulletTime expires after the physical play and preserves X payment";upgrade=0;
 auto s=fight();int defend=append(s,{DefendSilent,0},Zone::Hand),skewer=append(s,{Skewer,0},Zone::Hand);
 cast(s,BulletTime);CHECK(cost(s,defend)==0&&cost(s,skewer)==-1&&s.no_draw);
 s.expansion.turn_cost[defend]=3;CHECK(cost(s,defend)==2);CHECK(play(s,hand_index(s,defend),0));settle(s);
 CHECK(!s.expansion.turn_cost[defend]&&!s.silent.bullet_free[defend]&&cost(s,defend)==1);
 s.energy=3;CHECK(play(s,hand_index(s,skewer),0));settle(s);CHECK(!s.energy&&s.enemies[0].hp==22);
 scenario="Ordered Pinpoint discounts";s=fight();int pinpoint=append(s,{Pinpoint,0},Zone::Hand);cast(s,DefendSilent);CHECK(cost(s,pinpoint)==2);
 s.expansion.turn_cost[pinpoint]=3;s.silent.turn_origin[pinpoint]=s.silent.discount[pinpoint];CHECK(cost(s,pinpoint)==2);
 cast(s,DefendSilent);CHECK(cost(s,pinpoint)==1);CHECK(play(s,hand_index(s,pinpoint),0));settle(s);CHECK(cost(s,pinpoint)==1);
 fill(s,Zone::Draw,20);quiet_turn(s);CHECK(cost(s,pinpoint)==3&&!s.silent.discount[pinpoint]);
 scenario="Nightmare immutable snapshot and v13 continuation";s=fight();fill(s,Zone::Draw,20);
 int original=append(s,{Rampage,0},Zone::Hand);s.ironclad.damage_bonus[original]=7;s.expansion.turn_cost[original]=1;s.silent.retain[original]=2;
 int nightmare=append(s,{Nightmare,0},Zone::Hand);CHECK(play(s,hand_index(s,nightmare),0));choose(s,original);
 CHECK(s.silent.nightmare_active[0]&&s.silent.nightmares[0].iron_damage==7&&s.silent.nightmares[0].turn_cost==1);
 s.ironclad.damage_bonus[original]=13;s.expansion.turn_cost[original]=3;s.silent.retain[original]=0;restore(s);
 unsigned first=s.card_count;quiet_turn(s);CHECK(s.card_count==first+3&&!s.silent.nightmare_active[0]);
 for(unsigned id=first;id<s.card_count;id++)CHECK(s.cards[id].kind==Rampage&&s.ironclad.damage_bonus[id]==7&&s.expansion.turn_cost[id]==1&&s.silent.retain[id]==2&&!s.ironclad.forced_exhaust[id]);
 restore(s);
 scenario="Manual and automatic Grand Finale";s=fight();int finale=append(s,{GrandFinale,0},Zone::Hand);fill(s,Zone::Draw,4);
 auto before=s;CHECK(!can_play_card(s,finale)&&!play(s,0,0));CHECK(!std::memcmp(&before,&s,sizeof s));
 s=fight();fill(s,Zone::Draw,4);append(s,{GrandFinale,0},Zone::Draw);s.potions[0]=Potion::DistilledChaos;CHECK(use_potion(s,0,0));settle(s);CHECK(s.phase==Phase::Reward);
 scenario="Echo kill continuation and high hit counts";s=fight();s.enemies[0].hp=10;cast(s,EchoingSlash);CHECK(!s.enemies[0].hp&&s.enemies[1].hp==26);
 s=fight();s.silent.attacks_finished=9999;cast(s,Finisher);CHECK(!s.enemies[0].hp&&s.enemies[1].hp==46&&s.phase==Phase::Combat);restore(s);
 scenario="Fatal card reward groups";s=fight();s.enemies[0].hp=10;cast(s,TheHunt);CHECK(s.silent.extra_rewards==1);
 s.enemies[1].hp=10;cast(s,TheHunt,0,1);CHECK(s.phase==Phase::Reward&&s.silent.extra_rewards==2);restore(s);
 unsigned deck=s.deck_count;CHECK(choose_reward(s,0)&&s.silent.extra_rewards==1&&!(s.reward_claimed&2));
 CHECK(choose_reward(s,-1)&&!s.silent.extra_rewards&&!(s.reward_claimed&2));CHECK(choose_reward(s,1)&&(s.reward_claimed&2)&&s.deck_count==deck+2);restore(s);
 scenario="Reject malformed module state";s=fight();auto invalid=s;invalid.silent.power_count=1;invalid.silent.power_order[0]=(uint8_t)SPower::Afterimage;CHECK(!validate(invalid));
 invalid=s;invalid.silent.inky[0]=1;CHECK(!validate(invalid));
 invalid=s;invalid.silent.nightmare_active[0]=1;CHECK(!validate(invalid));
 append(s,{DefendSilent,0},Zone::Hand);cast(s,Nightmare);CHECK(s.silent.nightmare_active[0]);restore(s);
 invalid=s;invalid.silent.nightmares[0].iron_damage=1;CHECK(!validate(invalid));
 invalid=s;invalid.silent.nightmares[0].stomp_discount=1;CHECK(!validate(invalid));
 invalid=s;invalid.silent.nightmares[0].card={Skewer,0};invalid.silent.nightmares[0].bullet_free=1;CHECK(!validate(invalid));
 auto valid=s;valid.silent.nightmares[0].card={Stomp,0};valid.silent.nightmares[0].stomp_discount=2;valid.silent.nightmares[0].stomp_turn_origin=1;restore(valid);
}

static void multiplicative_rounding_reference(){
 scenario="ARM-safe multiplication against host integer reference";upgrade=0;
 const int bases[]={1,3,5,7,46,9999},strengths[]={0,9999},tracking[]={0,50,100,9999},cruelty[]={0,25,9999};
 for(int base:bases)for(int strength:strengths)for(int track:tracking)for(int cruel:cruelty)for(int vulnerable=0;vulnerable<2;vulnerable++)for(int doubled=0;doubled<3;doubled++){
  auto s=fight();s.silent.drawn_combat=(uint16_t)(base-1);s.strength=(int16_t)strength;
  s.enemies[0].weak=1;s.enemies[0].vulnerable=(int16_t)vulnerable;
  if(track){s.silent.powers[(unsigned)SPower::Tracking]=(int16_t)track;s.silent.power_order[s.silent.power_count++]=(uint8_t)SPower::Tracking;}
  if(doubled){s.silent.powers[(unsigned)SPower::DoubleDamage]=(int16_t)doubled;s.silent.power_order[s.silent.power_count++]=(uint8_t)SPower::DoubleDamage;}
  if(cruel){s.ironclad.powers[(unsigned)IPower::Cruelty]=(int16_t)cruel;s.ironclad.power_order[s.ironclad.power_count++]=(uint8_t)IPower::Cruelty;}
  uint64_t exact=(uint64_t)(base+strength)*(doubled?2u:1u)*(vulnerable?(unsigned)(150+cruel):100u)*(unsigned)(100+track)/10000u;
  int expected=exact>9999999?9999999:(int)exact;
  CHECK(damage(s,{Murder,0},0)==expected);
 }
}

static void shared_after_play_order(){
 scenario="Serpent Form and Panache acquisition order";upgrade=0;
 for(int serpent_first=0;serpent_first<2;serpent_first++){
  auto s=fight();cast(s,serpent_first?SerpentForm:Panache);cast(s,serpent_first?Panache:SerpentForm);
  s.enemies[0].hp=10;s.enemies[1].hp=46;s.expansion.panache_count[0]=4;s.rng_cards=2;
  restore(s);cast(s,DefendSilent);
  // With two targets the fixed test RNG first picks enemy 0. If Panache acts
  // first, it removes that target and Serpent Form must hit enemy 1 instead.
  CHECK(!s.enemies[0].hp&&s.enemies[1].hp==(serpent_first?36:32)&&s.block==5);
  CHECK(s.expansion.panache_count[0]==0);restore(s);
 }
}

int main(){
 complete_model_matrix();sly_choices_and_discard();shivs_and_power_triggers();temporal_resources();costs_clones_and_limits();multiplicative_rounding_reference();shared_after_play_order();
 std::printf("Silent completion regression: %u checks; %u base/upgraded variants; 86 single-player models + Shiv\n",checks,variants);return 0;
}
