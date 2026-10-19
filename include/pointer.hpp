#pragma once
#include <stdint.h>
namespace spire {
enum class HitKind : uint8_t {
    None, Menu, Choice, Hand, Enemy, Battlefield, Command,
    PileTab, PileCard, Relic, Potion, Scroll, Close, DetailClose, OpenPile, ReferenceNode, ReferenceAction, SeedCharacter, UiAction, LibraryItem, CombatChoice, StatusActor, StatusItem, OwnedRelic, InstanceCard, DeckCard
};
struct HitRegion {
    int16_t x{}, y{}, w{}, h{};
    HitKind kind = HitKind::None;
    int16_t value{};
    bool enabled = true;
};
constexpr int MaxHits = 96;
struct PointerState {
    int x = 160, y = 120;
    bool visible{}, held{}, dragging{}, dispatching{};
    int down_x{}, down_y{}, armed_card = -1;
    uint32_t context{}, revision{};
    HitRegion captured{};
};
} // namespace spire
