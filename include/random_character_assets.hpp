#pragma once

namespace spire {
// Full source textures, before the Random character's parent transforms.
// Noise R8 is transported in alpha so RGB565 cannot quantize its samples.
constexpr int I13_RandomGradient=3339;
constexpr int I13_RandomNoise=3340;
constexpr int I13_RandomLight=3341;
constexpr int I13_RandomDot=3342;
constexpr unsigned RandomCharacterUi13AuxiliaryCount=3343;
}
