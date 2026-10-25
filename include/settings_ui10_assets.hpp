#pragma once
#include "history_body_ui10_assets.hpp"
namespace spire {
constexpr unsigned SettingsUi10ContentVersion=39;
enum SettingsUi10Asset {
 I10_SettingsTab=HistoryBodyAuxiliaryCount,I10_SettingsTabStroke,
 I10_SettingsTicked,I10_SettingsUnticked,I10_ModifierTicked,I10_ModifierUnticked,
 I10_SettingsArrowLeft,I10_SettingsArrowRight,I10_SettingsArrowDown,
 I10_SettingsFeedback,I10_SettingsCredits,I10_SettingsReset,I10_CustomRandomize,
 I10_StandardPanel,I10_StandardPanelFocus,I10_StandardIcon,
 I10_DailyPanel,I10_DailyPanelFocus,I10_DailyIcon,
 I10_CustomPanel,I10_CustomPanelFocus,I10_CustomIcon,
 SettingsUi10AuxiliaryCount
};
static_assert(I10_SettingsTab==2132&&SettingsUi10AuxiliaryCount==2154,"Source settings and run setup artwork order");
}
