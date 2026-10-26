#pragma once
#include "ui12_assets.hpp"
namespace spire {
// Optional UI40 append; original image and animation IDs stay unchanged.
constexpr unsigned I13_SmallScrollbarTrackEdge=SleepingVfxAuxiliaryCount;
constexpr unsigned I13_SmallScrollbarTrackCenter=I13_SmallScrollbarTrackEdge+1;
constexpr unsigned I13_SmallScrollbarTrain=I13_SmallScrollbarTrackCenter+1;
constexpr unsigned FrontendUi13AuxiliaryCount=I13_SmallScrollbarTrain+1;
constexpr unsigned I13_CardEnchantment=FrontendUi13AuxiliaryCount;
constexpr unsigned I13_CardTransformMask=I13_CardEnchantment+1;
constexpr unsigned I13_CardTransformGlow=I13_CardTransformMask+1;
constexpr unsigned I13_CardTransformShine=I13_CardTransformGlow+1;
constexpr unsigned I13_CardTransformShineCount=9;
constexpr unsigned I13_CardTransformSpeck=I13_CardTransformShine+I13_CardTransformShineCount;
constexpr unsigned I13_CardTransformFlare=I13_CardTransformSpeck+1;
constexpr unsigned I13_CardTransformRing=I13_CardTransformFlare+1;
constexpr unsigned I13_CardTransformRingCount=8;
constexpr unsigned I13_EventButton=I13_CardTransformRing+I13_CardTransformRingCount;
constexpr unsigned I13_EventOutline=I13_EventButton+1;
constexpr unsigned RoomsUi13AuxiliaryCount=I13_EventOutline+1;
constexpr unsigned PortCreditsString=8703;
static_assert(I13_SmallScrollbarTrackEdge==2201&&FrontendUi13AuxiliaryCount==2204,
              "Original dropdown scrollbar append order");
static_assert(I13_CardEnchantment==2204&&RoomsUi13AuxiliaryCount==2228,
              "Original card-transform and room append order");
}
