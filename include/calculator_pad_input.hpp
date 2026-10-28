#pragma once
#include "input_bindings.hpp"
#include "touchpad.hpp"
namespace spire {
// A mechanical edge click is a direction; a central click remains the mouse.
// Classify only at press-down, so dragging a slider to an edge stays a drag.
struct CalculatorPadInput {
 uint16_t captured=0,dispatched=0;uint32_t repeated_at=0,started_at=0;bool pressed=false;
 // Pure classification is safe while a resource read owns the foreground.
 // A central press stays central even when its held finger reaches an edge.
 uint16_t capture(TouchSample s,bool keyboard_only){
  if(!s.valid||!s.width||!s.height||s.x>s.width||s.y>s.height||!s.pressed){captured=0;pressed=false;return 0;}
  if(!pressed){pressed=true;captured=0;
   if(unsigned(s.x)*4<s.width)captured=InputLeft;
   else if(unsigned(s.x)*4>unsigned(s.width)*3)captured=InputRight;
   else if(unsigned(s.y)*4<s.height)captured=InputDown;
   else if(unsigned(s.y)*4>unsigned(s.height)*3)captured=InputUp;
   else if(keyboard_only)captured=InputPad;
  }
  return captured;
 }
 template<class Emit>bool sample_key(uint16_t key,uint32_t now,Emit emit,bool allow_repeat=true){
  if(key!=dispatched){const bool owned=dispatched!=0;if(dispatched)emit(dispatched,false);dispatched=key;
   if(key){started_at=repeated_at=now;emit(key,true);}return owned||key!=0;
  }
  if(key&&key!=InputPad&&allow_repeat&&now-started_at>=400&&now-repeated_at>=100){repeated_at=now;emit(key,true);}
  return key!=0;
 }
 template<class App>bool dispatch_key(uint16_t key,uint32_t now,App&app,bool allow_repeat=true){
  const bool navigation=sample_key(key,now,[&](uint16_t code,bool down){app.physical_key(code,down);},allow_repeat);
  if(navigation)app.pointer_cancel(true);
  return navigation;
 }
 template<class App>bool dispatch(TouchSample s,uint32_t now,App&app){
  const bool navigation=sample(s,now,app.keyboard_only,[&](uint16_t key,bool down){app.physical_key(key,down);});
  // A mechanical direction owns only the pointer gesture. Keep the card
  // selected by that same physical event through target navigation/release.
  if(navigation)app.pointer_cancel(true);
  return navigation;
 }
 template<class Emit>bool sample(TouchSample s,uint32_t now,bool keyboard_only,Emit emit){
  return sample_key(capture(s,keyboard_only),now,emit);
 }
};
}
