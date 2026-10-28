// v15: one original enchantment per card, independent of enemy afflictions.
enum class EventEnchantment:uint8_t {
 None,Corrupted,Goopy,Instinct,Nimble,PerfectFit,Sharp,Slither,SoulsPower,Sown,
 Spiral,Steady,Swift,Vigorous,Adroit,Clone,Glam,Imbued,Momentum,RoyallyApproved,
 TezcatarasEmber,Count
};
constexpr unsigned EventEnchantmentKinds=(unsigned)EventEnchantment::Count;
enum class TinkerRider:uint8_t {
 None,Sapping,Violence,Choking,Energized,Wisdom,Chaos,Expertise,Curious,Improvement,Count
};
struct EventCardFacts {
 EventEnchantment enchantment=EventEnchantment::None;
 uint8_t science_type=0,rider=0;
 int16_t enchantment_amount=0,energy_offset=0,progress=0;
};
struct EventDeckSnapshot {
 Card card{};
 EventCardFacts facts{};
 uint16_t genetic=0,scythe=0;
};
enum class EventCardPower:uint8_t { HelloWorld,Curious,Improvement,FeedingFrenzy,Rebound,Count };
constexpr unsigned EventCardPowerKinds=(unsigned)EventCardPower::Count;
struct EventCardState {
 EventCardFacts deck[MaxDeck]{},cards[MaxCards]{};
 int16_t deck_origin[MaxCards]{};
 uint8_t enchantment_disabled[MaxCards]{};
 int16_t momentum[MaxCards]{},maul_damage[MaxCards]{};
 int16_t powers[EventCardPowerKinds]{};
 uint8_t power_order[EventCardPowerKinds+MaxPowerInstances]{},power_count=0;
 int16_t toric_block[MaxPowerInstances]{};
 uint8_t toric_turns[MaxPowerInstances]{};
 uint16_t plays_started=0;
 uint8_t end_hand_count=0,choice_copies=0;
 uint8_t rebound_return[MaxAutoFrames+1]{};
};
