#pragma once
#include "character_death_assets.hpp"
namespace spire {
constexpr unsigned HistoryJournalContentVersion=30;
constexpr unsigned HistoryJournalAuxiliaryFirst=2045,HistoryJournalAuxiliaryCount=2049;
constexpr unsigned HistoryJournalAnimationKinds=CharacterDeathAnimationKinds;
static_assert(HistoryJournalAuxiliaryFirst==CharacterDeathAuxiliaryCount,"Journal artwork only appends to v29");
enum HistoryJournalUiAsset {
 I8_HistoryGold=2045,I8_HistoryCard,I8_HistoryChest,I8_HistoryPotion
};
}
