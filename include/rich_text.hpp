#pragma once
#include <stdint.h>
#include <stddef.h>
#include "rich_rainbow.hpp"
#include "inline_icons.hpp"
namespace spire {
struct RichStyle { uint8_t color=0; bool bold=false,italic=false,wave=false,jitter=false,rainbow=false,thinky_dots=false; unsigned thinky_start=0;RichRainbow rainbow_parameters{}; };
struct RichChar { uint32_t codepoint; RichStyle style; unsigned index; };
inline int rich_bold_advance(const RichChar&ch){return ch.style.bold&&!inline_icon(ch.codepoint)?1:0;}
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
 const char* p_;const char*literal_end_=nullptr;bool literal_tag_=false;RichStyle style_{};unsigned index_=0,depth_=0,overflow_rainbows_=0;
 struct Entry{uint8_t tag;RichStyle before;bool literal;} stack_[16]{};
 static bool equal(const char*p,size_t n,const char*q){size_t i=0;while(q[i]){if(i>=n||p[i]!=q[i])return false;++i;}return i==n;}
 static bool space(char c){return c==' '||c=='\t';}
 static bool decimal(const char*p,const char*end,int&out){
  bool negative=false,digit=false;if(p<end&&(*p=='-'||*p=='+'))negative=*p++=='-';
  unsigned whole=0,fraction=0,places=0;
  while(p<end&&*p>='0'&&*p<='9'){digit=true;whole=whole*10+unsigned(*p++-'0');if(whole>10)return false;}
  if(p<end&&*p=='.'){++p;while(p<end&&*p>='0'&&*p<='9'){digit=true;if(++places>3)return false;fraction=fraction*10+unsigned(*p++-'0');}}
  if(!digit||p!=end)return false;while(places++<3)fraction*=10;
  unsigned value=whole*1000+fraction;if(value>10000)return false;out=negative?-int(value):int(value);return true;
 }
 static bool rainbow_options(const char*p,const char*end,RichRainbow&out){
  unsigned seen=0;
  while(p<end){while(p<end&&space(*p))++p;if(p==end)break;
   const char*name=p;while(p<end&&*p!='='&&!space(*p))++p;if(p==end||*p!='=')return false;
   unsigned option=equal(name,size_t(p-name),"freq")?1:equal(name,size_t(p-name),"sat")?2:equal(name,size_t(p-name),"val")?4:equal(name,size_t(p-name),"speed")?8:0;
   if(!option||(seen&option))return false;seen|=option;const char*value=++p;while(p<end&&!space(*p))++p;int number;
   if(!decimal(value,p,number))return false;
   if(option==1)out.frequency=int16_t(number);else if(option==8)out.speed=int16_t(number);
   else{if(number<0||number>1000)return false;if(option==2)out.saturation=uint16_t(number);else out.value=uint16_t(number);}
  }
  return true;
 }
 static unsigned tag(const char*p,size_t n){
  const char* names[]={"gold","red","blue","green","purple","orange","aqua","white","gray","b","i","sine","jitter","rainbow","thinky_dots"};
  for(unsigned i=0;i<15;++i)if(equal(p,n,names[i]))return i+1;return 0;
 }
 bool markup(){
  if(*p_!='['||literal_tag_||(literal_end_&&p_<literal_end_))return false;const char*start=p_+1;bool close=*start=='/';if(close)++start;
  const char*end=start;while(*end&&*end!=']'&&end-start<128)++end;
  const char*name_end=start;while(name_end<end&&!space(*name_end))++name_end;unsigned t=tag(start,(size_t)(name_end-start));
  if(*end!=']'){
   if(t==14&&!close){if(depth_<16&&!overflow_rainbows_)stack_[depth_++]={14,style_,true};else ++overflow_rainbows_;literal_tag_=true;}
   return false;
  }
  if(!t||(name_end<end&&(t!=14||close)))return false;
  if(close){
   if(t==14&&overflow_rainbows_){--overflow_rainbows_;literal_end_=end+1;return false;}
   for(unsigned i=depth_;i>0;--i)if(stack_[i-1].tag==t){bool literal=stack_[i-1].literal;style_=stack_[i-1].before;depth_=i-1;
    if(literal){literal_end_=end+1;return false;}p_=end+1;return true;}
   if(t==14)return false; // An unmatched or rejected opener must stay auditable.
  }else{
   RichRainbow parameters{};bool valid=t!=14||rainbow_options(name_end,end,parameters);
   if(t==14&&(depth_>=16||overflow_rainbows_)){++overflow_rainbows_;literal_end_=end+1;return false;}
   if(depth_<16){stack_[depth_++]={(uint8_t)t,style_,!valid};
    if(!valid){literal_end_=end+1;return false;}
    if(t<=9)style_.color=(uint8_t)t;else if(t==10)style_.bold=true;else if(t==11)style_.italic=true;else if(t==12)style_.wave=true;else if(t==13)style_.jitter=true;
    else if(t==14){style_.rainbow=true;style_.rainbow_parameters=parameters;}else{style_.thinky_dots=true;style_.thinky_start=index_;}
   }
  }
  p_=end+1;
  return true;
 }
public:
 explicit RichReader(const char*p):p_(p){}
 bool next(RichChar&out){
  while(*p_&&markup()){}if(!*p_)return false;
  uint32_t cp=(uint8_t)*p_++;
  if(cp>=128){int n=cp<224?1:cp<240?2:3;cp&=(1u<<(6-n))-1;while(n--){uint8_t b=(uint8_t)*p_;if((b&192)!=128){cp='?';break;}++p_;cp=(cp<<6)|(b&63);}}
  if(literal_tag_&&cp==']')literal_tag_=false;out={cp,style_,index_++};return true;
 }
};
}
