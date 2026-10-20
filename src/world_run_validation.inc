bool world_valid_run(const State&s){
    const auto&w=s.world;
    if(w.active>1||w.rng_ready>1||w.act_index>=WorldActCount||w.ascension>10||w.room_type>uint8_t(WorldRoom::Map)||
       w.room_open>1||w.event_combat_parent>1||w.parent_room_type>uint8_t(WorldRoom::Map)||w.final_event>1||w.finished>1||w.point_had_shop>1||w.finish_pending>1)return false;
    if(w.rng_ready){for(const auto&r:w.rng)if(!r.good||!(r.words[0]|r.words[1]|r.words[2]|r.words[3]))return false;}
    if(!w.active)return true;
    if(!w.rng_ready||s.practice||w.total_floor>64||w.rooms_completed>w.total_floor||w.combat_count>255||s.floor!=w.total_floor)return false;
    if(!w.map_ready[w.act_index]||w.finished!=(s.phase==Phase::Complete))return false;
    const uint32_t odds[]={w.unknown_monster_bits,w.unknown_elite_bits,w.unknown_shop_bits,w.unknown_treasure_bits,w.potion_drop_bits};
    for(auto bits:odds){Real value=world_float(bits);if(!value.finite()||value<Real::integer(-100)||Real::integer(100)<value)return false;}
    unsigned visited=0;
    for(unsigned a=0;a<WorldActCount;++a){
        const auto&q=w.rooms[a];
        if(q.ready!=1||q.region>=4||(a==0?q.region>1:q.region!=a+1)||!q.encounter_count||q.encounter_count>MaxWorldEncounterQueue||q.encounter_cursor>64||
           q.elite_count!=15||q.elite_cursor>64||!q.event_count||q.event_count>MaxWorldEventQueue||q.event_cursor>4096||
           !q.boss_count||q.boss_count>2||q.boss_cursor>q.boss_count||q.ancient==0||q.ancient>=unsigned(OriginalEventId::Count))return false;
        if(q.boss_count==2&&(a!=2||w.ascension<10||q.bosses[0]==q.bosses[1]))return false;
        for(unsigned i=0;i<q.encounter_count;++i){const auto*d=encounter_definition(q.encounters[i]);if(!d||(d->type!=EncounterClass::Weak&&d->type!=EncounterClass::Normal)||d->act!=q.region)return false;}
        for(unsigned i=0;i<q.elite_count;++i){const auto*d=encounter_definition(q.elites[i]);if(!d||d->type!=EncounterClass::Elite||d->act!=q.region)return false;}
        for(unsigned i=0;i<q.boss_count;++i){const auto*d=encounter_definition(q.bosses[i]);if(!d||d->type!=EncounterClass::Boss||d->act!=q.region)return false;}
        for(unsigned i=0;i<q.event_count;++i)if(!q.events[i]||q.events[i]>=unsigned(OriginalEventId::Count))return false;
        if(w.map_ready[a]>1)return false;
        const auto&m=w.maps[a];
        if(w.map_ready[a]){
            if(m.count>reference::MaxPoints||m.root>=m.count||m.boss>=m.count||(m.second_boss!=NoPoint&&m.second_boss>=m.count)||
               m.config.rows>reference::MaxRows||m.config.rows<10||m.config.variant>2||!m.final_rng.good)return false;
            for(unsigned i=0;i<m.count;++i){const auto&p=m.points[i];
                if(p.col>=reference::Columns||p.row>m.config.rows+1||unsigned(p.type)>unsigned(PointType::Start)||p.parents.used>reference::Columns||p.children.used>reference::Columns)return false;
                for(unsigned j=0;j<p.parents.used;++j)if(p.parents.slots[j]!=NoPoint&&p.parents.slots[j]>=m.count)return false;
                for(unsigned j=0;j<p.children.used;++j)if(p.children.slots[j]!=NoPoint&&p.children.slots[j]>=m.count)return false;
            }
            for(const auto&col:m.grid)for(auto id:col)if(id!=NoPoint&&id>=m.count)return false;
            if(!m.valid())return false;
        }else if(a<=w.act_index)return false;
        for(unsigned i=0;i<reference::MaxPoints;++i){
            if(w.visited[a][i]>1||(w.visited[a][i]&&(!w.map_ready[a]||i>=m.count||!m.points[i].active)))return false;
            visited+=w.visited[a][i];
        }
    }
    if(visited!=w.total_floor)return false;
    for(auto v:w.visited_events)if(v>1)return false;
    const auto&m=w.maps[w.act_index];
    if(w.current_point==NoPoint){if(w.room_open||s.phase!=Phase::Map||w.map_row||w.map_column!=3)return false;}
    else{
        if(w.current_point>=m.count||!m.points[w.current_point].active||!w.visited[w.act_index][w.current_point])return false;
        if(w.map_column!=m.points[w.current_point].col||w.map_row!=m.points[w.current_point].row)return false;
    }
    if(s.phase==Phase::Map&&(w.room_open||w.final_event))return false;
    if(w.event_combat_parent&&(!s.original_events.combat_pending||!s.original_events.active||w.parent_room_type==0))return false;
    if(w.reward_count>MaxWorldRewardCards||w.reward_relic_claimed>1||(w.shop_sale>=5&&w.shop_sale!=255)||w.treasure_gold_claimed>1||w.treasure_gold<0||w.treasure_gold>9999)return false;
    if(w.reward_relic!=NoWorldRelic&&w.reward_relic>=WorldRelicKinds)return false;
    if(w.treasure_relic!=NoWorldRelic&&w.treasure_relic>=WorldRelicKinds)return false;
    for(auto id:w.shop_relics)if(id!=NoWorldRelic&&id>=WorldRelicKinds)return false;
    for(unsigned i=0;i<w.reward_count;++i)if(w.reward_cards[i].kind>=CardKinds||w.reward_cards[i].upgraded>1||!event_card_facts_valid(w.reward_cards[i],w.reward_facts[i]))return false;
    for(unsigned i=0;i<ShopCardCount;++i)if(w.shop_cards[i].kind>=CardKinds||w.shop_cards[i].upgraded>1||!event_card_facts_valid(w.shop_cards[i],w.shop_facts[i]))return false;
    for(auto price:w.shop_base_prices)if(price<0||price>20000)return false;
    if((w.shop_pending_item>=ShopRemove&&w.shop_pending_item!=255)||w.shop_pending_price<0||w.shop_pending_price>9999||
       (w.shop_pending_item==255&&w.shop_pending_price)||w.shop_auto_active>1||w.shop_auto_cursor>ShopRemove+1||w.shop_free_remove>1)return false;
    if((w.shop_pending_item!=255||w.shop_auto_active||w.shop_free_remove)&&w.room_type!=unsigned(WorldRoom::Shop))return false;
    if(w.shop_free_remove&&s.phase!=Phase::Remove)return false;
    if(w.rest_count>MaxWorldRestOptions||w.rest_pending>w.rest_count||w.rest_deck_before>MaxDeck)return false;
    if(w.rest_count<32&&(w.rest_used>>w.rest_count))return false;
    for(unsigned i=0;i<w.rest_count;++i)if(w.rest_options[i]>=unsigned(WorldRestOption::Count))return false;
    if(w.rest_pending&&w.room_type!=unsigned(WorldRoom::Rest))return false;
    return true;
}
