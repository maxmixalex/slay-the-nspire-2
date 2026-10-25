#pragma once
#include <stdint.h>
#include "character_death_assets.hpp"
namespace spire {
// Only the original shared skeleton pixels outside the world frame crop.
// These native-size supplemental clips retain their source frame clocks;
// all earlier world clips and image pixels remain unchanged.
constexpr unsigned BestiaryKaiserContentVersion=34;
constexpr unsigned BestiaryKaiserAnimationFirst=CharacterDeathAnimationKinds;
constexpr unsigned BestiaryKaiserAnimationKinds=BestiaryKaiserAnimationFirst+18;
namespace bestiary_kaiser {
constexpr uint16_t Sources[18]={179,203,227,228,263,286,298,310,
                              842,866,890,913,926,949,973,1009,1021,1034};
constexpr int Width=302,Height=206,Top=23;
inline int left(unsigned side){return side==1?173:-114;}
inline int directory(unsigned source){
 unsigned low=0,high=18;while(low<high){const unsigned mid=(low+high)/2;
  if(Sources[mid]<source)low=mid+1;else high=mid;}
 return low<18&&Sources[low]==source?int(BestiaryKaiserAnimationFirst+low):-1;
}
}
static_assert(BestiaryKaiserAnimationFirst==1765&&BestiaryKaiserAnimationKinds==1783,
              "Bestiary supplements follow all original player/merchant deaths");
}
