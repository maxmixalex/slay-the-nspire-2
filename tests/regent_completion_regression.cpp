#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static unsigned checks,variants,active_kind=RegentAlignment,active_upgrade;
static const char* scenario="initial";
#define CHECK(x) do { ++checks; if(!(x)) { std::fprintf(stderr,"FAIL %s:%d [%s %s +%u] %s\n",__FILE__,__LINE__,scenario,definitions[active_kind].key,active_upgrade,#x); std::exit(1); } } while(0)

// Independent numeric facts from the locked reference audit; these values
// are not read from runtime Definition records. Per-card outcomes follow below.
struct Canonical {CardKind id;const char* model;int8_t base_cost,up_cost,stars;uint8_t base_keywords,up_keywords;CardType type;};
static const Canonical canonical[]={
 {RegentAlignment,"Alignment",0,0,2,0,0,CardType::Skill},
 {RegentArsenal,"Arsenal",1,1,0,0,2,CardType::Power},
 {RegentAstralPulse,"AstralPulse",0,0,3,0,0,CardType::Attack},
 {RegentBeatIntoShape,"BeatIntoShape",1,1,0,0,0,CardType::Attack},
 {RegentBegone,"Begone",1,1,0,0,0,CardType::Skill},
 {RegentBigBang,"BigBang",0,0,0,1,3,CardType::Skill},
 {RegentBlackHole,"BlackHole",1,1,0,0,0,CardType::Power},
 {RegentBombardment,"Bombardment",3,3,0,1,1,CardType::Attack},
 {RegentBulwark,"Bulwark",2,2,0,0,0,CardType::Skill},
 {RegentBundleOfJoy,"BundleOfJoy",1,1,0,1,1,CardType::Skill},
 {RegentCelestialMight,"CelestialMight",2,2,0,0,0,CardType::Attack},
 {RegentCharge,"Charge",1,1,0,0,0,CardType::Skill},
 {RegentChildOfTheStars,"ChildOfTheStars",1,1,0,0,0,CardType::Power},
 {RegentCloakOfStars,"CloakOfStars",0,0,1,0,0,CardType::Skill},
 {RegentCollisionCourse,"CollisionCourse",0,0,0,0,0,CardType::Attack},
 {RegentComet,"Comet",0,0,5,0,0,CardType::Attack},
 {RegentConqueror,"Conqueror",1,1,0,0,0,CardType::Skill},
 {RegentConvergence,"Convergence",1,1,0,0,0,CardType::Skill},
 {RegentCosmicIndifference,"CosmicIndifference",1,1,0,0,0,CardType::Skill},
 {RegentCrashLanding,"CrashLanding",1,1,0,0,0,CardType::Attack},
 {RegentCrescentSpear,"CrescentSpear",1,1,1,0,0,CardType::Attack},
 {RegentCrushUnder,"CrushUnder",1,1,0,0,0,CardType::Attack},
 {RegentDecisionsDecisions,"DecisionsDecisions",0,0,6,1,1,CardType::Skill},
 {RegentDefend,"DefendRegent",1,1,0,0,0,CardType::Skill},
 {RegentDevastate,"Devastate",1,1,4,0,0,CardType::Attack},
 {RegentDyingStar,"DyingStar",1,1,3,8,8,CardType::Attack},
 {RegentFallingStar,"FallingStar",0,0,2,0,0,CardType::Attack},
 {RegentForegoneConclusion,"ForegoneConclusion",1,1,0,0,0,CardType::Skill},
 {RegentFurnace,"Furnace",1,1,0,0,0,CardType::Power},
 {RegentGammaBlast,"GammaBlast",0,0,3,0,0,CardType::Attack},
 {RegentGatherLight,"GatherLight",1,1,0,0,0,CardType::Skill},
 {RegentGenesis,"Genesis",2,2,0,0,0,CardType::Power},
 {RegentGlimmer,"Glimmer",1,1,0,0,0,CardType::Skill},
 {RegentGlitterstream,"Glitterstream",2,2,0,0,0,CardType::Skill},
 {RegentGlow,"Glow",1,1,0,0,0,CardType::Skill},
 {RegentGuards,"Guards",2,2,0,1,1,CardType::Skill},
 {RegentGuidingStar,"GuidingStar",1,1,1,0,0,CardType::Attack},
 {RegentHeavenlyDrill,"HeavenlyDrill",-1,-1,0,0,0,CardType::Attack},
 {RegentHegemony,"Hegemony",2,2,0,0,0,CardType::Attack},
 {RegentHeirloomHammer,"HeirloomHammer",2,2,0,0,0,CardType::Attack},
 {RegentHiddenCache,"HiddenCache",1,1,0,0,0,CardType::Skill},
 {RegentIAmInvincible,"IAmInvincible",1,1,0,0,0,CardType::Skill},
 {RegentKinglyKick,"KinglyKick",4,4,0,0,0,CardType::Attack},
 {RegentKinglyPunch,"KinglyPunch",1,1,0,0,0,CardType::Attack},
 {RegentKnockoutBlow,"KnockoutBlow",3,3,0,0,0,CardType::Attack},
 {RegentKnowThyPlace,"KnowThyPlace",0,0,0,1,0,CardType::Skill},
 {RegentLunarBlast,"LunarBlast",0,0,0,0,0,CardType::Attack},
 {RegentMakeItSo,"MakeItSo",0,0,0,0,0,CardType::Attack},
 {RegentManifestAuthority,"ManifestAuthority",1,1,0,0,0,CardType::Skill},
 {RegentMeteorShower,"MeteorShower",0,0,2,0,0,CardType::Attack},
 {RegentMonarchsGaze,"MonarchsGaze",2,1,0,0,0,CardType::Power},
 {RegentMonologue,"Monologue",0,0,0,0,4,CardType::Skill},
 {RegentNeutronAegis,"NeutronAegis",1,1,5,0,0,CardType::Power},
 {RegentOrbit,"Orbit",2,1,0,0,0,CardType::Power},
 {RegentPaleBlueDot,"PaleBlueDot",1,1,0,0,0,CardType::Power},
 {RegentParry,"Parry",1,1,0,0,0,CardType::Power},
 {RegentParticleWall,"ParticleWall",0,0,2,0,0,CardType::Skill},
 {RegentPatter,"Patter",1,1,0,0,0,CardType::Skill},
 {RegentPhotonCut,"PhotonCut",1,1,0,0,0,CardType::Attack},
 {RegentPillarOfCreation,"PillarOfCreation",1,1,0,0,0,CardType::Power},
 {RegentProphesize,"Prophesize",2,2,0,0,0,CardType::Skill},
 {RegentQuasar,"Quasar",0,0,2,0,0,CardType::Skill},
 {RegentRadiate,"Radiate",0,0,0,0,0,CardType::Attack},
 {RegentRefineBlade,"RefineBlade",1,1,0,0,0,CardType::Skill},
 {RegentReflect,"Reflect",1,1,3,0,0,CardType::Skill},
 {RegentResonance,"Resonance",1,1,2,0,0,CardType::Skill},
 {RegentRoyalGamble,"RoyalGamble",0,0,5,1,5,CardType::Skill},
 {RegentRoyalties,"Royalties",1,1,0,0,0,CardType::Power},
 {RegentSeekingEdge,"SeekingEdge",1,1,0,0,0,CardType::Power},
 {RegentSevenStars,"SevenStars",2,1,7,0,0,CardType::Attack},
 {RegentShiningStrike,"ShiningStrike",1,1,0,0,0,CardType::Attack},
 {RegentSolarStrike,"SolarStrike",1,1,0,0,0,CardType::Attack},
 {RegentSpectrumShift,"SpectrumShift",2,1,0,0,0,CardType::Power},
 {RegentSpoilsOfBattle,"SpoilsOfBattle",1,1,0,0,0,CardType::Skill},
 {RegentStardust,"Stardust",0,0,-1,0,0,CardType::Attack},
 {RegentStrike,"StrikeRegent",1,1,0,0,0,CardType::Attack},
 {RegentSummonForth,"SummonForth",1,1,0,0,0,CardType::Skill},
 {RegentSupermassive,"Supermassive",1,1,0,0,0,CardType::Attack},
 {RegentSwordSage,"SwordSage",2,1,0,0,0,CardType::Power},
 {RegentTerraforming,"Terraforming",1,1,0,0,0,CardType::Skill},
 {RegentTheSealedThrone,"TheSealedThrone",1,0,3,0,0,CardType::Power},
 {RegentTheSmith,"TheSmith",1,1,4,0,0,CardType::Skill},
 {RegentTyranny,"Tyranny",1,1,0,0,2,CardType::Power},
 {RegentVenerate,"Venerate",1,1,0,0,0,CardType::Skill},
 {RegentVoidForm,"VoidForm",3,3,0,8,0,CardType::Power},
 {RegentWroughtInWar,"WroughtInWar",1,1,0,0,0,CardType::Attack},
 {MinionStrike,"MinionStrike",0,0,0,1,1,CardType::Attack},
 {MinionDiveBomb,"MinionDiveBomb",0,0,0,1,1,CardType::Attack},
 {MinionSacrifice,"MinionSacrifice",0,0,0,1,1,CardType::Skill},
 {Debris,"Debris",1,1,0,1,1,CardType::Status},
 {SovereignBlade,"SovereignBlade",2,1,0,4,4,CardType::Attack},
};

static int add(State&s,unsigned kind,unsigned up=0,Zone zone=Zone::Hand){
 CHECK(s.card_count<MaxCards);int id=s.card_count++;s.cards[id]={(CardKind)kind,(uint8_t)up};s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||p.count<MaxHand);p.ids[p.count++]=(uint16_t)id;return id;
}
static State fight(unsigned kind,unsigned up=0){
 State s{};new_run(s,0x41cef1ea,false,Character::Regent);s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=100;s.hp=s.max_hp=75;s.expansion.stars=100;
 s.deck_count=1;s.deck[0]={(CardKind)kind,(uint8_t)up};for(auto&e:s.enemies)e={120,120,0,0,0,2};add(s,kind,up);return s;
}
static void supply(State&s,int n,unsigned kind=RegentDefend,Zone zone=Zone::Draw){for(int j=0;j<n;j++)add(s,kind,0,zone);}
static bool contains(const Pile&p,int id){for(unsigned j=0;j<p.count;j++)if(p.ids[j]==id)return true;return false;}
static int count_kind(const State&s,unsigned kind,Zone zone){int n=0;for(unsigned j=0;j<s.card_count;j++)if(s.cards[j].kind==kind&&s.zones[j]==zone)++n;return n;}
static int find_kind(const State&s,unsigned kind){for(unsigned j=0;j<s.card_count;j++)if(s.cards[j].kind==kind)return (int)j;return -1;}
static int hp(const State&s){int n=0;for(unsigned j=0;j<s.enemy_count;j++)n+=s.enemies[j].hp;return n;}
static int power(const State&s,RPower p){return s.regent.powers[(int)p];}
static SaveData snapshot(const State&s){SaveData out{};CHECK(validate(s));CHECK(save(s,out));return out;}
static void same(const State&s,const SaveData&before){auto now=snapshot(s);CHECK(now.size==before.size&&!std::memcmp(now.bytes,before.bytes,now.size));}
static void roundtrip(State&s){auto before=snapshot(s);State copy{};CHECK(load(copy,before.bytes,before.size));same(copy,before);s=copy;}
static void ready(State&s){CHECK((s.phase==Phase::Combat||s.phase==Phase::Reward)&&s.playing<0&&!s.effect_count&&!s.expansion.depth);roundtrip(s);}
static void act(State&s,int id=0,int target=0){int at=-1;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==id)at=(int)j;CHECK(at>=0);CHECK(play(s,at,target));CHECK(s.phase!=Phase::Error&&s.phase!=Phase::Dead);}
static void pick(State&s,unsigned n,unsigned offset=0){
 CHECK(s.phase==Phase::ExtendedChoice&&n<=MaxHand&&n+offset<=s.expansion.choices.count);roundtrip(s);auto before=snapshot(s);
 uint16_t ids[MaxHand]{};for(unsigned j=0;j<n;j++)ids[j]=s.expansion.choices.ids[offset+j];uint16_t wrong=65535;
 CHECK(!choose_extended(s,&wrong,1));same(s,before);
 if(n>1){uint16_t duplicate[MaxHand]{};for(unsigned j=0;j<n;j++)duplicate[j]=ids[0];CHECK(!choose_extended(s,duplicate,n));same(s,before);}
 State restored{};CHECK(load(restored,before.bytes,before.size));CHECK(choose_extended(s,ids,n));CHECK(choose_extended(restored,ids,n));same(restored,snapshot(s));
}
static void settle(State&s){
 for(unsigned n=0;choice_phase(s.phase)&&n<64;n++){
  if(s.phase==Phase::ExtendedChoice)pick(s,choice_min(s));
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else{uint16_t selected[MaxHand];unsigned count=choice_min(s);for(unsigned j=0;j<count;j++)selected[j]=s.hand.ids[j];CHECK(choose_discard(s,selected,count));}
 }
 CHECK(!choice_phase(s.phase));ready(s);
}
static int cast(State&s,unsigned kind,unsigned up=0,int target=0){int id=add(s,kind,up);act(s,id,target);settle(s);return id;}
static void next_turn(State&s){for(auto&e:s.enemies)e.move=2;CHECK(end_turn(s));settle(s);CHECK(s.phase==Phase::Combat);}
static void transfer(State&s,int id,Zone zone){
 Zone old=s.zones[id];Pile&from=old==Zone::Hand?s.hand:old==Zone::Draw?s.draw:old==Zone::Discard?s.discard:old==Zone::Exhaust?s.exhaust:s.powers;
 unsigned at=0;while(at<from.count&&from.ids[at]!=id)++at;CHECK(at<from.count);
 for(unsigned j=at+1;j<from.count;j++)from.ids[j-1]=from.ids[j];--from.count;
 Pile&to=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||to.count<MaxHand);to.ids[to.count++]=(uint16_t)id;s.zones[id]=zone;
}
static void blade(State&s,int expected){int id=find_kind(s,SovereignBlade);CHECK(id>=0&&s.expansion.blade_damage[id]==expected);}
static void colorless_batch(const State&s,int expected,unsigned up){
 CHECK(s.hand.count==expected&&s.regent.cards_created==expected);
 for(unsigned i=0;i<s.hand.count;i++){Card c=s.cards[s.hand.ids[i]];CHECK(card_colorless(c.kind)&&card_can_generate(c.kind)&&c.upgraded==up);for(unsigned j=0;j<i;j++)CHECK(c.kind!=s.cards[s.hand.ids[j]].kind);}
}

static void card_contract(unsigned kind,unsigned up){
 active_kind=kind;active_upgrade=up;scenario="card outcome";int u=(int)up;State s=fight(kind,up);
 switch(kind){
 case RegentAlignment:act(s);CHECK(s.energy==102+u&&s.expansion.stars==98);break;
 case RegentArsenal:act(s);CHECK(power(s,RPower::Arsenal)==1);cast(s,RegentCollisionCourse);CHECK(s.strength==1&&s.regent.cards_created==1);break;
 case RegentAstralPulse:act(s);CHECK(hp(s)==240-4*(6+2*u));break;
 case RegentBeatIntoShape:cast(s,RegentStrike);cast(s,RegentStrike);act(s);CHECK(s.enemies[0].hp==103-2*u);blade(s,25+6*u);break;
 case RegentBegone:{int id=add(s,RegentKinglyKick,1);s.regent.kick_discount[id]=2;s.expansion.turn_cost[id]=1;act(s);CHECK(s.expansion.choice==XChoice::RTransformHand);pick(s,1);CHECK(s.cards[id].kind==MinionStrike&&s.cards[id].upgraded==up&&!s.expansion.turn_cost[id]&&!s.regent.kick_discount[id]&&s.regent.cards_created==1);break;}
 case RegentBigBang:supply(s,2);act(s);CHECK(s.energy==101&&s.expansion.stars==101&&s.hand.count==2);blade(s,15);break;
 case RegentBlackHole:act(s);cast(s,RegentVenerate);CHECK(hp(s)==240-2*(3+u));cast(s,RegentFallingStar);CHECK(hp(s)==240-4*(3+u)-8);break;
 case RegentBombardment:act(s);CHECK(s.enemies[0].hp==102-6*u&&s.zones[0]==Zone::Exhaust);supply(s,10);next_turn(s);CHECK(hp(s)==240-2*(18+6*u)&&s.zones[0]==Zone::Exhaust&&s.energy==3);break;
 case RegentBulwark:act(s);CHECK(s.block==12+3*u);blade(s,20+3*u);break;
 case RegentBundleOfJoy:act(s);colorless_batch(s,3+u,0);break;
 case RegentCelestialMight:act(s);CHECK(s.enemies[0].hp==120-6*(3+u));break;
 case RegentCharge:{supply(s,3);act(s);CHECK(s.expansion.choice==XChoice::RTransformDraw);pick(s,2);CHECK(count_kind(s,MinionDiveBomb,Zone::Draw)==2&&s.regent.cards_created==2);for(unsigned i=1;i<3;i++)CHECK(s.cards[i].upgraded==up);break;}
 case RegentChildOfTheStars:act(s);s.dexterity=7;cast(s,RegentFallingStar);CHECK(s.block==2*(2+u));break;
 case RegentCloakOfStars:act(s);CHECK(s.block==7+3*u&&s.expansion.stars==99);break;
 case RegentCollisionCourse:act(s);CHECK(s.enemies[0].hp==110-4*u&&count_kind(s,Debris,Zone::Hand)==1&&s.regent.cards_created==1);break;
 case RegentComet:act(s);CHECK(s.enemies[0].hp==87-11*u&&s.enemies[0].weak==3&&s.enemies[0].vulnerable==3&&s.expansion.stars==95);break;
 case RegentConqueror:act(s);blade(s,13+2*u);CHECK(s.regent.conqueror[0]==1);act(s,find_kind(s,SovereignBlade));CHECK(s.enemies[0].hp==120-2*(13+2*u));break;
 case RegentConvergence:{int retained=add(s,RegentDefend);supply(s,12);act(s);CHECK(power_value(s,XPower::Retain)==1);next_turn(s);CHECK(contains(s.hand,retained)&&s.energy==4&&s.expansion.stars==101+u&&!power(s,RPower::EnergyNextTurn)&&!power(s,RPower::StarsNextTurn));break;}
 case RegentCosmicIndifference:{int id=add(s,RegentStrike,0,Zone::Discard);act(s);CHECK(s.expansion.choice==XChoice::RDiscardTop);pick(s,1);CHECK(s.block==6+3*u&&s.draw.ids[s.draw.count-1]==id);break;}
 case RegentCrashLanding:act(s);CHECK(hp(s)==240-2*(21+5*u)&&s.hand.count==10&&count_kind(s,Debris,Zone::Hand)==10&&s.regent.cards_created==10);break;
 case RegentCrescentSpear:add(s,RegentCloakOfStars);add(s,RegentStardust,0,Zone::Exhaust);act(s);CHECK(s.enemies[0].hp==112-3*(2+u));break;
 case RegentCrushUnder:act(s);CHECK(hp(s)==240-2*(8+u)&&s.enemies[0].strength==-1-u&&s.enemies[1].strength==-1-u);supply(s,10);next_turn(s);CHECK(s.enemies[0].strength==2&&s.enemies[1].strength==2);break;
 case RegentDecisionsDecisions:{int chosen=add(s,RegentDefend,1);supply(s,8);act(s);CHECK(s.expansion.choice==XChoice::RDecisions&&s.hand.count==4+2*u);pick(s,1);CHECK(s.block==24&&s.regent.cards_finished_turn==4&&s.regent.skills_finished_turn==4&&s.zones[chosen]==Zone::Discard&&s.expansion.stars==94);break;}
 case RegentDefend:act(s);CHECK(s.block==5+3*u);break;
 case RegentDevastate:act(s);CHECK(s.enemies[0].hp==85-10*u&&s.expansion.stars==96);break;
 case RegentDyingStar:act(s);CHECK(hp(s)==240-2*(9+2*u)&&s.enemies[0].strength==-9-2*u&&s.enemies[1].strength==-9-2*u);break;
 case RegentFallingStar:act(s);CHECK(s.enemies[0].hp==112-4*u&&s.enemies[0].weak==1&&s.enemies[0].vulnerable==1);break;
 case RegentForegoneConclusion:supply(s,15);act(s);CHECK(power(s,RPower::ForegoneConclusion)==2+u);next_turn(s);CHECK(s.hand.count==7+u&&!power(s,RPower::ForegoneConclusion));break;
 case RegentFurnace:supply(s,12);act(s);next_turn(s);blade(s,15+2*u);CHECK(s.hand.count==6&&s.regent.cards_created==1);break;
 case RegentGammaBlast:act(s);CHECK(s.enemies[0].hp==107-5*u&&s.enemies[0].weak==2&&s.enemies[0].vulnerable==2);break;
 case RegentGatherLight:act(s);CHECK(s.block==8+3*u&&s.expansion.stars==101);break;
 case RegentGenesis:supply(s,10);act(s);CHECK(s.expansion.stars==100);next_turn(s);CHECK(s.expansion.stars==102+u);break;
 case RegentGlimmer:supply(s,8);act(s);CHECK(s.expansion.choice==XChoice::Top&&s.hand.count==3+u);pick(s,1);CHECK(s.hand.count==2+u);break;
 case RegentGlitterstream:s.dexterity=2;supply(s,10);act(s);CHECK(s.block==13+2*u&&s.block_next_turn==7+2*u);next_turn(s);CHECK(s.block==7+2*u);break;
 case RegentGlow:supply(s,15);act(s);CHECK(s.expansion.stars==101+u&&s.hand.count==1&&s.draw_next_turn==1);next_turn(s);CHECK(s.hand.count==6);break;
 case RegentGuards:{int a=add(s,RegentStrike),b=add(s,RegentDefend);act(s);CHECK(s.expansion.choice==XChoice::RTransformGuards&&choice_min(s)==0);pick(s,2);CHECK(s.cards[a].kind==MinionSacrifice&&s.cards[b].kind==MinionSacrifice&&s.cards[a].upgraded==up&&s.cards[b].upgraded==up&&s.regent.cards_created==2);break;}
 case RegentGuidingStar:act(s);CHECK(s.enemies[0].hp==108-u&&s.draw_next_turn==2+u);break;
 case RegentHeavenlyDrill:s.energy=4;act(s);CHECK(s.energy==0&&s.enemies[0].hp==120-8*(8+2*u));break;
 case RegentHegemony:supply(s,10);act(s);CHECK(s.enemies[0].hp==105-3*u&&power(s,RPower::EnergyNextTurn)==2+u);next_turn(s);CHECK(s.energy==5+u);break;
 case RegentHeirloomHammer:{int id=add(s,FlashOfSteel,1);s.expansion.replay[id]=2;s.expansion.turn_cost[id]=1;s.silent.sly[id]=2;s.silent.retain[id]=2;act(s);CHECK(s.expansion.choice==XChoice::RHeirloom);pick(s,1);CHECK(s.enemies[0].hp==100-5*u&&s.hand.count==2&&s.regent.cards_created==1);int copy=s.hand.ids[1];CHECK(s.cards[copy].kind==FlashOfSteel&&s.cards[copy].upgraded&&s.expansion.replay[copy]==2&&s.expansion.turn_cost[copy]==1&&s.silent.sly[copy]==2&&s.silent.retain[copy]==2);break;}
 case RegentHiddenCache:supply(s,10);act(s);CHECK(s.expansion.stars==101&&power(s,RPower::StarsNextTurn)==3+u);next_turn(s);CHECK(s.expansion.stars==104+u);break;
 case RegentIAmInvincible:act(s);CHECK(s.block==10+3*u);supply(s,10);transfer(s,0,Zone::Draw);s.block=0;s.enemies[0].move=0;CHECK(end_turn(s));settle(s);CHECK(s.hp==75-(u?0:2)&&s.zones[0]==Zone::Discard);break;
 case RegentKinglyKick:act(s);CHECK(s.enemies[0].hp==93-8*u&&s.energy==96);break;
 case RegentKinglyPunch:act(s);CHECK(s.enemies[0].hp==112-2*u);break;
 case RegentKnockoutBlow:s.enemies[0].hp=1;act(s);CHECK(!s.enemies[0].hp&&s.expansion.stars==105);break;
 case RegentKnowThyPlace:act(s);CHECK(s.enemies[0].weak==1&&s.enemies[0].vulnerable==1&&s.zones[0]==(up?Zone::Discard:Zone::Exhaust));break;
 case RegentLunarBlast:cast(s,RegentVenerate);cast(s,RegentVenerate);act(s);CHECK(s.enemies[0].hp==120-2*(4+u));break;
 case RegentMakeItSo:act(s);CHECK(s.enemies[0].hp==114-3*u&&s.zones[0]==Zone::Discard);cast(s,RegentVenerate);cast(s,RegentVenerate);cast(s,RegentVenerate);CHECK(s.zones[0]==Zone::Hand);break;
 case RegentManifestAuthority:act(s);CHECK(s.block==7+u);colorless_batch(s,1,up);break;
 case RegentMeteorShower:act(s);CHECK(hp(s)==240-2*(14+7*u)&&s.enemies[0].weak==2&&s.enemies[1].weak==2&&s.enemies[0].vulnerable==2&&s.enemies[1].vulnerable==2);break;
 case RegentMonarchsGaze:act(s);cast(s,RegentCelestialMight);CHECK(s.enemies[0].hp==102&&s.enemies[0].strength==-3);break;
 case RegentMonologue:act(s);CHECK(s.strength==0);cast(s,RegentStrike);CHECK(s.strength==1&&s.enemies[0].hp==114);cast(s,RegentStrike);CHECK(s.strength==2&&s.enemies[0].hp==107);supply(s,10);next_turn(s);CHECK(!s.strength&&!power(s,RPower::Monologue));break;
 case RegentNeutronAegis:act(s);CHECK(power_value(s,XPower::Plating)==8+3*u&&s.expansion.stars==95);break;
 case RegentOrbit:act(s);for(int n=0;n<4;n++)cast(s,RegentDefend);CHECK(s.energy==95+u&&s.regent.orbit_remainder[0]==0);break;
 case RegentPaleBlueDot:supply(s,15);act(s);for(int n=0;n<4;n++)cast(s,RegentDefend);CHECK(s.draw_next_turn==1+u&&s.regent.pale_activated==1);next_turn(s);CHECK(s.hand.count==6+u);break;
 case RegentParry:act(s);cast(s,SovereignBlade);CHECK(s.enemies[0].hp==110&&s.block==10+4*u);break;
 case RegentParticleWall:act(s);CHECK(s.block==9+3*u&&s.zones[0]==Zone::Hand&&s.expansion.stars==98);break;
 case RegentPatter:act(s);CHECK(s.block==8+2*u&&s.vigor==2+u);break;
 case RegentPhotonCut:supply(s,5);act(s);CHECK(s.enemies[0].hp==110-3*u&&s.expansion.choice==XChoice::Top);pick(s,1);CHECK(s.hand.count==up);break;
 case RegentPillarOfCreation:act(s);s.dexterity=7;cast(s,RegentCollisionCourse);CHECK(s.block==2+u&&s.regent.cards_created==1);break;
 case RegentProphesize:supply(s,12);act(s);CHECK(s.hand.count==6+3*u);break;
 case RegentQuasar:act(s);CHECK(s.expansion.choice==XChoice::Offer&&s.expansion.choices.count==3);pick(s,1);colorless_batch(s,1,up);break;
 case RegentRadiate:cast(s,RegentVenerate);act(s);CHECK(hp(s)==240-4*(3+u)&&s.regent.stars_gained_turn==2);break;
 case RegentRefineBlade:act(s);blade(s,18+4*u);CHECK(power(s,RPower::EnergyNextTurn)==1);break;
 case RegentReflect:act(s);CHECK(s.block==15+5*u&&power(s,RPower::Reflect)==1);s.block=5;supply(s,10);s.enemies[0].move=0;CHECK(end_turn(s));settle(s);CHECK(s.hp==68&&s.enemies[0].hp==115&&!power(s,RPower::Reflect));break;
 case RegentResonance:act(s);CHECK(s.strength==1+u&&s.enemies[0].strength==-1&&s.enemies[1].strength==-1);supply(s,10);next_turn(s);CHECK(s.enemies[0].strength==1&&s.enemies[1].strength==1);break;
 case RegentRoyalGamble:act(s);CHECK(s.expansion.stars==104&&s.zones[0]==Zone::Exhaust);break;
 case RegentRoyalties:{act(s);State control=s;control.regent.powers[(int)RPower::Royalties]=0;control.regent.power_count=0;control.regent.power_order[0]=0;damage_unpowered(s,0,120);damage_unpowered(s,1,120);resolve_pending(s);damage_unpowered(control,0,120);damage_unpowered(control,1,120);resolve_pending(control);CHECK(s.phase==Phase::Reward&&s.gold-control.gold==30+10*u);break;}
 case RegentSeekingEdge:act(s);blade(s,17+4*u);CHECK(power(s,RPower::SeekingEdge)==1&&!card_targets_enemy(s,{SovereignBlade,0}));act(s,find_kind(s,SovereignBlade),-1);CHECK(hp(s)==240-2*(17+4*u));break;
 case RegentSevenStars:act(s);CHECK(hp(s)==142&&s.expansion.stars==93);break;
 case RegentShiningStrike:act(s);CHECK(s.enemies[0].hp==112-3*u&&s.expansion.stars==102&&s.zones[0]==Zone::Draw&&s.draw.ids[s.draw.count-1]==0);break;
 case RegentSolarStrike:act(s);CHECK(s.enemies[0].hp==111-u&&s.expansion.stars==101+u);break;
 case RegentSpectrumShift:supply(s,12);act(s);next_turn(s);CHECK(s.hand.count==6&&s.regent.cards_created==1);{int found=0;for(unsigned j=0;j<s.hand.count;j++)found+=card_colorless(s.cards[s.hand.ids[j]].kind);CHECK(found==1);}break;
 case RegentSpoilsOfBattle:supply(s,5);act(s);CHECK(s.hand.count==3);blade(s,16+3*u);break;
 case RegentStardust:s.expansion.stars=3;act(s);CHECK(hp(s)==240-3*(5+2*u)&&s.expansion.stars==0&&s.energy==100);break;
 case RegentStrike:act(s);CHECK(s.enemies[0].hp==114-3*u);break;
 case RegentSummonForth:{int a=add(s,SovereignBlade,0,Zone::Draw),b=add(s,SovereignBlade,0,Zone::Discard),c=add(s,SovereignBlade,0,Zone::Exhaust);s.expansion.blade_damage[a]=14;s.expansion.blade_damage[b]=18;s.expansion.blade_damage[c]=30;act(s);CHECK(s.zones[a]==Zone::Hand&&s.zones[b]==Zone::Hand&&s.zones[c]==Zone::Hand&&s.expansion.blade_damage[a]==22+3*u&&s.expansion.blade_damage[b]==26+3*u&&s.expansion.blade_damage[c]==38+3*u&&!s.regent.cards_created);break;}
 case RegentSupermassive:cast(s,RegentCollisionCourse);cast(s,RegentCollisionCourse);act(s);CHECK(s.enemies[0].hp==95-2*(3+u)&&s.regent.cards_created==2);break;
 case RegentSwordSage:act(s);cast(s,SovereignBlade);CHECK(s.enemies[0].hp==100&&s.regent.cards_finished_turn==3);break;
 case RegentTerraforming:act(s);CHECK(s.vigor==7+3*u);break;
 case RegentTheSealedThrone:act(s);CHECK(s.expansion.stars==97);cast(s,RegentVenerate);CHECK(s.expansion.stars==100&&s.regent.stars_gained_turn==3);break;
 case RegentTheSmith:act(s);blade(s,40+10*u);CHECK(s.expansion.stars==96);break;
 case RegentTyranny:supply(s,12);act(s);next_turn(s);CHECK(s.hand.count==5&&s.exhaust.count==1&&power(s,RPower::Tyranny)==1);break;
 case RegentVenerate:act(s);CHECK(s.expansion.stars==102+u);break;
 case RegentVoidForm:supply(s,12);act(s);settle(s);CHECK(s.turn==2&&s.energy==3&&power(s,RPower::VoidForm)==2&&!s.regent.void_played_turn);{int a=add(s,RegentFallingStar),b=add(s,RegentDefend);CHECK(cost(s,b)==0&&card_star_cost(s,s.cards[a],a)==0);act(s,a);act(s,b);CHECK(s.energy==3&&s.expansion.stars==100&&s.regent.void_played_turn==2);int c=add(s,RegentDefend);CHECK(cost(s,c)==1);}break;
 case RegentWroughtInWar:act(s);CHECK(s.enemies[0].hp==113-2*u);blade(s,17+2*u);break;
 case MinionStrike:supply(s,3);act(s);CHECK(s.enemies[0].hp==114-3*u&&s.hand.count==1&&s.zones[0]==Zone::Exhaust);break;
 case MinionDiveBomb:act(s);CHECK(s.enemies[0].hp==107-3*u&&s.zones[0]==Zone::Exhaust);break;
 case MinionSacrifice:act(s);CHECK(s.block==7+3*u&&s.zones[0]==Zone::Exhaust);break;
 case Debris:act(s);CHECK(s.energy==99&&hp(s)==240&&!s.block&&s.zones[0]==Zone::Exhaust);break;
 case SovereignBlade:s.expansion.blade_damage[0]=23;act(s);CHECK(s.enemies[0].hp==97&&s.energy==98+u);break;
 default:CHECK(false);
 }
 settle(s);++variants;
}

static void matrix(){
 scenario="canonical pool";CardKind pool[CardKinds]{};CHECK(character_pool(Character::Regent,pool)==86);CHECK(sizeof canonical/sizeof canonical[0]==91);
 bool seen[CardKinds]{};
 for(const auto&c:canonical){
  active_kind=c.id;CHECK(!seen[c.id]);seen[c.id]=true;
  CHECK(card_in_pool(c.id,Character::Regent)==(c.id>=RegentAlignment&&c.id<=RegentWroughtInWar));
  for(unsigned up=0;up<(c.id==Debris?1u:2u);up++){
   active_upgrade=up;State s=fight(c.id,up);Card card=s.cards[0];unsigned bits=up?c.up_keywords:c.base_keywords;
   CHECK(cost(card)==(up?c.up_cost:c.base_cost));CHECK(card_star_cost(s,card,0)==c.stars);CHECK(definitions[c.id].type==c.type);
   CHECK(card_exhausts(card)==((bits&1)!=0));CHECK(card_innate(card)==((bits&2)!=0));CHECK(card_retains(card)==((bits&4)!=0));
   CHECK(((up?definitions[c.id].upgraded_keywords:definitions[c.id].keywords)&8)==(bits&8));
   if(c.id==RegentMeteorShower||c.id==RegentTheSealedThrone)CHECK(card_rarity(c.id)==5&&!card_can_generate(c.id));
   roundtrip(s);card_contract(c.id,up);
  }
 }
 for(unsigned id:pool){if(id)CHECK(seen[id]);}
 CHECK(variants==181);
}

static void starting_character(){
 scenario="starting character";active_kind=RegentStrike;active_upgrade=0;State s{};new_run(s,19,false,Character::Regent);
 CHECK(s.hp==75&&s.max_hp==75&&s.gold==99&&s.deck_count==10);CHECK(s.relics&DivineRight);
 int strike=0,defend=0,falling=0,venerate=0;for(unsigned i=0;i<s.deck_count;i++){Card c=s.deck[i];CHECK(!c.upgraded);strike+=c.kind==RegentStrike;defend+=c.kind==RegentDefend;falling+=c.kind==RegentFallingStar;venerate+=c.kind==RegentVenerate;}
 CHECK(strike==4&&defend==4&&falling==1&&venerate==1);enter_combat(s);CHECK(s.expansion.stars==3&&s.regent.stars_gained_turn==3&&s.hand.count==5&&!s.regent.cards_created);ready(s);
 CHECK(card_strike_tag(RegentStrike));CHECK(card_strike_tag(RegentSolarStrike));
 CHECK(card_strike_tag(RegentShiningStrike));CHECK(card_strike_tag(MinionStrike));
 State upgrade{};new_run(upgrade,31,false,Character::Regent);upgrade.phase=Phase::Upgrade;upgrade.deck_count=1;upgrade.deck[0]={Debris,0};auto before=snapshot(upgrade);CHECK(!upgrade_card(upgrade,0));same(upgrade,before);
}

static void stars_payment_and_before_play(){
 scenario="star payment and replay";active_kind=RegentFallingStar;active_upgrade=0;
 auto s=fight(RegentFallingStar);s.expansion.stars=1;CHECK(!can_play_card(s,0));auto before=snapshot(s);CHECK(!play(s,0,0));same(s,before);
 cast(s,RegentBlackHole);cast(s,RegentChildOfTheStars);s.expansion.stars=2;s.expansion.replay[0]=1;s.dexterity=7;
 act(s);CHECK(s.expansion.stars==0&&s.block==4&&s.enemies[0].hp==97&&s.enemies[1].hp==117);CHECK(s.regent.cards_finished_turn==4);ready(s);

 scenario="Sealed Throne precedes Radiate history";auto r=fight(RegentRadiate);cast(r,RegentBlackHole);cast(r,RegentTheSealedThrone);
 for(auto&e:r.enemies)e.hp=120;r.regent.stars_gained_turn=0;act(r);CHECK(r.regent.stars_gained_turn==1&&hp(r)==228);ready(r);

 scenario="automatic star X preserves resources";auto x=fight(RegentDefend);int id=add(x,RegentStardust,1,Zone::Draw);x.expansion.stars=4;x.expansion.replay[id]=1;
 queue_extension(x,XOp::AutoInstance,id);resolve_pending(x);CHECK(hp(x)==184&&x.expansion.stars==4&&x.energy==100&&x.regent.cards_finished_turn==2);ready(x);

 scenario="star X bypasses Void Form cost modifiers";auto v=fight(RegentVoidForm,1);supply(v,15);act(v);settle(v);v.expansion.stars=4;int stardust=add(v,RegentStardust);
 CHECK(card_star_cost(v,v.cards[stardust],stardust)==-1);act(v,stardust);CHECK(v.expansion.stars==0&&v.energy==3&&hp(v)==220&&v.regent.void_played_turn==1);ready(v);
}

static void orbit_instances_and_draw_growth(){
 scenario="independent Orbit counters";active_kind=RegentOrbit;active_upgrade=0;
 auto s=fight(RegentOrbit);act(s);cast(s,RegentDefend);cast(s,RegentOrbit);CHECK(s.regent.orbit_remainder[0]==3&&s.regent.orbit_remainder[1]==0);roundtrip(s);
 cast(s,RegentDefend);CHECK(s.energy==95&&s.regent.orbit_remainder[0]==0&&s.regent.orbit_remainder[1]==1);
 supply(s,20);next_turn(s);cast(s,RegentProphesize);CHECK(s.energy==1&&s.regent.orbit_remainder[0]==2&&s.regent.orbit_remainder[1]==3);
 act(s,s.hand.ids[0]);CHECK(s.energy==1&&s.regent.orbit_remainder[0]==3&&s.regent.orbit_remainder[1]==0);ready(s);

 scenario="Kingly Kick draw and explicit cost order";active_kind=RegentKinglyKick;auto k=fight(RegentKinglyKick);transfer(k,0,Zone::Draw);draw_cards(k,1);CHECK(cost(k,0)==3&&k.regent.kick_discount[0]==1);
 queue_extension(k,XOp::RandomCosts);resolve_pending(k);int price=cost(k,0);CHECK(k.regent.kick_turn_origin[0]==1);
 transfer(k,0,Zone::Draw);draw_cards(k,1);CHECK(k.regent.kick_discount[0]==2&&cost(k,0)==(price>0?price-1:0));act(k);CHECK(!k.expansion.turn_cost[0]&&cost(k,0)==2);ready(k);

 scenario="Kingly Punch upgrade keeps accumulated damage";active_kind=RegentKinglyPunch;auto p=fight(RegentKinglyPunch);transfer(p,0,Zone::Draw);draw_cards(p,1);CHECK(damage(p,p.cards[0],0,0)==12&&p.regent.punch_damage[0]==4);
 p.cards[0].upgraded=1;CHECK(damage(p,p.cards[0],0,0)==14);transfer(p,0,Zone::Draw);draw_cards(p,1);CHECK(damage(p,p.cards[0],0,0)==20&&p.regent.punch_damage[0]==10);act(p);CHECK(p.enemies[0].hp==100);ready(p);
}

static void blades_and_generation(){
 scenario="blade Forge and replay interactions";active_kind=SovereignBlade;active_upgrade=0;
 auto s=fight(SovereignBlade);cast(s,RegentSeekingEdge);cast(s,RegentConqueror);cast(s,RegentParry);cast(s,RegentSwordSage);
 CHECK(s.expansion.blade_damage[0]==20);s.strength=2;s.dexterity=3;s.vigor=4;act(s);CHECK(s.enemies[0].hp==24&&s.enemies[1].hp==72&&s.block==26&&!s.vigor);ready(s);

 scenario="exhausted swords excluded until summoned";auto f=fight(RegentTheSmith);int old=add(f,SovereignBlade,1,Zone::Exhaust);f.expansion.blade_damage[old]=41;act(f);
 CHECK(f.expansion.blade_damage[old]==41&&count_kind(f,SovereignBlade,Zone::Hand)==1&&f.regent.cards_created==1);int fresh=f.hand.ids[0];CHECK(f.expansion.blade_damage[fresh]==40);
 cast(f,RegentSummonForth);CHECK(f.zones[old]==Zone::Hand&&f.expansion.blade_damage[old]==49&&f.expansion.blade_damage[fresh]==48&&f.regent.cards_created==1);ready(f);

 scenario="generation powers and transformations";active_kind=RegentGuards;auto g=fight(RegentGuards,1);cast(g,RegentArsenal);cast(g,RegentPillarOfCreation,1);g.dexterity=9;
 int a=add(g,RegentKinglyPunch),b=add(g,RegentKinglyKick);g.regent.punch_damage[a]=11;g.regent.kick_discount[b]=3;g.expansion.replay[a]=2;
 act(g);pick(g,2);CHECK(g.cards[a].kind==MinionSacrifice&&g.cards[b].kind==MinionSacrifice&&g.cards[a].upgraded&&g.cards[b].upgraded&&g.strength==2&&g.block==6&&g.regent.cards_created==2&&!g.regent.punch_damage[a]&&!g.regent.kick_discount[b]&&!g.expansion.replay[a]);ready(g);
}

static void late_return_and_enemy_hooks(){
 scenario="fully blocked hits count for Forge and Gaze";active_kind=RegentCelestialMight;active_upgrade=0;
 auto s=fight(RegentCelestialMight);cast(s,RegentMonarchsGaze);s.enemies[0].block=100;act(s);
 CHECK(s.enemies[0].hp==120&&s.enemies[0].block==82&&s.regent.hits_turn[0]==3&&s.enemies[0].strength==-3);
 cast(s,RegentBeatIntoShape);blade(s,30);CHECK(s.regent.hits_turn[0]==4&&s.enemies[0].strength==-4);ready(s);

 scenario="weak before vulnerable consumes Artifact";active_kind=RegentComet;auto a=fight(RegentComet);a.silent.enemy_artifact[0]=1;act(a);CHECK(!a.enemies[0].weak&&a.enemies[0].vulnerable==3&&!a.silent.enemy_artifact[0]);ready(a);

 scenario="Make It So returns after all ordinary listeners";active_kind=RegentMakeItSo;auto m=fight(RegentMakeItSo);act(m);transfer(m,0,Zone::Exhaust);
 cast(m,RegentMonologue);cast(m,RegentVenerate);cast(m,RegentVenerate);CHECK(m.zones[0]==Zone::Hand&&m.strength==2&&m.regent.skills_finished_turn==3);ready(m);

 scenario="manual replay consumes one Void Form allowance";active_kind=RegentVoidForm;auto v=fight(RegentVoidForm,1);supply(v,20);act(v);settle(v);int skill=add(v,RegentDefend);v.expansion.replay[skill]=2;
 act(v,skill);CHECK(v.energy==3&&v.block==15&&v.regent.void_played_turn==1);int star=add(v,RegentCloakOfStars);CHECK(card_star_cost(v,v.cards[star],star)==0);act(v,star);CHECK(v.expansion.stars==100&&v.regent.void_played_turn==2);ready(v);

 scenario="Conqueror and Reflect stacks extend duration";active_kind=RegentConqueror;auto t=fight(RegentConqueror);act(t);cast(t,RegentConqueror);cast(t,RegentReflect);cast(t,RegentReflect);CHECK(t.regent.conqueror[0]==2&&power(t,RPower::Reflect)==2);
 supply(t,18);next_turn(t);CHECK(t.regent.conqueror[0]==1&&power(t,RPower::Reflect)==1);next_turn(t);CHECK(!t.regent.conqueror[0]&&!power(t,RPower::Reflect));ready(t);
}

static void suspended_choices_and_corruption(){
 scenario="Foregone Conclusion shuffle continuation";active_kind=RegentForegoneConclusion;active_upgrade=0;
 auto s=fight(RegentForegoneConclusion,1);supply(s,8,RegentDefend,Zone::Discard);act(s);queue_extension(s,XOp::Power,(int)XPower::Stratagem,1);resolve_pending(s);
 CHECK(end_turn(s));CHECK(s.phase==Phase::ExtendedChoice);roundtrip(s);settle(s);CHECK(s.hand.count>=8&&!power(s,RPower::ForegoneConclusion));ready(s);

 scenario="nested Decisions choices preserve physical frames";active_kind=RegentDecisionsDecisions;auto d=fight(RegentDecisionsDecisions);int top=add(d,RegentCosmicIndifference);add(d,RegentStrike,0,Zone::Discard);supply(d,8);
 act(d);CHECK(d.expansion.choice==XChoice::RDecisions);pick(d,1);CHECK(d.expansion.choice==XChoice::RDiscardTop&&d.expansion.depth==1&&d.playing==top);roundtrip(d);settle(d);CHECK(d.block==18&&d.regent.cards_finished_turn==4&&d.expansion.stars==94);ready(d);

 scenario="Regent suffix rejects malformed counters";active_kind=RegentOrbit;auto bad=fight(RegentOrbit);act(bad);State invalid=bad;invalid.regent.orbit_remainder[0]=4;CHECK(!validate(invalid));invalid=bad;invalid.regent.power_order[1]=1;CHECK(!validate(invalid));invalid=bad;invalid.regent.stars_spent[MaxAutoFrames]=2;CHECK(!validate(invalid));invalid=bad;invalid.regent.kick_discount[0]=1;CHECK(!validate(invalid));
}

static void shared_local_costs_and_star_expiry(){
 active_kind=RegentDevastate;active_upgrade=0;
 scenario="BulletTime Set hides earlier Transfigure Add, then both fixed resources expire after the physical play";
 {auto s=fight(RegentDevastate);cast(s,NecroTransfigure);CHECK(cost(s,0)==2&&card_star_cost(s,s.cards[0],0)==4);
  cast(s,BulletTime);CHECK(cost(s,0)==0&&card_star_cost(s,s.cards[0],0)==0&&s.expansion.star_turn_free[0]);roundtrip(s);
  s.energy=s.expansion.stars=0;act(s);settle(s);CHECK(!s.energy&&!s.expansion.stars&&cost(s,0)==2&&card_star_cost(s,s.cards[0],0)==4&&!s.expansion.star_turn_free[0]);
 }
 scenario="A Transfigure Add after BulletTime Set still costs one energy but no fixed stars";
 {auto s=fight(RegentDevastate);cast(s,BulletTime);cast(s,NecroTransfigure);CHECK(cost(s,0)==1&&card_star_cost(s,s.cards[0],0)==0);
  s.energy=1;s.expansion.stars=0;act(s);settle(s);CHECK(!s.energy&&cost(s,0)==2&&card_star_cost(s,s.cards[0],0)==4);
 }
 scenario="RocketPunch until-play reductions and BulletTime Set expire together";
 {auto s=fight(DefectRocketPunch);cast(s,DefectTurbo);cast(s,DefectTurbo);cast(s,DefectTurbo);CHECK(s.defect.cost_delta[0]==-3&&cost(s,0)==0);
  cast(s,BulletTime);CHECK(cost(s,0)==0&&s.defect.turn_origin[0]==-3);act(s);settle(s);CHECK(!s.defect.cost_delta[0]&&!s.expansion.turn_cost[0]&&cost(s,0)==2);
 }
 scenario="Negative local subtotals skip BorrowedTime, while a new zero Set enables its global addition";
 {auto s=fight(DefectRocketPunch);cast(s,DefectTurbo);cast(s,DefectTurbo);cast(s,DefectTurbo);cast(s,NecroBorrowedTime);cast(s,NecroBorrowedTime);
  CHECK(cost(s,0)==0&&card_free_preview_cost(s,s.cards[0],0)==2);cast(s,BulletTime);CHECK(cost(s,0)==2);ready(s);
 }
 scenario="TouchOfInsanity accepts star-only costs and preserves its combat star Set across plays and turns";
 {auto s=fight(RegentAstralPulse);s.potions[0]=Potion::TouchOfInsanity;CHECK(use_potion(s,0,0));CHECK(s.expansion.choice==XChoice::Free&&s.expansion.choices.count==1);pick(s,1);
  CHECK(s.expansion.star_combat_free[0]&&!s.expansion.star_turn_free[0]&&card_star_cost(s,s.cards[0],0)==0);s.expansion.stars=0;act(s);settle(s);CHECK(card_star_cost(s,s.cards[0],0)==0);
  supply(s,8);next_turn(s);CHECK(s.expansion.star_combat_free[0]&&card_star_cost(s,s.cards[0],0)==0);roundtrip(s);
 }
 scenario="TouchOfInsanity also sees a local cost hidden by a global free power";
 {auto s=fight(RegentDefend);cast(s,Pounce);CHECK(cost(s,0)==0);s.potions[0]=Potion::TouchOfInsanity;CHECK(use_potion(s,0,0));CHECK(s.expansion.choice==XChoice::Free&&s.expansion.choices.count==1);pick(s,1);CHECK(s.expansion.combat_cost[0]==1);ready(s);}
 scenario="LiquidMemories reduces both fixed costs until the recovered card is played";
 {auto s=fight(RegentDevastate);transfer(s,0,Zone::Discard);s.potions[0]=Potion::LiquidMemories;CHECK(use_potion(s,0,0));pick(s,1);
  CHECK(s.zones[0]==Zone::Hand&&cost(s,0)==0&&card_star_cost(s,s.cards[0],0)==0);act(s);settle(s);CHECK(cost(s,0)==1&&card_star_cost(s,s.cards[0],0)==4);
 }
 scenario="Energy-only free attacks and SneckoOil leave fixed star costs intact";
 {auto s=fight(RegentDevastate);cast(s,Unrelenting);CHECK(cost(s,0)==0&&card_star_cost(s,s.cards[0],0)==4&&!s.expansion.star_turn_free[0]);ready(s);}
 {auto s=fight(RegentReflect);supply(s,7);s.potions[0]=Potion::SneckoOil;CHECK(use_potion(s,0,0));settle(s);CHECK(card_star_cost(s,s.cards[0],0)==3&&!s.expansion.star_turn_free[0]&&!s.expansion.star_combat_free[0]);}
 scenario="BulletTime leaves star X and energy X resource payment intact";
 {auto s=fight(RegentStardust);int energy_x=add(s,RegentHeavenlyDrill);cast(s,BulletTime);CHECK(card_star_cost(s,s.cards[0],0)==-1&&cost(s,energy_x)==-1&&!s.expansion.turn_cost[energy_x]);
  s.energy=2;s.expansion.stars=3;act(s);settle(s);CHECK(s.energy==2&&!s.expansion.stars&&card_star_cost(s,s.cards[0],0)==-1);
 }
 scenario="The turn boundary clears unplayed fixed-star reductions";
 {auto s=fight(RegentCloakOfStars);cast(s,BulletTime);CHECK(card_star_cost(s,s.cards[0],0)==0);supply(s,8);next_turn(s);CHECK(!s.expansion.star_turn_free[0]&&card_star_cost(s,s.cards[0],0)==1);}
}

int main(){
 starting_character();matrix();stars_payment_and_before_play();orbit_instances_and_draw_growth();blades_and_generation();late_return_and_enemy_hooks();suspended_choices_and_corruption();shared_local_costs_and_star_expiry();
 std::printf("Regent completion regression: %u checks, %u base/upgraded/token variants passed\n",checks,variants);return 0;
}
