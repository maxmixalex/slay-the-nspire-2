#include "core.hpp"
#include "original_events.hpp"
namespace spire {
static int cap(int a,int b){return a<b?a:b;}
bool potion_in_pool(Potion p,Character c){
 const auto*d=potion_definition(p);if(!d)return false;
 if(p==Potion::Blood)return c==Character::Ironclad;
 return !d->pool||(d->pool==1&&c==Character::Ironclad)||(d->pool==2&&c==Character::Silent)||(d->pool==3&&c==Character::Necrobinder)||(d->pool==4&&c==Character::Defect)||(d->pool==5&&c==Character::Regent);
}
Potion random_potion(State&s,bool in_combat){
 // Pinned PotionFactory selects the rarity first: common 65%, uncommon 25%,
 // rare 10%. The adapted expedition keeps its own deterministic RNG stream.
 const unsigned roll=random_below(s.rng_world,100);
 const unsigned rarity=roll<10?3:roll<35?2:1;
 Potion pool[PotionKinds];unsigned n=0;
 for(unsigned p=1;p<=PotionKinds;p++){
  Potion potion=(Potion)p;
  if(!potion_in_pool(potion,s.character)||potion_definition(potion)->rarity!=rarity)continue;
  if(in_combat&&(potion==Potion::FairyInABottle||potion==Potion::Fruit||potion==Potion::RegenPotion))continue;
  pool[n++]=potion;
 }
 return n?pool[random_below(s.rng_world,n)]:Potion::Empty;
}
unsigned potion_slot_count(const State&s){return s.potion_slots>=LegacyPotionSlots&&s.potion_slots<=MaxPotionSlots?s.potion_slots:LegacyPotionSlots;}
Potion potion_at(const State&s,unsigned slot){return slot<potion_slot_count(s)?s.potions[slot]:Potion::Empty;}
bool potion_set(State&s,unsigned slot,Potion p){if(slot>=potion_slot_count(s)||unsigned(p)>PotionKinds)return false;s.potions[slot]=p;return true;}
bool add_potion_slots(State&s,unsigned count){
 unsigned before=potion_slot_count(s);if(count>MaxPotionSlots-before)return false;
 for(unsigned i=before;i<before+count;++i)s.potions[i]=Potion::Empty;
 s.potion_slots=(uint8_t)(before+count);return true;
}
bool procure_potion_at(State&s,Potion p,unsigned slot){
 if(!potion_definition(p)||slot>=potion_slot_count(s)||s.potions[slot]!=Potion::Empty||has_world_relic(s,world_relic_id("SOZU")))return false;
 s.potions[slot]=p;notify_history(s,RunHistoryEvent::PotionObtained,unsigned(p));world_relic_hook(s,WorldRelicHook::AfterPotionProcured,(int)p,(int)slot);return true;
}
bool procure_potion(State&s,Potion p){if(!potion_definition(p))return false;for(unsigned i=0;i<potion_slot_count(s);++i)if(s.potions[i]==Potion::Empty)return procure_potion_at(s,p,i);return false;}

void execute_potion(State&s,Potion p,int t){
 auto&x=s.expansion;
 auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};
 auto pw=[&](XPower k,int n){op(XOp::Power,(int)k,n);};
 switch(p){
 case Potion::AttackPotion:op(XOp::Offer,1,1);break;
 case Potion::BeetleJuice:op(XOp::AllCharactersBeetle,t,4);break;
 case Potion::BottledPotential:op(XOp::ShuffleAll);op(XOp::Draw,5);break;
 case Potion::Clarity:op(XOp::Draw,1);pw(XPower::Clarity,3);break;
 case Potion::ColorlessPotion:op(XOp::Offer,4,1);break;
 case Potion::CureAll:s.energy=(int16_t)cap(999,s.energy+1);op(XOp::Draw,2);break;
 case Potion::DistilledChaos:op(XOp::AutoTop,3);break;
 case Potion::DropletOfPrecognition:op(XOp::ChooseDraw);break;
 case Potion::Duplicator:pw(XPower::Duplicator,1);break;
 // The original explicitly calls CreateRandomPotionOutOfCombat for each empty
 // slot, even when drunk in combat. It permits recovery bottles and repeats.
 case Potion::EntropicBrew:for(unsigned i=0;i<potion_slot_count(s);++i)if(s.potions[i]==Potion::Empty)procure_potion_at(s,random_potion(s,false),i);break;
 case Potion::FairyInABottle:break; // automatic only; cannot enter the manual path
 case Potion::FyshOil:op(XOp::Strength,1);op(XOp::Dexterity,1);break;
 case Potion::GamblersBrew:op(XOp::DiscardRedraw);break;
 case Potion::GigantificationPotion:pw(XPower::Gigantic,1);break;
 case Potion::HeartOfIron:pw(XPower::Plating,7);break;
 case Potion::LiquidMemories:op(XOp::ChooseDiscard,0,1);break;
 case Potion::LuckyTonic:pw(XPower::Buffer,1);break;
 case Potion::MazalethsGift:pw(XPower::Ritual,1);break;
 case Potion::OrobicAcid:for(int i=1;i<=3;i++)op(XOp::Generate,i,1);break;
 case Potion::PotionOfBinding:for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp){apply_weak(s,i,1);apply_vulnerable(s,i,1);}break;
 case Potion::PowderedDemise:op(XOp::Demise,t,9);break;
 case Potion::PowerPotion:op(XOp::Offer,3,1);break;
 case Potion::RadiantTincture:s.energy=(int16_t)cap(999,s.energy+1);pw(XPower::Radiance,3);break;
 case Potion::RegenPotion:pw(XPower::Regen,5);break;
 case Potion::ShacklingPotion:for(int i=0;i<s.enemy_count;i++)op(XOp::StrengthLoss,i,7);break;
 case Potion::ShipInABottle:gain_unpowered_block(s,10);s.block_next_turn=(int16_t)cap(9999,s.block_next_turn+10);power_order_set(s,0,16,s.block_next_turn!=0);break;
 case Potion::SkillPotion:op(XOp::Offer,2,1);break;
 case Potion::SneckoOil:op(XOp::Draw,7);op(XOp::RandomCosts);break;
 case Potion::StableSerum:pw(XPower::Retain,2);break;
 case Potion::TouchOfInsanity:op(XOp::Free);break;
 case Potion::SoldiersStew:op(XOp::StrikeReplay);break;
 case Potion::Ashwater:op(XOp::ExhaustAny);break;
 case Potion::PoisonPotion:apply_poison(s,t,6);break;
 case Potion::GhostInAJar:pw(XPower::Intangible,1);break;
 case Potion::CunningPotion:op(XOp::Create,3,Shiv+256);break;
 case Potion::Ambergris:heal(s,s.max_hp/2);if(s.phase==Phase::Combat&&x.skip_enemy<255)++x.skip_enemy;break;
 case Potion::FoulPotion:
  if(s.phase==Phase::Shop)gain_gold(s,100);
  else if(original_event_can_use_foul(s))original_event_foul_potion(s);
  else{op(XOp::AllDamage,12);op(XOp::PlayerDamage,12);}break;
 case Potion::GlowwaterPotion:op(XOp::ExhaustHand);op(XOp::Draw,10);break;
 case Potion::PotionShapedRock:damage_unpowered(s,t,15);break;
 case Potion::BoneBrew:op(XOp::Summon,15);break;
 case Potion::PotOfGhouls:op(XOp::Create,2,Soul);break;
 case Potion::PotionOfDoom:op(XOp::NDoom,t,33);break;
 case Potion::FocusPotion:pw(XPower::Focus,2);break;
 case Potion::EssenceOfDarkness:op(XOp::ChannelDark,x.orb_slots);break;
 case Potion::PotionOfCapacity:op(XOp::DSlots,2);break;
 case Potion::StarPotion:op(XOp::RStars,3);break;
 case Potion::CosmicConcoction:op(XOp::Generate,4,(3<<2)|2);break;
 case Potion::KingsCourage:op(XOp::Forge,15);break;
 default:break;
 }
}
}
