#pragma once
#include "ftue_assets.hpp"
namespace spire {
// The original three-arrow upgrade preview shares one appended texture.
// Highlight is shader data: 290x181 format 0, with each 145x181 logical
// texel stored as (alpha UNORM16, RGB565), not 290 ordinary color pixels.
// Older v33 catalogs remain valid and retain every existing auxiliary ID.
constexpr unsigned SelectionContentVersion=33;
enum SelectionAsset {I9_UpgradeArrow=FtueAuxiliaryCount,I9_CardHighlightSdf,SelectionAuxiliaryCount};
static_assert(I9_UpgradeArrow==2058&&I9_CardHighlightSdf==2059&&SelectionAuxiliaryCount==2060,
              "Selection artwork follows the complete tutorial catalog");
}
