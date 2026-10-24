#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static unsigned checks,variants,active_kind,active_upgrade;
static bool covered[CardKinds][2]{};
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %s:%d %s [%s%s]\n",__FILE__,__LINE__,#x,definitions[active_kind].key,active_upgrade?"+":""); std::exit(1); } } while (0)

static int add(State&s,unsigned kind,unsigned up=0,Zone zone=Zone::Hand){
 CHECK(s.card_count<MaxCards);int id=s.card_count++;
 s.cards[id]={(CardKind)kind,(uint8_t)up};s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(p.count<MaxCards);p.ids[p.count++]=(uint16_t)id;return id;
}
static State fight(unsigned kind,unsigned up=0){
 State s{};new_run(s,0x41cef1ea,false);s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;s.energy=100;
 s.relics=s.relic_count=s.relic_flash=0;s.deck_count=1;s.deck[0]={(CardKind)kind,(uint8_t)up};
 for(auto&e:s.enemies)e={46,46,0,0,0,2};add(s,kind,up);return s;
}
static void supply(State&s,int n,unsigned kind=Defend,Zone zone=Zone::Draw){for(int j=0;j<n;j++)add(s,kind,0,zone);}
static bool contains(const Pile&p,int id){for(unsigned j=0;j<p.count;j++)if(p.ids[j]==id)return true;return false;}
static int total_hp(const State&s){int n=0;for(unsigned j=0;j<s.enemy_count;j++)n+=s.enemies[j].hp;return n;}
static int count_kind(const State&s,const Pile&p,unsigned kind){int n=0;for(unsigned j=0;j<p.count;j++)n+=s.cards[p.ids[j]].kind==kind;return n;}
static int power(const State&s,IPower p){return s.ironclad.powers[(unsigned)p];}
static void act_id(State&s,int id,int target=0){int at=-1;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==id)at=(int)j;CHECK(at>=0);CHECK(play(s,at,target));}
static void act(State&s,unsigned kind,int target=0){int id=-1;for(unsigned j=0;j<s.hand.count;j++)if(s.cards[s.hand.ids[j]].kind==kind){id=s.hand.ids[j];break;}CHECK(id>=0);act_id(s,id,target);}
static SaveData snapshot(const State&s){SaveData b{};CHECK(validate(s));CHECK(save(s,b));return b;}
static void same(const State&s,const SaveData&b){auto a=snapshot(s);CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,b.size));}
static void roundtrip(const State&s){auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));same(copy,b);}
static void ready(const State&s){CHECK(s.phase==Phase::Combat&&s.playing==-1&&!s.effect_count&&!s.expansion.depth);roundtrip(s);}
static void pick(State&s,unsigned n,unsigned offset=0){
 CHECK(s.phase==Phase::ExtendedChoice&&n<=MaxHand&&offset+n<=s.expansion.choices.count);auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));
 uint16_t ids[MaxHand]{};for(unsigned j=0;j<n;j++)ids[j]=s.expansion.choices.ids[offset+j];const uint16_t wrong=65535;
 CHECK(!choose_extended(s,&wrong,1));same(s,b);
 if(n>1){uint16_t duplicate[MaxHand]{};for(unsigned j=0;j<n;j++)duplicate[j]=ids[0];CHECK(!choose_extended(s,duplicate,n));same(s,b);}
 CHECK(choose_extended(s,ids,n));CHECK(choose_extended(copy,ids,n));same(copy,snapshot(s));
}
static void drain_choices(State&s){
 for(unsigned n=0;choice_phase(s.phase)&&n<32;n++){
  if(s.phase==Phase::ExtendedChoice)pick(s,choice_min(s));
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else {uint16_t ids[MaxHand]{};unsigned count=choice_min(s);for(unsigned j=0;j<count;j++)ids[j]=s.hand.ids[j];CHECK(choose_discard(s,ids,count));}
 }
 CHECK(!choice_phase(s.phase));
}
static void next_turn(State&s){for(auto&e:s.enemies){e.move=2;e.strength=0;}CHECK(end_turn(s));drain_choices(s);}
static void return_to_hand(State&s,int id){
 Pile&p=s.zones[id]==Zone::Discard?s.discard:s.zones[id]==Zone::Exhaust?s.exhaust:s.draw;int at=-1;
 for(unsigned j=0;j<p.count;j++)if(p.ids[j]==id)at=(int)j;CHECK(at>=0&&s.hand.count<MaxHand);
 for(unsigned j=(unsigned)at+1;j<p.count;j++)p.ids[j-1]=p.ids[j];--p.count;s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;
}
static void exhaust_only_hand_card(State&s,int id){
 int purity=add(s,Purity);act_id(s,purity);CHECK(s.phase==Phase::ExhaustManyChoice);
 uint16_t chosen=(uint16_t)id;CHECK(choose_exhaust_many(s,&chosen,1));
}

// Numerical contracts come from the pinned model audit. This is not an
// original-runtime differential oracle: it verifies the implemented engine.
static void card_contract(unsigned kind,unsigned up){
 active_kind=kind;active_upgrade=up;int u=(int)up;State s=fight(kind,up);
 switch(kind){
 case Strike:act(s,kind);CHECK(s.enemies[0].hp==46-6-3*u);break;
 case Defend:act(s,kind);CHECK(s.block==5+3*u);break;
 case Bash:act(s,kind);CHECK(s.enemies[0].hp==46-8-2*u&&s.enemies[0].vulnerable==2+u);break;
 case Anger:act(s,kind);CHECK(s.enemies[0].hp==46-6-2*u&&s.card_count==2&&s.discard.count==2&&s.cards[1].upgraded==up);break;
 case PommelStrike:supply(s,4);act(s,kind);CHECK(s.enemies[0].hp==46-9-u&&s.hand.count==1+up);break;
 case ShrugItOff:supply(s,3);act(s,kind);CHECK(s.block==8+3*u&&s.hand.count==1);break;
 case BurningPact:{int id=add(s,Defend);supply(s,6);act(s,kind);CHECK(s.phase==Phase::Combat&&s.zones[id]==Zone::Exhaust&&s.hand.count==2+up);break;}
 case TrueGrit:{int id=add(s,Defend);act(s,kind);drain_choices(s);CHECK(s.zones[id]==Zone::Exhaust&&s.block==7+2*u);break;}
 case TwinStrike:act(s,kind);CHECK(s.enemies[0].hp==46-2*(5+2*u));break;
 case Inflame:act(s,kind);CHECK(s.strength==2+u&&s.zones[0]==Zone::Power);break;
 case IronWave:act(s,kind);CHECK(s.block==5+2*u&&s.enemies[0].hp==46-5-2*u);break;
 case Whirlwind:s.energy=3;act(s,kind);CHECK(s.energy==0&&total_hp(s)==92-6*(5+3*u));break;
 case FeelNoPain:{act(s,kind);int id=add(s,Defend);exhaust_only_hand_card(s,id);CHECK(s.pain==3+u&&s.block==2*(3+u));break;}
 case DarkEmbrace:{act(s,kind);int id=add(s,Defend);supply(s,4);exhaust_only_hand_card(s,id);CHECK(s.embrace==1&&s.hand.count==2&&s.draw.count==2);break;}
 case Bloodletting:act(s,kind);CHECK(s.hp==77&&s.energy==102+u);break;
 case Hemokinesis:act(s,kind);CHECK(s.hp==78&&s.enemies[0].hp==46-15-5*u);break;
 case Impervious:act(s,kind);CHECK(s.block==30+10*u&&s.zones[0]==Zone::Exhaust);break;
 case Offering:supply(s,8);act(s,kind);CHECK(s.hp==74&&s.energy==102&&s.hand.count==3+2*up&&s.zones[0]==Zone::Exhaust);break;
 case Bludgeon:act(s,kind);CHECK(s.enemies[0].hp==46-32-10*u);break;
 case Thunderclap:act(s,kind);CHECK(total_hp(s)==92-2*(4+3*u)&&s.enemies[0].vulnerable==1&&s.enemies[1].vulnerable==1);break;
 case Uppercut:act(s,kind);CHECK(s.enemies[0].hp==33&&s.enemies[0].vulnerable==1+u&&s.enemies[0].weak==1+u);break;
 case BodySlam:s.block=9;act(s,kind);CHECK(s.enemies[0].hp==37&&s.block==9&&s.energy==99+u);break;
 case BattleTrance:supply(s,8);act(s,kind);CHECK(s.hand.count==3+up&&s.no_draw==1);draw_cards(s,1);CHECK(s.hand.count==3+up);break;
 case FiendFire:supply(s,2,Defend,Zone::Hand);act(s,kind);CHECK(s.exhaust.count==3&&!s.hand.count&&s.enemies[0].hp==46-2*(7+3*u));break;
 case Headbutt:{int id=add(s,Anger,0,Zone::Discard);act(s,kind);CHECK(s.phase==Phase::Combat);roundtrip(s);CHECK(s.enemies[0].hp==46-9-3*u&&s.draw.ids[s.draw.count-1]==id);break;}
 case SwordBoomerang:act(s,kind);CHECK(total_hp(s)==92-3*(3+u));break;
 case PerfectedStrike:add(s,Strike);add(s,SetupStrike);add(s,AshenStrike);act(s,kind);CHECK(s.enemies[0].hp==46-6-4*(2+u));break;
 case FlameBarrier:act(s,kind);CHECK(s.block==12+4*u&&s.flame_barrier==4+2*u);s.enemies[0].move=0;s.enemies[1].move=2;CHECK(end_turn(s));CHECK(s.enemies[0].hp==46-4-2*u&&s.hp==80);break;
 case DemonForm:act(s,kind);supply(s,8);next_turn(s);CHECK(s.strength==3+u);break;
 case Aggression:{act(s,kind);int id=add(s,Strike,0,Zone::Discard);supply(s,8);next_turn(s);CHECK(contains(s.hand,id)&&s.cards[id].upgraded&&s.hand.count==6&&card_innate({(CardKind)kind,(uint8_t)up})==(up!=0));break;}
 case Armaments:{int a=add(s,Strike),b=add(s,Defend);act(s,kind);if(!up){CHECK(s.expansion.choice==XChoice::IUpgrade);pick(s,1);CHECK(s.cards[a].upgraded&&!s.cards[b].upgraded);}else CHECK(s.cards[a].upgraded&&s.cards[b].upgraded);CHECK(s.block==5&&s.deck[0].upgraded==up);break;}
 case AshenStrike:supply(s,2,Defend,Zone::Exhaust);act(s,kind);CHECK(s.enemies[0].hp==46-6-2*(3+u));break;
 case Barricade:s.block=13;act(s,kind);supply(s,8);next_turn(s);CHECK(s.block==13&&power(s,IPower::Barricade)==1);break;
 case BloodWall:act(s,kind);CHECK(s.hp==78&&s.block==16+4*u);break;
 case Brand:{int id=add(s,Strike);act(s,kind);CHECK(s.phase==Phase::Combat&&s.hp==79&&s.zones[id]==Zone::Exhaust&&s.strength==1+u);break;}
 case Break:act(s,kind);CHECK(s.enemies[0].hp==46-20-10*u&&s.enemies[0].vulnerable==5+2*u);break;
 case Breakthrough:act(s,kind);CHECK(s.hp==79&&total_hp(s)==92-2*(9+4*u));break;
 case Bully:s.enemies[0].vulnerable=3;act(s,kind);CHECK(s.enemies[0].hp==46-(4+3*(2+u))*150/100);break;
 case Cascade:s.energy=2;supply(s,3,Shiv);act(s,kind);CHECK(s.energy==0&&s.exhaust.count==2+up&&total_hp(s)==92-4*(2+u)&&s.expansion.cards_finished==3+up);break;
 case Cinder:{int id=add(s,Defend);act(s,kind);CHECK(s.enemies[0].hp==46-18-6*u&&s.zones[id]==Zone::Exhaust);break;}
 case Colossus:act(s,kind);CHECK(s.block==4+3*u);s.enemies[0].vulnerable=2;s.enemies[0].move=0;s.enemies[1].move=2;CHECK(end_turn(s));CHECK(s.hp==78+2*u&&!power(s,IPower::Colossus));break;
 case Conflagration:act(s,kind);CHECK(total_hp(s)==92-4*(4+u));break;
 case Corruption:{act(s,kind);int skill=add(s,ShrugItOff),attack=add(s,Strike);CHECK(cost(s,skill)==0&&cost(s,attack)==1);act_id(s,skill);CHECK(s.zones[skill]==Zone::Exhaust&&s.block==8);break;}
 case CrimsonMantle:act(s,kind);CHECK(s.hp==80&&!s.block);supply(s,8);next_turn(s);CHECK(s.hp==79&&s.block==7+3*u&&s.ironclad.hp_losses_combat==1);break;
 case Cruelty:act(s,kind);s.enemies[0].vulnerable=2;add(s,Strike);act(s,Strike);CHECK(s.enemies[0].hp==46-6*(175+25*u)/100);break;
 case Dismantle:s.enemies[0].vulnerable=2;act(s,kind);CHECK(s.enemies[0].hp==46-2*((8+2*u)*150/100));break;
 case Dominate:s.enemies[0].vulnerable=2;act(s,kind);CHECK(s.enemies[0].vulnerable==3+u&&s.strength==3+u&&s.zones[0]==Zone::Exhaust);break;
 case DrumOfBattle:{supply(s,4);act(s,kind);CHECK(s.hand.count==2);auto ex=fight(Purity);int id=add(ex,kind,up);act(ex,Purity);uint16_t chosen=(uint16_t)id;CHECK(choose_exhaust_many(ex,&chosen,1));CHECK(ex.energy==102+u&&ex.zones[id]==Zone::Exhaust);ready(ex);break;}
 case EvilEye:{act(s,kind);CHECK(s.block==8+3*u);auto ex=fight(kind,up);int victim=add(ex,Defend);exhaust_only_hand_card(ex,victim);act(ex,kind);CHECK(ex.block==2*(8+3*u));ready(ex);break;}
 case ExpectAFight:s.strength=3;s.dexterity=2;act(s,kind);CHECK(s.block==15+u+(5+3*u)*3+2);break;
 case Feed:{act(s,kind);CHECK(s.enemies[0].hp==46-10-2*u&&s.max_hp==80);auto lethal=fight(kind,up);lethal.enemies[0].hp=1;act(lethal,kind);CHECK(!lethal.enemies[0].hp&&lethal.max_hp==83+u&&lethal.hp==83+u);ready(lethal);break;}
 case FightMe:act(s,kind);CHECK(s.enemies[0].hp==46-2*(5+u)&&s.strength==3+u&&s.enemies[0].strength==1);break;
 case ForgottenRitual:act(s,kind);CHECK(s.energy==102+u&&s.zones[0]==Zone::Exhaust);break;
 case Havoc:{int id=add(s,Defend,0,Zone::Draw);act(s,kind);CHECK(s.block==5&&s.zones[id]==Zone::Exhaust&&s.energy==99+u&&!s.ironclad.forced_exhaust[id]);break;}
 case Hellraiser:act(s,kind);supply(s,3,Strike);draw_cards(s,3);CHECK(!s.hand.count&&s.discard.count==3&&total_hp(s)==74&&s.expansion.cards_finished==4);break;
 case HowlFromBeyond:{act(s,kind);CHECK(total_hp(s)==92-2*(18+6*u)&&s.zones[0]==Zone::Discard);auto ex=fight(Defend);int id=add(ex,kind,up,Zone::Exhaust);supply(ex,8);next_turn(ex);CHECK(total_hp(ex)==92-2*(18+6*u)&&ex.zones[id]==Zone::Discard);ready(ex);break;}
 case InfernalBlade:act(s,kind);CHECK(s.hand.count==1&&s.zones[0]==Zone::Exhaust);{int id=s.hand.ids[0];CHECK(card_in_pool(s.cards[id].kind,Character::Ironclad)&&card_can_generate(s.cards[id].kind)&&definitions[s.cards[id].kind].type==CardType::Attack);CHECK(cost(s,id)==(s.cards[id].kind==Whirlwind?-1:0));}break;
 case Inferno:act(s,kind);supply(s,8);next_turn(s);CHECK(s.hp==79&&total_hp(s)==92-2*(6+3*u));add(s,Bloodletting);act(s,Bloodletting);CHECK(s.hp==76&&total_hp(s)==92-4*(6+3*u));break;
 case Juggernaut:act(s,kind);s.strength=9;s.enemies[0].vulnerable=s.enemies[1].vulnerable=3;add(s,Defend);act(s,Defend);CHECK(s.block==5&&total_hp(s)==92-6-2*u);break;
 case Juggling:{act(s,kind);add(s,Strike);add(s,Strike);int rampage=add(s,Rampage,up);act(s,Strike);act(s,Strike);act_id(s,rampage);CHECK(s.hand.count==1&&s.cards[s.hand.ids[0]].kind==Rampage&&!s.ironclad.damage_bonus[s.hand.ids[0]]&&s.ironclad.damage_bonus[rampage]==5+5*u&&card_innate({(CardKind)kind,(uint8_t)up})==(up!=0));break;}
 case Mangle:act(s,kind);CHECK(s.enemies[0].hp==46-20-6*u&&s.enemies[0].strength==-10-5*u);s.enemies[0].move=0;s.enemies[1].move=2;CHECK(end_turn(s));CHECK(s.enemies[0].strength==0&&s.hp==78+2*u);break;
 case MoltenFist:s.enemies[0].vulnerable=2;act(s,kind);CHECK(s.enemies[0].hp==46-(10+4*u)*150/100&&s.enemies[0].vulnerable==4&&s.zones[0]==Zone::Exhaust);break;
 case NotYet:s.hp=40;act(s,kind);CHECK(s.hp==50+3*u&&s.zones[0]==Zone::Exhaust);break;
 case OneTwoPunch:act(s,kind);supply(s,3,Strike,Zone::Hand);act(s,Strike);act(s,Strike);act(s,Strike);CHECK(total_hp(s)==92-6*(4+u)&&!power(s,IPower::OneTwoPunch));break;
 case PactsEnd:{supply(s,3,Defend,Zone::Exhaust);act(s,kind);CHECK(total_hp(s)==92-2*(18+6*u));auto low=fight(kind,up);supply(low,2,Defend,Zone::Exhaust);act(low,kind);CHECK(total_hp(low)==92&&low.expansion.cards_finished==1);ready(low);break;}
 case Pillage:add(s,Strike,0,Zone::Draw);add(s,Defend,0,Zone::Draw);add(s,Strike,0,Zone::Draw);add(s,Strike,0,Zone::Draw);act(s,kind);CHECK(s.hand.count==3&&s.draw.count==1&&s.enemies[0].hp==46-6-3*u);break;
 case PrimalForce:{int a=add(s,Rampage),b=add(s,Strike,1),skill=add(s,Defend);s.ironclad.damage_bonus[a]=9;s.expansion.replay[b]=2;s.expansion.combat_cost[a]=1;act(s,kind);CHECK(s.cards[a].kind==GiantRock&&s.cards[b].kind==GiantRock&&s.cards[a].upgraded==up&&s.cards[b].upgraded==up&&s.cards[skill].kind==Defend&&!s.ironclad.damage_bonus[a]&&!s.expansion.combat_cost[a]&&!s.expansion.replay[b]);break;}
 case Pyre:act(s,kind);CHECK(s.energy==98);supply(s,8);next_turn(s);CHECK(s.energy==4+u);break;
 case Rage:act(s,kind);supply(s,2,Strike,Zone::Hand);act(s,Strike);act(s,Strike);CHECK(s.block==2*(3+2*u));supply(s,8);next_turn(s);CHECK(!power(s,IPower::Rage));break;
 case Rampage:act(s,kind);CHECK(s.enemies[0].hp==36&&s.ironclad.damage_bonus[0]==5+5*u);roundtrip(s);return_to_hand(s,0);act_id(s,0);CHECK(s.enemies[0].hp==21-5*u&&s.ironclad.damage_bonus[0]==10+10*u);break;
 case Rupture:act(s,kind);add(s,Hemokinesis);act(s,Hemokinesis);CHECK(s.hp==78&&s.enemies[0].hp==31&&s.strength==1+u);add(s,Bloodletting);act(s,Bloodletting);CHECK(s.strength==2*(1+u));break;
 case SecondWind:{int a=add(s,Defend),b=add(s,DrumOfBattle,up),attack=add(s,Strike);int energy=s.energy;act(s,kind);CHECK(s.zones[a]==Zone::Exhaust&&s.zones[b]==Zone::Exhaust&&s.zones[attack]==Zone::Hand&&s.block==2*(5+2*u)&&s.energy==energy+1+u);break;}
 case SetupStrike:act(s,kind);CHECK(s.enemies[0].hp==46-7-2*u&&s.strength==3+u);supply(s,8);next_turn(s);CHECK(!s.strength&&!s.ironclad.setup_strength);break;
 case Spite:{add(s,Bloodletting);act(s,Bloodletting);act(s,kind);CHECK(s.enemies[0].hp==46-5*(2+u));auto clean=fight(kind,up);act(clean,kind);CHECK(clean.enemies[0].hp==41);ready(clean);break;}
 case Stampede:{act(s,kind);int id=add(s,Anger,up);supply(s,8);next_turn(s);CHECK(total_hp(s)==92-6-2*u&&s.zones[id]==Zone::Discard&&count_kind(s,s.discard,Anger)==2&&power(s,IPower::Stampede)==1);break;}
 case Stoke:supply(s,2,Defend,Zone::Hand);act(s,kind);CHECK(s.exhaust.count==2&&s.hand.count==2&&s.card_count==5);for(unsigned j=0;j<s.hand.count;j++){Card c=s.cards[s.hand.ids[j]];CHECK(c.upgraded==up&&card_in_pool(c.kind,Character::Ironclad)&&card_can_generate(c.kind));}break;
 case Stomp:add(s,Strike);add(s,Strike);act(s,Strike);act(s,Strike);CHECK(cost(s,0)==1);act(s,kind);CHECK(s.enemies[0].hp==46-12-12-3*u&&s.enemies[1].hp==46-12-3*u&&cost(s,0)==0);break;
 case StoneArmor:act(s,kind);CHECK(power_value(s,XPower::Plating)==4+2*u);supply(s,8);next_turn(s);CHECK(!s.block&&power_value(s,XPower::Plating)==3+2*u);break;
 case Taunt:act(s,kind);CHECK(s.block==6+u&&s.enemies[0].vulnerable==1+u);break;
 case TearAsunder:add(s,Bloodletting);add(s,Bloodletting);act(s,Bloodletting);act(s,Bloodletting);act(s,kind);CHECK(s.hp==74&&s.enemies[0].hp==46-3*(5+2*u));break;
 case Thrash:{s.strength=2;int victim=add(s,Strike,up);act(s,kind);CHECK(s.enemies[0].hp==46-2*(6+2*u)&&s.zones[victim]==Zone::Exhaust&&s.ironclad.damage_bonus[0]==8+3*u);break;}
 case Tremble:act(s,kind);CHECK(s.enemies[0].vulnerable==3+u&&s.zones[0]==Zone::Exhaust);break;
 case Unmovable:act(s,kind);add(s,Defend);act(s,Defend);CHECK(s.block==10);add(s,Finesse);act(s,Finesse);CHECK(s.block==14&&s.ironclad.block_events_turn==2);break;
 case Unrelenting:{act(s,kind);int a=add(s,Bludgeon),b=add(s,Strike);CHECK(cost(s,a)==0);act_id(s,a,1);CHECK(cost(s,b)==1&&!power(s,IPower::FreeAttack)&&s.enemies[0].hp==46-14-6*u&&s.enemies[1].hp==14);break;}
 case Vicious:act(s,kind);supply(s,8);add(s,Tremble);act(s,Tremble);CHECK(s.hand.count==1+up);add(s,MoltenFist);act(s,MoltenFist);CHECK(s.hand.count==2+2*up);break;
 default:CHECK(false);
 }
 drain_choices(s);ready(s);covered[kind][up]=true;++variants;
}

static void no_global_free_x_cost(){
 active_kind=Corruption;active_upgrade=0;auto s=fight(Corruption);act(s,Corruption);int cascade=add(s,Cascade);supply(s,3,Shiv);s.energy=2;
 CHECK(cost(s,cascade)==-1);act_id(s,cascade);CHECK(s.energy==0&&s.expansion.cards_finished==4&&s.zones[cascade]==Zone::Exhaust&&total_hp(s)==84);ready(s);
 auto attack=fight(Unrelenting);act(attack,Unrelenting);int whirl=add(attack,Whirlwind);attack.energy=2;CHECK(cost(attack,whirl)==-1);act_id(attack,whirl);
 CHECK(!attack.energy&&!power(attack,IPower::FreeAttack)&&total_hp(attack)==58);ready(attack);
}
static void grouped_power_events(){
 active_kind=CrimsonMantle;active_upgrade=0;auto s=fight(CrimsonMantle);act(s,CrimsonMantle);add(s,CrimsonMantle,1);act(s,CrimsonMantle);add(s,Rupture);act(s,Rupture);supply(s,8);next_turn(s);
 CHECK(s.hp==78&&s.block==17&&s.strength==1&&s.ironclad.hp_losses_combat==1);ready(s);
 auto multi=fight(Unmovable);act(multi,Unmovable);int wind=add(multi,SecondWind);supply(multi,2,Defend,Zone::Hand);act_id(multi,wind);
 CHECK(multi.block==20&&multi.ironclad.block_events_turn==2);add(multi,Defend);act(multi,Defend);CHECK(multi.block==25);ready(multi);
 auto stacked=fight(Unmovable);act(stacked,Unmovable);add(stacked,Unmovable);act(stacked,Unmovable);int victim=add(stacked,Defend);exhaust_only_hand_card(stacked,victim);
 add(stacked,EvilEye);act(stacked,EvilEye);CHECK(stacked.block==32&&stacked.ironclad.block_events_turn==2);add(stacked,Defend);act(stacked,Defend);CHECK(stacked.block==37);ready(stacked);
 auto exhausted=fight(DarkEmbrace);act(exhausted,DarkEmbrace);int wind2=add(exhausted,SecondWind),first=add(exhausted,Defend);supply(exhausted,4);act_id(exhausted,wind2);
 CHECK(exhausted.zones[first]==Zone::Exhaust&&exhausted.hand.count==1&&exhausted.exhaust.count==1&&exhausted.block==5);ready(exhausted);
}
static void unmovable_preview_during_choice(){
 active_kind=Unmovable;active_upgrade=0;auto s=fight(Unmovable);act(s,Unmovable);int arm=add(s,Armaments),wall=add(s,BloodWall);add(s,Strike);act_id(s,arm);
 CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::IUpgrade&&s.playing==arm&&s.block==10);
 CHECK(s.ironclad.block_events_turn==1&&s.ironclad.block_events_play[s.expansion.depth]==1);
 int value=-1;CHECK(character_description_value(s,s.cards[wall],-1,wall,"Block",value)&&value==16);
 CHECK(card_block(s,s.cards[wall],wall)==16);CHECK(card_block(s,s.cards[arm],arm)==10);
 roundtrip(s);pick(s,1);ready(s);
}
static void player_power_acquisition_order(){
 active_kind=Panache;active_upgrade=0;
 auto first=fight(Panache);act(first,Panache);add(first,Juggernaut);act(first,Juggernaut);add(first,Rage);act(first,Rage);
 supply(first,2,Production,Zone::Hand);act(first,Production);act(first,Production);
 CHECK(first.expansion.panache_count[0]==4);int attack=add(first,Strike);first.enemies[0].hp=7;first.enemies[1].hp=30;first.rng_cards=2;
 auto before=snapshot(first);State copy{};CHECK(load(copy,before.bytes,before.size));act_id(first,attack);act_id(copy,attack);
 // The strike leaves enemy 0 at 1 HP. Panache, acquired before Rage,
 // kills it before Juggernaut chooses, so the surviving enemy takes both.
 CHECK(first.enemies[0].hp==0&&first.enemies[1].hp==14&&first.block==3);same(copy,snapshot(first));ready(first);
 auto reverse=fight(Juggernaut);act(reverse,Juggernaut);add(reverse,Rage);act(reverse,Rage);add(reverse,Panache);act(reverse,Panache);
 supply(reverse,4,Production,Zone::Hand);for(int j=0;j<4;j++)act(reverse,Production);CHECK(reverse.expansion.panache_count[0]==4);
 add(reverse,Strike);reverse.enemies[0].hp=7;reverse.enemies[1].hp=30;reverse.rng_cards=2;act(reverse,Strike);
 // Here Rage precedes Panache. The same RNG chooses the low-HP enemy
 // for Juggernaut; only Panache then hits enemy 1.
 CHECK(reverse.enemies[0].hp==0&&reverse.enemies[1].hp==20&&reverse.block==3);ready(reverse);
 supply(reverse,8);next_turn(reverse);CHECK(!power(reverse,IPower::Rage));ready(reverse);
}
static void temporary_cost_order(){
 active_kind=Stomp;active_upgrade=0;auto s=fight(Stomp);add(s,Strike);act(s,Strike);CHECK(cost(s,0)==2);
 s.potions[0]=Potion::SneckoOil;CHECK(use_potion(s,0,0));int set=cost(s,0);CHECK(set>=0&&set<=3);CHECK(s.ironclad.stomp_discount[0]==1&&s.ironclad.stomp_turn_origin[0]==1);
 add(s,Strike);act(s,Strike);CHECK(cost(s,0)==(set?set-1:0));roundtrip(s);act_id(s,0);CHECK(!s.expansion.turn_cost[0]&&cost(s,0)==0);ready(s);
 auto free=fight(Stomp);add(free,Strike);act(free,Strike);free.potions[0]=Potion::TouchOfInsanity;CHECK(use_potion(free,0,0));CHECK(free.phase==Phase::Combat&&cost(free,0)==0&&free.ironclad.stomp_combat_origin[0]==1);
 supply(free,8);next_turn(free);CHECK(!free.ironclad.stomp_discount[0]&&cost(free,0)==0);ready(free);
}
static void autoplay_choices_and_pillage(){
 active_kind=Havoc;active_upgrade=0;auto s=fight(Havoc);int victim=add(s,Strike),arm=add(s,Armaments,0,Zone::Draw);add(s,Defend);act(s,Havoc);
 // OnPlayWrapper captures the result before Armaments opens its choice.
 // GetResultLocationForCardPlay consumes ExhaustOnNextPlay at that boundary;
 // the saved physical-play result, rather than the flag, retains Exhaust.
 CHECK(s.expansion.depth==1&&s.expansion.choice==XChoice::IUpgrade&&s.playing==arm&&s.zones[arm]==Zone::Limbo);
 CHECK(!s.ironclad.forced_exhaust[arm]&&s.expansion.result_location[1]&&s.expansion.result_location[0]);
 auto pending=snapshot(s);Zone pile=Zone::Power;bool top=true;
 for(unsigned n=0;n<3;n++){CHECK(card_play_result(s,arm,pile,top)&&pile==Zone::Exhaust&&!top);CHECK(card_play_result(s,0,pile,top)&&pile==Zone::Discard&&!top);same(s,pending);}
 roundtrip(s);
 State broken=s;broken.ironclad.forced_exhaust[arm]=2;CHECK(!validate(broken));pick(s,1);CHECK(s.cards[victim].upgraded&&s.zones[arm]==Zone::Exhaust&&!s.ironclad.forced_exhaust[arm]);ready(s);
 auto pillage=fight(Hellraiser);act(pillage,Hellraiser);int attack=add(pillage,Pillage);add(pillage,Defend,0,Zone::Draw);add(pillage,Strike,0,Zone::Draw);act_id(pillage,attack);
 CHECK(pillage.hand.count==1&&pillage.cards[pillage.hand.ids[0]].kind==Defend&&total_hp(pillage)==80&&pillage.expansion.cards_finished==3);ready(pillage);
 auto nested=fight(Hellraiser);act(nested,Hellraiser);int pill=add(nested,Pillage);add(nested,Defend,0,Zone::Draw);add(nested,Armaments,0,Zone::Draw);add(nested,PommelStrike,0,Zone::Draw);act_id(nested,pill);
 // The outer draw remains the strike even though that strike draws a skill.
 CHECK(nested.hand.count==2&&contains(nested.hand,3)&&contains(nested.hand,2)&&nested.draw.count==0&&total_hp(nested)==77);ready(nested);
}
static void clone_and_transform_cross_character(){
 active_kind=Juggling;active_upgrade=0;auto s=fight(Juggling);act(s,Juggling);add(s,Strike);add(s,Strike);int source=add(s,Anger);s.expansion.replay[source]=1;s.expansion.combat_cost[source]=1;
 act(s,Strike);act(s,Strike);act_id(s,source);CHECK(s.hand.count==1);int clone=s.hand.ids[0];CHECK(s.cards[clone].kind==Anger&&s.expansion.replay[clone]==1&&s.expansion.combat_cost[clone]==1);ready(s);
 auto thrash=fight(Thrash);thrash.vigor=7;int victim=add(thrash,Strike);act(thrash,Thrash);CHECK(thrash.enemies[0].hp==24&&thrash.ironclad.damage_bonus[0]==6&&!thrash.vigor&&thrash.zones[victim]==Zone::Exhaust);ready(thrash);
 auto token=fight(GiantRock);token.strength=3;token.enemies[0].vulnerable=2;int value=-1;CHECK(character_description_value(token,token.cards[0],0,0,"Damage",value)&&value==34);act(token,GiantRock);CHECK(token.enemies[0].hp==12);ready(token);
 auto up=fight(GiantRock,1);act(up,GiantRock);CHECK(up.enemies[0].hp==22);ready(up);
}
static void reject_module_corruption(){
 active_kind=Armaments;active_upgrade=0;auto s=fight(Armaments);add(s,Defend);add(s,Strike);act(s,Armaments);CHECK(s.expansion.choice==XChoice::IUpgrade);roundtrip(s);
 State bad=s;bad.expansion.choice_min=0;CHECK(!validate(bad));bad=s;bad.cards[bad.expansion.choices.ids[0]].upgraded=1;CHECK(!validate(bad));
 bad=s;bad.ironclad.power_count=1;bad.ironclad.power_order[0]=(uint8_t)IPower::Rage;CHECK(!validate(bad));
 bad=s;bad.ironclad.stomp_discount[1]=1;CHECK(!validate(bad));
 bad=s;bad.ironclad.attacks_finished=1;CHECK(!validate(bad));
 for(unsigned j=0;j<s.effect_count;j++)if(s.effects[j].kind==EffectKind::Finish){bad=s;bad.effects[j]={EffectKind::Extended,-1,(int16_t)XOp::IClone,1};CHECK(!validate(bad));break;}
 pick(s,1);ready(s);
}

int main(){
 CardKind pool[CardKinds]{};unsigned count=character_pool(Character::Ironclad,pool,false);CHECK(count==85);
 for(unsigned j=0;j<count;j++)for(unsigned up=0;up<2;up++)card_contract(pool[j],up);
 for(unsigned j=0;j<count;j++)CHECK(covered[pool[j]][0]&&covered[pool[j]][1]);
 CHECK(card_strike_tag(AshenStrike)&&card_strike_tag(SetupStrike));
 CHECK(!card_can_generate(Feed)&&!card_can_generate(NotYet)&&!card_can_generate(Break)&&!card_can_generate(Corruption));
 no_global_free_x_cost();grouped_power_events();unmovable_preview_during_choice();player_power_acquisition_order();temporary_cost_order();autoplay_choices_and_pillage();clone_and_transform_cross_character();reject_module_corruption();
 std::printf("ironclad completion: %u checks passed across %u base/upgrade variants plus Giant Rock and interaction contracts\n",checks,variants);return 0;
}
