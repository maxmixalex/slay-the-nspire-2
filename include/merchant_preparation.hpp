#pragma once
#include <stdint.h>
namespace spire {
// A cold inventory yields after one image or one card's text, so the host
// keeps sampling physical keys. No run state or additional raster is stored.
struct MerchantPreparation {
 static constexpr unsigned Capacity=96;
 int images[Capacity]{};
 uint8_t count=0,cursor=0,card=0;
 bool active=false,shown=false;
 uint32_t updates=0,cold_images=0;
 void reset(){*this=MerchantPreparation{};}
};
static_assert(sizeof(MerchantPreparation)<416,"Merchant preparation is only a bounded work queue");
}
