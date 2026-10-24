#pragma once
#include "card_resource_assets.hpp"
namespace spire {
// An append-only optional catalog in the existing NSP2/33 layout. Older
// catalogs remain readable; the seven layers and their text travel together.
constexpr unsigned FtueContentVersion=33;
enum FtueAsset {
 I9_FtuePopup=CardResourceAuxiliaryCount,
 I9_FtueConfirm,
 I9_FtueConfirmOutline,
 I9_FtueArrow,
 I9_FtueCombat0,
 I9_FtueCombat1,
 I9_FtueCombat2,
 FtueAuxiliaryCount
};
constexpr unsigned FtueTextFirst=8347,FtueTextCount=31;
static_assert(I9_FtuePopup==2051&&FtueAuxiliaryCount==2058,
              "Tutorial layers preserve every earlier auxiliary ID");
}
