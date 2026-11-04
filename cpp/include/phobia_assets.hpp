#pragma once
#include "extended_interface_assets.hpp"
namespace spire {
constexpr unsigned PhobiaContentVersion = 25;
constexpr unsigned PhobiaAnimationKinds = 1753;
enum PhobiaAsset {
    PhobiaFactory = ExtendedUiAuxiliaryCount,
    PhobiaHive,
    PhobiaKnowledgeDemon,
    PhobiaInsatiable,
    PhobiaAuxiliaryCount
};
static_assert(ExtendedUiAnimationKinds == 1710 && ExtendedUiAuxiliaryCount == 1877 &&
                  PhobiaAuxiliaryCount == 1881,
              "Phobia resources append to the complete legacy catalog");
} // namespace spire
