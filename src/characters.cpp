#include "core.hpp"
namespace spire {
const CharacterDefinition character_definitions[CharacterKinds]={
 {"IRONCLAD","铁甲战士",80,BurningBlood},
 {"SILENT","静默猎手",70,RingOfTheSnake},
 {"DEFECT","故障机器人",75,CrackedCore},
 {"REGENT","储君",75,DivineRight},
 {"NECROBINDER","亡灵契约师",66,BoundPhylactery}
};
CardColor card_color(unsigned kind){
 if(kind==Soul||kind==SovereignBlade||kind==GiantRock||(kind>=Dazed&&kind<CardKinds))return CardColor::Token;
 if(kind>=DefectAdaptiveStrike&&kind<=DefectZap)return CardColor::Defect;
 if(kind>=RegentAlignment&&kind<=RegentWroughtInWar)return CardColor::Regent;
 if(kind>=NecroAfterlife&&kind<=NecroWisp)return CardColor::Necrobinder;
 if(card_colorless(kind))return CardColor::Colorless;
 return ((kind>=StrikeSilent&&kind<=Shiv)||(kind>=DodgeAndRoll&&kind<=Predator)||(kind>=Abrasive&&kind<=WraithForm))?CardColor::Silent:CardColor::Ironclad;
}
bool card_generated(unsigned kind){return kind==Shiv||kind==Soul||kind==SovereignBlade||kind==GiantRock||(kind>=Dazed&&kind<CardKinds);}
bool card_upgradeable(Card c){return c.kind<CardKinds&&!c.upgraded&&!(c.kind==Dazed||c.kind==Wound||c.kind==Slimed||c.kind==Burn||c.kind==Void||c.kind==Debris);}
bool card_removable(Card c){return c.kind<CardKinds&&!((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&64);}
bool card_transformable(Card c){return card_removable(c);}
bool card_in_pool(unsigned kind,Character character){
 if(kind>=CardKinds||card_generated(kind))return false;
 if(character==Character::Defect)return kind>=DefectAdaptiveStrike&&kind<=DefectZap;
 if(character==Character::Regent)return kind>=RegentAlignment&&kind<=RegentWroughtInWar;
 if(character==Character::Necrobinder)return kind>=NecroAfterlife&&kind<=NecroWisp;
 if(character!=Character::Ironclad&&character!=Character::Silent)return false;
 return character==Character::Ironclad?
     (kind<=FiendFire||(kind>=Headbutt&&kind<=DemonForm)||(kind>=Aggression&&kind<=Vicious)):
     ((kind>=StrikeSilent&&kind<Shiv)||(kind>=DodgeAndRoll&&kind<=Predator)||(kind>=Abrasive&&kind<=WraithForm));
}
unsigned character_pool(Character character,CardKind* out,bool rewards_only){
 unsigned count=0;
 for(unsigned i=0;i<CardKinds;i++)if(card_in_pool(i,character)&&(!rewards_only||(card_rarity(i)>=2&&card_rarity(i)<=4))){
  if(out)out[count]=CardKind(i);++count;
 }
 return count;
}
unsigned random_reward_card(State&s){
 CardKind pool[CardKinds];unsigned n=character_pool(s.character,pool,true);
 return n?pool[random_below(s.rng_world,n)]:Strike;
}
}
