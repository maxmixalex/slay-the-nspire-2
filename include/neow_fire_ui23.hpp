#pragma once
#include <stdint.h>
#include "ancient_fire_ui23.hpp"

namespace spire::neow_fire_ui23 {
constexpr unsigned Auxiliary=7228,AuxiliaryCount=7229,FirstImage=59741,TexturePages=6;
constexpr unsigned FirstMask=FirstImage+TexturePages,ParameterImage=FirstMask+546,Images=ParameterImage+1;
constexpr unsigned Width=320,Height=240,MaskBytes=17485,Spans=705;
constexpr unsigned Frames=546,KeyInterval=32,Duration=18000;
constexpr unsigned NodesAt=64,ModelsAt=192,RowsAt=448,SpansAt=1412,Length=4232;
inline unsigned word(const uint8_t*p){return unsigned(p[0])|unsigned(p[1])<<8;}
inline uint32_t dword(const uint8_t*p){return word(p)|uint32_t(word(p+2))<<16;}

// Source foreground transmission. This lives beside Character's opaque
// raster, in a variant mutually exclusive with Tez/Vakuu particle state.
// No extra global/heap pool, and no pointer survives a Content image request.
struct Mask {
 const void* owner=nullptr;
 uint32_t generation=0;
 unsigned first=0,index=0;
 bool valid=false;
 uint8_t pixels[MaskBytes];

 template<class Picture>bool apply(const Picture&p,unsigned frame,void(*poll)()){
  if(!p.pixels||p.format||p.alpha||p.palette||p.w!=256||!p.h||p.h>240)return false;
  const auto*raw=p.pixels;const unsigned bytes=unsigned(p.w)*p.h*2;
  const unsigned length=dword(raw),records=word(raw+6);
  if(length<8||length>bytes||word(raw+4)!=frame||frame>=Frames||records>(length-8)/4)return false;
  if(!(frame&(KeyInterval-1))){
   for(unsigned at=0;at<MaskBytes;at+=5120){if(poll)poll();const unsigned count=MaskBytes-at<5120?MaskBytes-at:5120;__builtin_memset(pixels+at,0,count);}
  }
  unsigned at=8,cursor=0;
  for(unsigned record=0;record<records;++record){
   if(!(record&63u)&&poll)poll();
   if(length-at<4)return false;
   const unsigned skip=word(raw+at),code=word(raw+at+2),count=code&32767;at+=4;
   if(skip>MaskBytes-cursor)return false;cursor+=skip;
   if(count>MaskBytes-cursor||(!count&&(!skip||(code&32768))))return false;
   const unsigned used=code&32768?1:count;if(used>length-at)return false;
   if(code&32768)__builtin_memset(pixels+cursor,raw[at],count);
   else __builtin_memcpy(pixels+cursor,raw+at,count);
   cursor+=count;at+=used;
  }
  if(at!=length)return false;
  for(;at<bytes;++at)if(raw[at])return false;
  return true;
 }
 template<class Content>bool restore(Content&content,unsigned first_image,unsigned target,void(*poll)()=nullptr){
  if(target>=Frames){valid=false;return false;}
  const unsigned key=target&~(KeyInterval-1);unsigned next=key;
  if(valid&&owner==&content&&generation==content.generation()&&first==first_image&&index>=key&&index<=target){
   if(index==target)return true;next=index+1;
  }else valid=false;
  for(unsigned frame=next;frame<=target;++frame){
   if(!apply(content.image(int(first_image+frame)),frame,poll)){valid=false;return false;}
  }
  owner=&content;generation=content.generation();first=first_image;index=target;valid=true;return true;
 }
};
static_assert(sizeof(Mask)<18000,"Compact Neow transmission must fit the existing cinder-sized scene variant");

// Calculator presentation: retain one source fire-light pose instead of
// evaluating four procedural shaders every frame. Only the light is held;
// Mask still follows all 546 original full-resolution foreground poses.
// Two 8-bit radiance channels fit alongside Mask in the existing Tez slot.
struct Illumination {
 const void*owner=nullptr;
 uint32_t generation=0;
 bool valid=false;
 uint16_t pixels[MaskBytes];
 bool matches(const void*source,uint32_t epoch)const{return valid&&owner==source&&generation==epoch;}
};

struct Table {
 const uint8_t*data=nullptr;bool bounded=false;
 template<class Picture>bool read(const Picture&p,bool reduced){
  data=nullptr;bounded=false;if(!p.pixels||p.format||p.alpha||p.palette||p.w!=256||(p.h!=9&&p.h!=26))return false;
  const auto*r=p.pixels;if(r[0]!='N'||r[1]!='W'||r[2]!='F'||r[3]!='1')return false;
  const unsigned version=word(r+4);bounded=version==2;const unsigned length=Length+(bounded?(MaskBytes+1)/2:0);
  if((version!=1&&version!=2)||p.h!=(length+511)/512)return false;
  const unsigned header[]={version,reduced?2u:1u,4,Frames,KeyInterval,MaskBytes,FirstImage,TexturePages,FirstMask,ParameterImage,NodesAt,ModelsAt,RowsAt,SpansAt,Spans,length};
  for(unsigned i=0;i<16;++i)if(word(r+4+i*2)!=header[i])return false;
  if(dword(r+36)!=200000||dword(r+40)!=Duration)return false;
  if(bounded){if(dword(r+44)!=Length||dword(r+48)!=(MaskBytes+1)/2||(r[length-1]&240))return false;}
  else for(unsigned i=44;i<52;++i)if(r[i])return false;
  for(unsigned i=52;i<64;++i)if(r[i])return false;
  unsigned green=0,blue=0;
  for(unsigned n=0;n<4;++n){const auto*v=r+NodesAt+n*32;
   if(word(v+24)>255||word(v+26)>255||v[28]>1||v[29]||v[30]||v[31])return false;
   green+=word(v+24);blue+=word(v+26);
   for(unsigned channel=0;channel<2;++channel){
    const int64_t origin=ancient_fire_ui23::integer(v+channel*4),dx=ancient_fire_ui23::integer(v+8+channel*4),dy=ancient_fire_ui23::integer(v+16+channel*4);
    const int64_t bound=(origin<0?-origin:origin)+Width*(dx<0?-dx:dx)+Height*(dy<0?-dy:dy);
    if(bound>2147483647)return false;
   }
  }
  if(green>255||blue>255)return false; // four premultiplied contributions fit uint16
  for(unsigned m=0;m<2;++m){const auto*v=r+ModelsAt+m*128;
   for(unsigned i=0;i<32;++i){const int q=ancient_fire_ui23::integer(v+4*i);if(q<-67108864||q>67108864)return false;}
   const int lo=ancient_fire_ui23::integer(v+120),hi=ancient_fire_ui23::integer(v+124);
   if(lo<0||hi>16777216||hi-lo<65536)return false;
  }
  unsigned span=0,pixel=0;
  for(unsigned y=0;y<Height;++y){
   if(word(r+RowsAt+y*4)!=span||word(r+RowsAt+y*4+2)!=pixel)return false;
   const unsigned end=word(r+RowsAt+(y+1)*4);if(end<span||end>Spans)return false;
   unsigned right=0;for(;span<end;++span){const unsigned x=word(r+SpansAt+span*4),count=word(r+SpansAt+span*4+2);
    if(!count||x<right||x+count>Width)return false;right=x+count;pixel+=count;if(pixel>MaskBytes)return false;
   }
  }
  if(span!=Spans||pixel!=MaskBytes||word(r+RowsAt+Height*4+2)!=MaskBytes)return false;
  for(unsigned i=length;i<256*p.h*2;++i)if(r[i])return false;
  data=r;return true;
 }
 bool active(unsigned node,unsigned pixel)const{return !bounded||((data[Length+(pixel>>1)]>>((pixel&1u)*4+node))&1u);}
 const uint8_t*node(unsigned i)const{return data+NodesAt+i*32;}
 unsigned row_span(unsigned y)const{return word(data+RowsAt+y*4);}
 unsigned row_pixel(unsigned y)const{return word(data+RowsAt+y*4+2);}
 unsigned x(unsigned span)const{return word(data+SpansAt+span*4);}
 unsigned count(unsigned span)const{return word(data+SpansAt+span*4+2);}
};

// The authored Neow fire has zero inner RGB. Reuse the Tez field/filter and
// fixed-point primitives, evaluating only its source outer-alpha equation.
// Distortion light and distortion mask retain distinct original R/G values.
struct Shader {
 const ancient_fire_ui23::Field*field=nullptr;
 ancient_fire_ui23::Model model;
 int pan[4][2]{};unsigned inverse=0;bool lcd_sampling=false,empty_light_border=false;
 void bind(const ancient_fire_ui23::Field*f,const uint8_t*parameters,uint32_t clock){
  field=f;model.read(parameters);const unsigned time=clock%200000;
  // Only the pinned Neow field layout uses the measured LCD interpolation.
  // Other valid material layouts retain the original Q20 source equation.
  lcd_sampling=f[0].width==600&&f[0].height==636&&f[0].channels==1&&
      f[1].width==250&&f[1].height==250&&f[1].channels==3&&
      f[2].width==256&&f[2].height==256&&f[2].channels==3&&
      f[3].width==100&&f[3].height==100&&f[3].channels==2;
  // A clamped sample outside the source square reads only its outer edge.
  // Prove that edge is zero before skipping its address/interpolation work;
  // altered textures retain the complete sampler without an assumed border.
  empty_light_border=lcd_sampling;
  if(empty_light_border)for(unsigned i=0;i<256;++i)
   if(f[2].texel(i*3+2)|f[2].texel((255*256+i)*3+2)|
      f[2].texel(i*256*3+2)|f[2].texel((i*256+255)*3+2)){empty_light_border=false;break;}
  for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)pan[i][j]=int(int64_t(model.q[i*5+2+j])*time/16000);
  inverse=unsigned((uint64_t(1048576)<<24)/unsigned(model.q[31]-model.q[30]));
  // The two authored models share unit/half-unit UV scales. Validate that
  // pattern once; legacy/different valid tables retain the general equation.
  const auto&q=model.q;const int scale=q[0];
  if((scale==8388608||scale==16777216)&&q[1]==scale&&q[5]==scale&&q[6]==scale&&
     q[4]==16777216&&q[9]==16777216&&q[10]==16777216&&q[11]==16777216&&
     q[15]==16777216&&q[16]==16777216&&q[20]==16777216&&q[21]==33554432&&q[24]==16777216)
   inverse|=0x80000000u|(scale==8388608?0x40000000u:0u);
 }
 int sample(unsigned index,int u,int v,unsigned channel,bool repeat)const{
  if(!lcd_sampling)return field[index].sample(u,v,channel,repeat);
  if(index==0&&channel==0&&repeat)return field[0].fixed_lcd<600,636,1,0,true>(u,v);
  if(index==2&&channel==2&&!repeat){
   if(empty_light_border&&((unsigned(u)-1u)>=1048575u||(unsigned(v)-1u)>=1048575u))return 0;
   return field[2].fixed_lcd<256,256,3,2,false>(u,v);
  }
  return field[index].sample_lcd(u,v,channel,repeat);
 }
 void sample_rg(unsigned index,int u,int v,bool repeat,int&r,int&g)const{
  if(!lcd_sampling){field[index].sample_rg(u,v,repeat,r,g);return;}
  if(index==1&&repeat)field[1].fixed_lcd_rg<250,250,3,true>(u,v,r,g);
  else if(index==2&&repeat)field[2].fixed_lcd_rg<256,256,3,true>(u,v,r,g);
  else if(index==3&&!repeat)field[3].fixed_lcd_rg<100,100,2,false>(u,v,r,g);
  else field[index].sample_lcd_rg(u,v,repeat,r,g);
 }
 unsigned alpha_unit(int u,int v)const{
  using namespace ancient_fire_ui23;const auto&q=model.q;
  int ar,ag,br,bg;sample_rg(1,u+pan[2][0],v+pan[2][1],true,ar,ag);sample_rg(2,u+pan[3][0],v+pan[3][1],true,br,bg);
  int dx=scaled(ar,q[14])+scaled(br,q[19]),dy=scaled(ag,q[14])+scaled(bg,q[19]);
  int mr,mg;sample_rg(3,u+(q[26]>>4),scaled(v,q[25])+(q[27]>>4),false,mr,mg);dx=product(dx,mr);dy=product(dy,mg);
  const int base=sample(2,u+dx,v+dy,2,false);
  // Unit-strength noise and the mask each lie in [0,Q20]. Monotonic
  // bounds can prove the final smoothstep endpoint before remaining samples.
  if(base*32<=q[30])return 0;
  const int mask=1048576-sample(2,u+(q[22]>>4),2*(v-41943)+41943+(q[23]>>4),2,false);
  if(product(1048576+mask,base)*16<=q[30])return 0;
  if(product(mask,base)*16>=q[31])return 1048576;
  const int nu=inverse&0x40000000u?u>>1:u,nv=inverse&0x40000000u?v>>1:v;
  const int n1=sample(0,nu+pan[0][0],nv+pan[0][1],0,true);
  if(product(n1+mask,base)*16<=q[30])return 0;
  const int n2=sample(0,nu+pan[1][0],nv+pan[1][1],0,true);
  const int strength=product(product(n1,n2)+mask,base)*16;
  return smooth(strength,q[30],q[31],inverse&0x3fffffffu);
 }
 unsigned alpha(int u,int v)const{
  if((inverse&0x80000000u)&&unsigned(u)<1048576u&&unsigned(v)<1048576u)return alpha_unit(u,v);
  using namespace ancient_fire_ui23;const auto&q=model.q;
  const int n1=scaled(field[0].sample(scaled(u,q[0])+pan[0][0],scaled(v,q[1])+pan[0][1],0,true),q[4]);
  const int n2=scaled(field[0].sample(scaled(u,q[5])+pan[1][0],scaled(v,q[6])+pan[1][1],0,true),q[9]);
  const int ax=scaled(u,q[10])+pan[2][0],ay=scaled(v,q[11])+pan[2][1],bx=scaled(u,q[15])+pan[3][0],by=scaled(v,q[16])+pan[3][1];
  int ar,ag,br,bg;field[1].sample_rg(ax,ay,true,ar,ag);field[2].sample_rg(bx,by,true,br,bg);
  int dx=scaled(ar,q[14])+scaled(br,q[19]);
  int dy=scaled(ag,q[14])+scaled(bg,q[19]);
  const int mask=1048576-field[2].sample(scaled(u,q[20])+(q[22]>>4),scaled(v-41943,q[21])+41943+(q[23]>>4),2,false);
  const int mu=scaled(u,q[24])+(q[26]>>4),mv=scaled(v,q[25])+(q[27]>>4);
  int mr,mg;field[3].sample_rg(mu,mv,false,mr,mg);dx=product(dx,mr);dy=product(dy,mg);
  const int base=field[2].sample(u+dx,v+dy,2,false),strength=product(product(n1,n2)+mask,base)*16;
  return smooth(strength,q[30],q[31],inverse&0x3fffffffu);
 }
};
}
