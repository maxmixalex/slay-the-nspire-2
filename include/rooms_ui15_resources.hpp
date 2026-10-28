#pragma once
#include <stdint.h>
namespace spire {
// NCardSmithVfx's ImpactCircle is a3x3 one-shot flipbook, not one sprite
// containing all nine cells. Hammer/Sparks reuse the original UI14 textures.
constexpr unsigned I15_SmithImpactStart=5275,I15_SmithImpactCount=9;
constexpr unsigned RoomsUi15AuxiliaryCount=I15_SmithImpactStart+I15_SmithImpactCount;
inline unsigned smith_impact_frame(uint32_t age){return age<500?age*9/500:8;}
static_assert(RoomsUi15AuxiliaryCount==5284,"UI15 room resources must match rooms_ui15_assets.py");
}
