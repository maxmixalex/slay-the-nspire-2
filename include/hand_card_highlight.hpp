#pragma once
#include "core.hpp"
#include "card_highlight.hpp"
namespace spire {
// NHandCardHolder's playable, special-condition and warning colors. These
// are observations of existing combat facts; they do not call card actions.
inline unsigned hand_card_glow(const State&s,Card card,int instance,bool playable){
 bool red=false,gold=false;
 switch(card.kind){
 case EventCardEnthralled:red=true;break;
 case EventCardNormality:red=s.event_cards.plays_started>=3;break;
 case NecroSqueeze:case NecroFetch:case NecroSnap:case NecroSicEm:case SweepingGaze:
 case NecroSacrifice:case NecroRattle:case NecroProtector:case NecroHighFive:
 case NecroRightHandHand:case NecroBoneShards:case NecroFlatten:case NecroUnleash:case NecroPoke:
  red=s.expansion.osty_hp<=0;break;
 default:break;
 }
 if(red)return 3;if(!playable)return 0;
 switch(card.kind){
 case EventCardClash:case GrandFinale:gold=true;break;
 case DefectFtl:gold=s.defect.cards_finished<unsigned(card.upgraded?4:3);break;
 case RegentHeavenlyDrill:gold=s.energy>=4;break;
 case EvilEye:gold=s.ironclad.exhausted_turn>0;break;
 case NecroDeathsDoor:gold=s.necro.doom_applied_turn!=0;break;
 case NecroFlatten:gold=s.necro.osty_attacks_turn!=0;break;
 case NecroFetch:gold=instance>=0&&instance<s.card_count&&!s.necro.fetch_played[instance];break;
 case Spite:gold=s.ironclad.hp_losses_turn!=0;break;
 case PactsEnd:gold=s.exhaust.count>=3;break;
 case Restlessness:gold=s.hand.count==1;break;
 case Impatience:gold=true;for(unsigned i=0;i<s.hand.count;++i)if(card_type(s,s.cards[s.hand.ids[i]],s.hand.ids[i])==CardType::Attack)gold=false;break;
 case BubbleBubble:case Dismantle:case DefectGoForTheEyes:
  for(int i=0;i<s.enemy_count;++i)if(s.enemies[i].hp>0){
   if(card.kind==BubbleBubble)gold|=s.enemies[i].poison>0;
   else if(card.kind==Dismantle)gold|=s.enemies[i].vulnerable>0;
   else gold|=(enemy_intent(s,i).flags&EnemyIntentAttack)!=0;
  }break;
 default:break;
 }
 return gold?2u:1u;
}
struct HandCardHighlight {
 uint32_t changed=0,flashed=0;
 uint16_t from=0;
 uint8_t kind=0,color=1;
 bool selected=false,flashing=false;
 unsigned width(uint32_t now)const{
  const unsigned age=now-changed,target=kind?card_highlight::MaximumWidth:0;
  unsigned result=target;
  if(age<500){const unsigned q=(500-age)*1024u/500u;
   const unsigned amount=kind?1024u-q*q/1024u*q/1024u:age*1024u/500u;
   result=unsigned(int(from)+(int(target)-int(from))*int(amount)/1024);
  }
  if(flashing&&now-flashed<450){const unsigned t=now-flashed;
   if(t<100)result=card_highlight::MaximumWidth+(card_highlight::MaximumWidth*t/100u);
   else {const unsigned q=(450-t)*1024u/350u,rest=q*q/1024u*q/1024u;result=card_highlight::MaximumWidth+card_highlight::MaximumWidth*rest/1024u;}
  }return result;
 }
 unsigned observe(unsigned next,bool active_selection,uint32_t now){
  if(next!=kind){from=uint16_t(width(now));kind=uint8_t(next);if(next)color=uint8_t(next);else flashing=false;changed=now;}
  if(active_selection&&!selected&&next){flashing=true;flashed=now;}
  selected=active_selection;return width(now);
 }
};
}
