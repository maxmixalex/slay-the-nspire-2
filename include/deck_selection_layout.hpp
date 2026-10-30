#pragma once
#include <stdint.h>
#include "core.hpp"

namespace spire {
// NDeckUpgradeSelectScreen / NDeckCardSelectScreen, 1680x1260 -> 320x240.
// These are overlay presentation values, never canonical run/save fields.
namespace deck_selection_layout {
constexpr int GridTop=12,GridBottom=210,UpgradeGridBottom=190,GridInset=15;
constexpr int PromptY=214,PromptH=24,PromptSize=8,PromptPadding=5;
constexpr int PreviewW=96,PreviewH=135,PreviewY=49;
constexpr int BeforeX=38,AfterX=186,SingleX=112;
constexpr int ToggleX=3,ToggleY=195,ToggleIcon=14,ToggleSize=8;
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
