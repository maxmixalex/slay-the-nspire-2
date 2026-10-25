#pragma once
#include "reference_viewport.hpp"
#include "input_bindings.hpp"
#include "settings_focus.hpp"

namespace spire::settings_layout {
using reference_viewport::project;
// settings_screen.tscn: the source Clipper begins at187 and each panel at24.
// Keep the64px controls and82px General row pitch. Only the two implemented
// host panels are exposed; dropping Audio/Graphics does not enlarge the tabs.
constexpr int TabWidth=88,TabHeight=24,TabTop=10;
constexpr int tab_left(unsigned i){return 10+int(i)*90;}
constexpr int GeneralLeft=8,GeneralWidth=186;
constexpr int LabelLeft=10,ControlLeft=135,ControlWidth=58;
constexpr int RowHeight=21,LabelFont=10,TabFont=12;
constexpr int general_row(unsigned i){return 42+int(i)*23;}
constexpr int general_divider(unsigned i){return general_row(i)+22;}
constexpr unsigned GeneralRows=11,GeneralVisible=7,GeneralMaximum=GeneralRows-GeneralVisible;
constexpr int InputPanelLeft=8,InputWidth=186;
constexpr int InputControlLeft=135,KeyboardTop=40;
constexpr int InputResetTop=64,InputResetLeft=135;
constexpr int InputHeaderTop=88,InputHeaderHeight=26;
constexpr int InputLabelLeft=10,InputLabelWidth=73;
constexpr int binding_left(unsigned bank){return 85+int(bank)*55;}
constexpr int BindingWidth=53;
constexpr int input_row(unsigned i){return 115+int(i)*23;}
constexpr unsigned InputVisible=4,InputMaximum=InputCommandCount-InputVisible;
constexpr int InputBottom=207,InputTrackLeft=201;
constexpr int DropdownTop=64,DropdownHeight=140;
constexpr int dropdown_row(unsigned i){return DropdownTop+int(i)*20;}
constexpr int dropdown_row_height(unsigned i){return 20;}
constexpr int DropdownFont=8,TickSize=15,ModifierTickSize=10;
constexpr int TooltipLeft=213;
constexpr int scroll_top(bool input){return input?input_row(0):general_row(0);}
constexpr int scroll_height(bool input){return input?InputBottom-input_row(0):general_row(GeneralVisible)-general_row(0)-3;}
constexpr int scroll_maximum(bool input){return input?InputMaximum:GeneralMaximum;}
inline unsigned scroll_value(bool input,int y){const int top=scroll_top(input)+7,length=scroll_height(input)-14;
 int position=y-top;if(position<0)position=0;if(position>length)position=length;
 return unsigned((position*scroll_maximum(input)+length/2)/length);}
static_assert(GeneralRows==11&&InputVisible<InputCommandCount,"LCD settings use readable paged rows");
}
