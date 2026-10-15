#include "core.hpp"
namespace spire {
static int cap(int x,int top){return x>top?top:x;}
bool potion_in_pool(Potion p,Character character){
 const auto*d=potion_definition(p);if(!d)return false;
 if(p==Potion::Blood)return character==Character::Ironclad;
 return d->pool==0||(d->pool==1&&character==Character::Ironclad)||(d->pool==2&&character==Character::Silent);
}
Potion random_potion(State&s){
 Potion pool[PotionKinds];unsigned n=0;
 for(unsigned i=1;i<=PotionKinds;i++)if(potion_in_pool((Potion)i,s.character))pool[n++]=(Potion)i;
 return n?pool[random_below(s.rng_world,n)]:Potion::Empty;
}
bool procure_potion(State&s,Potion p){
 if(!potion_definition(p))return false;
 for(unsigned i=0;i<s.potion_slots;i++)if(s.potions[i]==Potion::Empty){s.potions[i]=p;return true;}return false;
}
void bulk_use_potion(State&s,Potion p,int target){
 auto op=[&](BulkOp k,int a=0,int b=0){bulk_enqueue(s,k,a,b);};
 auto power=[&](BulkPower k,int n){op(BulkOp::GrantPower,(int)k,n);};
 switch(p){
 case Potion::EntropicBrew:
  for(unsigned i=0;i<s.potion_slots;i++)if(s.potions[i]==Potion::Empty)s.potions[i]=random_potion(s);break;
 case Potion::Ambergris:
  heal(s,s.max_hp/2);if(s.phase==Phase::Combat&&s.bulk.skip_enemy<255)++s.bulk.skip_enemy;break;
 case Potion::AttackPotion:op(BulkOp::Offer,1,1);break;
 case Potion::BeetleJuice:s.bulk.enemy_beetle[target]=(int16_t)cap(s.bulk.enemy_beetle[target]+4,9999);break;
 case Potion::BottledPotential:op(BulkOp::ShuffleAll);op(BulkOp::Draw,5);break;
 case Potion::Clarity:op(BulkOp::Draw,1);power(BulkPower::Clarity,3);break;
 case Potion::ColorlessPotion:op(BulkOp::Offer,4,1);break;
 case Potion::CureAll:s.energy=(int16_t)cap(s.energy+1,999);op(BulkOp::Draw,2);break;
 case Potion::DistilledChaos:for(int i=0;i<3;i++)op(BulkOp::AutoplayTop);break;
 case Potion::DropletOfPrecognition:op(BulkOp::ChooseDraw);break;
 case Potion::Duplicator:power(BulkPower::Duplicator,1);break;
 case Potion::FyshOil:op(BulkOp::Strength,1);op(BulkOp::Dexterity,1);break;
 case Potion::GamblersBrew:op(BulkOp::DiscardRedraw);break;
 case Potion::GigantificationPotion:power(BulkPower::Gigantic,1);break;
 case Potion::HeartOfIron:power(BulkPower::Plating,7);break;
 case Potion::LiquidMemories:op(BulkOp::ChooseDiscard,0,1);break;
 case Potion::LuckyTonic:power(BulkPower::Buffer,1);break;
 case Potion::MazalethsGift:power(BulkPower::Ritual,1);break;
 case Potion::OrobicAcid:for(int i=1;i<=3;i++)op(BulkOp::RandomGenerated,i,1);break;
 case Potion::PotionOfBinding:
  for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp){s.enemies[i].weak=(int16_t)cap(s.enemies[i].weak+1,9999);s.enemies[i].vulnerable=(int16_t)cap(s.enemies[i].vulnerable+1,9999);}break;
 case Potion::PowderedDemise:op(BulkOp::EnemyDemise,target,9);break;
 case Potion::PowerPotion:op(BulkOp::Offer,3,1);break;
 case Potion::RadiantTincture:s.energy=(int16_t)cap(s.energy+1,999);power(BulkPower::Radiance,3);break;
 case Potion::RegenPotion:power(BulkPower::Regen,5);break;
 case Potion::ShacklingPotion:for(int i=0;i<s.enemy_count;i++)op(BulkOp::TemporaryEnemyStrength,i,7);break;
 case Potion::ShipInABottle:s.block=(int16_t)cap(s.block+10,9999);s.block_next_turn=(int16_t)cap(s.block_next_turn+10,9999);break;
 case Potion::SkillPotion:op(BulkOp::Offer,2,1);break;
 case Potion::SneckoOil:op(BulkOp::Draw,7);op(BulkOp::ShuffleCosts);break;
 case Potion::StableSerum:power(BulkPower::RetainHand,2);break;
 case Potion::TouchOfInsanity:op(BulkOp::FreeCombat);break;
 case Potion::SoldiersStew:op(BulkOp::AllStrikesReplay);break;
 case Potion::Ashwater:op(BulkOp::ExhaustAny);break;
 case Potion::PoisonPotion:
  if(s.enemies[target].hp)s.enemies[target].poison=(int16_t)cap(s.enemies[target].poison+6+((s.relics&SneckoSkull)?1:0),9999);break;
 case Potion::GhostInAJar:power(BulkPower::Intangible,1);break;
 case Potion::CunningPotion:op(BulkOp::CreateShivs,3,1);break;
 case Potion::FoulPotion:
  if(s.phase==Phase::Shop)gain_gold(s,100);
  else{op(BulkOp::UnpoweredAll,12);op(BulkOp::PlayerDamage,12);}break;
 case Potion::GlowwaterPotion:op(BulkOp::ExhaustWholeHand);op(BulkOp::Draw,10);break;
 case Potion::PotionShapedRock:damage_unpowered(s,target,15);break;
 default:break; // The automatic Fairy has no manual-use path.
 }
}
}
