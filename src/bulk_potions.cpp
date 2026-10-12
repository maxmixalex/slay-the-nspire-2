#include "core.hpp"
namespace spire {
static int smaller(int a,int b){return a<b?a:b;}
bool potion_in_pool(Potion p,Character character){
    const auto*d=potion_definition(p);if(!d)return false;
    if(p==Potion::Blood)return character==Character::Ironclad;
    return d->pool==0||(d->pool==1&&character==Character::Ironclad)||(d->pool==2&&character==Character::Silent);
}
Potion random_potion(State&s){
    // Acquisition is restricted to the actual supported character/shared
    // registries. The existing expedition RNG/rarity factory remains adapted.
    Potion ids[PotionKinds];unsigned n=0;
    for(unsigned i=1;i<=PotionKinds;i++)if(potion_in_pool((Potion)i,s.character))ids[n++]=(Potion)i;
    return n?ids[random_below(s.rng_world,n)]:Potion::Empty;
}
bool procure_potion(State&s,Potion p){
    if(!potion_definition(p))return false;
    for(unsigned i=0;i<s.potion_slots;i++)if(s.potions[i]==Potion::Empty){s.potions[i]=p;return true;}return false;
}
void bulk_potion_changed(State&){ /* Belt Buckle is a derived stat, never a saved adjustment. */ }
void bulk_use_potion(State&s,Potion p,int target){
    auto power=[&](BulkPower k,int n){bulk_enqueue(s,BulkOp::GrantPower,(int)k,n);};
    bool combat=s.phase==Phase::Combat;
    if(p==Potion::EntropicBrew){
        for(unsigned i=0;i<s.potion_slots;i++)if(s.potions[i]==Potion::Empty)s.potions[i]=random_potion(s);return;
    }
    if(p==Potion::Ambergris){heal(s,s.max_hp/2);if(combat)s.bulk.skip_enemy=1;return;}
    if(p==Potion::FoulPotion&&s.phase==Phase::Shop){gain_gold(s,100);return;}
    if(!combat)return;
    s.bulk.potion_active=1;
    switch(p){
    case Potion::AttackPotion:bulk_enqueue(s,BulkOp::Offer,1,1);break;
    case Potion::BeetleJuice:s.bulk.enemy_beetle[target]=(int16_t)smaller(9999,s.bulk.enemy_beetle[target]+4);break;
    case Potion::BottledPotential:bulk_enqueue(s,BulkOp::ShuffleAll);bulk_enqueue(s,BulkOp::Draw,5);break;
    case Potion::Clarity:bulk_enqueue(s,BulkOp::Draw,1);power(BulkPower::Clarity,3);break;
    case Potion::ColorlessPotion:bulk_enqueue(s,BulkOp::Offer,4,1);break;
    case Potion::CureAll:s.energy=(int16_t)smaller(999,s.energy+1);bulk_enqueue(s,BulkOp::Draw,2);break;
    case Potion::DistilledChaos:for(int i=0;i<3;i++)bulk_enqueue(s,BulkOp::AutoplayTop);break;
    case Potion::DropletOfPrecognition:bulk_enqueue(s,BulkOp::ChooseDraw);break;
    case Potion::Duplicator:power(BulkPower::Duplicator,1);break;
    case Potion::FyshOil:bulk_enqueue(s,BulkOp::Strength,1);bulk_enqueue(s,BulkOp::Dexterity,1);break;
    case Potion::GamblersBrew:bulk_enqueue(s,BulkOp::DiscardRedraw);break;
    case Potion::GigantificationPotion:power(BulkPower::Gigantic,1);break;
    case Potion::HeartOfIron:power(BulkPower::Plating,7);break;
    case Potion::LiquidMemories:bulk_enqueue(s,BulkOp::ChooseDiscard,0,1);break;
    case Potion::LuckyTonic:power(BulkPower::Buffer,1);break;
    case Potion::MazalethsGift:power(BulkPower::Ritual,1);break;
    case Potion::OrobicAcid:for(int i=1;i<=3;i++)bulk_enqueue(s,BulkOp::RandomGenerated,i,1);break;
    case Potion::PotionOfBinding:for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp){s.enemies[i].weak=(int16_t)smaller(9999,s.enemies[i].weak+1);s.enemies[i].vulnerable=(int16_t)smaller(9999,s.enemies[i].vulnerable+1);}break;
    case Potion::PowderedDemise:bulk_enqueue(s,BulkOp::EnemyDemise,target,9);break;
    case Potion::PowerPotion:bulk_enqueue(s,BulkOp::Offer,3,1);break;
    case Potion::RadiantTincture:s.energy=(int16_t)smaller(999,s.energy+1);power(BulkPower::Radiance,3);break;
    case Potion::RegenPotion:power(BulkPower::Regen,5);break;
    case Potion::ShacklingPotion:for(int i=0;i<s.enemy_count;i++)bulk_enqueue(s,BulkOp::TemporaryEnemyStrength,i,7);break;
    case Potion::ShipInABottle:s.block=(int16_t)smaller(9999,s.block+10);s.block_next_turn=(int16_t)smaller(9999,s.block_next_turn+10);break;
    case Potion::SkillPotion:bulk_enqueue(s,BulkOp::Offer,2,1);break;
    case Potion::SneckoOil:bulk_enqueue(s,BulkOp::Draw,7);bulk_enqueue(s,BulkOp::ShuffleCosts);break;
    case Potion::StableSerum:power(BulkPower::RetainHand,2);break;
    case Potion::TouchOfInsanity:bulk_enqueue(s,BulkOp::FreeCombat);break;
    case Potion::SoldiersStew:bulk_enqueue(s,BulkOp::AllStrikesReplay);break;
    case Potion::Ashwater:bulk_enqueue(s,BulkOp::ExhaustAny);break;
    case Potion::PoisonPotion:if(s.enemies[target].hp)s.enemies[target].poison=(int16_t)smaller(9999,s.enemies[target].poison+6+((s.relics&SneckoSkull)?1:0));break;
    case Potion::GhostInAJar:power(BulkPower::Intangible,1);break;
    case Potion::CunningPotion:bulk_enqueue(s,BulkOp::CreateShivs,3,1);break;
    case Potion::FoulPotion:for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,12);bulk_player_damage(s,12,false);break;
    case Potion::GlowwaterPotion:bulk_enqueue(s,BulkOp::ExhaustWholeHand);bulk_enqueue(s,BulkOp::Draw,10);break;
    case Potion::PotionShapedRock:damage_unpowered(s,target,15);break;
    default:break; // Automatic Fairy and anytime bottles are handled before this switch.
    }
    if(s.hp>0&&combat_phase(s.phase))bulk_enqueue(s,BulkOp::FinishPotion);
}
}
