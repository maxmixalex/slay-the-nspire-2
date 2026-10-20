// Original-run state is appended after the historical expedition fields.
// Legacy save readers never reinterpret their xorshift streams as these RNGs.
constexpr unsigned WorldActCount=3,MaxWorldEncounterQueue=32,MaxWorldEventQueue=32;
constexpr unsigned MaxWorldRewardCards=8;
struct WorldActRooms {
    uint8_t ready=0,region=0;
    uint8_t encounter_count=0,elite_count=0,event_count=0,boss_count=0,ancient=0;
    // RoomSet uses monotonically increasing visits and modulo lookups; skipped
    // event candidates also advance its cursor.
    uint16_t encounter_cursor=0,elite_cursor=0,event_cursor=0,boss_cursor=0;
    EncounterId encounters[MaxWorldEncounterQueue]{},elites[15]{},bosses[2]{};
    uint8_t events[MaxWorldEventQueue]{};
};
struct WorldState {
    uint8_t active=0,rng_ready=0,act_index=0,ascension=0;
    uint16_t total_floor=0,rooms_completed=0,combat_count=0;
    int16_t map_column=0,map_row=0;
    uint64_t run_seed=0;
    char seed_text[65]{};
    reference::Random rng[WorldRngCount]{};
    uint8_t room_type=0,current_point=reference::NoPoint;
    uint8_t map_ready[3]{};
    reference::ReferenceMap maps[3]{};
    WorldActRooms rooms[WorldActCount]{};
    uint8_t visited[WorldActCount][reference::MaxPoints]{};
    uint8_t visited_events[67]{};
    uint8_t room_open=0,event_combat_parent=0,parent_room_type=0,final_event=0,finished=0;
    uint8_t point_had_shop=0,finish_pending=0;
    // IEEE binary32 probabilities are serialized without target float ABI.
    uint32_t unknown_monster_bits=0x3dcccccdU,unknown_elite_bits=0xbf800000U;
    uint32_t unknown_shop_bits=0x3cf5c28fU,unknown_treasure_bits=0x3ca3d70aU;
    uint32_t potion_drop_bits=0x3ecccccdU;
    uint8_t reward_count=0,reward_relic_claimed=0;
    Card reward_cards[MaxWorldRewardCards]{};
    EventCardFacts reward_facts[MaxWorldRewardCards]{};
    RelicId reward_relic=NoWorldRelic,treasure_relic=NoWorldRelic;
    RelicId shop_relics[3]{NoWorldRelic,NoWorldRelic,NoWorldRelic};
    Card shop_cards[ShopCardCount]{};
    EventCardFacts shop_facts[ShopCardCount]{};
    int16_t shop_base_prices[ShopRemove+1]{};
    uint8_t shop_sale=0,treasure_gold_claimed=0;
    int16_t treasure_gold=0;
};
