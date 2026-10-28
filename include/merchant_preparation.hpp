#pragma once
#include <stdint.h>
namespace spire {
// Static inventory bytes yield in bounded decoder steps. The optional host
// clock bounds each preparation pass on the32768Hz foreground timer.
// Visible stock gets at most8ms while the room is interactive; its extra idle
// skin uses2ms. Offline replay uses four steps, independent of host wall time.
struct GlyphPreparationBudget {
 static constexpr unsigned Ticks=65,InventoryTicks=262,OfflineSteps=4,MaximumSteps=128;
 uint32_t(*clock)()=nullptr;
 uint32_t started=0;
 unsigned ticks=Ticks;
 unsigned remaining=OfflineSteps;
 bool pending=false;
 explicit GlyphPreparationBudget(uint32_t(*timer)()=nullptr,unsigned limit=Ticks):clock(timer),started(timer?timer():0),ticks(limit),remaining(timer?MaximumSteps:OfflineSteps){}
 bool take(){
  if(!remaining)return false;
  if(clock&&uint32_t(clock()-started)>=ticks){remaining=0;return false;}
  --remaining;return true;
 }
};
struct MerchantPreparation {
 // The format's image directory is bounded by65535. Store its IDs at that
 // width, so the same384 queue bytes also hold the unfocused merchant clip
 // that becomes visible immediately when its rug opens.
 static constexpr unsigned Capacity=192;
 uint16_t images[Capacity]{};
 uint8_t count=0,cursor=0,card=0,stock_count=0;
 bool active=false,shown=false,planned=false,text_ready=false,prices_ready=false;
 uint32_t updates=0,cold_images=0;
 void reset(){*this=MerchantPreparation{};}
};
static_assert(sizeof(MerchantPreparation)<416,"Merchant preparation is only a bounded work queue");
}
