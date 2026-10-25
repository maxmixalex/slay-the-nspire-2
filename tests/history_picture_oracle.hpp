#pragma once
#include "ui_history_layout.hpp"
#include <algorithm>
#include <cmath>
#include <initializer_list>
// Draw receives raw13x13 RGBA planes and returns the full320x240 canvas.
// Its implementation is supplied by the real App test or a standalone canvas.
template<class Draw,class Check> static void history_picture_oracle(Draw draw,Check check){
 constexpr uint16_t paper=0x2947;uint8_t raw[13*13*2]{},alpha[13*13]{};
 for(int y=0;y<13;++y)for(int x=0;x<13;++x){const unsigned at=y*13+x,color=(((x*2+3)&31)<<11)|(((y*5+7)&63)<<5)|((x+2*y+5)&31);
  raw[2*at]=uint8_t(color);raw[2*at+1]=uint8_t(color>>8);alpha[at]=(x+y)%4?255:64;
 }
 for(bool aligned:{false,true})for(int size:{9,12,13})for(int angle:{-80,0,80})for(int edge:{0,1,2}){
  const int cx=edge==0?100:edge==1?0:319,cy=edge==0?80:edge==1?0:239;
  const int sine=int(std::lround(1024*std::sin(angle*std::acos(-1.)/2880))),cosine=int(std::lround(1024*std::cos(angle*std::acos(-1.)/2880)));
  const unsigned opacity=aligned?97:255,parent_opacity=243;
  const auto actual=draw(raw,alpha,cx,cy,size,angle,aligned,opacity,parent_opacity);check(actual.size()==320*240);
  const int extent=(std::abs(cosine)*size+std::abs(sine)*size)/2048+2,left=std::max(0,cx-extent),top=std::max(0,cy-extent);
  const int right=std::min(320,cx+extent),bottom=std::min(240,cy+extent);
  for(int y=0;y<240;++y)for(int x=0;x<320;++x){int u=-1,v=-1;
   if(aligned){
    // Direct inverse matrix at each pixel, independently of the runtime's
    // remainder stepping and conservative bounding rectangle.
    const long long dx=2*(x-cx)-(size&1),dy=2*(y-cy)-(size&1),den=2*size*1024;
    const long long un=(size*1024+dx*cosine+dy*sine)*13,vn=(size*1024-dx*sine+dy*cosine)*13;
    if(un>=0&&vn>=0&&un<13*den&&vn<13*den){u=int(un/den);v=int(vn/den);}
   }else if(x>=left&&x<right&&y>=top&&y<bottom){
    // Preserve the older UI's already-shipped integer-center convention.
    const int sx=13*512+((left-cx)*cosine+(y-cy)*sine)*13/size+(x-left)*(cosine*13/size);
    const int sy=13*512+(-(left-cx)*sine+(y-cy)*cosine)*13/size+(x-left)*(-sine*13/size);
    if(sx>=0&&sy>=0&&sx<13*1024&&sy<13*1024){u=sx/1024;v=sy/1024;}
   }
   uint16_t expected=paper;
   if(u>=0&&v>=0){const unsigned at=v*13+u,ink=raw[2*at]|unsigned(raw[2*at+1])<<8;
    unsigned coverage=(alpha[at]*opacity+127)/255;coverage=(coverage*parent_opacity+127)/255;
    const unsigned r=((paper>>11)*(255-coverage)+(ink>>11)*coverage+127)/255;
    const unsigned g=(((paper>>5)&63)*(255-coverage)+((ink>>5)&63)*coverage+127)/255;
    const unsigned b=((paper&31)*(255-coverage)+(ink&31)*coverage+127)/255;expected=uint16_t((r<<11)|(g<<5)|b);
   }
   check(actual[y*320+x]==expected);
  }
 }
}
