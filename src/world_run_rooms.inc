namespace {
static bool world_at_most(Real a,Real b){return !(b<a);}
static int world_round(Real value){int32_t result=0;return reference::math::round_even_int(value,result)?result:0;}
static Real world_float_between(reference::Random&rng,uint32_t low,uint32_t high){
    Real a=world_float(low),range=world_float_round(world_float(high)-a);
    return world_float_round(Real::fraction53(rng.next()>>11)*range+a);
}
static WorldRelicPool world_relic_rarity(reference::Random&rng){
    Real roll=world_random_float(rng);
    return roll<world_float(0x3f000000U)?WorldRelicPool::Common:roll<world_float(0x3f547ae1U)?WorldRelicPool::Uncommon:WorldRelicPool::Rare;
}
static WorldRoom world_roll_unknown(State&s,unsigned point){
    auto&w=s.world;const auto&m=w.maps[w.act_index];
    const auto&children=m.points[point].children;bool only_shops=children.count()!=0;
    for(unsigned i=0;i<children.used;++i){unsigned id=children.slots[i];if(id!=NoPoint&&m.points[id].type!=PointType::Shop)only_shops=false;}
    bool allowed[4]={!has_world_relic(s,WorldRelicJuzuBracelet),true,true,!w.point_had_shop&&!only_shops};
    if(world_relic_golden_path(s,w.act_index)||event_card_forces_historian(s,w.act_index))for(auto&v:allowed)v=false;
    const WorldRoom kinds[]={WorldRoom::Monster,WorldRoom::Elite,WorldRoom::Treasure,WorldRoom::Shop};
    const uint32_t base[]={0x3dcccccdU,0xbf800000U,0x3ca3d70aU,0x3cf5c28fU};
    uint32_t*odds[]={&w.unknown_monster_bits,&w.unknown_elite_bits,&w.unknown_treasure_bits,&w.unknown_shop_bits};
    // Even an event-only hook consumes the original UnknownMapPoint draw.
    Real roll=world_random_float(world_rng(s,WorldRng::UnknownMapPoint)),sum=Real::integer(0);
    WorldRoom result=WorldRoom::Event;
    for(unsigned i=0;i<4;++i)if(allowed[i]&&!(world_float(*odds[i])<Real::integer(0))){
        sum=world_float_round(sum+world_float(*odds[i]));if(world_at_most(roll,sum)){result=kinds[i];break;}
    }
    for(unsigned i=0;i<4;++i){
        if(result==kinds[i])*odds[i]=base[i];
        else if(allowed[i])*odds[i]=world_float_bits(world_float_round(world_float(*odds[i])+world_float(base[i])));
    }
    return result;
}
static bool world_make_rewards(State&s){
    auto&w=s.world;auto&rng=world_rng(s,WorldRng::Rewards);
    s.last_gold=0;s.reward_claimed=0;s.reward_potion=Potion::Empty;w.reward_count=0;w.reward_relic=NoWorldRelic;w.reward_relic_claimed=0;
    for(auto&c:w.reward_cards)c={};for(auto&f:w.reward_facts)f={};
    if(!encounter_rewards_allowed(s.enemy_world.encounter)){
        s.reward_claimed=7;w.reward_relic_claimed=1;return world_finish_room(s);
    }
    WorldRoom room=WorldRoom(w.room_type);
    if(room==WorldRoom::Boss&&w.act_index==2){s.reward_claimed=7;w.reward_relic_claimed=1;return true;}
    bool potion=has_world_relic(s,WorldRelicWhiteBeastStatue);
    if(!potion){
        Real chance=world_float(w.potion_drop_bits);
        if(room==WorldRoom::Elite)chance=world_float_round(chance+world_float(0x3e000000U));
        potion=world_random_float(rng)<chance;
        w.potion_drop_bits=world_float_bits(world_float_round(world_float(w.potion_drop_bits)+(potion?-world_float(0x3dcccccdU):world_float(0x3dcccccdU))));
    }
    int lo=room==WorldRoom::Boss?100:encounter_min_gold(s.enemy_world.encounter,w.ascension);
    int hi=room==WorldRoom::Boss?100:encounter_max_gold(s.enemy_world.encounter,w.ascension);
    Real proportion=Real::integer(1);
    if(room==WorldRoom::Monster){unsigned spawned=encounter_spawned_models(s),escaped=encounter_escaped_enemies(s);
        if(spawned)proportion=world_float_round(Real::integer(1)-world_float_round(Real::integer(int(escaped))/Real::integer(int(spawned))));
        lo=world_round(world_float_round(Real::integer(lo)*proportion));hi=world_round(world_float_round(Real::integer(hi)*proportion));
    }
    if(Real::integer(0)<proportion)s.last_gold=int16_t(rng.range(lo,hi+1));else s.reward_claimed|=1;
    if(potion){if(world_random_potions(s,rng,1,&s.reward_potion)!=1)return world_fail(s);}else s.reward_claimed|=4;
    WorldCardOptions options;options.source=WorldCardSource::Encounter;
    options.odds=room==WorldRoom::Boss?WorldCardOdds::BossEncounter:room==WorldRoom::Elite?WorldCardOdds::EliteEncounter:WorldCardOdds::RegularEncounter;
    options.flags=WorldIsCardReward|WorldIsFromCombat;
    unsigned count=unsigned(world_relic_modify_card_reward_count(s,3,options));
    if(count>MaxWorldRewardCards||!world_create_reward(s,rng,count,options,w.reward_cards,w.reward_facts,MaxWorldRewardCards,&count))return world_fail(s);
    w.reward_count=uint8_t(count);for(unsigned i=0;i<3;++i)s.reward[i]=i<count?w.reward_cards[i].kind:Strike;
    if(room==WorldRoom::Elite)w.reward_relic=random_world_relic(s);
    else w.reward_relic_claimed=1;
    world_relic_before_reward_offered(s,count!=0);return s.phase!=Phase::Error;
}
static unsigned world_shop_rarity(State&s){
    if(!s.world_factory.initialized){s.world_factory.initialized=1;s.world_factory.rarity_offset_bits=0xbd4ccccdU;}
    Real rare=world_float_round(world_float(s.world.ascension>=7?0x3d3851ecU:0x3db851ecU)+world_float(s.world_factory.rarity_offset_bits));
    Real roll=world_random_float(world_rng(s,WorldRng::Rewards));
    return roll<rare?4:roll<world_float_round(rare+world_float(0x3ebd70a4U))?3:2;
}
static int world_card_base_price(Card c){
    unsigned rarity=unsigned(card_rarity(c.kind));int base=rarity==4?150:rarity==3?75:50;
    if(card_colorless(c.kind))base=world_round(world_float_round(Real::integer(base)*world_float(0x3f933333U)));
    return base;
}
static int world_random_price(State&s,int base,bool relic=false){
    Real modifier=world_float_between(world_rng(s,WorldRng::Shops),relic?0x3f59999aU:0x3f733333U,relic?0x3f933333U:0x3f866666U);
    return world_round(world_float_round(Real::integer(base)*modifier));
}
static bool world_populate_shop_card(State&s,unsigned slot,bool sale){
    auto&w=s.world;WorldCardOptions options;options.pools[0]=slot<5?WorldCardPool::Own:WorldCardPool::Colorless;
    options.rarity_mask=world_rarity_mask(2)|world_rarity_mask(3)|world_rarity_mask(4);
    const CardType types[]={CardType::Attack,CardType::Attack,CardType::Skill,CardType::Skill,CardType::Power};
    if(slot<5)options.type_mask=world_type_mask(types[slot]);
    CardKind pool[CardKinds];unsigned count=world_card_candidates(s,options,pool,CardKinds),n=0;
    for(unsigned i=0;i<count;++i){bool used=false;for(unsigned j=0;j<ShopCardCount;++j)
        if((!(s.shop_sold&(1U<<j))||j==slot)&&w.shop_cards[j].kind==pool[i])used=true;
        if(!used)pool[n++]=pool[i];
    }
    unsigned rarity=slot<5?world_shop_rarity(s):slot==5?3:4,attempts=0;
    for(;attempts<3;++attempts){bool found=false;for(unsigned i=0;i<n;++i)if(unsigned(card_rarity(pool[i]))==rarity)found=true;
        if(found)break;rarity=rarity==4?2:rarity+1;
    }
    if(attempts==3)return world_fail(s);
    count=0;for(unsigned i=0;i<n;++i)if(unsigned(card_rarity(pool[i]))==rarity)pool[count++]=pool[i];
    if(!count)return world_fail(s);
    w.shop_cards[slot]={pool[world_rng(s,WorldRng::Shops).below(count)],0};w.shop_facts[slot]={};
    // Merchant upgrades use the Rewards stream even though their base chance
    // is a large negative sentinel. Relic upgrades happen afterwards.
    world_random_float(world_rng(s,WorldRng::Rewards));
    world_relic_modify_merchant_cards(s,w.shop_cards+slot,w.shop_facts+slot,1);
    int price=world_random_price(s,world_card_base_price(w.shop_cards[slot]));
    if(sale)price=world_random_price(s,world_card_base_price(w.shop_cards[slot]))/2;
    w.shop_base_prices[slot]=int16_t(price);s.shop_cards[slot]=w.shop_cards[slot].kind;s.shop_sold&=uint16_t(~(1U<<slot));return true;
}
static int world_relic_base_price(RelicId id){
    if(id>=WorldRelicKinds)return 0;
    unsigned rarity=world_relic_definitions[id].rarity;
    return rarity==2?175:rarity==3?225:rarity==4?275:rarity==5?200:1;
}
static bool world_populate_shop_relic(State&s,unsigned slot,WorldRelicPool rarity){
    RelicId id=world_relic_pull(s,rarity,true,true);if(id==NoWorldRelic)return world_fail(s);
    s.world.shop_relics[slot]=id;s.shop_relics[slot]=id<RelicKinds?RelicMask(1)<<id:0;
    s.world.shop_base_prices[ShopRelicStart+slot]=int16_t(world_random_price(s,world_relic_base_price(id),true));
    s.shop_sold&=uint16_t(~(1U<<(ShopRelicStart+slot)));return true;
}
static bool world_populate_shop_potion(State&s,unsigned slot,Potion potion=Potion::Empty){
    if(potion==Potion::Empty&&world_random_potions(s,world_rng(s,WorldRng::Shops),1,&potion)!=1)return world_fail(s);
    const auto*d=potion_definition(potion);if(!d)return world_fail(s);
    s.shop_potions[slot]=uint8_t(potion);s.world.shop_base_prices[ShopPotionStart+slot]=int16_t(world_random_price(s,d->rarity==3?100:d->rarity==2?75:50));
    s.shop_sold&=uint16_t(~(1U<<(ShopPotionStart+slot)));return true;
}
static void world_update_shop(State&s){
    for(unsigned i=0;i<ShopCardCount;++i)if(!(s.shop_sold&(1U<<i))){
        world_relic_modify_merchant_cards(s,s.world.shop_cards+i,s.world.shop_facts+i,1);s.shop_cards[i]=s.world.shop_cards[i].kind;
    }
    for(int i=0;i<ShopRemove;++i)s.shop_prices[i]=int16_t(world_shop_price(s,i));
}
static bool world_make_shop(State&s){
    auto&w=s.world;s.phase=Phase::Shop;s.shop_sold=(1U<<ShopRemove)-1;
    for(auto&c:w.shop_cards)c={};for(auto&f:w.shop_facts)f={};
    w.shop_sale=uint8_t(world_rng(s,WorldRng::Shops).below(5));
    for(unsigned i=0;i<ShopCardCount;++i)if(!world_populate_shop_card(s,i,i==w.shop_sale))return false;
    const WorldRelicPool rarities[]={world_relic_rarity(world_rng(s,WorldRng::Rewards)),world_relic_rarity(world_rng(s,WorldRng::Rewards)),WorldRelicPool::Shop};
    for(unsigned i=0;i<3;++i)if(!world_populate_shop_relic(s,i,rarities[i]))return false;
    Potion potions[3];if(world_random_potions(s,world_rng(s,WorldRng::Shops),3,potions)!=3)return world_fail(s);
    for(unsigned i=0;i<3;++i)if(!world_populate_shop_potion(s,i,potions[i]))return false;
    w.shop_base_prices[ShopRemove]=int16_t((w.ascension>=6?100:75)+(w.ascension>=6?50:25)*s.removals);
    world_update_shop(s);return true;
}
static bool world_buy(State&s,int item){
    if(s.phase!=Phase::Shop||item<0||item>ShopRemove||(s.shop_sold&(1U<<item)))return false;
    int price=world_shop_price(s,item);if(s.gold<price)return false;
    if(item==ShopRemove){
        bool eligible=false;for(unsigned i=0;i<s.deck_count;++i)if(card_removable(s,s.deck[i],i))eligible=true;
        if(!eligible||s.deck_count<=1)return false;s.phase=Phase::Remove;++s.revision;return true;
    }
    if(item<ShopCardCount){
        if(s.deck_count>=MaxDeck)return false;
        if(event_add_deck_card(s,s.world.shop_cards[item],&s.world.shop_facts[item])<0)return false;
        s.gold-=int16_t(price);record(s,LogKind::Buy,s.shop_cards[item]);
    }else if(item<ShopPotionStart){
        s.gold-=int16_t(price);if(!gain_world_relic(s,s.world.shop_relics[item-ShopRelicStart]))return false;
    }else{
        if(!procure_potion(s,Potion(s.shop_potions[item-ShopPotionStart])))return false;s.gold-=int16_t(price);
    }
    s.shop_sold|=uint16_t(1U<<item);
    if(has_world_relic(s,WorldRelicTheCourier)){
        if(item<ShopCardCount){if(!world_populate_shop_card(s,unsigned(item),false))return false;if(s.world.shop_sale==item)s.world.shop_sale=255;}
        else if(item<ShopPotionStart){if(!world_populate_shop_relic(s,unsigned(item-ShopRelicStart),world_relic_rarity(world_rng(s,WorldRng::Rewards))))return false;}
        else if(!world_populate_shop_potion(s,unsigned(item-ShopPotionStart)))return false;
    }
    world_update_shop(s);world_relic_hook(s,WorldRelicHook::AfterItemPurchased,item,price);++s.revision;return s.phase!=Phase::Error;
}
static bool world_should_treasure(const State&s){
    for(unsigned i=0;i<s.world_relics.count;++i){const auto&r=s.world_relics.entries[i];
        if(r.id==WorldRelicSilverCrucible&&!(r.flags&WorldRelicMelted)&&r.values[1]<=1)return false;
    }return true;
}
static bool world_prepare_treasure(State&s){
    if(!world_should_treasure(s))return true;
    s.world.treasure_relic=world_relic_pull_shared(s,world_relic_rarity(world_rng(s,WorldRng::TreasureRoomRelics)));
    return s.world.treasure_relic!=NoWorldRelic;
}
static bool world_open_treasure(State&s){
    if(s.phase!=Phase::Treasure||s.treasure_state)return false;
    s.treasure_state=1;
    if(world_should_treasure(s)){
        int gold=world_rng(s,WorldRng::Rewards).range(42,53);if(s.world.ascension>=3)gold=gold*3/4;
        s.world.treasure_gold=int16_t(gold);s.world.treasure_gold_claimed=1;gain_gold(s,gold);
        // Spoils attaches its quest to the first treasure in column/row order,
        // including when Golden Compass replaced the hourglass afterwards.
        const auto&m=s.world.maps[s.world.act_index];unsigned first=NoPoint;
        for(unsigned col=0;col<reference::Columns&&first==NoPoint;++col)for(unsigned row=0;row<m.config.rows;++row){
            unsigned id=m.grid[col][row];if(id!=NoPoint&&m.points[id].type==PointType::Treasure){first=id;break;}
        }
        if(s.world.current_point==first)for(int i=int(s.deck_count)-1;i>=0;--i)
            if(s.deck[i].kind==EventCardSpoilsMap&&s.event_cards.deck[i].progress==s.world.act_index){
                if(!event_card_spoils_complete(s,i))return world_fail(s);s.world.treasure_gold+=600;
            }
    }
    ++s.revision;return true;
}
static bool world_take_treasure(State&s){
    if(s.phase!=Phase::Treasure||s.treasure_state!=1)return false;
    s.treasure_state=2;
    if(s.world.treasure_relic!=NoWorldRelic&&!gain_world_relic(s,s.world.treasure_relic))return false;
    ++s.revision;return true;
}
} // namespace

int world_shop_price(const State&s,int item){
    if(item<0||item>ShopRemove)return 0;
    int base=item==ShopRemove?(s.world.ascension>=6?100:75)+(s.world.ascension>=6?50:25)*s.removals:s.world.shop_base_prices[item];
    // Both source hooks multiply decimal values; truncate once after all hooks.
    int numerator=base,denominator=1;
    for(unsigned i=0;i<s.world_relics.count;++i){const auto&r=s.world_relics.entries[i];if(r.flags&WorldRelicMelted)continue;
        if(r.id==WorldRelicMembershipCard)denominator*=2;
        if(r.id==WorldRelicTheCourier){numerator*=4;denominator*=5;}
    }
    return min(9999,numerator/denominator);
}
bool world_claim_reward_card(State&s,int index){
    if(s.phase!=Phase::Reward||(s.reward_claimed&2)||index< -1||index>=int(s.world.reward_count))return false;
    if(index>=0&&event_add_deck_card(s,s.world.reward_cards[index],&s.world.reward_facts[index])<0)return false;
    s.reward_claimed|=2;++s.revision;return true;
}
bool world_claim_reward_relic(State&s){
    auto&w=s.world;if(s.phase!=Phase::Reward||w.reward_relic_claimed||w.reward_relic==NoWorldRelic)return false;
    w.reward_relic_claimed=1;bool result=gain_world_relic(s,w.reward_relic);++s.revision;return result;
}
