#pragma once
#include "neow_fire_ui23.hpp"
namespace spire::character_unlock_assets {
// Append-only source character-select textures. Character order is the engine
// enum: Ironclad, Silent, Defect, Regent, Necrobinder, then Random.
constexpr unsigned AuxiliaryFirst=neow_fire_ui23::AuxiliaryCount;
constexpr unsigned LockedPortraitFirst=AuxiliaryFirst,LockedPortraitCount=6;
constexpr unsigned Lock=LockedPortraitFirst+LockedPortraitCount,Unlocked=Lock+1;
constexpr unsigned AuxiliaryCount=Unlocked+1;
constexpr unsigned LockedTitle=8704,LockedRelicTitle=8705,LockedRelicDescription=8706,StringCount=8707;
// Accept previously installed UI24/25 resources without restoring their UI.
constexpr unsigned LegacyStringCount=8961;
constexpr unsigned FirstImage=neow_fire_ui23::Images,Images=FirstImage+8;
static_assert(AuxiliaryFirst==7229&&AuxiliaryCount==7237&&FirstImage==60294,
              "Character lock catalog follows the complete Ancient catalog");
}
