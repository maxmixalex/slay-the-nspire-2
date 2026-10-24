#pragma once
namespace spire::reference_viewport {
// Pinned NGame.ApplyDisplaySettings: FourByThree uses a 1680 x 1260
// content viewport. The calculator keeps the same aspect at 320 x 240.
constexpr int Width=1680,Height=1260;
constexpr int project(int value){return (value*4+(value<0?-10:10))/21;}
constexpr int project_half(int twice_value){return (twice_value*2+(twice_value<0?-10:10))/21;}
}
