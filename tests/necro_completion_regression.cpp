#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static unsigned checks,variants,upgrade;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d [%s +%u] %s\n",__FILE__,__LINE__,scenario,upgrade,#x);std::exit(1);}}while(0)

// Expected costs/keywords are literal facts from the pinned assembly.
// Effect assertions below use independent numeric scenarios, never Definition
// damage/block fields as an oracle.
struct Canonical{CardKind id;const char*name;int cost,up_cost,kw,up_kw,rarity;CardType type;};
static const Canonical canonical[]={
 {NecroAfterlife,"Afterlife",1,1,1,1,2,CardType::Skill},
 {NecroBansheesCry,"BansheesCry",9,7,0,0,4,CardType::Attack},
 {NecroBlightStrike,"BlightStrike",1,1,0,0,2,CardType::Attack},
 {NecroBodyguard,"Bodyguard",1,1,0,0,1,CardType::Skill},
 {NecroBoneShards,"BoneShards",1,1,0,0,3,CardType::Attack},
 {NecroBorrowedTime,"BorrowedTime",1,1,0,0,3,CardType::Skill},
 {NecroBury,"Bury",4,4,0,0,3,CardType::Attack},
 {NecroCalcify,"Calcify",1,1,0,0,3,CardType::Power},
 {NecroCallOfTheVoid,"CallOfTheVoid",1,1,0,2,4,CardType::Power},
 {NecroCaptureSpirit,"CaptureSpirit",1,1,0,0,3,CardType::Skill},
 {NecroCleanse,"Cleanse",1,1,0,0,3,CardType::Skill},
 {NecroCountdown,"Countdown",1,1,0,0,3,CardType::Power},
 {NecroDanseMacabre,"DanseMacabre",1,1,0,0,3,CardType::Power},
 {NecroDeathMarch,"DeathMarch",1,1,0,0,3,CardType::Attack},
 {NecroDeathbringer,"Deathbringer",2,2,0,0,3,CardType::Skill},
 {NecroDeathsDoor,"DeathsDoor",1,1,0,0,3,CardType::Skill},
 {NecroDebilitate,"Debilitate",1,1,0,0,3,CardType::Attack},
 {NecroDefendNecrobinder,"DefendNecrobinder",1,1,0,0,1,CardType::Skill},
 {NecroDefile,"Defile",1,1,8,8,2,CardType::Attack},
 {NecroDefy,"Defy",1,1,8,8,2,CardType::Skill},
 {NecroDelay,"Delay",2,2,0,0,3,CardType::Skill},
 {NecroDemesne,"Demesne",3,2,8,8,4,CardType::Power},
 {NecroDevourLife,"DevourLife",1,1,0,0,4,CardType::Power},
 {NecroDirge,"Dirge",-1,-1,1,1,3,CardType::Skill},
 {NecroDrainPower,"DrainPower",1,1,0,0,2,CardType::Attack},
 {NecroDredge,"Dredge",1,1,1,5,3,CardType::Skill},
 {NecroEidolon,"Eidolon",2,1,1,1,4,CardType::Skill},
 {NecroEndOfDays,"EndOfDays",3,3,0,0,4,CardType::Skill},
 {NecroEnfeeblingTouch,"EnfeeblingTouch",1,1,8,8,3,CardType::Skill},
 {NecroEradicate,"Eradicate",-1,-1,4,4,4,CardType::Attack},
 {NecroFear,"Fear",1,1,8,8,2,CardType::Attack},
 {NecroFetch,"Fetch",0,0,0,0,3,CardType::Attack},
 {NecroFlatten,"Flatten",2,2,0,0,2,CardType::Attack},
 {NecroForbiddenGrimoire,"ForbiddenGrimoire",2,1,64,64,5,CardType::Power},
 {NecroFriendship,"Friendship",1,1,0,0,3,CardType::Power},
 {NecroGraveWarden,"GraveWarden",1,1,0,0,2,CardType::Skill},
 {NecroGraveblast,"Graveblast",1,1,1,0,2,CardType::Attack},
 {NecroHang,"Hang",1,1,0,0,4,CardType::Attack},
 {NecroHaunt,"Haunt",1,1,0,0,3,CardType::Power},
 {NecroHighFive,"HighFive",2,2,0,0,3,CardType::Attack},
 {NecroInvoke,"Invoke",1,1,0,0,2,CardType::Skill},
 {NecroLethality,"Lethality",1,1,8,8,3,CardType::Power},
 {NecroMelancholy,"Melancholy",3,3,0,0,3,CardType::Skill},
 {NecroMisery,"Misery",0,0,0,4,4,CardType::Attack},
 {NecroNecroMastery,"NecroMastery",2,2,0,0,4,CardType::Power},
 {NecroNegativePulse,"NegativePulse",1,1,0,0,2,CardType::Skill},
 {NecroNeurosurge,"Neurosurge",0,0,0,0,4,CardType::Power},
 {NecroNoEscape,"NoEscape",1,1,0,0,3,CardType::Skill},
 {NecroOblivion,"Oblivion",0,0,0,0,4,CardType::Skill},
 {NecroPagestorm,"Pagestorm",1,0,0,0,3,CardType::Power},
 {NecroParse,"Parse",1,1,8,8,3,CardType::Skill},
 {NecroPoke,"Poke",0,0,0,0,2,CardType::Attack},
 {NecroProtector,"Protector",1,0,0,0,5,CardType::Attack},
 {NecroPullAggro,"PullAggro",2,2,0,0,2,CardType::Skill},
 {NecroPullFromBelow,"PullFromBelow",1,1,0,0,3,CardType::Attack},
 {NecroPutrefy,"Putrefy",1,1,1,1,3,CardType::Skill},
 {NecroRattle,"Rattle",1,1,0,0,3,CardType::Attack},
 {NecroReanimate,"Reanimate",3,3,1,1,4,CardType::Skill},
 {NecroReap,"Reap",3,3,4,4,2,CardType::Attack},
 {NecroReaperForm,"ReaperForm",3,3,0,4,4,CardType::Power},
 {NecroReave,"Reave",1,1,0,0,2,CardType::Attack},
 {NecroRightHandHand,"RightHandHand",0,0,0,0,3,CardType::Attack},
 {NecroSacrifice,"Sacrifice",1,0,4,4,4,CardType::Skill},
 {NecroScourge,"Scourge",1,1,0,0,2,CardType::Skill},
 {NecroSculptingStrike,"SculptingStrike",1,1,0,0,2,CardType::Attack},
 {NecroSeance,"Seance",1,0,8,8,4,CardType::Skill},
 {NecroSentryMode,"SentryMode",2,1,0,0,4,CardType::Power},
 {NecroSeverance,"Severance",2,2,0,0,3,CardType::Attack},
 {NecroSharedFate,"SharedFate",0,0,1,1,4,CardType::Skill},
 {NecroShroud,"Shroud",1,1,0,0,3,CardType::Power},
 {NecroSicEm,"SicEm",1,1,0,0,3,CardType::Attack},
 {NecroSleightOfFlesh,"SleightOfFlesh",2,2,0,0,3,CardType::Power},
 {NecroSnap,"Snap",1,1,0,0,2,CardType::Attack},
 {NecroSoulStorm,"SoulStorm",1,1,0,0,4,CardType::Attack},
 {NecroSow,"Sow",1,1,4,4,2,CardType::Attack},
 {NecroSpiritOfAsh,"SpiritOfAsh",1,1,0,0,4,CardType::Power},
 {NecroSpur,"Spur",1,1,4,4,3,CardType::Skill},
 {NecroSqueeze,"Squeeze",3,3,0,0,4,CardType::Attack},
 {NecroStrikeNecrobinder,"StrikeNecrobinder",1,1,0,0,1,CardType::Attack},
 {NecroTheScythe,"TheScythe",2,2,1,1,4,CardType::Attack},
 {NecroTimesUp,"TimesUp",2,2,0,4,4,CardType::Attack},
 {NecroTransfigure,"Transfigure",1,1,1,0,4,CardType::Skill},
 {NecroUndeath,"Undeath",0,0,0,0,4,CardType::Skill},
 {NecroUnleash,"Unleash",1,1,0,0,1,CardType::Attack},
 {NecroVeilpiercer,"Veilpiercer",1,1,0,0,3,CardType::Attack},
 {NecroWisp,"Wisp",0,0,1,5,2,CardType::Skill},
 {SweepingGaze,"SweepingGaze",0,0,9,9,7,CardType::Attack}
};
static State fight(){
 State s{};new_run(s,817,false,Character::Necrobinder);
 s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=30;s.hp=s.max_hp=66;
 s.deck_count=1;s.deck[0]={NecroDefendNecrobinder,0};
 s.expansion.osty_hp=10;s.expansion.osty_max_hp=14;s.necro.osty_was_alive=1;
 for(auto&e:s.enemies)e={1000,1000,0,0,0,2};return s;
}
static int append(State&s,Card c,Zone zone){
 CHECK(s.card_count<MaxCards);int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||p.count<MaxHand);p.ids[p.count++]=(uint16_t)id;return id;
}
static void fill(State&s,Zone zone,int count,Card c={NecroDefendNecrobinder,0}){while(count--)append(s,c,zone);}
static int hand_index(const State&s,int id){for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]==id)return (int)i;return -1;}
static int count_kind(const State&s,CardKind k,Zone z){int n=0;for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind==k&&s.zones[i]==z)++n;return n;}
static int power(const State&s,NPower p){return s.necro.powers[(unsigned)p];}
static void restore(State&s){
 CHECK(validate(s));SaveData a{},b{};CHECK(save(s,a));State copy{};CHECK(load(copy,a.bytes,a.size));CHECK(save(copy,b));
 CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));s=copy;
}
static void settle(State&s){
 for(int step=0;step<100;step++){
  CHECK(s.phase!=Phase::Error&&s.phase!=Phase::Dead);restore(s);
  if(s.phase==Phase::Combat||s.phase==Phase::Reward)return;
  if(s.phase==Phase::ExtendedChoice){uint16_t ids[MaxHand];unsigned n=choice_min(s);CHECK(n<=MaxHand);for(unsigned i=0;i<n;i++)ids[i]=choice_pile(s).ids[i];CHECK(choose_extended(s,ids,n));}
  else if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned i=0;i<s.discard_required;i++)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,s.discard_required));}
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else CHECK(false);
 }
 CHECK(false);
}
static int cast(State&s,CardKind kind,unsigned up=0,int target=0){
 int id=append(s,{kind,(uint8_t)up},Zone::Hand);CHECK(play(s,hand_index(s,id),target));settle(s);return id;
}
static void quiet_turn(State&s){for(auto&e:s.enemies)e.move=2;CHECK(end_turn(s));settle(s);CHECK(s.phase==Phase::Combat);}
static void move_hand(State&s,int id){
 Pile*from=s.zones[id]==Zone::Draw?&s.draw:s.zones[id]==Zone::Discard?&s.discard:s.zones[id]==Zone::Exhaust?&s.exhaust:nullptr;CHECK(from&&s.hand.count<MaxHand);
 bool found=false;for(unsigned i=0;i<from->count;i++)if(from->ids[i]==id){for(unsigned j=i+1;j<from->count;j++)from->ids[j-1]=from->ids[j];--from->count;found=true;break;}CHECK(found);
 s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;
}
static void choose(State&s,int id){CHECK(s.phase==Phase::ExtendedChoice);restore(s);uint16_t selection=(uint16_t)id;CHECK(choose_extended(s,&selection,1));settle(s);}
static void complete_model_matrix(){
 CardKind ids[CardKinds];CHECK(character_pool(Character::Necrobinder,ids)==86);CHECK(sizeof canonical/sizeof canonical[0]==87);
 bool seen[CardKinds]{};
 for(const auto&m:canonical)for(upgrade=0;upgrade<2;upgrade++){
  scenario=m.name;if(!upgrade){CHECK(!seen[m.id]);seen[m.id]=true;}
  CHECK(card_in_pool(m.id,Character::Necrobinder)==(m.id!=SweepingGaze));
  Card c{m.id,(uint8_t)upgrade};int kw=upgrade?m.up_kw:m.kw;
  CHECK(cost(c)==(upgrade?m.up_cost:m.cost));CHECK(card_rarity(m.id)==m.rarity);CHECK(definitions[m.id].type==m.type);
  CHECK(((upgrade?definitions[m.id].upgraded_keywords:definitions[m.id].keywords)|(definitions[m.id].exhausts?1:0))==kw);
  CHECK(card_exhausts(c)==((kw&1)!=0));CHECK(card_innate(c)==((kw&2)!=0));CHECK(card_retains(c)==((kw&4)!=0));
  auto s=fight();int id=append(s,c,Zone::Hand);fill(s,Zone::Hand,3);fill(s,Zone::Draw,20);fill(s,Zone::Discard,4);
  int soul0=append(s,{Soul,0},Zone::Exhaust),soul1=append(s,{Soul,0},Zone::Exhaust);
  s.expansion.doom[0]=20;s.necro.drawn_turn=2;s.necro.ethereal_played=2;s.necro.osty_attacks_turn=1;
  if(m.id==NecroBansheesCry)s.necro.cost_delta[id]=-4;
  if(m.id==NecroFlatten)s.necro.flatten_zero[id]=1;
  if(m.id==NecroEidolon)s.necro.ethereal[soul0]=s.necro.ethereal[soul1]=1;
  if(cost(c)<0)s.energy=3;
  int starting_energy=s.energy,price=cost(s,id);if(price<0)price=starting_energy;
  restore(s);CHECK(can_play_card(s,id));CHECK(play(s,0,0));settle(s);CHECK(s.playing==-1&&!s.effect_count);
  CHECK(s.zones[id]==(m.type==CardType::Power?Zone::Power:(kw&1)?Zone::Exhaust:Zone::Discard));
  int hit=0,all=0,random=0,block=0,draw=0,energy=0,summon=0;
  switch(m.id){
  case NecroAfterlife:summon=upgrade?9:6;break;
  case NecroBansheesCry:all=33;break;
  case NecroBlightStrike:hit=upgrade?10:8;CHECK(s.expansion.doom[0]==20+hit);break;
  case NecroBodyguard:summon=upgrade?7:5;break;
  case NecroBoneShards:all=block=upgrade?12:9;CHECK(s.expansion.osty_hp==0&&s.necro.creature_deaths==1);break;
  case NecroBorrowedTime:energy=upgrade?6:4;CHECK(power(s,NPower::BorrowedTime)==1);CHECK(cost(s,s.hand.ids[0])==2);break;
  case NecroBury:hit=upgrade?63:52;break;
  case NecroCalcify:CHECK(power(s,NPower::Calcify)==(upgrade?6:4));break;
  case NecroCallOfTheVoid:CHECK(power(s,NPower::CallOfTheVoid)==1);break;
  case NecroCaptureSpirit:hit=upgrade?4:3;CHECK(count_kind(s,Soul,Zone::Draw)==(upgrade?4:3));break;
  case NecroCleanse:summon=upgrade?5:3;CHECK(s.draw.count==19&&s.exhaust.count==3);break;
  case NecroCountdown:CHECK(power(s,NPower::Countdown)==(upgrade?9:6));break;
  case NecroDanseMacabre:CHECK(power(s,NPower::DanseMacabre)==(upgrade?6:4));break;
  case NecroDeathMarch:hit=upgrade?21:16;break;
  case NecroDeathbringer:CHECK(s.expansion.doom[0]==(upgrade?46:41)&&s.expansion.doom[1]==(upgrade?26:21));CHECK(s.enemies[0].weak==1&&s.enemies[1].weak==1);break;
  case NecroDeathsDoor:block=upgrade?7:6;break;
  case NecroDebilitate:hit=upgrade?12:10;CHECK(s.necro.debilitate[0]==(upgrade?3:2));break;
  case NecroDefendNecrobinder:block=upgrade?8:5;break;
  case NecroDefile:hit=upgrade?17:13;break;
  case NecroDefy:block=upgrade?9:6;CHECK(s.enemies[0].weak==1);break;
  case NecroDelay:block=upgrade?13:11;CHECK(power(s,NPower::EnergyNextTurn)==(upgrade?2:1));break;
  case NecroDemesne:CHECK(power(s,NPower::Demesne)==1);break;
  case NecroDevourLife:CHECK(power(s,NPower::DevourLife)==(upgrade?2:1));break;
  case NecroDirge:summon=upgrade?12:9;CHECK(count_kind(s,Soul,Zone::Draw)==3);for(unsigned j=0;j<s.draw.count;j++)if(s.cards[s.draw.ids[j]].kind==Soul)CHECK(s.cards[s.draw.ids[j]].upgraded==upgrade);break;
  case NecroDrainPower:{hit=upgrade?12:10;int n=0;for(unsigned j=0;j<s.discard.count;j++)if(s.discard.ids[j]!=(unsigned)id&&s.cards[s.discard.ids[j]].upgraded)++n;CHECK(n==(upgrade?3:2));break;}
  case NecroDredge:CHECK(s.hand.count==6&&s.discard.count==1);break;
  case NecroEidolon:draw=4;CHECK(s.exhaust.count==3&&s.necro.ethereal_played==4);break;
  case NecroEndOfDays:CHECK(s.expansion.doom[0]==(upgrade?57:49)&&s.expansion.doom[1]==(upgrade?37:29));break;
  case NecroEnfeeblingTouch:CHECK(s.enemies[0].strength==-(upgrade?11:8)&&s.expansion.restore_strength[0]==(upgrade?11:8));break;
  case NecroEradicate:hit=upgrade?42:33;break;
  case NecroFear:hit=upgrade?8:7;CHECK(s.enemies[0].vulnerable==(upgrade?2:1));break;
  case NecroFetch:hit=upgrade?6:3;draw=1;CHECK(s.necro.fetch_played[id]);break;
  case NecroFlatten:hit=upgrade?16:12;CHECK(cost(s,id)==0);break;
  case NecroForbiddenGrimoire:CHECK(power(s,NPower::ForbiddenGrimoire)==1);break;
  case NecroFriendship:CHECK(s.strength==-(upgrade?1:2)&&power(s,NPower::Friendship)==1);break;
  case NecroGraveWarden:block=upgrade?11:8;CHECK(count_kind(s,Soul,Zone::Draw)==1);break;
  case NecroGraveblast:hit=upgrade?6:4;CHECK(s.hand.count==4);break;
  case NecroHang:hit=upgrade?13:10;CHECK(s.necro.hang[0]==2);break;
  case NecroHaunt:CHECK(power(s,NPower::Haunt)==(upgrade?9:7));break;
  case NecroHighFive:all=upgrade?13:11;CHECK(s.enemies[0].vulnerable==(upgrade?3:2)&&s.enemies[1].vulnerable==(upgrade?3:2));break;
  case NecroInvoke:CHECK(power(s,NPower::SummonNextTurn)==(upgrade?3:2)&&power(s,NPower::EnergyNextTurn)==(upgrade?3:2));break;
  case NecroLethality:CHECK(power(s,NPower::Lethality)==(upgrade?75:50));break;
  case NecroMelancholy:block=upgrade?17:13;break;
  case NecroMisery:hit=upgrade?9:7;CHECK(s.expansion.doom[1]==20);break;
  case NecroNecroMastery:summon=upgrade?8:5;CHECK(power(s,NPower::NecroMastery)==1);break;
  case NecroNegativePulse:block=upgrade?6:5;CHECK(s.expansion.doom[0]==(upgrade?31:27)&&s.expansion.doom[1]==(upgrade?11:7));break;
  case NecroNeurosurge:energy=upgrade?4:3;draw=2;CHECK(power(s,NPower::Neurosurge)==3&&s.necro.player_doom==0);break;
  case NecroNoEscape:CHECK(s.expansion.doom[0]==(upgrade?45:40));break;
  case NecroOblivion:CHECK(s.necro.oblivion[0]==(upgrade?4:3)&&s.expansion.doom[0]==20);break;
  case NecroPagestorm:CHECK(power(s,NPower::Pagestorm)==1);break;
  case NecroParse:draw=upgrade?4:3;break;
  case NecroPoke:hit=upgrade?9:6;break;
  case NecroProtector:hit=upgrade?29:24;break;
  case NecroPullAggro:summon=upgrade?5:4;block=upgrade?9:7;break;
  case NecroPullFromBelow:hit=upgrade?14:10;break;
  case NecroPutrefy:CHECK(s.enemies[0].weak==(upgrade?3:2)&&s.enemies[0].vulnerable==(upgrade?3:2));break;
  case NecroRattle:hit=upgrade?18:14;CHECK(s.necro.osty_attacks_turn==2);break;
  case NecroReanimate:summon=upgrade?25:20;break;
  case NecroReap:hit=upgrade?33:27;break;
  case NecroReaperForm:CHECK(power(s,NPower::ReaperForm)==1);break;
  case NecroReave:hit=upgrade?13:10;CHECK(count_kind(s,Soul,Zone::Draw)==1);for(unsigned j=0;j<s.draw.count;j++)if(s.cards[s.draw.ids[j]].kind==Soul)CHECK(s.cards[s.draw.ids[j]].upgraded==upgrade);break;
  case NecroRightHandHand:hit=upgrade?6:4;break;
  case NecroSacrifice:block=42;CHECK(s.expansion.osty_hp==0&&s.necro.creature_deaths==1);break;
  case NecroScourge:draw=upgrade?2:1;CHECK(s.expansion.doom[0]==(upgrade?36:33));break;
  case NecroSculptingStrike:hit=upgrade?12:9;CHECK(s.necro.ethereal[s.hand.ids[0]]);break;
  case NecroSeance:CHECK(count_kind(s,Soul,Zone::Draw)==1&&s.draw.count==20);break;
  case NecroSentryMode:CHECK(power(s,NPower::SentryMode)==1);break;
  case NecroSeverance:hit=upgrade?18:13;CHECK(count_kind(s,Soul,Zone::Draw)==1&&count_kind(s,Soul,Zone::Hand)==1&&count_kind(s,Soul,Zone::Discard)==1);break;
  case NecroSharedFate:CHECK(s.strength==-2&&s.enemies[0].strength==-(upgrade?3:2)&&!s.expansion.restore_strength[0]);break;
  case NecroShroud:CHECK(power(s,NPower::Shroud)==(upgrade?4:3));break;
  case NecroSicEm:hit=upgrade?6:5;CHECK(s.necro.sic_em[0]==(upgrade?4:3)&&s.expansion.osty_hp==10);break;
  case NecroSleightOfFlesh:CHECK(power(s,NPower::SleightOfFlesh)==(upgrade?13:9));break;
  case NecroSnap:hit=upgrade?10:7;CHECK(s.silent.retain[s.hand.ids[0]]==1);break;
  case NecroSoulStorm:hit=upgrade?21:17;break;
  case NecroSow:all=upgrade?11:8;break;
  case NecroSpiritOfAsh:CHECK(power(s,NPower::SpiritOfAsh)==(upgrade?5:4));break;
  case NecroSpur:CHECK(s.expansion.osty_hp==(upgrade?19:17)&&s.expansion.osty_max_hp==(upgrade?19:17));break;
  case NecroSqueeze:hit=upgrade?30:25;break;
  case NecroStrikeNecrobinder:hit=upgrade?9:6;break;
  case NecroTheScythe:hit=13;CHECK(s.necro.scythe_value[id]==(upgrade?7:5)&&!s.necro.deck_origin[id]);break;
  case NecroTimesUp:hit=20;break;
  case NecroTransfigure:CHECK(s.expansion.replay[s.hand.ids[0]]==1&&cost(s,s.hand.ids[0])==2);break;
  case NecroUndeath:block=upgrade?9:7;CHECK(count_kind(s,NecroUndeath,Zone::Discard)==2);break;
  case NecroUnleash:hit=upgrade?19:16;break;
  case NecroVeilpiercer:hit=upgrade?13:10;CHECK(power(s,NPower::Veilpiercer)==1);break;
  case NecroWisp:energy=1;break;
  case SweepingGaze:random=upgrade?15:10;break;
  default:CHECK(false);
  }
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000-hit-2*all-random);
  if(!random){CHECK(s.enemies[0].hp==1000-hit-all);CHECK(s.enemies[1].hp==1000-all);}
  CHECK(s.block==block);CHECK(s.energy==starting_energy-price+energy);
  if(draw)CHECK(s.necro.drawn_turn==2+draw);
  if(summon){CHECK(s.expansion.osty_hp==10+summon);CHECK(s.expansion.osty_max_hp==14+summon);}
  restore(s);++variants;
 }
}
static void install(State&s,NPower p,int amount){queue_extension(s,XOp::NPower,(int)p,amount);resolve_pending(s);settle(s);}
static int generated(State&s,Card c){int id=s.card_count;queue_extension(s,XOp::Create,1,card_token(c));resolve_pending(s);settle(s);CHECK(s.card_count==id+1);return id;}
static void repeat_instance(State&s,int id,int target=0){if(s.zones[id]!=Zone::Hand)move_hand(s,id);CHECK(play(s,hand_index(s,id),target));settle(s);}

static void osty_damage_and_life(){
 scenario="Osty uses Calcify, preserves player Vigor, and ignores player Strength";upgrade=0;
 {auto s=fight();cast(s,NecroCalcify);s.strength=10;s.vigor=8;
  CHECK(damage(s,{NecroPoke,0},0)==10);cast(s,NecroPoke);CHECK(s.enemies[0].hp==990&&s.vigor==8);
  cast(s,NecroStrikeNecrobinder);CHECK(s.enemies[0].hp==966&&!s.vigor);restore(s);
 }
 scenario="Osty retains owner-card PenNib and Gigantification multipliers";
 {auto s=fight();CHECK(gain_relic(s,PenNib));s.relic_counters[9]=9;gain_power(s,XPower::Gigantic,1);s.strength=19;s.vigor=8;
  CHECK(damage(s,{NecroPoke,0},0)==36);cast(s,NecroPoke);CHECK(s.enemies[0].hp==964&&s.vigor==8&&relic_counter(s,PenNib)==0);restore(s);
 }
 scenario="Lethality applies once, including an Osty attack, with one final rounding";
 {auto s=fight();cast(s,NecroLethality,1);s.enemies[0].vulnerable=1;
  CHECK(damage(s,{NecroPoke,0},0)==15);int id=append(s,{NecroPoke,0},Zone::Hand);s.expansion.replay[id]=1;
  CHECK(play(s,hand_index(s,id),0));settle(s);CHECK(s.enemies[0].hp==976&&s.necro.osty_attacks_turn==2);
 }
 scenario="Block then Osty then player overflow; NecroMastery reflects actual pet HP lost";
 {auto s=fight();install(s,NPower::NecroMastery,1);s.block=4;
  player_hp_loss(s,20,true,true);resolve_pending(s);settle(s);
  CHECK(s.hp==60&&!s.block&&!s.expansion.osty_hp&&s.expansion.osty_max_hp==14);
  CHECK(s.enemies[0].hp==990&&s.enemies[1].hp==990&&s.necro.creature_deaths==1);
  cast(s,NecroBodyguard);CHECK(s.expansion.osty_hp==5&&s.expansion.osty_max_hp==5);
  cast(s,NecroSpur);CHECK(s.expansion.osty_hp==8&&s.expansion.osty_max_hp==8);
  cast(s,NecroSacrifice);CHECK(s.block==24&&!s.expansion.osty_hp&&s.necro.creature_deaths==2);
  CHECK(s.enemies[0].hp==982&&s.enemies[1].hp==982);
 }
 scenario="Buffer prevents only player overflow, while non-attack HP loss bypasses Osty";
 {auto s=fight();install(s,NPower::NecroMastery,1);gain_power(s,XPower::Buffer,1);s.block=4;
  player_hp_loss(s,20,true,true);resolve_pending(s);settle(s);CHECK(s.hp==66&&!power_value(s,XPower::Buffer)&&!s.expansion.osty_hp&&s.enemies[0].hp==990);
  cast(s,NecroBodyguard);player_hp_loss(s,4,false,false);resolve_pending(s);settle(s);CHECK(s.hp==62&&s.expansion.osty_hp==5&&s.enemies[0].hp==990);
 }
 scenario="Missing Osty rejects HighFive and suppresses every Fetch command";
 {auto s=fight();s.expansion.osty_hp=0;s.necro.osty_was_alive=0;fill(s,Zone::Draw,6);
  int high=append(s,{NecroHighFive,0},Zone::Hand);CHECK(!can_play_card(s,high)&&!play(s,hand_index(s,high),0));
  cast(s,NecroFetch);CHECK(s.enemies[0].hp==1000&&!s.necro.drawn_turn&&!s.necro.osty_attacks_turn);restore(s);
 }
 scenario="SicEm summons on each fully blocked Osty hit; Rattle counts commands";
 {auto s=fight();cast(s,NecroSicEm);s.enemies[0].block=100;
  cast(s,NecroRattle);CHECK(s.expansion.osty_hp==16&&s.expansion.osty_max_hp==20);
  CHECK(s.necro.osty_attacks_turn==2&&s.enemies[0].hp==995&&s.enemies[0].block==86);
  cast(s,NecroRattle);CHECK(s.expansion.osty_hp==25&&s.necro.osty_attacks_turn==3&&s.enemies[0].block==65);
  quiet_turn(s);CHECK(!s.necro.sic_em[0]&&!s.necro.osty_attacks_turn);
 }
 scenario="Squeeze scans all combat piles and excludes only its own physical instance";
 {auto s=fight();append(s,{NecroPoke,0},Zone::Draw);append(s,{NecroFetch,0},Zone::Discard);append(s,{NecroRattle,0},Zone::Exhaust);
  int id=append(s,{NecroSqueeze,1},Zone::Hand);CHECK(damage(s,s.cards[id],0,id)==48);repeat_instance(s,id);CHECK(s.enemies[0].hp==952);
 }
}

static void doom_and_debuffs(){
 scenario="Doom and Weak each trigger SleightOfFlesh; only Doom triggers Shroud";
 {auto s=fight();cast(s,NecroShroud);cast(s,NecroSleightOfFlesh);cast(s,NecroDeathbringer);
  CHECK(s.block==6&&s.enemies[0].hp==982&&s.enemies[1].hp==982&&s.necro.doom_applied_turn);
  CHECK(s.expansion.doom[0]==21&&s.expansion.doom[1]==21);restore(s);
 }
 scenario="Artifact blocks Doom and its secondary effects atomically";
 {auto s=fight();cast(s,NecroShroud);cast(s,NecroSleightOfFlesh);s.silent.enemy_artifact[0]=1;cast(s,NecroNoEscape);
  CHECK(!s.expansion.doom[0]&&!s.block&&!s.necro.doom_applied_turn&&s.enemies[0].hp==1000&&!s.silent.enemy_artifact[0]);
 }
 scenario="BlightStrike and ReaperForm use blocked plus actual HP damage";
 {auto s=fight();cast(s,NecroReaperForm);s.enemies[0].block=5;cast(s,NecroBlightStrike);
  CHECK(s.enemies[0].hp==997&&s.expansion.doom[0]==16);
  cast(s,NecroPoke);CHECK(s.enemies[0].hp==991&&s.expansion.doom[0]==22);
 }
 scenario="NoEscape thresholds use complete groups of ten existing Doom";
 for(int existing: {9,10,19,20}){auto s=fight();s.expansion.doom[0]=(int16_t)existing;cast(s,NecroNoEscape);
  CHECK(s.expansion.doom[0]==existing+10+(existing/10)*5);
 }
 scenario="DeathsDoor repeats independent Dexterity-adjusted block commands";
 {auto s=fight();s.dexterity=3;cast(s,NecroDeathsDoor);CHECK(s.block==9);cast(s,NecroNoEscape);s.block=0;cast(s,NecroDeathsDoor);CHECK(s.block==27);}
 scenario="Ordinary Doom executes at enemy-side end; EndOfDays checks immediately";
 {auto s=fight();s.enemies[0].hp=20;cast(s,NecroNoEscape);cast(s,NecroNoEscape);CHECK(s.enemies[0].hp==20);
  quiet_turn(s);CHECK(!s.enemies[0].hp&&s.necro.creature_deaths==1&&s.enemies[1].hp==1000);
 }
 {auto s=fight();s.enemies[0].hp=29;cast(s,NecroEndOfDays);CHECK(!s.enemies[0].hp&&s.necro.creature_deaths==1&&s.turn==1);}
 scenario="Player Doom from Neurosurge triggers Shroud then executes at own side end";
 {auto s=fight();fill(s,Zone::Draw,16);cast(s,NecroShroud);cast(s,NecroNeurosurge);CHECK(!s.necro.player_doom);
  quiet_turn(s);CHECK(s.necro.player_doom==3&&s.block==3&&s.necro.doom_applied_turn);
  s.hp=3;CHECK(end_turn(s));CHECK(s.phase==Phase::Dead&&!s.hp&&!s.effect_count&&s.playing==-1);restore(s);
 }
 scenario="Debilitate doubles the effect, not the stack count, of Weak and Vulnerable";
 {auto s=fight();cast(s,NecroDebilitate);apply_weak(s,0,3);apply_vulnerable(s,0,3);resolve_pending(s);settle(s);s.enemies[0].move=0;
  CHECK(damage(s,{NecroPoke,0},0)==12&&enemy_damage(s,0)==6);cast(s,NecroPoke);CHECK(s.enemies[0].hp==978);
  quiet_turn(s);CHECK(s.necro.debilitate[0]==1);quiet_turn(s);CHECK(!s.necro.debilitate[0]);
 }
 scenario="Hang doubles every later Hang against the same target, including other copies";
 {auto s=fight();cast(s,NecroHang);cast(s,NecroHang);cast(s,NecroHang,1);CHECK(s.enemies[0].hp==918&&s.necro.hang[0]==8);}
 scenario="Oblivion captures old stacks for each CardPlay and each replay";
 {auto s=fight();cast(s,NecroOblivion);CHECK(!s.expansion.doom[0]);cast(s,NecroOblivion);CHECK(s.expansion.doom[0]==3&&s.necro.oblivion[0]==6);
  int wisp=append(s,{NecroWisp,0},Zone::Hand);s.expansion.replay[wisp]=1;repeat_instance(s,wisp);CHECK(s.expansion.doom[0]==15);
  quiet_turn(s);CHECK(!s.necro.oblivion[0]);
 }
 scenario="Misery copies all modeled debuffs in acquisition order through Artifact";
 {auto s=fight();apply_vulnerable(s,0,2);apply_weak(s,0,3);apply_poison(s,0,4);resolve_pending(s);settle(s);
  s.enemies[0].strength=-5;s.expansion.beetle[0]=6;s.expansion.demise[0]=7;s.expansion.doom[0]=8;s.silent.strangle[0]=9;
  s.necro.debilitate[0]=2;s.necro.hang[0]=4;s.necro.oblivion[0]=3;s.necro.sic_em[0]=5;s.regent.conqueror[0]=2;s.silent.enemy_artifact[1]=1;
  cast(s,NecroMisery);CHECK(!s.enemies[1].vulnerable&&s.enemies[1].weak==3&&s.enemies[1].poison==4&&s.enemies[1].strength==-5);
  CHECK(s.expansion.beetle[1]==6&&s.expansion.demise[1]==7&&s.expansion.doom[1]==8&&s.silent.strangle[1]==9);
  CHECK(s.necro.debilitate[1]==2&&s.necro.hang[1]==4&&s.necro.oblivion[1]==3&&s.necro.sic_em[1]==5&&s.regent.conqueror[1]==2&&!s.silent.enemy_artifact[1]);restore(s);
 }
 scenario="Misery freezes its debuffs before damage-triggered ReaperForm or Envenom";
 {auto s=fight();cast(s,NecroReaperForm);cast(s,NecroMisery);CHECK(s.expansion.doom[0]==7&&!s.expansion.doom[1]);}
 {auto s=fight();cast(s,Envenom);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(s.enemies[0].poison==1&&!s.enemies[1].poison&&s.silent.enemy_artifact[1]==1);restore(s);
 }
}

static void temporary_strength_and_misery(){
 scenario="Every temporary Strength source keeps its distinct restoration wrapper";
 const CardKind models[]={DarkShackles,Mangle,NecroEnfeeblingTouch,PiercingWail,RegentCrushUnder,RegentDyingStar,RegentMonarchsGaze};
 const int amounts[]={9,10,8,6,1,9,1};
 for(unsigned source=0;source<NTempStrengthCount;source++){
  auto s=fight();s.expansion.stars=99;cast(s,models[source]);if(source==6)cast(s,NecroStrikeNecrobinder);
  CHECK(s.necro.temp_strength[0][source]==amounts[source]&&s.expansion.restore_strength[0]==amounts[source]);
  CHECK(s.necro.debuff_order[0][0]==(unsigned)NDebuff::Strength+1&&s.necro.debuff_order[0][1]==(unsigned)NDebuff::DarkShackles+source+1);restore(s);
 }
 scenario="Misery copies temporary loss and restores it at the same side-end boundary";
 {auto s=fight();cast(s,NecroEnfeeblingTouch);cast(s,NecroMisery);
  CHECK(s.enemies[1].strength==-8&&s.expansion.restore_strength[1]==8&&s.necro.temp_strength[1][2]==8);quiet_turn(s);
  CHECK(s.enemies[1].strength==2&&!s.expansion.restore_strength[1]&&!s.necro.temp_strength[1][2]);restore(s);
 }
 scenario="Misery separates permanent and temporary negative Strength before Artifact";
 {auto s=fight();s.enemies[0].strength=-5;cast(s,NecroEnfeeblingTouch);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(s.enemies[1].strength==-8&&s.expansion.restore_strength[1]==8&&!s.silent.enemy_artifact[1]);
 }
 scenario="Misery preserves signed positive reconciliation while Artifact blocks the wrapper";
 {auto s=fight();s.enemies[0].strength=5;cast(s,NecroEnfeeblingTouch);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(s.enemies[0].strength==-3&&s.enemies[1].strength==5&&!s.expansion.restore_strength[1]&&!s.silent.enemy_artifact[1]);
 }
 scenario="A nonnegative source Strength is excluded before temporary reconciliation";
 {auto s=fight();s.enemies[0].strength=10;cast(s,NecroEnfeeblingTouch);cast(s,NecroMisery);
  CHECK(s.enemies[0].strength==2&&s.enemies[1].strength==-8&&s.expansion.restore_strength[1]==8);
 }
 scenario="Distinct temporary wrappers consume Artifact in actual acquisition order";
 {auto s=fight();cast(s,NecroEnfeeblingTouch);cast(s,DarkShackles);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(s.enemies[1].strength==-9&&s.expansion.restore_strength[1]==9&&s.necro.temp_strength[1][0]==9&&!s.necro.temp_strength[1][2]);
 }
 scenario="Temporary wrappers trigger Sleight only through unblocked internal Strength";
 {auto s=fight();cast(s,NecroSleightOfFlesh);cast(s,NecroEnfeeblingTouch);CHECK(s.enemies[0].hp==991);cast(s,NecroMisery);CHECK(s.enemies[1].hp==991);}
 {auto s=fight();cast(s,NecroSleightOfFlesh);s.silent.enemy_artifact[0]=2;cast(s,NecroEnfeeblingTouch);
  CHECK(s.enemies[0].hp==1000&&!s.enemies[0].strength&&!s.expansion.restore_strength[0]&&s.silent.enemy_artifact[0]==1);
 }
 scenario="Strength keeps its original power position while crossing from positive to negative";
 {auto s=fight();cast(s,FightMe);apply_weak(s,0,1);resolve_pending(s);settle(s);cast(s,NecroSharedFate);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(!s.enemies[1].strength&&s.enemies[1].weak==1&&!s.silent.enemy_artifact[1]);
 }
 scenario="Conqueror is a real acquired Debuff for Misery and SleightOfFlesh";
 {auto s=fight();cast(s,NecroSleightOfFlesh);cast(s,RegentConqueror);CHECK(s.enemies[0].hp==991);
  apply_weak(s,0,1);resolve_pending(s);settle(s);s.silent.enemy_artifact[1]=1;cast(s,NecroMisery);
  CHECK(!s.regent.conqueror[1]&&s.enemies[1].weak==1&&s.enemies[1].hp==991&&!s.silent.enemy_artifact[1]);restore(s);
 }
}

static void damage_listener_order(){
 scenario="AfterDamageGiven powers complete in cross-character acquisition order";
 const CardKind powers[]={Envenom,NecroReaperForm,RegentMonarchsGaze};
 const unsigned orders[][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 for(const auto&order:orders){
  auto s=fight();for(unsigned i:order)cast(s,powers[i]);s.silent.enemy_artifact[0]=1;restore(s);cast(s,NecroStrikeNecrobinder);
  CHECK(s.enemies[0].poison==(order[0]==0?0:1)&&s.expansion.doom[0]==(order[0]==1?0:6));
  CHECK(s.enemies[0].strength==(order[0]==2?0:-1)&&s.expansion.restore_strength[0]==(order[0]==2?0:1)&&!s.silent.enemy_artifact[0]);
  CHECK(s.regent.hits_turn[0]==1);restore(s);
 }
 scenario="Osty qualifies for ReaperForm's pet exception, but not Envenom or MonarchsGaze";
 {auto s=fight();for(CardKind p:powers)cast(s,p);cast(s,NecroPoke);
  CHECK(s.enemies[0].hp==994&&s.expansion.doom[0]==6&&!s.enemies[0].poison&&!s.enemies[0].strength&&!s.expansion.restore_strength[0]);
  CHECK(!s.regent.hits_turn[0]);restore(s);
 }
 scenario="Blocked player hits count for BeatIntoShape and skip Envenom's Artifact attempt";
 {auto s=fight();for(CardKind p:powers)cast(s,p);s.enemies[0].block=10;s.silent.enemy_artifact[0]=1;cast(s,NecroStrikeNecrobinder);
  CHECK(s.enemies[0].hp==1000&&s.enemies[0].block==4&&s.regent.hits_turn[0]==1);
  CHECK(!s.enemies[0].poison&&!s.expansion.doom[0]&&s.enemies[0].strength==-1&&s.expansion.restore_strength[0]==1&&!s.silent.enemy_artifact[0]);restore(s);
 }
}

static void turns_draws_and_souls(){
 scenario="Demesne and Friendship raise reset energy; Delay and Invoke add only next turn";
 {auto s=fight();fill(s,Zone::Draw,20);cast(s,NecroDemesne,1);cast(s,NecroFriendship);cast(s,NecroDelay,1);cast(s,NecroInvoke);
  quiet_turn(s);CHECK(s.energy==9&&s.hand.count==6&&s.necro.drawn_turn==6&&s.strength==-2);
  CHECK(!power(s,NPower::EnergyNextTurn)&&!power(s,NPower::SummonNextTurn));
  quiet_turn(s);CHECK(s.energy==5&&s.hand.count==6);
 }
 scenario="Countdown and Neurosurge run after hand draw, each feeding Shroud";
 {auto s=fight();fill(s,Zone::Draw,20);cast(s,NecroCountdown);cast(s,NecroNeurosurge);cast(s,NecroShroud);
  quiet_turn(s);CHECK(s.necro.drawn_turn==5&&s.necro.player_doom==3&&s.block==6&&s.expansion.doom[0]+s.expansion.doom[1]==6);
 }
 scenario="CallOfTheVoid and SentryMode create cards before ordinary hand draw";
 {auto s=fight();fill(s,Zone::Draw,20);install(s,NPower::CallOfTheVoid,2);install(s,NPower::SentryMode,1);int before=s.card_count;
  quiet_turn(s);CHECK(s.hand.count==8&&s.necro.drawn_turn==5&&s.card_count==before+3);
  int random=0,gaze=0;for(int id=before;id<s.card_count;id++){
   if(s.cards[id].kind==SweepingGaze){++gaze;CHECK(card_exhausts(s.cards[id]));}
   else{++random;CHECK(s.necro.ethereal[id]&&card_in_pool(s.cards[id].kind,Character::Necrobinder)&&card_can_generate(s.cards[id].kind));}
  }CHECK(random==2&&gaze==1);
 }
 scenario="Pagestorm observes actual draws and safely recurses through Ethereal draws";
 {auto s=fight();cast(s,NecroPagestorm);fill(s,Zone::Draw,3);append(s,{NecroDefile,0},Zone::Draw);append(s,{NecroDefy,0},Zone::Draw);
  draw_cards(s,1);settle(s);CHECK(s.hand.count==3&&s.necro.drawn_turn==3&&s.draw.count==2);
  s.no_draw=1;draw_cards(s,5);settle(s);CHECK(s.hand.count==3&&s.necro.drawn_turn==3);
 }
 scenario="Soul triggers DevourLife and unblocked Haunt independently of attack modifiers";
 {auto s=fight();fill(s,Zone::Draw,12);cast(s,NecroDevourLife,1);cast(s,NecroHaunt);s.strength=11;s.vigor=9;
  s.enemies[0].block=s.enemies[1].block=40;cast(s,Soul);CHECK(s.necro.drawn_turn==2&&s.expansion.osty_hp==12&&s.expansion.osty_max_hp==16);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==1993&&s.enemies[0].block==40&&s.enemies[1].block==40&&s.vigor==9);
 }
 scenario="Severance creates Souls in Draw, Discard, then Hand without disturbing old draw order";
 {auto s=fight();fill(s,Zone::Draw,4);uint16_t old[4];for(int i=0;i<4;i++)old[i]=s.draw.ids[i];int before=s.card_count;cast(s,NecroSeverance);
  CHECK(s.cards[before+1].kind==Soul&&s.zones[before+1]==Zone::Draw&&s.zones[before+2]==Zone::Discard&&s.zones[before+3]==Zone::Hand);
  int n=0;for(unsigned i=0;i<s.draw.count;i++)if(s.cards[s.draw.ids[i]].kind!=Soul)CHECK(s.draw.ids[i]==old[n++]);CHECK(n==4);
 }
 scenario="BorrowedTime expires at the side boundary, and Veilpiercer consumes on zero-cost Ethereal";
 {auto s=fight();int wisp=append(s,{NecroWisp,0},Zone::Hand);s.necro.ethereal[wisp]=1;cast(s,NecroBorrowedTime);CHECK(cost(s,wisp)==1);
  cast(s,NecroVeilpiercer);CHECK(cost(s,wisp)==0);repeat_instance(s,wisp);CHECK(!power(s,NPower::Veilpiercer));quiet_turn(s);CHECK(!power(s,NPower::BorrowedTime));
 }
}

static void physical_cards_and_choices(){
 scenario="Fetch draws once per physical instance each turn even with replays";
 {auto s=fight();fill(s,Zone::Draw,20);int fetch=append(s,{NecroFetch,0},Zone::Hand);s.expansion.replay[fetch]=1;repeat_instance(s,fetch);
  CHECK(s.necro.drawn_turn==1&&s.enemies[0].hp==994);repeat_instance(s,fetch);CHECK(s.necro.drawn_turn==1&&s.enemies[0].hp==988);
  quiet_turn(s);unsigned drawn=s.necro.drawn_turn;repeat_instance(s,fetch);CHECK(s.necro.drawn_turn==drawn+1);
 }
 scenario="Flatten zero lasts through its own physical plays, then expires at side end";
 {auto s=fight();int flat=append(s,{NecroFlatten,0},Zone::Hand);CHECK(cost(s,flat)==2);cast(s,NecroPoke);CHECK(cost(s,flat)==0);
  s.energy=0;repeat_instance(s,flat);CHECK(cost(s,flat)==0);repeat_instance(s,flat);CHECK(cost(s,flat)==0&&s.enemies[0].hp==970);
  quiet_turn(s);CHECK(cost(s,flat)==2&&!s.necro.flatten_zero[flat]);
 }
 scenario="A later Flatten Set hides a Transfigure addition, which earlier Set does not hide";
 {auto s=fight();int flat=append(s,{NecroFlatten,0},Zone::Hand);cast(s,NecroPoke);int trans=append(s,{NecroTransfigure,0},Zone::Hand);
  CHECK(play(s,hand_index(s,trans),0));CHECK(s.phase==Phase::Combat&&cost(s,flat)==1&&s.expansion.replay[flat]==1);restore(s);
  cast(s,NecroPoke);CHECK(cost(s,flat)==0);repeat_instance(s,flat);CHECK(cost(s,flat)==0);
 }
 scenario="BansheesCry includes prior finished Ethereal plays when newly generated";
 {auto s=fight();int cry=append(s,{NecroBansheesCry,0},Zone::Hand);cast(s,NecroDefile);cast(s,NecroDefy);CHECK(s.necro.ethereal_played==2&&cost(s,cry)==5);
  int fresh=generated(s,{NecroBansheesCry,0});CHECK(cost(s,fresh)==5&&s.necro.cost_delta[fresh]==-4);
 }
 scenario="Melancholy reduces only existing physical instances on each actual death";
 {auto s=fight();int old=append(s,{NecroMelancholy,0},Zone::Hand);cast(s,NecroSacrifice);CHECK(cost(s,old)==2);
  int fresh=generated(s,{NecroMelancholy,0});CHECK(cost(s,fresh)==3);s.enemies[0].hp=6;cast(s,NecroStrikeNecrobinder);
  CHECK(cost(s,old)==1&&cost(s,fresh)==2&&s.necro.creature_deaths==2);cast(s,NecroBodyguard);CHECK(cost(s,old)==1);
 }
 scenario="Transfigure replays X-cost cards without adding an ordinary energy cost";
 {auto s=fight();int attack=append(s,{NecroEradicate,0},Zone::Hand),trans=append(s,{NecroTransfigure,0},Zone::Hand);
  CHECK(play(s,hand_index(s,trans),0));CHECK(s.phase==Phase::Combat&&cost(s,attack)==-1&&!s.necro.cost_delta[attack]&&s.expansion.replay[attack]==1);restore(s);
  s.energy=2;repeat_instance(s,attack);CHECK(s.enemies[0].hp==956&&!s.energy);
 }
 scenario="Undeath copies combat keywords and additive costs, without physical history";
 {auto s=fight();int id=append(s,{NecroUndeath,1},Zone::Hand);s.necro.ethereal[id]=1;s.necro.cost_delta[id]=2;s.silent.retain[id]=1;s.silent.sly[id]=1;
  repeat_instance(s,id);CHECK(s.block==9&&s.discard.count==2);int clone=s.discard.ids[0]==id?s.discard.ids[1]:s.discard.ids[0];
  CHECK(s.cards[clone].kind==NecroUndeath&&s.cards[clone].upgraded&&s.necro.ethereal[clone]&&s.necro.cost_delta[clone]==2&&s.silent.retain[clone]==1&&s.silent.sly[clone]==1&&!s.necro.deck_origin[clone]);
 }
 scenario="Seance transforms even Eternal combat cards into a fresh base Soul in the same position";
 {auto s=fight();fill(s,Zone::Draw,2);int target=append(s,{NecroForbiddenGrimoire,1},Zone::Draw);fill(s,Zone::Draw,2);
  s.necro.ethereal[target]=1;s.necro.cost_delta[target]=2;s.silent.retain[target]=1;s.expansion.combat_cost[target]=3;s.expansion.replay[target]=2;
  int seance=append(s,{NecroSeance,0},Zone::Hand);CHECK(play(s,hand_index(s,seance),0));choose(s,target);
  CHECK(s.draw.count==5&&s.draw.ids[2]==target&&s.cards[target].kind==Soul&&!s.cards[target].upgraded&&!s.necro.ethereal[target]&&!s.necro.cost_delta[target]&&!s.silent.retain[target]&&!s.expansion.combat_cost[target]&&!s.expansion.replay[target]);
 }
 scenario="Dredge requests only the remaining hand capacity and validates a whole selection";
 {auto s=fight();fill(s,Zone::Hand,8);fill(s,Zone::Discard,4);int dredge=append(s,{NecroDredge,0},Zone::Hand);CHECK(play(s,hand_index(s,dredge),0));
  CHECK(s.phase==Phase::ExtendedChoice&&choice_min(s)==2);restore(s);auto copy=s;uint16_t wrong[2]={s.discard.ids[0],s.discard.ids[0]};CHECK(!choose_extended(s,wrong,2));CHECK(!std::memcmp(&s,&copy,sizeof s));
  uint16_t selected[2]={s.discard.ids[1],s.discard.ids[3]};CHECK(choose_extended(s,selected,2));settle(s);CHECK(s.hand.count==10&&s.discard.count==2);
 }
 scenario="RightHandHand returns in actual discard order under a full hand";
 {auto s=fight();int first=append(s,{NecroRightHandHand,0},Zone::Discard),second=append(s,{NecroRightHandHand,1},Zone::Discard);
  s.discard.ids[0]=(uint16_t)second;s.discard.ids[1]=(uint16_t)first;fill(s,Zone::Hand,9);cast(s,NecroBury);
  CHECK(s.zones[second]==Zone::Hand&&s.zones[first]==Zone::Discard&&s.hand.count==10);
 }
 scenario="Nightmare snapshots BansheesCry costs without retroactively subtracting history twice";
 {auto s=fight();fill(s,Zone::Draw,12);int cry=append(s,{NecroBansheesCry,0},Zone::Hand);cast(s,NecroDefile);cast(s,NecroDefy);
  int nightmare=append(s,{Nightmare,0},Zone::Hand);CHECK(play(s,hand_index(s,nightmare),0));CHECK(s.phase==Phase::Combat);restore(s);quiet_turn(s);
  int count=0;for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroBansheesCry){++count;CHECK(cost(s,id)==5);}CHECK(count==4);
 }
 scenario="Transfigure composes with RocketPunch local reductions before the final zero clamp";
 {auto s=fight();int rocket=append(s,{DefectRocketPunch,0},Zone::Hand);cast(s,DefectTurbo);cast(s,DefectTurbo);cast(s,DefectTurbo);
  CHECK(s.defect.cost_delta[rocket]==-3&&cost(s,rocket)==0);cast(s,NecroTransfigure);
  CHECK(s.necro.cost_delta[rocket]==1&&cost(s,rocket)==0&&s.expansion.replay[rocket]==1);restore(s);
 }
 scenario="Global next-card free powers apply after Transfigure local cost additions";
 const CardKind free_sources[]={Unrelenting,Pounce,DefectSynthesis};
 const CardKind free_targets[]={NecroPoke,NecroDefendNecrobinder,NecroShroud};
 const int transfigured_prices[]={1,2,2};
 for(unsigned i=0;i<3;i++){
  auto s=fight();int target=append(s,{free_targets[i],0},Zone::Hand);cast(s,NecroTransfigure);CHECK(cost(s,target)==transfigured_prices[i]);
  cast(s,free_sources[i]);CHECK(cost(s,target)==0);restore(s);
 }
 {auto s=fight();s.expansion.stars=99;int target=append(s,{NecroDefendNecrobinder,0},Zone::Hand);cast(s,NecroTransfigure);cast(s,RegentVoidForm);CHECK(cost(s,target)==0);restore(s);}
}

static void eidolon_and_persistence(){
 scenario="Eidolon excludes unplayable Ethereal cards from its captured candidates";
 {auto s=fight();int dazed=append(s,{Dazed,0},Zone::Exhaust),void_card=append(s,{Void,0},Zone::Exhaust);int energy=s.energy;
  cast(s,NecroEidolon);CHECK(s.zones[dazed]==Zone::Exhaust&&s.zones[void_card]==Zone::Exhaust);
  CHECK(s.exhaust.count==3&&s.expansion.cards_finished==1&&!s.necro.ethereal_played&&!s.necro.drawn_turn&&s.energy==energy-2);restore(s);
 }
 scenario="Eidolon captures identities without prematurely removing later Souls";
 {auto s=fight();fill(s,Zone::Draw,12);int storm=append(s,{NecroSoulStorm,0},Zone::Exhaust),a=append(s,{Soul,0},Zone::Exhaust),b=append(s,{Soul,0},Zone::Exhaust);
  s.necro.ethereal[storm]=s.necro.ethereal[a]=s.necro.ethereal[b]=1;cast(s,NecroEidolon);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==1983&&s.necro.drawn_turn==4&&s.necro.ethereal_played==3&&s.exhaust.count==3&&s.zones[storm]==Zone::Discard);
 }
 scenario="Eidolon nested selection survives save/load and resumes captured caller frames";
 {auto s=fight();fill(s,Zone::Draw,6);int target=append(s,{NecroDefendNecrobinder,0},Zone::Hand),strike=append(s,{NecroSculptingStrike,0},Zone::Exhaust),soul=append(s,{Soul,0},Zone::Exhaust);
  s.necro.ethereal[strike]=s.necro.ethereal[soul]=1;append(s,{NecroDefendNecrobinder,0},Zone::Hand);int eidolon=append(s,{NecroEidolon,0},Zone::Hand);CHECK(play(s,hand_index(s,eidolon),0));
  CHECK(s.phase==Phase::ExtendedChoice&&s.playing==strike&&s.expansion.depth==1);restore(s);choose(s,target);
  CHECK(s.necro.ethereal[target]&&s.necro.drawn_turn==2&&s.zones[eidolon]==Zone::Exhaust&&s.playing==-1&&!s.expansion.depth);
 }
 scenario="An Eidolon automatic card preserves resolved EnergyValue while spending no energy";
 {auto s=fight();cast(s,NecroDanseMacabre);int right=append(s,{NecroRightHandHand,0},Zone::Discard),bury=append(s,{NecroBury,0},Zone::Exhaust);
  s.necro.ethereal[bury]=1;int before=s.energy;cast(s,NecroEidolon);CHECK(s.energy==before-2&&s.block==8&&s.zones[right]==Zone::Hand);
 }
 scenario="A lethal first Eidolon play leaves all unplayed captured cards in Exhaust";
 {auto s=fight();s.enemy_count=1;s.enemies[0].hp=10;int storm=append(s,{NecroSoulStorm,0},Zone::Exhaust),soul=append(s,{Soul,0},Zone::Exhaust);
  s.necro.ethereal[storm]=s.necro.ethereal[soul]=1;int eidolon=cast(s,NecroEidolon);
  CHECK(s.phase==Phase::Reward&&s.zones[soul]==Zone::Exhaust&&s.zones[eidolon]==Zone::Exhaust&&!s.necro.drawn_turn&&!s.expansion.depth);restore(s);
 }
 scenario="TheScythe binds growth to its permanent deck copy across combat resets";
 {auto s=fight();s.deck_count=2;s.deck[0]={NecroDefendNecrobinder,0};s.deck[1]={NecroTheScythe,1};s.necro.scythe_deck[1]=7;enter_combat(s);
  s.enemies[0].hp=s.enemies[0].max_hp=1000;s.energy=30;int scythe=-1;for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroTheScythe)scythe=(int)id;
  CHECK(scythe>=0&&s.necro.deck_origin[scythe]==2&&damage(s,s.cards[scythe],0,scythe)==20);repeat_instance(s,scythe);
  CHECK(s.necro.scythe_deck[1]==14&&s.necro.scythe_value[scythe]==14);enter_combat(s);
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==NecroTheScythe){CHECK(s.necro.deck_origin[id]==2&&s.necro.scythe_value[id]==14&&damage(s,s.cards[id],0,id)==27);}restore(s);
 }
 scenario="Grimoire removal updates both characters' permanent metadata and rejects Eternal";
 {auto s=fight();s.deck_count=4;s.deck[0]={NecroDefendNecrobinder,0};s.deck[1]={NecroTheScythe,0};s.deck[2]={NecroForbiddenGrimoire,0};s.deck[3]={DefectGeneticAlgorithm,0};
  s.necro.scythe_deck[1]=12;s.defect.genetic_deck[3]=8;int scythe=append(s,{NecroTheScythe,0},Zone::Exhaust);s.necro.scythe_value[scythe]=12;s.necro.deck_origin[scythe]=2;
  s.phase=Phase::Reward;s.necro.grimoire_pending=2;restore(s);CHECK(necro_grimoire_choice(s,0));CHECK(s.deck_count==3&&s.necro.scythe_deck[0]==12&&s.necro.deck_origin[scythe]==1&&s.defect.genetic_deck[2]==8&&s.necro.grimoire_pending==1);
  auto copy=s;CHECK(!necro_grimoire_choice(s,1)&&!necro_grimoire_choice(s,-2));CHECK(!std::memcmp(&s,&copy,sizeof s));CHECK(necro_grimoire_choice(s,-1)&&!s.necro.grimoire_pending);restore(s);
 }
 scenario="ForbiddenGrimoire earns one optional removal per stack after combat";
 {auto s=fight();cast(s,NecroForbiddenGrimoire);cast(s,NecroForbiddenGrimoire,1);s.enemies[0].hp=s.enemies[1].hp=1;cast(s,NecroSow);
  CHECK(s.phase==Phase::Reward&&s.necro.grimoire_pending==2);CHECK(necro_grimoire_choice(s,-1)&&s.necro.grimoire_pending==1);CHECK(necro_grimoire_choice(s,-1)&&!s.necro.grimoire_pending);restore(s);
 }
}

static void character_start_and_validation(){
 scenario="Pinned Necrobinder starting deck, HP and BoundPhylactery";
 {State s{};new_run(s,291,false,Character::Necrobinder);CHECK(s.hp==66&&s.max_hp==66&&s.deck_count==10&&(s.relics&BoundPhylactery));
  int strike=0,defend=0,body=0,unleash=0;for(unsigned i=0;i<s.deck_count;i++){strike+=s.deck[i].kind==NecroStrikeNecrobinder;defend+=s.deck[i].kind==NecroDefendNecrobinder;body+=s.deck[i].kind==NecroBodyguard;unleash+=s.deck[i].kind==NecroUnleash;}
  CHECK(strike==4&&defend==4&&body==1&&unleash==1);enter_combat(s);CHECK(s.expansion.osty_hp==1&&s.expansion.osty_max_hp==1&&s.hand.count==5);
  for(auto&e:s.enemies)e.move=2;quiet_turn(s);CHECK(s.expansion.osty_hp==2&&s.expansion.osty_max_hp==2);restore(s);
 }
 scenario="Reward and generation filters exclude Ancient and explicitly disabled models";
 {auto s=fight();CHECK(!card_can_generate(NecroForbiddenGrimoire)&&!card_can_generate(NecroProtector)&&!card_can_generate(NecroEidolon)&&!card_can_generate(NecroTransfigure));
  for(int i=0;i<256;i++){unsigned k=random_reward_card(s);CHECK(card_in_pool(k,Character::Necrobinder)&&card_rarity(k)>=2&&card_rarity(k)<=4);}
 }
 scenario="Live descriptions use dynamic pet HP, Doom, draws, and permanent Scythe growth";
 {auto s=fight();int out=-1;s.necro.drawn_turn=3;s.expansion.doom[0]=27;
  CHECK(character_description_value(s,{NecroUnleash,1},0,-1,"OstyDamage",out)&&out==19);
  CHECK(character_description_value(s,{NecroSacrifice,0},0,-1,"CalculatedBlock",out)&&out==42);
  CHECK(character_description_value(s,{NecroDeathMarch,0},0,-1,"CalculatedDamage",out)&&out==20);
  CHECK(character_description_value(s,{NecroNoEscape,1},0,-1,"CalculatedDoom",out)&&out==25);
  CHECK(character_description_value(s,{NecroTimesUp,0},0,-1,"CalculatedDamage",out)&&out==27);
 }
 scenario="Malformed keywords, persistent mappings, power order and continuation operands reject";
 {auto s=fight();int id=append(s,{NecroPoke,0},Zone::Hand);restore(s);auto bad=s;bad.necro.ethereal[id]=2;CHECK(!validate(bad));
  bad=s;bad.necro.fetch_played[id]=1;CHECK(!validate(bad));bad=s;bad.necro.flatten_zero[id]=1;CHECK(!validate(bad));
  bad=s;bad.necro.scythe_deck[0]=1;CHECK(!validate(bad));bad=s;bad.necro.powers[(unsigned)NPower::Haunt]=1;CHECK(!validate(bad));
  bad=s;bad.necro.deck_origin[id]=1;CHECK(!validate(bad));bad=s;bad.necro.debuff_count[0]=1;CHECK(!validate(bad));
  bad=s;bad.necro.play_index[MaxAutoFrames]=1;CHECK(!validate(bad));
  bad=s;queue_extension(bad,XOp::NScytheGrow,id,5);CHECK(!validate(bad));
  bad=s;queue_extension(bad,XOp::NDoom,MaxEnemies,3);CHECK(!validate(bad));
 }
}
int main(){
 complete_model_matrix();osty_damage_and_life();doom_and_debuffs();temporary_strength_and_misery();damage_listener_order();turns_draws_and_souls();physical_cards_and_choices();eidolon_and_persistence();character_start_and_validation();
 std::printf("Necrobinder completion: %u canonical variants, %u checks passed\n",variants,checks);return 0;
}
