#pragma once
#include "architect_attack_assets.hpp"
namespace spire {
constexpr unsigned I13_DoomGlow = ArchitectAttackAuxiliaryCount;
constexpr unsigned I13_DoomHole = I13_DoomGlow + 1;
constexpr unsigned I13_DoomGlobs = I13_DoomHole + 1;
constexpr unsigned I13_DoomFlame = I13_DoomGlobs + 1;
constexpr unsigned I13_DoomFill = I13_DoomFlame + 1;
constexpr unsigned I13_DoomSpear = I13_DoomFill + 1;
constexpr unsigned I13_DoomNoise = I13_DoomSpear + 1;
constexpr unsigned I13_DoomFlameMask = I13_DoomNoise + 1;
constexpr unsigned DoomAuxiliaryCount = I13_DoomFlameMask + 1;
static_assert(I13_DoomGlow == 3331 && DoomAuxiliaryCount == 3339,
              "Original Doom source texture order");
} // namespace spire
