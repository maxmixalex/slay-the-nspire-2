#pragma once
#include "ui7_assets.hpp"
namespace spire {
constexpr unsigned PhobiaContentVersion=25;
constexpr unsigned PhobiaAnimationKinds=1753;
enum PhobiaAsset {
 PhobiaFactory=Ui7AuxiliaryCount,
 PhobiaHive,
 PhobiaKnowledgeDemon,
 PhobiaInsatiable,
 PhobiaAuxiliaryCount
};
static_assert(Ui7AnimationKinds==1710&&Ui7AuxiliaryCount==1877&&PhobiaAuxiliaryCount==1881,
              "Phobia resources append to the complete UI7 catalog");
}
