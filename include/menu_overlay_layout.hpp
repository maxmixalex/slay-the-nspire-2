#pragma once
namespace spire {
namespace pause_menu_layout {
// FourByThree (1680x1260), centered VBox: 64px header + 4px separator
// + five 372x80px solo buttons. Its minimum height is 468px.
constexpr int X=125,W=71,H=15,TitleY=72,TitleFont=12,Font=6;
constexpr int row_y(unsigned row){return (464*4+int(row)*320+10)/21;}
}
namespace potion_popup_layout {
// potion_popup.tscn: 259x239, with 236x86 use/discard controls.
constexpr int W=49,H=46,ControlX=2,ControlW=45,ControlH=16,UseY=10,DiscardY=27,Font=6;
}
}
