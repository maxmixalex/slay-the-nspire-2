#pragma once
#include "reference_viewport.hpp"
#include "ui_history_presentation.hpp"
#include "health_bar.hpp"
namespace spire::run_setup_layout {
using reference_viewport::project;
constexpr int MenuTop=26,MenuWidth=92,MenuHeight=181;
constexpr int menu_left(unsigned i){return 14+int(i)*100;}
constexpr int ModifiersLeft=162,ModifiersTop=37,ModifiersWidth=146,ModifiersHeight=170;
constexpr int ModifierContentWidth=143,ModifierTextWidth=124;
constexpr int CustomPortraitTop=174,CustomPortraitHeight=36,CustomPortraitWidth=27;
constexpr int portrait_left(unsigned i){return 7+int(i)*30;}
constexpr int portrait_center(unsigned i){return portrait_left(i)+CustomPortraitWidth/2;}
// Native FontVariation measurement: KreonBold spacing2 at36px is164px;
// the substituted Noto Regular title is72px. HBox separation is89px.
constexpr int randomize_left(bool){return 246;}
constexpr int randomize_top() {return 14;}
constexpr int RandomizeWidth=62,RandomizeHeight=19;
constexpr int seed_input_left(bool){return 51;}
constexpr int SeedTop=87,SeedBottom=108,SeedRight=151;
struct SubmenuVisual {unsigned scale,focus;};
inline SubmenuVisual submenu_visual(UiMenuTransition&m,bool focused,uint32_t now){
 if(!m.bound){m={};m.bound=true;m.start=now;}
 const auto sample=[&](){if(m.state)return SubmenuVisual{1025,4096};
  const unsigned age=now-m.start,linear=age>=500?4096:age*4096/500,inverse=4096-linear;
  const unsigned cubic=4096-inverse*inverse/4096*inverse/4096;
  return SubmenuVisual{unsigned(int(m.from_scale)+(1000-int(m.from_scale))*int(cubic)/4096),
   unsigned(m.from_focus)*(4096-health_bar::expo_out(age>=1000?1000:age))/4096};};
 if(bool(m.state)!=focused){const auto prior=sample();m.from_scale=prior.scale;m.from_focus=uint16_t(prior.focus);m.state=focused;m.start=now;}
 return sample();
}
}
