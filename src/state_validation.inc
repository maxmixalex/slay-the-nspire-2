bool validate(const State&s){
    if((int)s.phase>(int)Phase::ExtendedChoice||s.practice>1||s.floor>MapRows||s.max_hp<1||s.max_hp>999||s.hp<0||s.hp>s.max_hp||s.energy<0||s.energy>999||s.turn<0||s.turn>20000)return false;
    if((unsigned)s.character>=CharacterKinds||s.noxious_fumes<0||s.noxious_fumes>9999||s.accuracy<0||s.accuracy>9999||s.discard_required>MaxHand)return false;
    if(s.phase!=Phase::DiscardChoice&&s.discard_required)return false;
    if(s.phase==Phase::DiscardChoice&&(!s.discard_required||s.discard_required>s.hand.count))return false;
    if(s.exhaust_limit>MaxHand||s.no_block<0||s.no_block>9999||s.attacked_this_turn>1)return false;
    if(s.phase!=Phase::ExhaustManyChoice&&s.exhaust_limit)return false;
    if(s.phase==Phase::ExhaustManyChoice&&(!s.exhaust_limit||s.exhaust_limit>s.hand.count))return false;
    if(s.demon_form<0||s.demon_form>9999||s.infinite_blades<0||s.infinite_blades>MaxCards||
       s.block_next_turn<0||s.block_next_turn>9999||s.draw_next_turn<0||s.draw_next_turn>MaxCards||
       s.flame_barrier<0||s.flame_barrier>9999)return false;
    if(s.relic_flash&~((1u<<RelicKinds)-1))return false;
    if(s.no_draw>1||s.reward_claimed>7||(unsigned)s.reward_potion>PotionKinds||s.potion_chance>100)return false;
    if(s.temporary_strength<0||s.temporary_strength>s.strength||s.temporary_dexterity<0||s.temporary_dexterity>s.dexterity||
       s.vigor<0||s.vigor>9999||s.attack_vigor<0||s.attack_vigor>9999||s.thorns<0||s.thorns>9999)return false;
    if(s.block<0||s.block>9999||s.strength<0||s.strength>9999||s.pain<0||s.pain>9999||s.embrace<0||s.embrace>99)return false;
    if(s.deck_count<1||s.deck_count>MaxDeck||s.card_count>MaxCards||s.hand.count>MaxHand||s.effect_count>MaxEffects||s.log_count>24||s.power_count>2)return false;
    if(s.expansion.depth>MaxAutoFrames||s.enemy_count>2||s.playing< -1||s.playing>=s.card_count)return false;
    if(s.current_node< -1||s.current_node>=MapNodes||s.next_row>MapRows||s.encounter>1||s.gold<0||s.gold>9999||s.last_gold<0||s.last_gold>99)return false;
    if((unsigned)s.event_id>=EventKinds||s.event_page>4||s.event_remaining>2||s.event_first< -1||s.event_first>=s.deck_count||s.event_value<0||s.event_value>9999)return false;
    if(s.shop_sold>16383)return false;
    for(auto id:s.shop_relics)if(id&&!relic_definition(id))return false;
    for(auto p:s.shop_potions)if(p>PotionKinds)return false;
    if(s.relic_count>RelicKinds||s.dexterity<0||s.dexterity>9999||s.double_attack>1)return false;
    uint32_t owned=0;
    for(unsigned i=0;i<s.relic_count;i++){unsigned index=s.relic_order[i];if(index>=RelicKinds)return false;unsigned bit=relic_definitions[index].id;if(owned&bit)return false;owned|=bit;}
    if(owned!=s.relics)return false;
    for(int i=0;i<RelicKinds;i++){
        unsigned id=relic_definitions[i].id,limit=relic_counter_limit(id);
        unsigned maximum=id==CentennialPuzzle?1:limit?limit-1:0;
        if(s.relic_counters[i]>maximum||(!(s.relics&id)&&s.relic_counters[i]))return false;
    }
    if(s.treasure_state>2)return false;
    if(s.phase!=Phase::Treasure&&(s.treasure_state||s.treasure_relic))return false;
    if(!s.treasure_state&&s.treasure_relic)return false;
    if(s.treasure_relic&&(!relic_definition(s.treasure_relic)||s.treasure_relic==BurningBlood||s.treasure_relic==RingOfTheSnake))return false;
    if(s.treasure_relic&&s.treasure_state==1&&(s.relics&s.treasure_relic))return false;
    if(s.treasure_relic&&s.treasure_state==2&&!(s.relics&s.treasure_relic))return false;
    for(auto p:s.potions)if((unsigned)p>PotionKinds)return false;
    for(auto c:s.shop_cards)if(c>=CardKinds)return false;
    for(auto p:s.shop_prices)if(p<0||p>9999)return false;
    for(int i=0;i<MapNodes;i++){auto n=s.map[i];if((int)n.room>(int)Room::Final||n.links>7||n.visited>1||n.event>=EventKinds)return false;}
    if(s.phase==Phase::Map&&s.next_row>=MapRows)return false;
    if(s.phase==Phase::Transform&&((s.event_id!=EventId::Grove&&s.event_id!=EventId::AromaOfChaos)||s.event_remaining<1))return false;
    if((unsigned)s.event_deck_action>(unsigned)EventDeckAction::Transform)return false;
    if(s.event_deck_action!=EventDeckAction::None){
        if(s.event_page!=0||s.event_first!=-1)return false;
        if(s.event_deck_action==EventDeckAction::Upgrade&&(s.event_id!=EventId::AromaOfChaos||s.phase!=Phase::Upgrade))return false;
        if(s.event_deck_action==EventDeckAction::Remove&&(s.event_id!=EventId::DoorsOfLightAndDark||s.phase!=Phase::Remove))return false;
        if(s.event_deck_action==EventDeckAction::Transform&&(s.event_id!=EventId::AromaOfChaos||s.phase!=Phase::Transform||s.event_remaining!=1))return false;
        if(s.event_deck_action!=EventDeckAction::Transform&&s.event_remaining)return false;
    }else if(s.phase==Phase::Transform&&s.event_id==EventId::AromaOfChaos)return false;
    if(combat_phase(s.phase)&&(!s.enemy_count||s.turn<1))return false;
    for(int i=0;i<s.deck_count;i++)if(s.deck[i].kind>=CardKinds||s.deck[i].upgraded>1)return false;
    for(int i=0;i<s.card_count;i++)if(s.cards[i].kind>=CardKinds||s.cards[i].upgraded>1)return false;
    for(int i=0;i<3;i++)if(s.reward[i]>=CardKinds)return false;
    for(int i=0;i<s.power_count;i++)if(s.power_order[i]>1||(i&&s.power_order[0]==s.power_order[1]))return false;
    uint8_t seen[MaxCards]={};
    for(int z=0;z<5;z++){const Pile&p=pile(s,(Zone)z);if(p.count>MaxCards)return false;
        for(int i=0;i<p.count;i++){int id=p.ids[i];if(id>=s.card_count||seen[id]||s.zones[id]!=(Zone)z)return false;seen[id]=1;}}
    if(s.playing>=0){if(seen[s.playing]||s.zones[s.playing]!=Zone::Limbo)return false;seen[s.playing]=1;}
    for(unsigned i=0;i<s.expansion.depth;i++){
        int id=s.expansion.frames[i].playing;if(id<0)continue;
        if(id>=s.card_count||seen[id]||s.zones[id]!=Zone::Limbo)return false;seen[id]=1;
    }
    for(int i=0;i<s.card_count;i++)if(!seen[i])return false;
    for(int i=0;i<s.enemy_count;i++){const auto&e=s.enemies[i];if(e.hp<0||e.hp>e.max_hp||e.max_hp<42||e.max_hp>46||e.block<0||e.block>9999||e.strength< -9999||e.strength>9999||e.vulnerable<0||e.vulnerable>9999||e.move>2||e.weak<0||e.weak>9999||e.poison<0||e.poison>9999)return false;}
    for(int i=0;i<s.effect_count;i++){
        const auto&e=s.effects[i];
        if(e.kind==EffectKind::Extended){if(!valid_extension_effect(s,e))return false;continue;}
        if((unsigned)e.kind>(unsigned)EffectKind::EscapeSkillBlock||e.value<0||e.value>9999||e.target<0||e.target>=(e.kind==EffectKind::WhirlwindHits?1000:MaxEnemies)||e.aux)return false;
        if(e.kind==EffectKind::ExhaustCard&&e.value>=s.card_count)return false;
    }
    if(choice_phase(s.phase)&&(!s.effect_count||!choice_pile(s).count))return false;
    if(choice_phase(s.phase)&&s.phase!=Phase::ExtendedChoice&&s.playing<0)return false;
    if(!choice_phase(s.phase)&&s.phase!=Phase::Error&&(s.playing>=0||s.effect_count))return false;
    if(!validate_extension(s))return false;
    return true;
}
