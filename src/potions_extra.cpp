#include "core.hpp"
namespace spire {
static int cap(int a,int b){return a<b?a:b;}
bool potion_in_pool(Potion p,Character c){
 const auto*d=potion_definition(p);if(!d)return false;
 if(p==Potion::Blood)return c==Character::Ironclad;
 return !d->pool||(d->pool==1&&c==Character::Ironclad)||(d->pool==2&&c==Character::Silent);
}
Potion random_potion(State&s){
 Potion pool[PotionKinds];unsigned n=0;for(unsigned p=1;p<=PotionKinds;p++)if(potion_in_pool((Potion)p,s.character))pool[n++]=(Potion)p;
 return n?pool[random_below(s.rng_world,n)]:Potion::Empty;
}
bool procure_potion(State&s,Potion p){if(!potion_definition(p))return false;for(auto&slot:s.potions)if(slot==Potion::Empty){slot=p;return true;}return false;}
void execute_potion(State&s,Potion p,int t){
 auto&x=s.expansion;
 auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};
 auto pw=[&](XPower k,int n){op(XOp::Power,(int)k,n);};
 switch(p){
 case Potion::AttackPotion:op(XOp::Offer,1,1);break;
 case Potion::BeetleJuice:x.beetle[t]=(int16_t)cap(9999,x.beetle[t]+4);break;
 case Potion::BottledPotential:op(XOp::ShuffleAll);op(XOp::Draw,5);break;
 case Potion::Clarity:op(XOp::Draw,1);pw(XPower::Clarity,3);break;
 case Potion::ColorlessPotion:op(XOp::Offer,4,1);break;
 case Potion::CureAll:s.energy=(int16_t)cap(999,s.energy+1);op(XOp::Draw,2);break;
 case Potion::DistilledChaos:for(int i=0;i<3;i++)op(XOp::AutoTop);break;
 case Potion::DropletOfPrecognition:op(XOp::ChooseDraw);break;
 case Potion::Duplicator:pw(XPower::Duplicator,1);break;
 case Potion::EntropicBrew:for(auto&slot:s.potions)if(slot==Potion::Empty)slot=random_potion(s);break;
 case Potion::FairyInABottle:break; // automatic only; cannot enter the manual path
 case Potion::FyshOil:op(XOp::Strength,1);op(XOp::Dexterity,1);break;
 case Potion::GamblersBrew:op(XOp::DiscardRedraw);break;
 case Potion::GigantificationPotion:pw(XPower::Gigantic,1);break;
 case Potion::HeartOfIron:pw(XPower::Plating,7);break;
 case Potion::LiquidMemories:op(XOp::ChooseDiscard,0,1);break;
 case Potion::LuckyTonic:pw(XPower::Buffer,1);break;
 case Potion::MazalethsGift:pw(XPower::Ritual,1);break;
 case Potion::OrobicAcid:for(int i=1;i<=3;i++)op(XOp::Generate,i,1);break;
 case Potion::PotionOfBinding:for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp){s.enemies[i].weak=(int16_t)cap(9999,s.enemies[i].weak+1);s.enemies[i].vulnerable=(int16_t)cap(9999,s.enemies[i].vulnerable+1);}break;
 case Potion::PowderedDemise:op(XOp::Demise,t,9);break;
 case Potion::PowerPotion:op(XOp::Offer,3,1);break;
 case Potion::RadiantTincture:s.energy=(int16_t)cap(999,s.energy+1);pw(XPower::Radiance,3);break;
 case Potion::RegenPotion:pw(XPower::Regen,5);break;
 case Potion::ShacklingPotion:for(int i=0;i<s.enemy_count;i++)op(XOp::StrengthLoss,i,7);break;
 case Potion::ShipInABottle:s.block=(int16_t)cap(9999,s.block+10);s.block_next_turn=(int16_t)cap(9999,s.block_next_turn+10);break;
 case Potion::SkillPotion:op(XOp::Offer,2,1);break;
 case Potion::SneckoOil:op(XOp::Draw,7);op(XOp::RandomCosts);break;
 case Potion::StableSerum:pw(XPower::Retain,2);break;
 case Potion::TouchOfInsanity:op(XOp::Free);break;
 case Potion::SoldiersStew:op(XOp::StrikeReplay);break;
 case Potion::Ashwater:op(XOp::ExhaustAny);break;
 case Potion::PoisonPotion:if(s.enemies[t].hp)s.enemies[t].poison=(int16_t)cap(9999,s.enemies[t].poison+6);break;
 case Potion::GhostInAJar:pw(XPower::Intangible,1);break;
 case Potion::CunningPotion:op(XOp::Create,3,Shiv+256);break;
 case Potion::Ambergris:heal(s,s.max_hp/2);if(s.phase==Phase::Combat&&x.skip_enemy<255)++x.skip_enemy;break;
 case Potion::FoulPotion:if(s.phase==Phase::Shop)gain_gold(s,100);else{op(XOp::AllDamage,12);op(XOp::PlayerDamage,12);}break;
 case Potion::GlowwaterPotion:op(XOp::ExhaustHand);op(XOp::Draw,10);break;
 case Potion::PotionShapedRock:damage_unpowered(s,t,15);break;
 case Potion::BoneBrew:op(XOp::Summon,15);break;
 case Potion::PotOfGhouls:op(XOp::Create,2,Soul);break;
 case Potion::PotionOfDoom:if(s.enemies[t].hp)x.doom[t]=(int16_t)cap(9999,x.doom[t]+33);break;
 case Potion::FocusPotion:pw(XPower::Focus,2);break;
 case Potion::EssenceOfDarkness:op(XOp::ChannelDark,x.orb_slots);break;
 case Potion::PotionOfCapacity:x.orb_slots=(uint8_t)cap(MaxOrbs,x.orb_slots+2);break;
 case Potion::StarPotion:x.stars=(int16_t)cap(9999,x.stars+3);break;
 case Potion::CosmicConcoction:for(int i=0;i<3;i++)op(XOp::Generate,4,2);break;
 case Potion::KingsCourage:op(XOp::Forge,15);break;
 default:break;
 }
}
}
