#pragma once
#include "history_journal_ui8_assets.hpp"
namespace spire {
// The original Game Over shader mask is sampled at display-pixel centers.
// Its two horizontal bilinear taps are retained as an exact 0..510 scalar in
// the existing 16-bit image storage, rather than quantized to RGB565 red.
constexpr unsigned GameOverTransitionContentVersion=31;
constexpr unsigned GameOverTransitionAuxiliaryCount=2050;
constexpr int I9_GameOverTransition=2049;
static_assert(I9_GameOverTransition==HistoryJournalAuxiliaryCount,"Preserve every v30 auxiliary index");
namespace game_over_transition {
constexpr unsigned Width=320,Height=240,Denominator=510;
constexpr unsigned Alpha=230; // round(0.9 * 255), source material alpha.
// step(1-texture.r, mix(-0.1, 1.1, threshold)). The threshold clock uses
// the presentation's existing 12-bit source easing. Computing this once per
// frame leaves one scalar comparison per pixel and no floating-point work.
inline int cutoff(unsigned phase){if(phase>4096)phase=4096;return 561-int(612*phase/4096);}
inline bool covered(unsigned sample,unsigned phase){return int(sample)>=cutoff(phase);}
}
}
