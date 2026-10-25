#pragma once
#include "input_bindings.hpp"
#include "touchpad.hpp"
namespace spire {
// A mechanical edge click is a direction; a central click remains the mouse.
// Classify only at press-down, so dragging a slider to an edge stays a drag.
struct CalculatorPadInput {
 uint16_t captured=0;uint32_t repeated_at=0,started_at=0;bool pressed=false;
 template<class Emit>bool sample(TouchSample s,uint32_t now,bool keyboard_only,Emit emit){
  if(!s.valid||!s.width||!s.height||s.x>s.width||s.y>s.height||!s.pressed){const bool owned=captured!=0;if(captured)emit(captured,false);captured=0;pressed=false;return owned;}
  if(!pressed){pressed=true;
   if(s.width&&s.height){
    if(unsigned(s.x)*4<s.width)captured=InputLeft;
    else if(unsigned(s.x)*4>unsigned(s.width)*3)captured=InputRight;
    else if(unsigned(s.y)*4<s.height)captured=InputDown;
    else if(unsigned(s.y)*4>unsigned(s.height)*3)captured=InputUp;
    else if(keyboard_only)captured=InputPad;
   }
   if(captured){started_at=repeated_at=now;emit(captured,true);}
  }else if(captured&&captured!=InputPad&&now-started_at>=400&&now-repeated_at>=100){repeated_at=now;emit(captured,true);}
  return captured!=0;
 }
};
}
