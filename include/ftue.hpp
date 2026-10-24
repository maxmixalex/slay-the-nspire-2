#pragma once
#include <stdint.h>
namespace spire {
// Pinned single-player FTUE identities. Keep the persisted bit order stable;
// screen pages and highlight animation are transient presentation state.
enum class FtueId:uint8_t {
    AcceptTutorials,CombatRules,MapSelect,CanPlayCards,CannotPlayCard,
    PowerCard,Shuffle,CombatReward,RelicReward,ObtainPotion,Merchant,
    RestSite,AscensionSingleplayer,Count
};
constexpr unsigned FtueCount=unsigned(FtueId::Count);
constexpr uint16_t FtueSeenMask=(uint16_t(1)<<FtueCount)-1;
constexpr uint16_t FtueEnabled=uint16_t(1)<<13,FtueInitialized=uint16_t(1)<<14;
constexpr uint16_t FtueFresh=FtueInitialized|FtueEnabled;
constexpr bool ftue_flags_valid(uint16_t flags)noexcept{
    return !(flags&uint16_t(~(FtueSeenMask|FtueEnabled|FtueInitialized)))&&
        (!flags||bool(flags&FtueInitialized));
}
constexpr uint16_t ftue_bit(FtueId id)noexcept{
    return unsigned(id)<FtueCount?uint16_t(uint16_t(1)<<unsigned(id)):0;
}
inline const char*ftue_key(FtueId id)noexcept{
    static constexpr const char*keys[]={"accept_tutorials_ftue","combat_rules_ftue","map_select_ftue",
        "can_play_cards_ftue","cannot_play_card_ftue","power_card_ftue","shuffle_ftue","combat_reward_ftue",
        "obtain_relic_ftue","obtain_potion_ftue","merchant_ftue","rest_site_ftue","ascension_singleplayer_ftue"};
    return unsigned(id)<FtueCount?keys[unsigned(id)]:"";
}
static_assert(FtueCount==13,"NSH8 keeps thirteen single-player tutorial bits");
} // namespace spire
