#pragma once
#include <stdint.h>
#include "core.hpp"

namespace spire {
// NDeckUpgradeSelectScreen / NDeckCardSelectScreen, 1680x1260 -> 320x240.
// These are overlay presentation values, never canonical run/save fields.
namespace deck_selection_layout {
constexpr int GridTop=15,GridBottom=240,GridInset=15;
constexpr int PromptY=229,PromptH=8,PromptSize=5,PromptPadding=3;
constexpr int PreviewW=57,PreviewH=80,PreviewY=80;
constexpr int BeforeX=78,AfterX=185,SingleX=131;
constexpr int ToggleX=3,ToggleY=227,ToggleIcon=9,ToggleSize=4;
constexpr unsigned PreviewBackstop=199;
}
struct DeckSelectionPresentation {
    uint32_t revision=UINT32_MAX,opened=0,preview_opened=0;
    uint8_t phase=255;
    bool bound=false,show_upgrades=false,preview=false;
    void reset(){*this=DeckSelectionPresentation{};}
};
static_assert(sizeof(DeckSelectionPresentation)<=16,"Deck selection must not add a card/state cache");
enum class WorldSelectionKind:uint8_t {None,Deck,Upgrade,Transform,Enchant,Simple,Bundle};
struct WorldSelectionCard {
    Card card{};
    EventCardFacts facts{};
    int16_t deck=-1;
    int16_t x=0,y=0,w=0,h=0;
    bool upgraded=false,hover=true;
};
}
