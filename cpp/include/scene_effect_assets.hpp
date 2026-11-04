#pragma once
#include "scene_interface_assets.hpp"
namespace spire {
// Optional NSP2/40 append; original image and animation IDs stay unchanged.
constexpr unsigned I13_SmallScrollbarTrackEdge = SleepingVfxAuxiliaryCount;
constexpr unsigned I13_SmallScrollbarTrackCenter = I13_SmallScrollbarTrackEdge + 1;
constexpr unsigned I13_SmallScrollbarTrain = I13_SmallScrollbarTrackCenter + 1;
constexpr unsigned FrontendScrollbarAuxiliaryCount = I13_SmallScrollbarTrain + 1;
constexpr unsigned I13_CardEnchantment = FrontendScrollbarAuxiliaryCount;
constexpr unsigned I13_CardTransformMask = I13_CardEnchantment + 1;
constexpr unsigned I13_CardTransformGlow = I13_CardTransformMask + 1;
constexpr unsigned I13_CardTransformShine = I13_CardTransformGlow + 1;
constexpr unsigned I13_CardTransformShineCount = 9;
constexpr unsigned I13_CardTransformSpeck = I13_CardTransformShine + I13_CardTransformShineCount;
constexpr unsigned I13_CardTransformFlare = I13_CardTransformSpeck + 1;
constexpr unsigned I13_CardTransformRing = I13_CardTransformFlare + 1;
constexpr unsigned I13_CardTransformRingCount = 8;
constexpr unsigned I13_EventButton = I13_CardTransformRing + I13_CardTransformRingCount;
constexpr unsigned I13_EventOutline = I13_EventButton + 1;
constexpr unsigned RoomEffectsAuxiliaryCount = I13_EventOutline + 1;
constexpr unsigned I13_ArchitectReading = RoomEffectsAuxiliaryCount;
constexpr unsigned I13_ArchitectReadingCount = 45;
constexpr unsigned I13_ArchitectSilentReading = I13_ArchitectReading + I13_ArchitectReadingCount;
constexpr unsigned I13_ArchitectSilentReadingCount = 79;
constexpr unsigned I13_ArchitectSilentFastReading =
    I13_ArchitectSilentReading + I13_ArchitectSilentReadingCount;
constexpr unsigned I13_ArchitectSilentFastReadingCount = 75;
constexpr unsigned I13_ArchitectTrailOuter =
    I13_ArchitectSilentFastReading + I13_ArchitectSilentFastReadingCount;
constexpr unsigned I13_ArchitectTrailCount = 40;
constexpr unsigned I13_ArchitectTrailInner = I13_ArchitectTrailOuter + I13_ArchitectTrailCount;
constexpr unsigned ArchitectAuxiliaryCount = I13_ArchitectTrailInner + I13_ArchitectTrailCount;
constexpr unsigned PortCreditsString = 8703;
static_assert(I13_SmallScrollbarTrackEdge == 2201 && FrontendScrollbarAuxiliaryCount == 2204,
              "Original dropdown scrollbar append order");
static_assert(I13_CardEnchantment == 2204 && RoomEffectsAuxiliaryCount == 2228,
              "Original card-transform and room append order");
static_assert(I13_ArchitectReading == 2228 && ArchitectAuxiliaryCount == 2507,
              "Original Architect two-track and pen-trail append order");
} // namespace spire
