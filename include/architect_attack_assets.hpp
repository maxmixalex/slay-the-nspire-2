#pragma once
#include "ui13_assets.hpp"
namespace spire {
struct ArchitectAttackAsset {
 uint16_t first,frames,lifetime;
 uint8_t fps,frame_ms,planes;
 uint8_t pivot_x=160,pivot_y=120;
 enum:uint8_t {Warp=1,Add=2};
 unsigned frame(unsigned age)const{const unsigned i=frame_ms?age/frame_ms:age*fps/1000u;return i<frames?i:frames-1;}
};
// Source effect order is ArchitectPresentation::Vfx, followed by FireBurst.
// Each extra plane retains the same authored sample times as its main plane.
static constexpr ArchitectAttackAsset ArchitectAttackAssets[]={
 {2507,38,1250,30,0,0},{2545,66,2200,30,0,ArchitectAttackAsset::Warp},
 {2677,4,268,0,67,0},{2681,90,3000,30,0,0},{2771,90,3000,30,0,0},
 {2861,30,1000,30,0,0},{2891,10,334,30,0,0,245},{2901,60,2000,30,0,0},
 {2961,60,2000,30,0,0},{3021,5,334,15,0,0},{3026,5,167,30,0,0},
 {3031,60,2000,30,0,ArchitectAttackAsset::Warp},
 {3151,60,2000,30,0,ArchitectAttackAsset::Add},{3271,60,2000,30,0,0}
};
constexpr unsigned ArchitectFireBurst=13;
constexpr unsigned ArchitectAttackUi13AuxiliaryCount=3331;
static_assert(ArchitectUi13AuxiliaryCount==ArchitectAttackAssets[0].first,
              "Architect reading/trails precede the original attack effects");
static_assert(ArchitectAttackAssets[13].first+ArchitectAttackAssets[13].frames==ArchitectAttackUi13AuxiliaryCount,
              "Every final-event source effect has a bounded private image range");
}
