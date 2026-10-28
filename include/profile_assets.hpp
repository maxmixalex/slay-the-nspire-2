#pragma once
#include "phobia_assets.hpp"
namespace spire {
constexpr unsigned ProfileContentVersion=26,ProfileAnimationKinds=PhobiaAnimationKinds;
enum ProfileAsset {
 ProfileIcon1=PhobiaAuxiliaryCount,ProfileIcon2,ProfileIcon3,
 ProfilePanel,ProfilePanelFocus,ProfileCurrentMarker,
 ProfileDelete,ProfileDeleteFocus,ProfileAuxiliaryCount
};
static_assert(ProfileIcon1==1881&&ProfileAuxiliaryCount==1889,"Profile resource order");
}
