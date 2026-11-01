#pragma once
#include <stdint.h>

namespace spire::card_affliction {
constexpr unsigned Q=1048576,PageBytes=131072;
inline unsigned word(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
inline uint32_t dword(const uint8_t*p){return word(p)|uint32_t(word(p+2))<<16;}
inline int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
inline int product(int a,int b){return int(int64_t(a)*b>>20);}

// Borrowed Content views. Callers bind all required pages and verify residency
// before drawing; no Content request is permitted during the borrowed window.
struct Bytes {
 const uint8_t*const*pages=nullptr;unsigned length=0;
 unsigned byte(unsigned at)const{return pages[at>>17][at&(PageBytes-1)];}
 unsigned u16(unsigned at)const{return byte(at)|(byte(at+1)<<8);}
 unsigned u32(unsigned at)const{return u16(at)|(u16(at+2)<<16);}
 bool range(unsigned at,unsigned count)const{return at<=length&&count<=length-at;}
};
struct Field {
 Bytes bytes;unsigned base=0,width=0,height=0,channels=0,cols=0,data=0,tiles=0;
 bool bind(Bytes b,unsigned offset){
  if(!b.range(offset,32)||b.u32(offset)!=0x31464341u||b.u16(offset+4)!=2)return false;
  const unsigned w=b.u16(offset+6),h=b.u16(offset+8),c=b.u16(offset+10),columns=b.u16(offset+12),rows=b.u16(offset+14);
  const unsigned directory=b.u32(offset+16),payload=b.u32(offset+20),count=b.u32(offset+24),size=b.u32(offset+28);
  if(!w||w>1024||!h||h>1024||!c||c>4||columns!=(w+31)/32||rows!=(h+31)/32||directory!=32||
     payload!=((32+columns*rows*2+3)&~3u)||count>columns*rows||size!=payload+count*1024*c||!b.range(offset,size))return false;
  bytes=b;base=offset;width=w;height=h;channels=c;cols=columns;data=payload;tiles=count;return true;
 }
 unsigned texel(unsigned x,unsigned y,unsigned channel)const{
  const unsigned tile=bytes.u16(base+32+((y>>5)*cols+(x>>5))*2);
  if(!tile||tile>tiles||channel>=channels)return 0;
  return bytes.byte(base+data+(channel*tiles+tile-1)*1024+(y&31)*32+(x&31));
 }
 // Same normalized half-texel bilinear phase as Godot source sampling. Q20
 // intermediates retain source byte precision; no source raster is resized.
 int sample(int u,int v,unsigned channel,bool repeat=false)const{
  if(repeat){u=int(unsigned(u)&(Q-1));v=int(unsigned(v)&(Q-1));}
  else{u=clamp(u,0,Q);v=clamp(v,0,Q);}
  const int px=u*int(width)-int(Q/2),py=v*int(height)-int(Q/2),x=px>>20,y=py>>20;
  const unsigned fx=unsigned(px)&(Q-1),fy=unsigned(py)&(Q-1);
  unsigned x0,x1,y0,y1;
  if(repeat){x0=unsigned(x<0?x+int(width):x);x1=x0+1==width?0:x0+1;y0=unsigned(y<0?y+int(height):y);y1=y0+1==height?0:y0+1;}
  else{x0=unsigned(clamp(x,0,int(width)-1));x1=unsigned(clamp(x+1,0,int(width)-1));y0=unsigned(clamp(y,0,int(height)-1));y1=unsigned(clamp(y+1,0,int(height)-1));}
  unsigned aa,ab,ba,bb;
  if(((x0^x1)|(y0^y1))<32){
   const unsigned tile=bytes.u16(base+32+((y0>>5)*cols+(x0>>5))*2);
   if(!tile||tile>tiles||channel>=channels)return 0;
   const unsigned first=base+data+(channel*tiles+tile-1)*1024;
   const unsigned a0=first+(y0&31)*32+(x0&31),a1=first+(y0&31)*32+(x1&31);
   const unsigned b0=first+(y1&31)*32+(x0&31),b1=first+(y1&31)*32+(x1&31);
   if(((a0^a1)|(a0^b0)|(a0^b1))<PageBytes){
    const auto*p=bytes.pages[a0>>17];aa=p[a0&(PageBytes-1)];ab=p[a1&(PageBytes-1)];ba=p[b0&(PageBytes-1)];bb=p[b1&(PageBytes-1)];
   }else{aa=bytes.byte(a0);ab=bytes.byte(a1);ba=bytes.byte(b0);bb=bytes.byte(b1);}
  }else{aa=texel(x0,y0,channel);ab=texel(x1,y0,channel);ba=texel(x0,y1,channel);bb=texel(x1,y1,channel);}
  const int a=int(aa)*int(Q)+(int(ab)-int(aa))*int(fx);
  const int b=int(ba)*int(Q)+(int(bb)-int(ba))*int(fx);
  const unsigned q=unsigned(a+((int64_t(b-a)*fy+Q/2)>>20));
  return int((uint64_t(q)*16843009u+2147483648u)>>32);
 }
};
}
