#pragma once
#include <stdint.h>
#include "resource_transfer.hpp"
#include "presentation_blend.hpp"

namespace spire::neow_post_ui23 {
constexpr unsigned AuxiliaryFirst=7224,AuxiliaryCount=7225,FirstImage=59729,Images=59730;
constexpr unsigned Width=256,FullHeight=56,StaticHeight=100;
constexpr unsigned Left=72,Top=144,Right=274,Bottom=240,StaticCount=202*96;
constexpr unsigned NoiseAt=128,MaskAt=16512,LodAt=26512,StaticMaskAt=128+StaticCount*2;
inline unsigned word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline int integer(const uint8_t*p){const uint32_t v=word(p)|(uint32_t(word(p+2))<<16);return v<=0x7fffffffu?int(v):-1-int(~v);}
inline int clamp(int v,int high){return v<0?0:v>high?high:v;}
inline unsigned lerp(unsigned a,unsigned b,unsigned f){return (a*(256-f)+b*f+128)>>8;}
inline uint16_t mix(uint16_t a,uint16_t b,unsigned f){
 // Red/blue occupy separate16-bit lanes; their weighted sums and rounding
 // remain below8192, so no lane can carry into its neighbour. This is the
 // same round-to-nearest Q8 interpolation as three independent channels.
 const uint32_t ar=((uint32_t(a)&0xf800u)<<5)|(a&31u),br=((uint32_t(b)&0xf800u)<<5)|(b&31u);
 const uint32_t rb=(((ar<<8)+(br-ar)*f+0x00800080u)>>8)&0x001f001fu;
 const uint32_t ag=a&0x07e0u,bg=b&0x07e0u;
 const uint32_t g=(((ag<<8)+(bg-ag)*f+0x1000u)>>8)&0x07e0u;
 return uint16_t(((rb>>5)&0xf800u)|g|(rb&31u));
}
inline uint16_t average4(uint16_t a,uint16_t b,uint16_t c,uint16_t d){
 return uint16_t(((((a>>11)+(b>>11)+(c>>11)+(d>>11)+2)>>2)<<11)|
  (((((a>>5)&63)+((b>>5)&63)+((c>>5)&63)+((d>>5)&63)+2)>>2)<<5)|
  (((a&31)+(b&31)+(c&31)+(d&31)+2)>>2));
}
inline uint16_t blend(uint16_t a,uint16_t b,unsigned alpha){
 return presentation::source_over_rgb565(a,b,alpha);
}
struct Table {
 const uint8_t*data=nullptr;bool still=false;
 template<class Picture>bool read(Picture p,bool reduced){
  data=nullptr;still=reduced;
  const unsigned height=reduced?StaticHeight:FullHeight,mask=reduced?StaticMaskAt:MaskAt,lod=reduced?mask+10000:LodAt;
  if(!p.pixels||p.w!=Width||p.h!=height||p.format||p.alpha||p.palette)return false;
  const uint16_t header[]={0x4e57,1,FirstImage,AuxiliaryFirst,Width,uint16_t(height),128,100,NoiseAt,
    uint16_t(mask),uint16_t(lod),1000,15,uint16_t(reduced),NoiseAt,StaticCount};
  for(unsigned i=0;i<16;++i)if(word(p.pixels+i*2)!=header[i])return false;
  const int geometry[]={-6069744,-21062688,83839,145269};
  for(unsigned i=0;i<4;++i)if(integer(p.pixels+32+i*4)!=geometry[i])return false;
  const unsigned bounds[]={Left,Top,Right,Bottom};
  for(unsigned i=0;i<4;++i)if(word(p.pixels+48+i*2)!=bounds[i])return false;
  data=p.pixels;return true;
 }
 // Return R8 inQ8, retaining subtexel precision before displacement/LOD.
 static unsigned bilinear(const uint8_t*bytes,unsigned size,int u,int v,bool repeat){
  const int xx=int((int64_t(u)*size)>>8)-32768,yy=int((int64_t(v)*size)>>8)-32768;
  int x0=xx>>16,x1=x0+1,y0=yy>>16,y1=y0+1;
  if(repeat){x0&=int(size)-1;x1&=int(size)-1;y0&=int(size)-1;y1&=int(size)-1;}
  else{x0=clamp(x0,int(size)-1);x1=clamp(x1,int(size)-1);y0=clamp(y0,int(size)-1);y1=clamp(y1,int(size)-1);}
  const unsigned fx=unsigned(xx&65535)>>8,fy=unsigned(yy&65535)>>8;
  const unsigned a=bytes[y0*size+x0]*(256-fx)+bytes[y0*size+x1]*fx;
  const unsigned b=bytes[y1*size+x0]*(256-fx)+bytes[y1*size+x1]*fx;
  return (a*(256-fy)+b*fy+128)>>8;
 }
 unsigned noise(unsigned x,unsigned y,unsigned phase)const{
  if(still)return word(data+NoiseAt+((y-Top)*(Right-Left)+x-Left)*2);
  const int u=int((uint32_t(-6069744+int(x)*83839)-phase)&0xffffffu);
  const int v=int((uint32_t(-21062688+int(y)*145269)-phase)&0xffffffu);
  return bilinear(data+NoiseAt,128,u,v,true);
 }
 unsigned alpha(unsigned x,unsigned y)const{
  const int u=-6069744+int(x)*83839,v=-21062688+int(y)*145269;
  if(unsigned(u)>=16777216u||unsigned(v)>=16777216u)return 0;
  return (bilinear(data+(still?StaticMaskAt:MaskAt),100,u,v,false)+128)>>8;
 }
 unsigned lod(int dx_x,int dx_y,int dy_x,int dy_y)const{
  // Squared source footprint inQ8; the pinned noise derivative bound keeps
  // rho<2, so mip0/1 cover the complete source filter footprint.
  const uint64_t a=uint64_t(int64_t(dx_x)*dx_x+int64_t(dx_y)*dx_y),b=uint64_t(int64_t(dy_x)*dy_x+int64_t(dy_y)*dy_y);
  const unsigned index=unsigned(((a>b?a:b)+(1u<<23))>>24);
  if(index>1024)return 256;
  return word(data+(still?StaticMaskAt+10000:LodAt)+index*2);
 }
};
struct Snapshot {uint16_t full[320*240],mip1[160*120];};
static_assert(sizeof(Snapshot)==192000&&sizeof(Snapshot)<=ResourceTransferCapacity,"Post snapshots reuse the existing transfer arena");
inline void snapshot(Snapshot&out,const uint16_t*pixels,int pitch){
 for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x)out.full[y*320+x]=pixels[y*pitch+x];
 for(unsigned y=0;y<120;++y)for(unsigned x=0;x<160;++x){const unsigned at=y*640+x*2;
  out.mip1[y*160+x]=average4(out.full[at],out.full[at+1],out.full[at+320],out.full[at+321]);}
}
// The fixed output rectangle and source displacement limits (+/-2.4px,
// +/-1.8px) can read full pixels only inside x69..276,y142..239.
// Mip1 additionally needs original x68..277,y140..239. Populate that exact
// superset before any writes; the Snapshot capacity/layout is unchanged.
inline void snapshot_region(Snapshot&out,const uint16_t*pixels,int pitch){
 for(unsigned y=140;y<240;++y)for(unsigned x=68;x<278;++x)out.full[y*320+x]=pixels[y*pitch+x];
 for(unsigned y=70;y<120;++y)for(unsigned x=34;x<139;++x){const unsigned at=y*640+x*2;
  out.mip1[y*160+x]=average4(out.full[at],out.full[at+1],out.full[at+320],out.full[at+321]);}
}
inline uint16_t sample(const uint16_t*source,unsigned width,unsigned height,int x,int y){
 const unsigned l=unsigned(clamp(x>>16,int(width)-1)),r=unsigned(clamp((x>>16)+1,int(width)-1));
 const unsigned t=unsigned(clamp(y>>16,int(height)-1)),b=unsigned(clamp((y>>16)+1,int(height)-1));
 const unsigned fx=unsigned(x&65535)>>8,fy=unsigned(y&65535)>>8;
 return mix(mix(source[t*width+l],source[t*width+r],fx),mix(source[b*width+l],source[b*width+r],fx),fy);
}
struct Axis {uint8_t low,high,fraction;};
inline Axis noise_axis(int position,unsigned phase){
 const unsigned uv=(uint32_t(position)-phase)&0xffffffu;
 const int pixel=int(uv>>1)-32768,index=pixel>>16;
 return {uint8_t(index&127),uint8_t((index+1)&127),uint8_t(unsigned(pixel)>>8)};
}
inline Axis mask_axis(int position){
 if(unsigned(position)>=16777216u)return {255,255,0};
 const int pixel=int(unsigned(position)*100u>>8)-32768,index=pixel>>16;
 return {uint8_t(clamp(index,99)),uint8_t(clamp(index+1,99)),uint8_t(unsigned(pixel)>>8)};
}
template<unsigned Size>inline unsigned bilinear_axes(const uint8_t*bytes,Axis x,Axis y){
 const unsigned top=y.low*Size,bottom=y.high*Size;
 const unsigned a=bytes[top+x.low]*(256u-x.fraction)+bytes[top+x.high]*x.fraction;
 const unsigned b=bytes[bottom+x.low]*(256u-x.fraction)+bytes[bottom+x.high]*x.fraction;
 return (a*(256u-y.fraction)+b*y.fraction+128u)>>8;
}
inline void render(uint16_t*pixels,int pitch,const Snapshot&source,const Table&table,uint32_t clock){
 const unsigned phase=unsigned(uint64_t(clock%1000u)*16777216u/1000u);
 // Source UV is separable. Per-column clamp/wrap, texel indices and weights
 // are shared by all 96 rows; these 1212 stack bytes are not a persistent cache.
 Axis noise_x[Right-Left],mask_x[Right-Left];
 for(unsigned x=Left;x<Right;++x){const int u=-6069744+int(x)*83839;
  noise_x[x-Left]=noise_axis(u,phase);mask_x[x-Left]=mask_axis(u);}
 const auto*mask=table.data+(table.still?StaticMaskAt:MaskAt);
 const auto*lod_values=table.data+(table.still?StaticMaskAt+10000:LodAt);
 for(unsigned y=Top;y<Bottom;y+=2){
  const Axis noise_y[]={noise_axis(-21062688+int(y)*145269,phase),noise_axis(-21062688+int(y+1)*145269,phase)};
  const Axis mask_y[]={mask_axis(-21062688+int(y)*145269),mask_axis(-21062688+int(y+1)*145269)};
  for(unsigned x=Left;x<Right;x+=2){
   // Mask coverage is independent of time and displacement. A zero quad
   // cannot affect the framebuffer or its neighbours; skip its noise/LOD.
   unsigned coverage[4],visible=0;
   for(unsigned i=0;i<4;++i){const Axis mx=mask_x[x+(i&1)-Left],my=mask_y[i>>1];
    coverage[i]=mx.low==255||my.low==255?0:(bilinear_axes<100>(mask,mx,my)+128)>>8;
    visible|=coverage[i];
   }
   if(!visible)continue;
   int sx[4],sy[4];
   for(unsigned i=0;i<4;++i){const unsigned xx=x+(i&1),yy=y+(i>>1);
    const unsigned n=table.still?word(table.data+NoiseAt+((yy-Top)*(Right-Left)+xx-Left)*2):bilinear_axes<128>(table.data+NoiseAt,noise_x[xx-Left],noise_y[i>>1]);
    sx[i]=int(xx*65536)+int((int64_t(n)*315806+32768)>>16)-157286;
    sy[i]=int(yy*65536)+int((int64_t(n)*236855+32768)>>16)-117965;
   }
   uint64_t horizontal[2],vertical[2];
   for(unsigned row=0;row<2;++row){const unsigned at=row*2;const int dx=sx[at+1]-sx[at],dy=sy[at+1]-sy[at];horizontal[row]=uint64_t(int64_t(dx)*dx+int64_t(dy)*dy);}
   for(unsigned col=0;col<2;++col){const int dx=sx[col+2]-sx[col],dy=sy[col+2]-sy[col];vertical[col]=uint64_t(int64_t(dx)*dx+int64_t(dy)*dy);}
   for(unsigned i=0;i<4;++i){const unsigned xx=x+(i&1),yy=y+(i>>1),alpha=coverage[i];
    if(!alpha)continue;
    const uint64_t a=horizontal[i>>1],b=vertical[i&1];const unsigned index=unsigned(((a>b?a:b)+(1u<<23))>>24);
    const unsigned lod=index>1024?256:word(lod_values+index*2);
    uint16_t color=sample(source.full,320,240,sx[i],sy[i]);
    if(lod)color=mix(color,sample(source.mip1,160,120,(sx[i]>>1)-16384,(sy[i]>>1)-16384),lod);
    pixels[yy*pitch+xx]=alpha==255?color:blend(source.full[yy*320+xx],color,alpha);
   }
  }
 }
}

inline bool apply(uint16_t*pixels,int pitch,const Table&table,uint32_t clock){
 if(!pixels||pitch<320||!table.data)return false;
 // The caller has already fetched every source view and checked its content
 // generation. No callbacks, image requests or cache operations occur here.
 ResourceTransferLease lease(ResourceTransferUse::Bytes);if(!lease)return false;
 auto*copy=::new(detail::InPlaceAddress{lease.bytes()}) Snapshot;
 snapshot_region(*copy,pixels,pitch);render(pixels,pitch,*copy,table,table.still?0:clock);
 return true;
}
}
