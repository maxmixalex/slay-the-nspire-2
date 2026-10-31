#pragma once
#include <stdint.h>
#include "character_select_delta.hpp"

namespace spire::retired_assets_ui22 {
// A complete, separately guarded resource profile. An older launcher rejects
// this endpoint instead of interpreting retired legacy images as live art.
constexpr unsigned AuxiliaryFirst=character_select_delta::AuxiliaryCount;
constexpr unsigned AuxiliaryCount=AuxiliaryFirst+1;
constexpr unsigned MarkerImage=58612,Images=MarkerImage+1;
constexpr uint32_t Magic=0x4c323252u,Revision=1,PlanTag=0x47b3fe1eu,RetiredImages=1598;
static_assert(AuxiliaryFirst==7211&&AuxiliaryCount==7212,"Pinned UI22 retirement profile");
}
