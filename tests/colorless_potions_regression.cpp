#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static unsigned checks,card_cases,potion_cases;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static void append(State&s,Card c,Zone zone){
 int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:s.discard;
 p.ids[p.count++]=(uint16_t)id;
}
static State fight(Card card={Defend,0}){
 State s{};new_run(s,71,false);s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;s.energy=20;
 s.relics=s.relic_count=s.relic_flash=0;s.deck_count=1;s.deck[0]=card;
 for(auto&e:s.enemies)e={46,46,0,0,0,0};
 append(s,card,Zone::Hand);
 for(int i=0;i<18;i++)append(s,{i%3?Defend:Strike,0},Zone::Draw);
 for(int i=0;i<5;i++)append(s,{i%2?Defend:Strike,0},Zone::Discard);
 return s;
}
static SaveData snapshot(const State&s){SaveData b{};CHECK(validate(s));CHECK(save(s,b));CHECK(b.bytes[4]==SaveVersion);return b;}
static void same(const State&s,const SaveData&b){auto a=snapshot(s);CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));}
static void rt(const State&s){auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));same(copy,b);}
static void resolve_choices(State&s){
 for(int steps=0;choice_phase(s.phase)&&steps<40;steps++){
  rt(s);const Pile&p=choice_pile(s);uint16_t ids[MaxHand]{};
  // The outcome matrix exercises taking an offer; zero now correctly means Skip.
  unsigned n=s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Offer?1:choice_min(s);
  for(unsigned i=0;i<n;i++)ids[i]=p.ids[i];
  if(s.phase==Phase::ExtendedChoice){
   auto before=snapshot(s);uint16_t wrong=65535;CHECK(!choose_extended(s,&wrong,1));same(s,before);
   CHECK(choose_extended(s,ids,n));
  }else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,ids[0]));
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::DiscardChoice)CHECK(choose_discard(s,ids,n));
  else CHECK(choose_exhaust_many(s,ids,n));
 }
 CHECK(!choice_phase(s.phase));rt(s);
}
static int count_kind(const State&s,Zone z,unsigned kind){int n=0;for(unsigned i=0;i<pile(s,z).count;i++)n+=s.cards[pile(s,z).ids[i]].kind==kind;return n;}
static int total_hp(const State&s){return s.enemies[0].hp+s.enemies[1].hp;}
static void registry(){
 unsigned n=0;for(unsigned k=0;k<CardKinds;k++)if(card_colorless(k)){++n;CHECK(card_rarity(k)==3||card_rarity(k)==4);CHECK(!card_generated(k));}
 CHECK(n==53);CHECK(PotionKinds==64);CHECK(CardKinds==558);
 for(int p=1;p<=PotionKinds;p++){auto*d=potion_definition((Potion)p);CHECK(d&&*d->key&&*d->name&&*d->description);}
 CHECK(!potion_in_pool(Potion::BoneBrew,Character::Ironclad));CHECK(!potion_in_pool(Potion::Ambergris,Character::Silent));
 CHECK(potion_in_pool(Potion::Ashwater,Character::Ironclad));CHECK(!potion_in_pool(Potion::Blood,Character::Silent));
 State s{};new_run(s,71);for(int i=0;i<3000;i++){auto p=random_potion(s);CHECK(potion_in_pool(p,s.character));}
}
static void all_cards(){
 for(unsigned k=0;k<CardKinds;k++)if(card_colorless(k))for(unsigned up=0;up<2;up++){
  auto s=fight({(CardKind)k,(uint8_t)up});s.hp=40;append(s,{Defend,0},Zone::Hand);s.gold=3;
  int before=total_hp(s);CHECK(play(s,0,0));resolve_choices(s);
  CHECK(s.zones[0]==(definitions[k].type==CardType::Power?Zone::Power:card_exhausts({(CardKind)k,(uint8_t)up})?Zone::Exhaust:Zone::Discard));
  CHECK(s.expansion.cards_finished>=1);++card_cases;
  if(definitions[k].type==CardType::Attack&&k!=GoldAxe)CHECK(total_hp(s)<before);
  switch(k){
  case Alchemize:CHECK(s.potions[0]!=Potion::Empty);break;
  case Automation:CHECK(power_value(s,XPower::Automation)==1);break;
  case Calamity:CHECK(power_value(s,XPower::Calamity)==1);break;
  case DarkShackles:CHECK(s.enemies[0].strength==-9-6*(int)up&&s.expansion.restore_strength[0]==9+6*(int)up);break;
  case Discovery:CHECK(s.hand.count==2&&cost(s,s.hand.ids[1])==0);break;
  case DramaticEntrance:CHECK(total_hp(s)==92-2*(11+4*(int)up));break;
  case Entropy:CHECK(power_value(s,XPower::Entropy)==1);break;
  case Equilibrium:CHECK(s.block==13+3*(int)up&&power_value(s,XPower::Retain)==1);break;
  case EternalArmor:CHECK(power_value(s,XPower::Plating)==9+3*(int)up);break;
  case Fasten:CHECK(card_block(s,{Defend,0})==9+2*(int)up);break;
  case Fisticuffs:CHECK(s.block==7+2*(int)up);break;
  case GoldAxe:CHECK(total_hp(s)==92);break;
  case HiddenGem:{int n=0;for(unsigned i=0;i<s.card_count;i++)n+=s.expansion.replay[i];CHECK(n==2+(int)up);break;}
  case JackOfAllTrades:CHECK(s.hand.count==2+up);break;
  case Jackpot:CHECK(s.hand.count==4);break;
  case Mayhem:CHECK(power_value(s,XPower::Mayhem)==1);break;
  case MindBlast:CHECK(total_hp(s)==74);break;
  case Nostalgia:CHECK(power_value(s,XPower::Nostalgia)==1);break;
  case Omnislice:CHECK(total_hp(s)==92-2*(8+3*(int)up));break;
  case Panache:CHECK(power_value(s,XPower::Panache)==10+4*(int)up);break;
  case PrepTime:CHECK(power_value(s,XPower::PrepTime)==4+2*(int)up);break;
  case Production:CHECK(s.energy==22+(int)up);break;
  case Prowess:CHECK(s.strength==1+(int)up&&s.dexterity==1+(int)up);break;
  case RollingBoulder:CHECK(power_value(s,XPower::RollingBoulder)==5+5*(int)up);break;
  case Salvo:CHECK(power_value(s,XPower::Retain)==1);break;
  case SecretTechnique:CHECK(s.hand.count==2&&definitions[s.cards[s.hand.ids[1]].kind].type==CardType::Skill);break;
  case SecretWeapon:CHECK(s.hand.count==2&&definitions[s.cards[s.hand.ids[1]].kind].type==CardType::Attack);break;
  case SeekerStrike:CHECK(s.hand.count==2);break;
  case Splash:{CHECK(s.hand.count==2);int id=s.hand.ids[1];Card c=s.cards[id];
   CHECK(definitions[c.kind].type==CardType::Attack&&!card_colorless(c.kind)&&!card_in_pool(c.kind,s.character)&&card_rarity(c.kind)>=2&&card_rarity(c.kind)<=4);
   CHECK(cost(s,id)==0||cost(c)==-1);CHECK(card_star_cost(s,c,id)==0||card_star_cost(s,c)==-1);break;}
  case Stratagem:CHECK(power_value(s,XPower::Stratagem)==1);break;
  case TheBomb:CHECK(s.expansion.bomb_turns[0]==3&&s.expansion.bomb_damage[0]==40+10*(int)up);break;
  case TheGambit:CHECK(s.block==50+25*(int)up&&power_value(s,XPower::Gambit)==1);break;
  case ThinkingAhead:CHECK(s.hand.count==2);break;
  case UltimateDefend:CHECK(s.block==11+4*(int)up);break;
  default:break;
  }
  rt(s);
 }
 CHECK(card_cases==106);
}
static void all_potions(){
 for(int p=1;p<=PotionKinds;p++){
  Potion kind=(Potion)p;auto s=fight();s.hp=40;s.block=6;s.potions[0]=kind;s.expansion.orb_slots=3;
  append(s,{Strike,0},Zone::Hand);append(s,{Defend,0},Zone::Hand);append(s,{Strike,0},Zone::Hand);
  const auto*d=potion_definition(kind);
  if(d->automatic){CHECK(!can_use_potion(s,0,0));s.hp=1;s.block=0;CHECK(end_turn(s));CHECK(s.hp==12&&s.potions[0]==Potion::Empty);rt(s);++potion_cases;continue;}
  auto before=snapshot(s);CHECK(can_use_potion(s,0,0));CHECK(use_potion(s,0,0));resolve_choices(s);++potion_cases;
  if(kind!=Potion::EntropicBrew)CHECK(s.potions[0]==Potion::Empty);
  CHECK(s.expansion.potion_active==0);CHECK(snapshot(s).size>=before.size-1000);
  switch(kind){
  case Potion::Fire:CHECK(s.enemies[0].hp==26);break;
  case Potion::Block:CHECK(s.block==18);break;
  case Potion::Blood:CHECK(s.hp==56);break;
  case Potion::Energy:CHECK(s.energy==22);break;
  case Potion::Strength:CHECK(s.strength==2);break;
  case Potion::Dexterity:CHECK(s.dexterity==2);break;
  case Potion::Swift:CHECK(s.hand.count==7);break;
  case Potion::Explosive:CHECK(total_hp(s)==72);break;
  case Potion::Vulnerable:CHECK(s.enemies[0].vulnerable==3);break;
  case Potion::Weak:CHECK(s.enemies[0].weak==3);break;
  case Potion::Fruit:CHECK(s.max_hp==85&&s.hp==45);break;
  case Potion::Forge:for(int i=0;i<4;i++)CHECK(s.cards[s.hand.ids[i]].upgraded);break;
  case Potion::Flex:CHECK(s.strength==5&&s.temporary_strength==5);break;
  case Potion::Speed:CHECK(s.dexterity==5&&s.temporary_dexterity==5);break;
  case Potion::Bronze:CHECK(s.thorns==3);break;
  case Potion::Fortifier:CHECK(s.block==18);break;
  case Potion::AttackPotion:case Potion::ColorlessPotion:case Potion::PowerPotion:case Potion::SkillPotion:
   CHECK(s.hand.count==5&&cost(s,s.hand.ids[4])==0);break;
  case Potion::BeetleJuice:CHECK(s.expansion.beetle[0]==4&&enemy_damage(s,0)==8);break;
  case Potion::BottledPotential:CHECK(s.hand.count==5&&s.discard.count==0);break;
  case Potion::Clarity:CHECK(s.hand.count==5&&power_value(s,XPower::Clarity)==3);break;
  case Potion::CureAll:CHECK(s.hand.count==6&&s.energy==21);break;
  case Potion::DistilledChaos:CHECK(s.expansion.cards_finished==3);break;
  case Potion::DropletOfPrecognition:CHECK(s.hand.count==5);break;
  case Potion::Duplicator:CHECK(power_value(s,XPower::Duplicator)==1);break;
  case Potion::EntropicBrew:CHECK(s.potions[0]!=Potion::Empty&&s.potions[1]!=Potion::Empty);break;
  case Potion::FyshOil:CHECK(s.strength==1&&s.dexterity==1);break;
  case Potion::GamblersBrew:CHECK(s.hand.count==4);break;
  case Potion::GigantificationPotion:CHECK(power_value(s,XPower::Gigantic)==1);break;
  case Potion::HeartOfIron:CHECK(power_value(s,XPower::Plating)==7);break;
  case Potion::LiquidMemories:CHECK(s.hand.count==5&&cost(s,s.hand.ids[4])==0);break;
  case Potion::LuckyTonic:CHECK(power_value(s,XPower::Buffer)==1);break;
  case Potion::MazalethsGift:CHECK(power_value(s,XPower::Ritual)==1);break;
  case Potion::OrobicAcid:CHECK(s.hand.count==7);for(int i=4;i<7;i++)CHECK(cost(s,s.hand.ids[i])==0);break;
  case Potion::PotionOfBinding:for(int i=0;i<MaxEnemies;i++)CHECK(s.enemies[i].weak==(i<s.enemy_count?1:0)&&s.enemies[i].vulnerable==(i<s.enemy_count?1:0));break;
  case Potion::PowderedDemise:CHECK(s.expansion.demise[0]==9);break;
  case Potion::RadiantTincture:CHECK(s.energy==21&&power_value(s,XPower::Radiance)==3);break;
  case Potion::RegenPotion:CHECK(power_value(s,XPower::Regen)==5);break;
  case Potion::ShacklingPotion:for(int i=0;i<MaxEnemies;i++)CHECK(s.enemies[i].strength==(i<s.enemy_count?-7:0));break;
  case Potion::ShipInABottle:CHECK(s.block==16&&s.block_next_turn==10);break;
  case Potion::SneckoOil:CHECK(s.hand.count==10);for(int i=0;i<10;i++)CHECK(cost(s,s.hand.ids[i])>=0&&cost(s,s.hand.ids[i])<=3);break;
  case Potion::StableSerum:CHECK(power_value(s,XPower::Retain)==2);break;
  case Potion::TouchOfInsanity:CHECK(s.expansion.combat_cost[0]==1);break;
  case Potion::SoldiersStew:for(unsigned i=0;i<s.card_count;i++)CHECK(s.expansion.replay[i]==(unsigned)card_strike_tag(s.cards[i].kind));break;
  case Potion::Ashwater:CHECK(s.hand.count==4);break;
  case Potion::PoisonPotion:CHECK(s.enemies[0].poison==6);break;
  case Potion::GhostInAJar:CHECK(power_value(s,XPower::Intangible)==1);break;
  case Potion::CunningPotion:CHECK(count_kind(s,Zone::Hand,Shiv)==3);break;
  case Potion::Ambergris:CHECK(s.hp==80&&s.expansion.skip_enemy==1);break;
  case Potion::FoulPotion:CHECK(total_hp(s)==68&&s.hp==34&&s.block==0);break;
  case Potion::GlowwaterPotion:CHECK(s.hand.count==10&&s.exhaust.count==4);break;
  case Potion::PotionShapedRock:CHECK(s.enemies[0].hp==31);break;
  case Potion::BoneBrew:CHECK(s.expansion.osty_hp==15&&s.expansion.osty_max_hp==15);break;
  case Potion::PotOfGhouls:CHECK(count_kind(s,Zone::Hand,Soul)==2);break;
  case Potion::PotionOfDoom:CHECK(s.expansion.doom[0]==33);break;
  case Potion::FocusPotion:CHECK(power_value(s,XPower::Focus)==2);break;
  case Potion::EssenceOfDarkness:CHECK(s.expansion.orb_count==3&&s.expansion.dark[0]==6);break;
  case Potion::PotionOfCapacity:CHECK(s.expansion.orb_slots==5);break;
  case Potion::StarPotion:CHECK(s.expansion.stars==3);break;
  case Potion::CosmicConcoction:CHECK(s.hand.count==7);for(int i=4;i<7;i++)CHECK(s.cards[s.hand.ids[i]].upgraded&&card_colorless(s.cards[s.hand.ids[i]].kind));break;
  case Potion::KingsCourage:CHECK(count_kind(s,Zone::Hand,SovereignBlade)==1&&s.expansion.blade_damage[s.hand.ids[4]]==25);break;
  default:CHECK(false);
  }
  rt(s);
 }
 CHECK(potion_cases==64);
}
static void interactions(){
 auto s=fight({GoldAxe,0});s.expansion.cards_finished=7;s.gold=999;CHECK(play(s,0,0));CHECK(s.enemies[0].hp==39);rt(s);
 // The locked original includes overkill in the amount converted/copied.
 s=fight({Fisticuffs,0});s.strength=90;s.enemies[0].hp=3;s.enemies[0].block=2;CHECK(play(s,0,0));CHECK(s.block==97);rt(s);
 s=fight({Omnislice,0});s.strength=90;s.enemies[0].hp=3;s.enemies[0].block=2;CHECK(play(s,0,0));CHECK(s.enemies[1].hp==0&&s.expansion.last_damage==98);rt(s);
 // Player Plating grants end-turn block, then loses one at the next start.
 s=fight({EternalArmor,0});CHECK(play(s,0,0));CHECK(end_turn(s));CHECK(s.block==0&&power_value(s,XPower::Plating)==8);rt(s);
 s=fight({TwinStrike,0});s.potions[0]=Potion::GigantificationPotion;CHECK(use_potion(s,0,0));CHECK(damage(s,{TwinStrike,0},0)==15);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==16&&!power_value(s,XPower::Gigantic));rt(s);
 s=fight({Discovery,0});CHECK(play(s,0,0));CHECK(s.phase==Phase::ExtendedChoice);auto saved=snapshot(s);State copy{};CHECK(load(copy,saved.bytes,saved.size));uint16_t id=s.expansion.choices.ids[1];CHECK(choose_extended(s,&id,1)&&choose_extended(copy,&id,1));same(copy,snapshot(s));
 s=fight({Catastrophe,0});s.draw.count=0;s.discard.count=0;s.card_count=1;append(s,{Discovery,0},Zone::Draw);CHECK(play(s,0,0));CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.depth==1);rt(s);resolve_choices(s);CHECK(s.expansion.cards_finished==2&&s.expansion.depth==0);rt(s);
 s=fight({Defend,0});s.potions[0]=Potion::ColorlessPotion;CHECK(use_potion(s,0,0));CHECK(s.phase==Phase::ExtendedChoice&&s.playing==-1);rt(s);resolve_choices(s);CHECK(!s.expansion.potion_active);rt(s);
 s=fight({Defend,0});s.potions[0]=Potion::FoulPotion;s.enemies[0].hp=s.enemies[1].hp=1;s.hp=1;s.block=0;CHECK(use_potion(s,0,0));CHECK(s.phase==Phase::Dead);rt(s);
 s=fight({Defend,0});s.potions[0]=Potion::FoulPotion;s.phase=Phase::Shop;int gold=s.gold;CHECK(use_potion(s,0,0));CHECK(s.gold==gold+100&&s.hp==80);rt(s);
 // Malformed suspended frames cannot overrun the fixed stack during validation.
 s=fight();s.expansion.depth=255;CHECK(!validate(s));
}
static void legacy_v11(const char*path){
 // Produced by the unmodified afe33815 v11 writer, with Entropy played before
 // Rolling Boulder. That writer did not persist their acquisition boundary.
 FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t> bytes;unsigned value;
 while(std::fscanf(f,"%2x",&value)==1)bytes.push_back((uint8_t)value);
 std::fclose(f);
 CHECK(bytes.size()==12580&&bytes[4]==11);
 State s{};CHECK(load(s,bytes.data(),bytes.size()));
 CHECK(s.phase==Phase::Combat&&s.cards[s.powers.ids[0]].kind==Entropy);
 CHECK(power_value(s,XPower::Entropy)==1&&s.expansion.boulder[0]==5);
 CHECK(s.expansion.entropy_boulder_prefix==1);
 auto migrated=snapshot(s);CHECK(migrated.size>bytes.size()+4);
 // The v11 payload remains byte-identical; v12 appended the boundary and
 // v13 appends explicit character state without changing that prefix.
 CHECK(!std::memcmp(migrated.bytes+8,bytes.data()+8,bytes.size()-12));rt(s);
 auto invalid=s;invalid.expansion.entropy_boulder_prefix=2;SaveData refused{};
 CHECK(!validate(invalid)&&!save(invalid,refused));
 CHECK(end_turn(s));CHECK(s.phase==Phase::ExtendedChoice&&total_hp(s)==82);
 CHECK(s.expansion.boulder[0]==10);rt(s);resolve_choices(s);rt(s);
}
int main(int argc,char**argv){registry();all_cards();all_potions();interactions();if(argc>1)legacy_v11(argv[1]);std::printf("colorless/potions: %u checks; %u card variants; %u potions\n",checks,card_cases,potion_cases);}
