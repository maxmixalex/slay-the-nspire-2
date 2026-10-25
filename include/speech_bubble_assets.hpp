#pragma once
#include "selection_assets.hpp"
namespace spire {
// Projected original Blue/Right speech and popup images follow all v36 assets.
// Their ordinary image format leaves RGR2 and every previous catalog intact.
constexpr unsigned SpeechBubbleContentVersion=37;
enum SpeechBubbleAsset {I10_FakeMerchantWelcome=InspectionAuxiliaryCount,
    I10_PopupVertical,I10_PopupCancel,I10_PopupConfirm,I10_PopupCancelOutline,I10_PopupConfirmOutline,
    SpeechBubbleAuxiliaryCount};
static_assert(I10_FakeMerchantWelcome==2061&&I10_PopupVertical==2062&&I10_PopupConfirmOutline==2066&&SpeechBubbleAuxiliaryCount==2067,
              "Room artwork must preserve all earlier auxiliary indices");
}
