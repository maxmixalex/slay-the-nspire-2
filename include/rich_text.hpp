#pragma once
#include <stdint.h>
#include <stddef.h>
namespace spire {
struct RichStyle { uint8_t color=0; bool bold=false,italic=false,wave=false,jitter=false,rainbow=false,thinky_dots=false; unsigned thinky_start=0; };
struct RichChar { uint32_t codepoint; RichStyle style; unsigned index; };
// RichTextThinkyDots uses a 400 ms positive half-sine, a 4 s rest and
// a 100 ms delay per relative character. Quantize to one screen pixel so
// small calculator glyphs remain legible. This clock is presentation only.
inline int rich_thinky_offset(uint32_t elapsed_ms,unsigned relative_index){
 if(relative_index>elapsed_ms/100)return 0;
 unsigned phase=(elapsed_ms-relative_index*100)%4400;
 return phase>=44&&phase<=356?-1:0;
}
// Tags are presentation only. The parser owns no heap storage and never consumes RNG.
class RichReader {
 const char* p_;RichStyle style_{};unsigned index_=0,depth_=0;
 struct Entry{uint8_t tag;RichStyle before;} stack_[16]{};
 static bool equal(const char*p,size_t n,const char*q){size_t i=0;while(q[i]){if(i>=n||p[i]!=q[i])return false;++i;}return i==n;}
 static unsigned tag(const char*p,size_t n){
  const char* names[]={"gold","red","blue","green","purple","orange","aqua","white","gray","b","i","sine","jitter","rainbow","thinky_dots"};
  for(unsigned i=0;i<15;++i)if(equal(p,n,names[i]))return i+1;return 0;
 }
 bool markup(){
  if(*p_!='[')return false;const char*start=p_+1;bool close=*start=='/';if(close)++start;
  const char*end=start;while(*end&&*end!=']'&&end-start<16)++end;
  if(*end!=']')return false;unsigned t=tag(start,(size_t)(end-start));if(!t)return false;
  p_=end+1;
  if(close){for(unsigned i=depth_;i>0;--i)if(stack_[i-1].tag==t){style_=stack_[i-1].before;depth_=i-1;break;}}
  else if(depth_<16){stack_[depth_++]={(uint8_t)t,style_};if(t<=9)style_.color=(uint8_t)t;else if(t==10)style_.bold=true;else if(t==11)style_.italic=true;else if(t==12)style_.wave=true;else if(t==13)style_.jitter=true;else if(t==14)style_.rainbow=true;else{style_.thinky_dots=true;style_.thinky_start=index_;}}
  return true;
 }
public:
 explicit RichReader(const char*p):p_(p){}
 bool next(RichChar&out){
  while(*p_&&markup()){}if(!*p_)return false;
  uint32_t cp=(uint8_t)*p_++;
  if(cp>=128){int n=cp<224?1:cp<240?2:3;cp&=(1u<<(6-n))-1;while(n--){uint8_t b=(uint8_t)*p_;if((b&192)!=128){cp='?';break;}++p_;cp=(cp<<6)|(b&63);}}
  out={cp,style_,index_++};return true;
 }
};
}
