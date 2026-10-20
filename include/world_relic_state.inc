// Included inside spire after EventCardFacts; no transient pointer is saved.
constexpr unsigned MaxWorldRelicSteps=64,MaxWorldRelicOffers=192,WorldRelicBagSize=128;
constexpr unsigned MaxWorldRelicHookFrames=32;
constexpr unsigned MaxWorldRelicRewards=32,MaxWorldRelicRewardGroups=8;
struct WorldRelicCardSnapshot {
 uint16_t token=0,genetic=0,scythe=0;
 EventCardFacts facts{};
};
struct WorldRelicInstance {
 RelicId id=NoWorldRelic;
 uint16_t floor=0,serial=0;
 uint8_t flags=0,skin=0;
 int32_t values[8]{};
 uint8_t marked_points[8]{};
 uint16_t card_tokens[2]{};
 EventCardFacts card_facts[2]{};
};
struct WorldRelicStep {
 WorldRelicOp op=WorldRelicOp::None;
 uint16_t owner=0;
 int32_t a=0,b=0,c=0;
};
struct WorldRelicHookFrame {
 WorldRelicHook hook=WorldRelicHook::BeforeCombatStart;
 uint8_t active=0;
 uint16_t count=0,cursor=0,serials[WorldRelicCapacity]{};
 int32_t args[3]{};
};
struct WorldRelicReward {
 uint8_t group=0,kind=0,flags=0,count=0; // kind: 1 card, 2 relic, 3 potion, 4 gold.
 uint16_t value=0,tokens[8]{};
 EventCardFacts facts[8]{};
 uint16_t rarity_mask=0,creation_flags=0;
 uint8_t pools[11]{},pool_count=0,odds=0,source=0;
};
struct WorldRelicPrepared {
 RelicId id=NoWorldRelic;
 int16_t values[2]{};
};
struct WorldRelicState {
 uint8_t active=0,bags_ready=0;
 uint16_t count=0,last_serial=0;
 WorldRelicInstance entries[WorldRelicCapacity]{};
 // Shared and player bags, each split into Common/Uncommon/Rare/Shop deques.
 uint16_t bag_sizes[2][4]{};
 RelicId bags[2][4][WorldRelicBagSize]{};
 uint8_t step_count=0,step_cursor=0,running=0;
 WorldRelicStep steps[MaxWorldRelicSteps]{};
 WorldRelicChoice choice=WorldRelicChoice::None;
 uint8_t return_phase=0,choice_filter=0,choice_cancelable=0;
 uint16_t choice_owner=0,choice_min=0,choice_max=0;
 EventEnchantment choice_enchantment=EventEnchantment::None;
 int16_t choice_amount=0;
 uint16_t offer_count=0,offers[MaxWorldRelicOffers]{};
 EventCardFacts offer_facts[MaxWorldRelicOffers]{};
 uint16_t selected_count=0,selected[MaxDeck]{};
 uint8_t reward_count=0,group_count=0,choice_group=0,choice_reward=0;
 uint8_t group_active[MaxWorldRelicRewardGroups]{},group_skip[MaxWorldRelicRewardGroups]{};
 uint16_t group_owner[MaxWorldRelicRewardGroups]{};
 WorldRelicReward rewards[MaxWorldRelicRewards]{};
 uint8_t prepared_count=0;
 WorldRelicPrepared prepared[16]{};
 uint8_t tooth_count=0;
 WorldRelicCardSnapshot tooth_cards[5]{};
 uint32_t play_serial_counter=0,play_serial[MaxAutoFrames+1]{};
 uint8_t card_is_dupe[MaxCards]{};
 uint16_t manual_plays_turn=0;
 int16_t self_forming_clay=0,reptile_trinket=0,helical_dart=0;
 uint8_t byrdpip_present=0,legion_present=0;
 uint16_t last_flash=NoWorldRelic;
 uint32_t flash_revision=0;
 // Snapshot listener identity, not slot numbers: replacement/removal during
 // one hook must not change which later listeners were present at its start.
 WorldRelicHookFrame hook_frames[MaxWorldRelicHookFrames]{};
};
