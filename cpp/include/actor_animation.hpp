#pragma once
#include "layered_animation.hpp"
#include "bestiary_kaiser_assets.hpp"

namespace spire {
// Direct metadata lookup adds no decoded-image lookup or per-frame IO.
// Entries name private parameter images; zero means use the earlier path.
// The existing fixed directory reservation contains this small append.
constexpr unsigned ActorAnimationBaseAuxiliary = LayeredAnimationAuxiliaryCount;
constexpr unsigned ActorAnimationClipCount = BestiaryKaiserAnimationKinds;
constexpr unsigned ActorAnimationAuxiliaryCount =
    ActorAnimationBaseAuxiliary + ActorAnimationClipCount + 1;
inline int actor_animation_auxiliary(int clip) {
    return clip == LayeredAnimationOstyDeadLoop
               ? int(ActorAnimationBaseAuxiliary + ActorAnimationClipCount)
           : clip >= 0 && unsigned(clip) < ActorAnimationClipCount
               ? int(ActorAnimationBaseAuxiliary) + clip
               : -1;
}
} // namespace spire
