#pragma once
#include "reference_viewport.hpp"
#include "input_bindings.hpp"
#include "settings_focus.hpp"

namespace spire::settings_layout {
using reference_viewport::project;
// settings_screen.tscn: the source Clipper begins at187 and each panel at24.
// Keep the64px controls and82px General row pitch. Only the two implemented
// host panels are exposed; dropping Audio/Graphics does not enlarge the tabs.
constexpr int TabWidth=project(256),TabHeight=project(90),TabTop=project(72);
constexpr int tab_left(unsigned i){return project(840-258+int(i)*260);}
constexpr int GeneralLeft=project(334),GeneralWidth=project(1012);
constexpr int LabelLeft=project(346),ControlLeft=project(1014),ControlWidth=project(320);
constexpr int RowHeight=project(64),LabelFont=project(28),TabFont=project(32);
constexpr int general_row(unsigned i){return project(211+int(i)*82);}
constexpr int general_divider(unsigned i){return project(211+int(i)*82+72);}
constexpr unsigned GeneralRows=11; // Includes the source Reset Gameplay row.
constexpr int InputPanelLeft=project(290),InputWidth=project(1100);
constexpr int InputControlLeft=project(1058),KeyboardTop=project(251);
constexpr int InputResetTop=project(341),InputResetLeft=project(1070);
constexpr int InputHeaderTop=project(443),InputHeaderHeight=project(36);
constexpr int InputLabelLeft=project(302),InputLabelWidth=project(308);
// Removing the unsupported controller column leaves two expanding bindings.
constexpr int binding_left(unsigned bank){return project(634+int(bank)*390);}
constexpr int BindingWidth=project(366);
constexpr int input_row(unsigned i){return project(491+int(i)*64);}
constexpr unsigned InputVisible=10,InputMaximum=InputCommandCount-InputVisible;
constexpr int InputBottom=project(1131),InputTrackLeft=project(1484);
constexpr int DropdownTop=project(275),DropdownHeight=project(88);
constexpr int dropdown_row(unsigned i){return project(275+int(i)*44);}
constexpr int dropdown_row_height(unsigned i){return dropdown_row(i+1)-dropdown_row(i);}
constexpr int DropdownFont=project(22),TickSize=10,ModifierTickSize=7;
static_assert(GeneralRows==11&&InputVisible<InputCommandCount,"All implemented General options fit the source panel");
}
