#pragma once
#include <stdint.h>
namespace spire::ancient_materials_ui23 {
constexpr unsigned AuxiliaryFirst=7217,AuxiliaryCount=7220,FirstImage=59677,Images=59700;
inline unsigned word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline int32_t integer(const uint8_t*p){const uint32_t n=word(p)|(uint32_t(word(p+2))<<16);return n<=0x7fffffffu?int32_t(n):-1-int32_t(~n);}
inline unsigned mix(unsigned a,unsigned b,unsigned f){return ((a<<8)+(b-a)*f+128)>>8;}
inline unsigned clamp(int n,unsigned maximum){return n<0?0:unsigned(n)>maximum?maximum:unsigned(n);}
inline uint16_t color_mix(uint16_t a,uint16_t b,unsigned f){
 const unsigned red=mix(a>>11,b>>11,f),green=mix((a>>5)&63,(b>>5)&63,f),blue=mix(a&31,b&31,f);
 return uint16_t((red<<11)|(green<<5)|blue);
}
struct Table {
 const uint8_t*data=nullptr;unsigned scene=0,lights=0;
 template<class Picture>bool read(Picture p,unsigned expected){
  data=nullptr;scene=expected;lights=expected==0?4:expected==1?2:0;
  if(expected>2||!p.pixels||p.w!=256||p.h!=(expected==1?8:2)||p.format||p.alpha||p.palette)return false;
  const uint16_t words[]={0x4d41,1,uint16_t(expected),FirstImage,4,uint16_t(FirstImage+(expected==1?4:13)),uint16_t(expected==1?9:5),
   FirstImage+18,FirstImage+19,uint16_t(lights),10000,2000,256,112,uint16_t(expected==1?960:320),uint16_t(expected==1?720:240)};
  for(unsigned i=0;i<16;++i)if(word(p.pixels+2*i)!=words[i])return false;
  for(unsigned i=0;i<lights;++i){const auto*r=p.pixels+32+i*14;
   const unsigned x=word(r),y=word(r+2),w=word(r+4),h=word(r+6),blend=word(r+8),page=word(r+10),at=word(r+12);
   if(x+w>320||y+h>240||!w||!h||h>224||blend>1||(page!=0&&page!=2)||at+w>256)return false;
  }
  if(expected){
   if(word(p.pixels+416)!=(expected==1?23:40)||word(p.pixels+418)!=(expected==1?23:2)||
      word(p.pixels+420)!=(expected==1?50:250)||word(p.pixels+422)!=word(p.pixels+420)||(word(p.pixels+442)!=(expected==1?0x3152:0x3252)&&word(p.pixels+442)!=0x5350))return false;
   for(unsigned i=0;i<8;++i){const int n=integer(p.pixels+384+i*4);if(n<=-int32_t(1u<<26)||n>=int32_t(1u<<28)||((i==2||i==3||i>=6)&&n<=0))return false;}
  }
  data=p.pixels;return true;
 }
 unsigned opacity(uint32_t room_elapsed)const{
  // Fixed one-particle10s source emitter, including its authored2s preprocess.
  // Texture1D sampling clamps half-texel endpoints and bilinearly samples the
  // exact256-byte source ramp; it does not approximate eight control points.
  const unsigned phase=(room_elapsed%10000+2000)%10000;
  const int x=int(uint64_t(phase)*256*65536/10000)-32768;
  const unsigned left=clamp(x>>16,255),right=clamp((x>>16)+1,255),fraction=unsigned(x&65535)>>8;
  return mix(data[128+left],data[128+right],fraction);
 }
};
struct Water {
 const uint8_t*pages[9]{};const uint8_t*noise[2]{};
 unsigned noise_width=0;int uv0[2]{},step[2]{},low[2]{},factor[2]{};
 int pan=0,half_strength=0;
 void parameters(const Table&t,uint32_t continuous_clock){
  noise_width=word(t.data+420);
  for(unsigned k=0;k<2;++k){uv0[k]=integer(t.data+384+k*4);step[k]=integer(t.data+392+k*4);low[k]=integer(t.data+400+k*4);
   factor[k]=int(int64_t(integer(t.data+408+k*4))*(k?720:960)/256);}
  pan=int((uint64_t(continuous_clock%3600000u)*word(t.data+416)*65536/1000000)&65535);
  half_strength=int(uint64_t(word(t.data+418))*16777216/2000);
 }
 unsigned noise_at(unsigned x,unsigned y)const{
  const unsigned at=y*noise_width+x;
  return noise_width==50?noise[0][at]:noise[at/31250][at%31250];
 }
 unsigned noise_sample(int u,int v)const{
  const int x=(((u>>8)-pan)&65535)*int(noise_width)-32768,y=(((v>>8)-pan)&65535)*int(noise_width)-32768;
  int left=x>>16,top=y>>16,right=left+1,bottom=top+1;
  if(left<0)left+=noise_width;if(top<0)top+=noise_width;
  if(right>=int(noise_width))right-=noise_width;if(bottom>=int(noise_width))bottom-=noise_width;
  const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
  return mix(mix(noise_at(left,top),noise_at(right,top),fx),mix(noise_at(left,bottom),noise_at(right,bottom),fx),fy);
 }
 uint16_t pixel(unsigned x,unsigned y)const{
  const unsigned col=x/320,row=y/240,at=((y-row*240)*320+x-col*320)*2;
  return uint16_t(word(pages[row*3+col]+at));
 }
 uint16_t sample(int u,int v,unsigned screen_x,unsigned screen_y)const{
  if(noise_width==250){
   const unsigned phase=noise_sample(u,v)*4,first=phase>=1020?3:phase/255;
   const unsigned fraction=((phase-first*255)*256+127)/255,at=(screen_y*320+screen_x)*2;
   return color_mix(uint16_t(word(pages[first]+at)),uint16_t(word(pages[first+1]+at)),fraction);
  }
  const int shift=int((int64_t(noise_sample(u,v))-255)*half_strength/255);
  const int x=int((int64_t(u+shift-low[0])*factor[0])>>24)-32768;
  const int y=int((int64_t(v+shift-low[1])*factor[1])>>24)-32768;
  const unsigned left=clamp(x>>16,959),right=clamp((x>>16)+1,959),top=clamp(y>>16,719),bottom=clamp((y>>16)+1,719);
  const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
  return color_mix(color_mix(pixel(left,top),pixel(right,top),fx),color_mix(pixel(left,bottom),pixel(right,bottom),fx),fy);
 }
};
}
