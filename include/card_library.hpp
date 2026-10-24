#pragma once
#include <stdint.h>
#include "core.hpp"

namespace spire {
// Sidebar controls follow card_library.tscn, independent from saved gameplay.
enum LibraryControl : int {
    LibrarySearch,LibraryClearSearch,LibraryPoolFirst,LibraryPoolLast=LibraryPoolFirst+7,
    LibrarySortType,LibraryTypeFirst,LibraryTypeLast=LibraryTypeFirst+3,
    LibrarySortRarity,LibraryRarityFirst,LibraryRarityLast=LibraryRarityFirst+3,
    LibrarySortCost,LibraryCostFirst,LibraryCostLast=LibraryCostFirst+4,
    LibrarySortAlphabet,LibraryViewUpgrades,LibraryControls
};
struct CardLibraryPresentation {
    uint8_t types=0,rarities=0,costs=0,sort_descending=15,sort_reverse=0;
    uint8_t sort_priority[4]{1,0,2,3}; // rarity, type, canonical cost, localized title
    int16_t control=LibraryPoolFirst;
    uint32_t epoch=1,query_changed=0,opened=0;
    bool editing=false,query_pending=false;
    char draft[128]{},query[128]{};
    mutable uint32_t cached_epoch=0,cached_generation=0;
    mutable int cached_filter=-1;
    mutable bool cached_upgrades=false;
    mutable uint16_t count=0,ids[CardKinds]{};
    void invalidate(){if(++epoch==0)++epoch;}
};
struct CollectionPresentation {
    static constexpr unsigned MaxRows=96;
    struct Row {uint16_t first=0;uint8_t count=0,category=0;int16_t y=0;};
    uint16_t ids[WorldRelicKinds]{},count=0;
    Row rows[MaxRows]{};uint8_t row_count=0;
    int16_t category_y[16]{};uint8_t category_kind[16]{},category_count=0,tab=255;
    uint32_t generation=0,started=0;
    struct Hover {uint32_t start=0;int16_t index=-1;bool focused=false;} hover[32]{};
};
} // namespace spire
