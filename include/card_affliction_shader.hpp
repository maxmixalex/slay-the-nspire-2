#pragma once
#include "card_affliction_fields.hpp"

namespace spire::card_affliction {
enum Material : unsigned {BoundBorder=1,BoundMain,Entangled,Galvanized,Hexed,Ringing,Smog,Leaf};
struct Pixel {int r=0,g=0,b=0,a=0;};
inline int mix(int a,int b,int t){return a+product(b-a,t);}
inline unsigned root(unsigned n){unsigned r=0,b=1u<<30;while(b>n)b>>=2;
 while(b){if(n>=r+b){n-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}return r;}
inline int angle(int y,int x){
 // Source polar helper divides atan2 by 2*3.1416. CORDIC constants use
 // that same denominator; vectoring is bounded before each signed shift.
 static constexpr int at[]={131072,77376,40884,20753,10417,5213,2607,1304,652,326,163,81,41,20,10,5,3,1,1};
 if(!x&&!y)return 0;
 int result=0;if(x<0){result=y>=0?int(Q/2):-int(Q/2);x=-x;y=-y;}
 x*=128;y*=128;
 for(unsigned i=0;i<19;++i){const int ox=x;if(y>0){x+=y>>i;y-=ox>>i;result+=at[i];}
  else if(y<0){x-=y>>i;y+=ox>>i;result-=at[i];}}
 return result;
}
inline void polar(int u,int v,int&radius,int&turn,int zoom=int(Q)){
 const int x=u-int(Q/2),y=v-int(Q/2),sx=x>>4,sy=y>>4;
 radius=product(int(root(unsigned(sx*sx)+unsigned(sy*sy)))*32,zoom)&int(Q-1);
 turn=angle(y,x)&int(Q-1);
}
inline unsigned smooth_reciprocal(int width){
 const unsigned w=unsigned(width);
 return w>256&&w<=Q?unsigned(((uint64_t(1)<<40)+w-1)/w):0;
}
inline int smooth(int n,int lo,int width,unsigned reciprocal){
 if(n<=lo)return 0;
 if(n>=lo+width)return Q;
 unsigned q;
 if(reciprocal){
  // With0<n-lo<width<=Q, ceil(2^40/width) overestimates the
  // Q20 quotient by at most one. Both products are32x32->64;
  // one exact remainder comparison restores the original division.
  const unsigned value=unsigned(n-lo);
  q=unsigned(uint64_t(value)*reciprocal>>20);
  if(uint64_t(q)*unsigned(width)>(uint64_t(value)<<20))--q;
 }else q=unsigned(int64_t(n-lo)*Q/width);
 const int square=product(int(q),int(q));
 return product(square,3*int(Q)-2*int(q));
}
inline int smooth(int n,int lo,int width){return smooth(n,lo,width,smooth_reciprocal(width));}
struct Curve {
 const uint8_t*values=nullptr;unsigned count=0,channels=0;Bytes bytes{};unsigned base=0;
 int sample(int phase,unsigned channel)const{
  if((!values&&!bytes.pages)||count<2||channel>=channels)return 0;
  const int p=clamp(phase,0,Q)*int(count)-int(Q/2),i=p>>20;
  const unsigned a=unsigned(clamp(i,0,int(count)-1)),b=unsigned(clamp(i+1,0,int(count)-1));
  const unsigned aa=(a*channels+channel)*4,bb=(b*channels+channel)*4;
  const int av=int32_t(values?dword(values+aa):bytes.u32(base+aa)),bv=int32_t(values?dword(values+bb):bytes.u32(base+bb));
  return av+int((int64_t(bv-av)*(unsigned(p)&(Q-1))+Q/2)>>20);
 }
};
struct Shader {
 // Field roles are fixed and source-validated by the producer: primary,
 // scalar noise, mask, and optional brightness strip. Curves remain signed
 // Q20 (Ringing's thin-ring coordinate is negative during part of its life).
 const Field*field[4]{};Curve curve[2];
 const int32_t*parameters=nullptr;
 unsigned kind=0;bool repeat=false;
 int tint[4]{int(Q),int(Q),int(Q),int(Q)};
 int pan[4]{},constant[5]{};unsigned custom=0;
 static int sine(unsigned phase){
  static constexpr int16_t quarter[]={0,1608,3212,4808,6393,7962,9512,11039,12540,14010,15447,16846,18205,19520,20788,22006,23170,24279,25330,26320,27246,28106,28899,29622,30274,30853,31357,31786,32138,32413,32610,32729,32767};
  phase&=Q-1;const bool negative=phase>=Q/2;phase%=Q/2;if(phase>Q/4)phase=Q/2-phase;
  const unsigned at=phase>>13,part=phase&8191;
  const int value=at==32?32768:int((unsigned(quarter[at])*(8192-part)+unsigned(quarter[at+1])*part+4096)>>13);
  return (negative?-value:value)*32;
 }
 void prepare(uint32_t milliseconds){
  // All authored rates are rational decimal values with a joint200s period.
  // Reducing that common period prevents signed UV overflow without turning
  // any material into a finite set of sampled animation phases.
  const unsigned time=milliseconds%200000u;
  auto moving=[&](int numerator,unsigned denominator){return int(int64_t(numerator)*time*Q/(1000*denominator))&int(Q-1);};
  if(kind==BoundBorder)pan[0]=moving(1,4);
  else if(kind==BoundMain)constant[0]=int(Q)+product(524288,field[3]->sample(moving(1,2),0,0,repeat));
  else if(kind==Entangled)pan[0]=moving(7,40);
  else if(kind==Galvanized){pan[0]=moving(-3,10);pan[1]=moving(67,100);
   const int blink=smooth(field[0]->sample(pan[1],0,2,repeat),209715,629146);constant[3]=blink;
   for(unsigned i=0;i<3;++i)constant[i]=curve[0].sample(blink,i);
   constant[4]=curve[0].sample(blink,3);
  }else if(kind==Hexed){pan[0]=moving(-1,4);pan[1]=moving(1,10);}
  else if(kind==Ringing){const int phase=int(uint64_t(milliseconds%2500u)*Q/2500);
   for(unsigned i=0;i<3;++i)constant[i]=curve[1].sample(phase,i);
   pan[0]=curve[0].sample(phase,0);pan[1]=curve[0].sample(phase,1);
  }else if(kind==Smog){pan[0]=moving(parameters[11],100);pan[1]=moving(parameters[12],100);
   constant[0]=int(smooth_reciprocal(parameters[10]));}
  else if(kind==Leaf){
   const int noise=field[0]->sample(int(custom),int(custom)+moving(1,10),1,true);
   const int rotation=product(noise-int(Q/2),183501); // .35*(2*n-1)/4 turns.
   constant[0]=sine(unsigned(rotation));constant[1]=sine(unsigned(rotation)+Q/4);
  }else if(kind==10)constant[0]=int((uint64_t(custom)*396u/100u)>>20)&3;
 }
 Pixel sample(int u,int v)const{
  Pixel out;int radius,turn;
  const auto&main=*field[0];
  if(kind==BoundBorder){
   const int alpha=product(tint[3],field[2]->sample(u,v,0));out.a=smooth(alpha,419430,629146);if(!out.a)return out;
   polar(u,v,radius,turn);const int intensity=main.sample(radius+pan[0],turn*3,0,repeat);
   out.r=product(intensity,tint[0]);out.g=product(intensity,tint[1]);out.b=product(intensity,tint[2]);
  }else if(kind==BoundMain){
   out.a=product(main.sample(u,v,1,repeat),tint[3]);if(!out.a)return out;
   const int intensity=product(main.sample(u,v,0,repeat),constant[0]);
   out.r=product(intensity,tint[0]);out.g=product(intensity,tint[1]);out.b=product(intensity,tint[2]);
  }else if(kind==Entangled){
   const int distortion=product(field[1]->sample(u,v+pan[0],0,repeat),18874);
   const int shifted=u+product(distortion,main.sample(u,v,1,repeat));
   out.a=product(main.sample(shifted,v,2,repeat),tint[3]);if(!out.a)return out;
   const int value=main.sample(shifted,v,0,repeat);
   out.r=product(curve[0].sample(value,0),tint[0]);out.g=product(curve[0].sample(value,1),tint[1]);out.b=product(curve[0].sample(value,2),tint[2]);
  }else if(kind==Galvanized){
   const int base=smooth(main.sample(u,v,3,repeat),734003,314573);if(!base)return out;
   polar(u,v,radius,turn);const int d=main.sample(radius+pan[0],turn,2,repeat);
   const int offset=product(d-int(Q/2),-419430),strength=product(d,-157286);
   const int su=u+product(u+offset-int(Q/2),strength),sv=v+product(v+offset-int(Q/2),strength);
   const int a=main.sample(su,sv,0,repeat),b=product(product(main.sample(su,sv,1,repeat),constant[4]),constant[3]);
   out.a=product(product(a>b?a:b,base),tint[3]);
   out.r=product(constant[0],tint[0]);out.g=product(constant[1],tint[1]);out.b=product(constant[2],tint[2]);
  }else if(kind==Hexed){
   out.a=product(main.sample(u,v,1,repeat),tint[3]);if(!out.a)return out;
   polar(u,v,radius,turn);const int noise=smooth(field[1]->sample(radius+pan[0],turn+pan[1],0,repeat),524288,209715),r=main.sample(u,v,0,repeat);
   int*channels[]={&out.r,&out.g,&out.b};for(unsigned i=0;i<3;++i)*channels[i]=product(mix(parameters[8+i],mix(parameters[i],parameters[4+i],noise),r),tint[i]);
  }else if(kind==Ringing){
   int alpha=0;for(unsigned i=0;i<3;++i){const int value=product(main.sample(u,v,i,repeat),constant[i]);if(value>alpha)alpha=value;}
   polar(u,v,radius,turn,838861);const int thin=product(main.sample(radius/2+pan[0],turn/2,3,repeat),pan[1]);
   out.a=product(alpha>thin?alpha:thin,tint[3]);out.r=tint[0];out.g=tint[1];out.b=tint[2];
  }else if(kind==Smog){
   const int mask=field[2]->sample(u,v,0),solid=field[2]->sample(u,v,1);if(!mask&&!solid)return out;
   const int a=main.sample(product(int(Q)-u,parameters[0])+pan[0],product(int(Q)-v,parameters[1])+parameters[3],0,repeat);
   const int b=main.sample(product(int(Q)-u,parameters[4])+pan[1],product(int(Q)-v,parameters[5])+parameters[7],0,repeat);
   const int smoke=smooth(product(mix(a,b,parameters[8]),mask),parameters[9],parameters[10],unsigned(constant[0]));
   out.a=product(smoke,tint[3])+solid;
   out.r=product(curve[0].sample(smoke,0),tint[0]);out.g=product(curve[0].sample(smoke,1),tint[1]);out.b=product(curve[0].sample(smoke,2),tint[2]);
  }else if(kind>=Leaf&&kind<=10){
   if(kind==Leaf){const int x=u-int(Q/2),y=v-838861;
    // Original rotate.gdshaderinc multiplies a row vector by its matrix.
    u=product(x,constant[1])+product(y,constant[0])+int(Q/2);
    v=-product(x,constant[0])+product(y,constant[1])+838861;
   }else if(kind==10){u=(u+(constant[0]&1)*int(Q))/2;v=(v+(constant[0]>>1)*int(Q))/2;}
   const int value=main.sample(u,v,0,kind==Leaf);
   out.a=product(kind==9?value:main.sample(u,v,kind==Leaf?2:1,kind==Leaf),tint[3]);
   if(!out.a)return out;
   out.r=product(curve[0].sample(value,0),tint[0]);out.g=product(curve[0].sample(value,1),tint[1]);out.b=product(curve[0].sample(value,2),tint[2]);
  }
  return out;
 }
};
}
