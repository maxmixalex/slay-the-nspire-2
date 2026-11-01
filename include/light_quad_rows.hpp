#pragma once
#include "nonu_fog_ui23.hpp"
namespace spire {
// Advance both authored light UV row equations exactly, including the inner
// truncation of the shear term. Only construction divides wide integers.
struct LightQuadRows {
 nonu_fog_ui23::AffineRows u,v;
 int64_t shear_value,shear_step;
 int width;
 LightQuadRows(int dx,int dy,int sine,int cosine,int w,int h,int shear):
  u((int64_t(dx)*cosine+int64_t(dy)*sine)*65536-
       (-int64_t(dx)*sine+int64_t(dy)*cosine)*shear,
    int64_t(sine)*16777216-int64_t(cosine)*shear*256,int64_t(w)*1024),
  v((-int64_t(dx)*sine+int64_t(dy)*cosine)*64,int64_t(cosine)*16384,h),
  shear_value((-int64_t(dx)*sine+int64_t(dy)*cosine)*shear),
  shear_step(int64_t(cosine)*shear*256),width(w){}
 int x()const{
  int64_t q=u.quotient,r=(u.remainder>>10)+
      (shear_value>0&&(u.remainder&1023)!=0);
  if(r>=width){++q;r-=width;}
  return 32768+int(q+(q<0&&r!=0));
 }
 int y()const{return 32768+v.value();}
 void next(){u.next();v.next();shear_value+=shear_step;}
};
}
