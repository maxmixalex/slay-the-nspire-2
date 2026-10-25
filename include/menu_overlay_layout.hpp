#pragma once
namespace spire {
namespace pause_menu_layout {
// FourByThree (1680x1260), centered VBox: 64px header + 4px separator
// + five 372x80px solo buttons. Its minimum height is 468px.
constexpr int X=72,W=176,H=27,TitleY=33,TitleFont=18,Font=10;
constexpr int row_y(unsigned row){return 65+int(row)*29;}
}
namespace potion_popup_layout {
// potion_popup.tscn: 259x239, with 236x86 use/discard controls.
constexpr int W=74,H=69,ControlX=3,ControlW=68,ControlH=24,UseY=15,DiscardY=41,Font=10;
}
}
