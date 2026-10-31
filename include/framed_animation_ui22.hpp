#pragma once
#include "framed_animation_ui20.hpp"
#include "bestiary_kaiser_assets.hpp"

namespace spire {
// Direct metadata lookup adds no decoded-image lookup or per-frame IO.
// Entries name private parameter images; zero means use the earlier path.
// The existing fixed directory reservation contains this small append.
constexpr unsigned FramedUi22BaseAuxiliary=FramedUi20AuxiliaryCount;
constexpr unsigned FramedUi22ClipCount=BestiaryKaiserAnimationKinds;
constexpr unsigned FramedUi22AuxiliaryCount=FramedUi22BaseAuxiliary+FramedUi22ClipCount+1;
inline int framed_ui22_auxiliary(int clip){
 return clip==FramedUi20OstyDeadLoop?int(FramedUi22BaseAuxiliary+FramedUi22ClipCount):
  clip>=0&&unsigned(clip)<FramedUi22ClipCount?int(FramedUi22BaseAuxiliary)+clip:-1;
}
}
