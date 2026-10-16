#include "core.hpp"
namespace spire {
const CharacterDefinition character_definitions[CharacterKinds]={
 {"IRONCLAD","铁甲战士",80,BurningBlood},
 {"SILENT","静默猎手",70,RingOfTheSnake}
};
CardColor card_color(unsigned kind){
 if(kind==Soul||kind==SovereignBlade)return CardColor::Token;
 if(card_colorless(kind))return CardColor::Colorless;
 return ((kind>=StrikeSilent&&kind<=Shiv)||(kind>=DodgeAndRoll&&kind<=Predator))?CardColor::Silent:CardColor::Ironclad;
}
bool card_generated(unsigned kind){return kind==Shiv||kind==Soul||kind==SovereignBlade;}
bool card_in_pool(unsigned kind,Character character){
 if(kind>=CardKinds||card_generated(kind))return false;
 return character==Character::Ironclad?
     (kind<=FiendFire||(kind>=Headbutt&&kind<=DemonForm)):
     ((kind>=StrikeSilent&&kind<Shiv)||(kind>=DodgeAndRoll&&kind<=Predator));
}
unsigned character_pool(Character character,uint8_t* out,bool rewards_only){
 unsigned count=0;
 for(unsigned i=0;i<CardKinds;i++)if(card_in_pool(i,character)&&(!rewards_only||card_rarity(i)>=2)){
  if(out)out[count]=uint8_t(i);++count;
 }
 return count;
}
unsigned random_reward_card(State&s){
 uint8_t pool[CardKinds];unsigned n=character_pool(s.character,pool,true);
 return n?pool[random_below(s.rng_world,n)]:Strike;
}
}
