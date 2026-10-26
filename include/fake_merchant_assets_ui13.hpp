#pragma once
#include <stdint.h>

namespace spire {
// NFakeMerchant's custom room uses its own two environment skeletons and
// idle_loop, plus Character.CreateVisuals().relaxed_loop from normal atlases.
// Each sequence starts with a disjoint, exactly invariant pixel plane. The
// following frames contain every remaining pixel at the source registration.
struct FakeMerchantSequence {uint16_t base,count;int16_t x,y,w,h;};
constexpr FakeMerchantSequence FakeMerchantSequences[]={
 {3343,144,0,0,320,218}, {3488,144,0,0,320,218},
 {3633,80,185,60,90,120}, {3714,80,185,60,90,120},
 {3795,288,0,0,160,190}, {4084,168,0,0,160,190},
 {4253,288,0,0,160,190}, {4542,320,0,0,160,190},
 {4863,144,0,0,160,190}
};
constexpr unsigned I13_FakeMerchantBottom=3343;
constexpr unsigned I13_FakeMerchantNoise=5008;
constexpr unsigned I13_FakeMerchantGlitter=5009;
constexpr unsigned I13_FakeMerchantSquareSpeck=5010;
constexpr unsigned FakeMerchantUi13AuxiliaryCount=5011;
constexpr unsigned FakeMerchantNominalFps=24;
constexpr unsigned fake_merchant_frame(unsigned sequence,uint64_t elapsed_us){
 return unsigned(elapsed_us*FakeMerchantNominalFps/1000000u%FakeMerchantSequences[sequence].count);
}
static_assert(FakeMerchantSequences[8].base+FakeMerchantSequences[8].count+1==I13_FakeMerchantNoise,
              "Fake Merchant original complete animation periods");
}
