#pragma once
#include "reference_viewport.hpp"
#include "ui_history_presentation.hpp"
#include "health_bar.hpp"
namespace spire::run_setup_layout {
using reference_viewport::project;
constexpr int MenuTop=project(277),MenuWidth=project(330),MenuHeight=project(705);
constexpr int menu_left(unsigned i){return project(275+int(i)*400);}
constexpr int ModifiersLeft=project(872),ModifiersTop=project(168),ModifiersWidth=project(566),ModifiersHeight=project(930);
constexpr int ModifierContentWidth=project(554),ModifierTextWidth=project(494);
constexpr int CustomPortraitTop=project(916),CustomPortraitHeight=project(154),CustomPortraitWidth=project(100);
constexpr int portrait_left(unsigned i){return project(161+int(i)*116);}
constexpr int portrait_center(unsigned i){return project(211+int(i)*116);}
// Native FontVariation measurement: KreonBold spacing2 at36px is164px;
// the substituted Noto Regular title is72px. HBox separation is89px.
constexpr int randomize_left(bool english){return project(864+(english?164:72)+89);}
constexpr int randomize_top() {return project(113);}
constexpr int RandomizeWidth=project(200),RandomizeHeight=project(50);
constexpr int seed_input_left(bool english){return project(273+(english?62:84)+4);}
constexpr int SeedTop=project(538),SeedBottom=project(614),SeedRight=project(711);
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
