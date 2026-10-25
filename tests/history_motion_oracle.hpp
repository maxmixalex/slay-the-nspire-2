#pragma once
#include "ui_history_layout.hpp"
#include <cmath>
#include <initializer_list>
// Standalone source-equation checks: no App layout or linked core is needed.
template<class Check> static void history_motion_oracle(Check check){
 using namespace spire;
 check(sizeof(UiHistoryPresentation)<=128);
 unsigned histogram[81]{};
 for(unsigned uniform=0;uniform<65536;++uniform){const int angle=history_angle::gaussian(uniform);check(angle>=0&&angle<=80);++histogram[angle];}
 unsigned cumulative=0;
 for(unsigned i=0;i<80;++i){cumulative+=histogram[i];
  const auto expected=std::lround(65536*std::erf((i+.5)/80/std::sqrt(2.))/std::erf(1/std::sqrt(2.)));
  check(cumulative==unsigned(expected));
 }
 for(int angle=-80;angle<=80;++angle){const auto rotation=history_angle::rotation(angle);const double radians=angle*std::acos(-1.)/2880;
  check(rotation.sine==std::lround(1024*std::sin(radians)));check(rotation.cosine==std::lround(1024*std::cos(radians)));
 }
 const auto expected=[](int from,int to,unsigned age,unsigned duration){return age>=duration?to:from+int((to-from)*(1-std::exp2(-10.*age/duration)));};
 UiHistoryPresentation motion;motion.open(1000);int negative=-1,positive=-1;
 for(unsigned i=0;i<UiHistoryFloors;++i){const int angle=motion.angle(i,1000);check(std::abs(angle)==motion.base_angle(i));if(angle<=-16)negative=int(i);if(angle>=16)positive=int(i);}
 check(negative>=0&&positive>=0);const auto first_seed=motion.cosmetic_seed;
 for(int id:{negative,positive}){
  const int initial=motion.angle(id,1000),base=motion.base_angle(id);motion.observe(id,1000);
  for(unsigned elapsed:{0u,5u,12u,25u,49u,50u})check(std::abs(motion.angle(id,1000+elapsed)-expected(initial,0,elapsed,50))<=1);
  motion.observe(-1,1050);check(motion.angle(id,1050)==0);
  for(unsigned elapsed:{0u,25u,100u,250u,500u,999u,1000u})check(std::abs(motion.angle(id,1050+elapsed)-expected(0,base,elapsed,1000))<=1);
  check(motion.angle(id,2050)==base&&motion.outline(id,2050).alpha==64);
 }
 // A killed focus tween resumes its current angle, and every unfocus ends
 // at the positive base, even when the initially chosen sign was negative.
 motion.open(3000);negative=-1;for(unsigned i=0;i<UiHistoryFloors;++i)if(motion.angle(i,3000)<-16){negative=int(i);break;}check(negative>=0);
 motion.observe(negative,3000);const int interrupted=motion.angle(negative,3012);motion.observe(-1,3012);check(motion.angle(negative,3012)==interrupted);
 const int recovery=motion.angle(negative,3075);motion.observe(negative,3075);check(motion.angle(negative,3075)==recovery);check(motion.angle(negative,3125)==0);
 motion.observe(-1,3125);const int base=motion.base_angle(negative);check(motion.angle(negative,4125)==base);
 // Retiring the bounded transition slot must not resurrect the initial
 // negative angle or the _Ready-only outline alpha47.
 for(int i=0;i<12;++i)motion.observe(UiHistoryLayout::RelicFirst+i,5000+i*1100);
 check(motion.angle(negative,18000)==base);check(motion.outline(negative,18000).alpha==64);
 check(motion.angle(-1,18000)==0&&motion.angle(UiHistoryLayout::CardFirst,18000)==0);
 motion.open(19000);check(motion.cosmetic_seed!=first_seed);check(!motion.touched(negative));
}
