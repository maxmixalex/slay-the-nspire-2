#pragma once
#include "frontend_assets.hpp"

namespace spire {
// NSP2/23 appends pinned UI artwork and locale entries. Historical images,
// strings, glyph ordering, save formats and gameplay identifiers stay stable.
constexpr unsigned Ui6ContentVersion=23;
constexpr unsigned Ui6PowerWearsOffString=8105;
enum Ui6Asset {
    I6_CloseX=FrontendAuxiliaryCount,
    I6_TargetArrowHead,
    I6_TargetArrowSegment,
    I6_CursorDefault,
    I6_CursorTilted,
    I6_CursorInspect,
    I6_PileSortBar,
    I6_PileSortBarSilent,
    I6_PileSortBarDefect,
    I6_PileSortBarRegent,
    I6_PileSortBarNecrobinder,
    I6_SortDescending,
    I6_ScrollbarTrack,
    I6_ScrollbarGrabber,
    I6_ScrollbarEdge,
    I6_CommonBanner,
    I6_CardRemoveStart,
    I6_CardRemoveLast=I6_CardRemoveStart+23,
    I6_PopupVertical,
    I6_PopupCancel,
    I6_PopupConfirm,
    I6_PopupCancelOutline,
    I6_PopupConfirmOutline,
    Ui6AuxiliaryCount
};
constexpr unsigned I6_CardRemoveCount=24;
static_assert(I6_PileSortBar==1281&&I6_CommonBanner==1290,
              "UI6 catalog must match patch_ui6_assets.py");
}
