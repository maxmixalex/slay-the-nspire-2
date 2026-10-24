#pragma once
#include "frontend_ui7_assets.hpp"
namespace spire {
constexpr unsigned Ui7ContentVersion=24;
enum Ui7Asset {
 I7_PotionErrorBackground=FrontendUi7AuxiliaryCount,
 I7_BlockBreakLeft,
 I7_BlockBreakRight,
 I7_LongRice,
 I7_ShortRice,
 I7_Speck,
 Ui7AuxiliaryCount
};
static_assert(I7_PotionErrorBackground==1432&&Ui7AuxiliaryCount==1438,"UI7 asset order");
}
