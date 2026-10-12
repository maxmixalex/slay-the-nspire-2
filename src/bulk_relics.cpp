#include "core.hpp"
namespace spire {
#include "bulk_relic_registry.inc"
static int smaller(int a,int b){return a<b?a:b;}
static void notify(State&s,unsigned id){s.relic_flash|=id;++s.relic_revision;}
static void block(State&s,int n){n=smaller(n,9999-s.block);s.block+=(int16_t)n;record(s,LogKind::Block,n);}
static void energy(State&s,int n){s.energy=(int16_t)smaller(999,s.energy+n);record(s,LogKind::Energy,n);}
static void random_damage(State&s,int n){int ids[MaxEnemies],count=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[count++]=i;if(count)damage_unpowered(s,ids[random_below(s.rng_cards,count)],n);}
bool relic_in_pool(unsigned id,Character character){
    if(id==BurningBlood||id==RingOfTheSnake)return false;
    unsigned i=RelicOrdinal(id);if(i>=RelicKinds)return false;if(i<30)return true;
    unsigned p=added_relic_pool[i-30];return p==0||(p==1&&character==Character::Ironclad)||(p==2&&character==Character::Silent);
}
void bulk_add_strength(State&s,int n){
    if(n>0&&(s.relics&RuinedHelmet)&&!s.bulk.combat_flags[RelicOrdinal(RuinedHelmet)]&&combat_phase(s.phase)){
        s.bulk.combat_flags[RelicOrdinal(RuinedHelmet)]=1;n*=2;notify(s,RuinedHelmet);
    }
    n=smaller(n,9999-s.strength);s.strength+=(int16_t)n;record(s,LogKind::Strength,n);
}
void bulk_add_dexterity(State&s,int n,bool temporary){
    n=smaller(n,9999-s.dexterity);s.dexterity+=(int16_t)n;
    if(temporary)s.temporary_dexterity+=(int16_t)n;record(s,LogKind::Dexterity,n);
}
int bulk_effective_dexterity(const State&s){
    bool bottle=false;for(unsigned i=0;i<s.potion_slots;i++)if(s.potions[i]!=Potion::Empty)bottle=true;
    return s.dexterity+((!bottle&&(s.relics&BeltBuckle))?2:0);
}
int bulk_attack_bonus(const State&s,Card c){
    return ((s.relics&RedSkull)&&s.hp*2<=s.max_hp?3:0)+((s.relics&MiniatureCannon)&&c.upgraded?3:0)+((s.relics&StrikeDummy)&&card_strike_tag(c.kind)?3:0);
}
int bulk_card_block(State&s,Card card,int n){
    if(n<=0||s.no_block)return 0;
    n+=bulk_effective_dexterity(s);
    if(card_defend_tag(card.kind))n+=bulk_power(s,BulkPower::Fasten);
    if((s.relics&Vambrace)&&!s.bulk.combat_flags[RelicOrdinal(Vambrace)]){s.bulk.combat_flags[RelicOrdinal(Vambrace)]=1;n*=2;notify(s,Vambrace);}
    return smaller(9999,n<0?0:n);
}
void bulk_upgrade_random(State&s,int count,CardType type){
    uint16_t eligible[MaxDeck];unsigned n=0;
    for(unsigned i=0;i<s.deck_count;i++)if(!s.deck[i].upgraded&&definitions[s.deck[i].kind].type==type)eligible[n++]=(uint16_t)i;
    for(int i=0;i<count&&n;i++){unsigned k=random_below(s.rng_world,n);s.deck[eligible[k]].upgraded=1;record(s,LogKind::Upgrade,s.deck[eligible[k]].kind);eligible[k]=eligible[--n];}
}
void bulk_relic_obtained(State&s,unsigned id){
    if(id==PotionBelt)s.potion_slots=(uint8_t)smaller(MaxPotionSlots,s.potion_slots+2);
    else if(id==LeesWaffle){increase_max_hp(s,7);heal(s,s.max_hp);}
    else if(id==LoomingFruit)increase_max_hp(s,31);
    else if(id==WarPaint)bulk_upgrade_random(s,2,CardType::Skill);
    else if(id==Whetstone)bulk_upgrade_random(s,2,CardType::Attack);
}
void bulk_relic_hook(State&s,RelicHook hook,unsigned id){
    bool fired=false;unsigned index=RelicOrdinal(id);
    if(hook==RelicHook::CombatStart){
        if(id==Bellows){for(unsigned i=0;i<s.hand.count;i++)s.cards[s.hand.ids[i]].upgraded=1;fired=true;}
        else if(id==FestivePopper){for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,9);fired=true;}
        else if(id==GamblingChip){bulk_enqueue(s,BulkOp::DiscardRedraw);fired=true;}
        else if(id==Gorget){bulk_power_add(s,BulkPower::Plating,4);fired=true;}
        else if(id==PetrifiedToad){fired=procure_potion(s,Potion::PotionShapedRock);}
        else if(id==RedMask){for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)++s.enemies[i].weak;fired=true;}
        else if(id==RingingTriangle){bulk_power_add(s,BulkPower::RetainHand,1);fired=true;}
        else if(id==StoneCracker){uint16_t ids[MaxCards];unsigned n=0;for(unsigned i=0;i<s.draw.count;i++)if(!s.cards[s.draw.ids[i]].upgraded)ids[n++]=s.draw.ids[i];
            for(int i=0;i<2&&n;i++){unsigned j=random_below(s.rng_cards,n);s.cards[ids[j]].upgraded=1;ids[j]=ids[--n];}fired=true;}
        else if(id==Toolbox){bulk_enqueue(s,BulkOp::Offer,4);fired=true;}
        else if(id==VexingPuzzlebox){bulk_enqueue(s,BulkOp::RandomGenerated,0,1);fired=true;}
        else if(id==NinjaScroll){bulk_enqueue(s,BulkOp::CreateShivs,3);fired=true;}
        else if(id==TwistedFunnel){for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)s.enemies[i].poison+=(int16_t)(4+((s.relics&SneckoSkull)?1:0));fired=true;}
        else if(id==Pantograph&&s.current_node>=0&&s.map[s.current_node].room==Room::Final){heal(s,25);fired=true;}
    }else if(hook==RelicHook::TurnStart){
        if(id==Bread){s.energy=(int16_t)(s.turn==1?(s.energy>2?s.energy-2:0):smaller(999,s.energy+1));fired=true;}
        else if(id==Candelabra&&s.turn==2){energy(s,2);fired=true;}
        else if(id==Chandelier&&s.turn==3){energy(s,3);fired=true;}
        else if(id==Pendulum){auto&n=s.relic_counters[index];if(++n==3){n=0;bulk_enqueue(s,BulkOp::Draw,1);fired=true;}}
        else if(id==Pocketwatch&&s.turn>1&&s.bulk.previous_turn_cards<=3){s.draw_next_turn=(int16_t)smaller(MaxCards,s.draw_next_turn+3);fired=true;}
        else if(id==SparklingRouge&&s.turn==3){bulk_add_strength(s,1);bulk_add_dexterity(s,1);fired=true;}
        else if(id==VenerableTeaSet&&s.turn==1&&s.bulk.tea_ready){energy(s,2);s.bulk.tea_ready=0;fired=true;}
        else if(id==VeryHotCocoa&&s.turn==1){energy(s,4);fired=true;}
        else if(id==Brimstone){bulk_add_strength(s,2);for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp&&s.enemies[i].strength<9999)++s.enemies[i].strength;fired=true;}
    }else if(hook==RelicHook::TurnEnd){
        if(id==CloakClasp){block(s,s.hand.count);fired=true;}
        else if(id==ParryingShield&&s.block>=10){random_damage(s,6);fired=true;}
        else if(id==RippleBasin&&!s.attacked_this_turn){block(s,4);fired=true;}
        else if(id==ScreamingFlagon&&!s.hand.count){for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,20);fired=true;}
        else if(id==StoneCalendar&&s.turn==7){for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,52);fired=true;}
    }else if(hook==RelicHook::Victory){
        if(id==MeatOnTheBone&&s.hp*2<=s.max_hp){heal(s,12);fired=true;}
        else if(id==LavaLamp&&!s.bulk.hp_lost_combat){for(auto&v:s.bulk.reward_upgraded)v=1;fired=true;}
    }
    if(fired)notify(s,id);
}
void bulk_relic_played(State&s,Card card,unsigned id){
    unsigned index=RelicOrdinal(id);CardType type=definitions[card.kind].type;bool fired=false;
    if(id==GamePiece&&type==CardType::Power){bulk_enqueue(s,BulkOp::Draw,1,0,true);fired=true;}
    else if(id==Kusarigama&&type==CardType::Attack){auto&n=s.relic_counters[index];if(++n==3){n=0;random_damage(s,6);fired=true;}}
    else if(id==TuningFork&&type==CardType::Skill){auto&n=s.relic_counters[index];if(++n==10){n=0;block(s,7);fired=true;}}
    else if(id==Permafrost&&type==CardType::Power&&!s.bulk.combat_flags[index]){s.bulk.combat_flags[index]=1;block(s,7);fired=true;}
    else if(id==MummifiedHand&&type==CardType::Power){uint16_t ids[MaxHand];unsigned n=0;for(unsigned i=0;i<s.hand.count;i++)if(cost(s,s.hand.ids[i])>0)ids[n++]=s.hand.ids[i];if(n){s.bulk.turn_cost[ids[random_below(s.rng_cards,n)]]=1;fired=true;}}
    else if(id==RainbowRing){auto&bits=s.bulk.turn_flags[index];bits|=(uint16_t)(1u<<(int)type);if((bits&7)==7&&!(bits&8)){bits|=8;bulk_add_strength(s,1);bulk_add_dexterity(s,1);fired=true;}}
    else if(id==RazorTooth&&type!=CardType::Power&&s.playing>=0){s.cards[s.playing].upgraded=1;fired=true;}
    else if(id==HelicalDart&&card.kind==Shiv){bulk_add_dexterity(s,1,true);fired=true;}
    if(fired)notify(s,id);
}
void bulk_deck_added(State&s,Card&c){
    CardType type=definitions[c.kind].type;
    if((type==CardType::Attack&&(s.relics&MoltenEgg))||(type==CardType::Skill&&(s.relics&ToxicEgg))||(type==CardType::Power&&(s.relics&FrozenEgg)))c.upgraded=1;
    if(s.relics&LuckyFysh)gain_gold(s,15);
    if(s.relics&BookOfFiveRings){auto&n=s.relic_counters[RelicOrdinal(BookOfFiveRings)];if(++n==5){n=0;heal(s,20);notify(s,BookOfFiveRings);}}
}
void bulk_room_entered(State&s,Room room){
    if(room==Room::Camp){if(s.relics&EternalFeather)heal(s,3*(s.deck_count/5));if(s.relics&VenerableTeaSet)s.bulk.tea_ready=1;}
    if(room==Room::Event&&(s.relics&Planisphere))heal(s,5);
}
int shop_price(const State&s,int n){
    if(s.relics&MembershipCard)n=n/2;
    if(s.relics&TheCourier)n=n*4/5;
    return n<0?0:n;
}
void bulk_merchant_restock(State&s,int slot){
    if(!(s.relics&TheCourier)||slot>=ShopRemove)return;
    if(slot<ShopCardCount){
        unsigned k=random_reward_card(s);
        if(slot>=5){uint8_t ids[CardKinds];unsigned n=0;for(unsigned i=0;i<CardKinds;i++)if(card_colorless(i))ids[n++]=(uint8_t)i;k=ids[random_below(s.rng_world,n)];}
        s.shop_cards[slot]=(uint8_t)k;int rarity=card_rarity(k);s.shop_prices[slot]=(int16_t)(rarity==4?150:rarity==3?75:50);
    }else if(slot<ShopPotionStart){s.shop_relics[slot-ShopRelicStart]=random_relic(s);if(!s.shop_relics[slot-ShopRelicStart])return;s.shop_prices[slot]=150;}
    else{auto p=random_potion(s);s.shop_potions[slot-ShopPotionStart]=(uint8_t)p;const auto*d=potion_definition(p);s.shop_prices[slot]=d->rarity==3?100:d->rarity==2?75:50;}
    s.shop_sold&=(uint16_t)~(1u<<slot);
}
}
