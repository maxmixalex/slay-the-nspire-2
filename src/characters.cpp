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
 if(kind>=EventCardBadLuck&&kind<CardKinds)return event_card_color(kind);
 if(kind==Soul||kind==SovereignBlade||kind==GiantRock||(kind>=Dazed&&kind<CardKinds))return CardColor::Token;
 if(kind>=DefectAdaptiveStrike&&kind<=DefectZap)return CardColor::Defect;
 if(kind>=RegentAlignment&&kind<=RegentWroughtInWar)return CardColor::Regent;
 if(kind>=NecroAfterlife&&kind<=NecroWisp)return CardColor::Necrobinder;
 if(card_colorless(kind))return CardColor::Colorless;
 return ((kind>=StrikeSilent&&kind<=Shiv)||(kind>=DodgeAndRoll&&kind<=Predator)||(kind>=Abrasive&&kind<=WraithForm))?CardColor::Silent:CardColor::Ironclad;
}
bool card_generated(unsigned kind){return kind==Shiv||kind==Soul||kind==SovereignBlade||kind==GiantRock||(kind>=Dazed&&kind<CardKinds);}
bool card_upgradeable(Card c){
 if(c.kind>=CardKinds||c.upgraded)return false;
 if(c.kind>=EventCardBadLuck&&(definitions[c.kind].type==CardType::Curse||definitions[c.kind].type==CardType::Quest||definitions[c.kind].type==CardType::Status))return false;
 return !(c.kind==Dazed||c.kind==Wound||c.kind==Slimed||c.kind==Burn||c.kind==Void||c.kind==Debris);
}
bool card_removable(Card c){return c.kind<CardKinds&&!((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&64);}
bool card_transformable(Card c){return card_removable(c);}
CardType card_type(const State&s,Card c,int instance,int deck_index){
 if(instance<0&&deck_index<0&&s.playing>=0&&s.playing<s.card_count&&s.cards[s.playing].kind==c.kind&&s.cards[s.playing].upgraded==c.upgraded)instance=s.playing;
 CardType result=c.kind<CardKinds?definitions[c.kind].type:CardType::Status;
 event_card_type(s,c,instance,deck_index,result);return result;
}
uint8_t card_keywords(const State&s,Card c,int instance,int deck_index){
 if(c.kind>=CardKinds)return 128;
 if(instance<0&&deck_index<0&&s.playing>=0&&s.playing<s.card_count&&s.cards[s.playing].kind==c.kind&&s.cards[s.playing].upgraded==c.upgraded)instance=s.playing;
 const auto&d=definitions[c.kind];uint8_t bits=c.upgraded?d.upgraded_keywords:d.keywords;
 if(d.exhausts)bits|=1;
 if(c.kind==Purity||(c.kind==Scrawl&&c.upgraded))bits|=4;
 if(c.kind==Backstab||(c.kind==InfiniteBlades&&c.upgraded))bits|=2;
 return event_card_keywords(s,c,instance,deck_index,bits);
}
bool card_removable(const State&s,Card c,int deck_index){return c.kind<CardKinds&&!(card_keywords(s,c,-1,deck_index)&64);}
bool card_transformable(const State&s,Card c,int deck_index){return card_removable(s,c,deck_index);}
bool card_retains(const State&s,Card c,int instance,int deck_index){return (card_keywords(s,c,instance,deck_index)&4)!=0;}
bool card_innate(const State&s,Card c,int instance,int deck_index){return (card_keywords(s,c,instance,deck_index)&2)!=0;}
bool card_exhausts(const State&s,Card c,int instance,int deck_index){return (card_keywords(s,c,instance,deck_index)&1)!=0;}
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
