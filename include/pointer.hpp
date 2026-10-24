#pragma once
#include <stdint.h>
namespace spire {
enum class HitKind : uint8_t {
    None, Menu, Choice, Hand, Enemy, Battlefield, Command,
    PileTab, PileCard, Relic, Potion, Scroll, Close, DetailClose, OpenPile, ReferenceNode, ReferenceAction, SeedCharacter, UiAction, LibraryItem, CombatChoice, StatusActor, StatusItem, OwnedRelic, InstanceCard, DeckCard,
    WorldNode, EnemySelect, WorldChoice, CrystalCell, EnemyIntent, BattleStatus,
    PotionSlot, PotionPage, PotionMerchant, HudInfo, PileSort, PileUpgrades, DebugItem,
    GridScroll, BestiaryEntry, BestiaryMove, BestiaryFilter, BestiaryMode, BestiaryScroll, ProgressScroll, ProgressThumb, HistoryCard, HistoryRelic, HistoryFloor, LibraryControl, RunModifier, Orb, MapDrawingTool, InputBinding, CombatPeek, AncientDialogueNext, AuxiliaryControl, RewardScroll, PowerInstance
};
struct HitRegion {
    int16_t x{}, y{}, w{}, h{};
    HitKind kind = HitKind::None;
    int16_t value{};
    bool enabled = true;
};
constexpr int MaxHits = 384; // Crystal cells, or expanded creature power rows and HUD.
struct PointerState {
    int x = 160, y = 120;
    bool visible{}, held{}, dragging{}, dispatching{};
    int down_x{}, down_y{}, armed_card = -1;
    uint32_t context{}, revision{}, motion_time{};
    HitRegion captured{};
};
} // namespace spire
