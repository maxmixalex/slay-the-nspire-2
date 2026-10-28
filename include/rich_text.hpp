#pragma once
#include <stdint.h>
#include <stddef.h>
#include "rich_rainbow.hpp"
#include "inline_icons.hpp"
#include "unicode_text.hpp"
namespace spire {
struct RichStyle { uint8_t color=0; bool bold=false,italic=false,wave=false,jitter=false,rainbow=false,thinky_dots=false; unsigned thinky_start=0;RichRainbow rainbow_parameters{}; };
struct RichChar { uint32_t codepoint; RichStyle style; unsigned index; uint32_t previous=0; uint8_t previous_font=0; };
// Original styles select their own font metrics. They are never simulated by
// widening or drawing a second offset copy of the regular glyph.
inline unsigned rich_font_style(const RichChar&ch){return inline_icon(ch.codepoint)?0u:unsigned(ch.style.bold)|(unsigned(ch.style.italic)<<1);}
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
 uint32_t previous_=0;uint8_t previous_font_=0;
 void emit(RichChar&out,uint32_t cp){out={cp,style_,index_++,previous_,previous_font_};previous_=cp=='\n'?0:cp;previous_font_=uint8_t(rich_font_style(out));}
 const char* p_;const char*literal_end_=nullptr;bool literal_tag_=false;RichStyle style_{};unsigned index_=0,depth_=0,overflow_rainbows_=0,overflow_sizes_=0,overflow_codes_=0;
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
  for(unsigned i=0;i<15;++i)if(equal(p,n,names[i]))return i+1;
  if(equal(p,n,"code"))return 17;
  if(equal(p,n,"pink"))return 18;
  if(equal(p,n,"shake"))return 19;
  return n>=9&&equal(p,9,"font_size")&&(n==9||p[9]=='=')?16:0;
 }
 static bool font_size_options(const char*p,const char*end){
  if(end-p<11||!equal(p,10,"font_size="))return false;
  p+=10;unsigned value=0;bool digits=false;
  while(p<end&&*p>='0'&&*p<='9'){digits=true;value=value*10+unsigned(*p++-'0');if(value>1024)return false;}
  while(p<end&&space(*p))++p;
  return digits&&value&&p==end;
 }
 bool markup(){
  if(*p_!='['||literal_tag_||(literal_end_&&p_<literal_end_))return false;const char*start=p_+1;bool close=*start=='/';if(close)++start;
  const char*end=start;while(*end&&*end!=']'&&end-start<128)++end;
  const char*name_end=start;while(name_end<end&&!space(*name_end))++name_end;unsigned t=tag(start,(size_t)(name_end-start));
  const bool scoped=t==14||t==16||t==17;unsigned&overflow=t==16?overflow_sizes_:t==17?overflow_codes_:overflow_rainbows_;
  if(*end!=']'){
   if(scoped&&!close){if(depth_<16&&!overflow)stack_[depth_++]={uint8_t(t),style_,true};else ++overflow;literal_tag_=true;}
   return false;
  }
  if(!t||(t==16&&close&&!equal(start,size_t(end-start),"font_size"))||
     (name_end<end&&(!scoped||close)))return false;
  if(close){
   if(scoped&&overflow){--overflow;literal_end_=end+1;return false;}
   for(unsigned i=depth_;i>0;--i)if(stack_[i-1].tag==t){bool literal=stack_[i-1].literal;style_=stack_[i-1].before;depth_=i-1;
    if(literal){literal_end_=end+1;return false;}p_=end+1;return true;}
   if(scoped)return false; // An unmatched or rejected opener must stay auditable.
  }else{
   RichRainbow parameters{};bool valid=t==14?rainbow_options(name_end,end,parameters):t==16?font_size_options(start,end):t!=17||name_end==end;
   if(scoped&&(depth_>=16||overflow)){++overflow;literal_end_=end+1;return false;}
   if(depth_<16){stack_[depth_++]={(uint8_t)t,style_,!valid};
    if(!valid){literal_end_=end+1;return false;}
    if(t<=9)style_.color=(uint8_t)t;else if(t==10)style_.bold=true;else if(t==11)style_.italic=true;else if(t==12)style_.wave=true;else if(t==13)style_.jitter=true;
    else if(t==14){style_.rainbow=true;style_.rainbow_parameters=parameters;}else if(t==15){style_.thinky_dots=true;style_.thinky_start=index_;}
    else if(t==18)style_.color=10;else if(t==19)style_.jitter=true;
    // Original 18/22/28px stage directions project below the declared 6px
    // scene minimum. Consume their BBCode scope and keep the fitted scene's
    // selected size; the reader's deliberately larger review size also wins.
    // Code scopes preserve their text and nested styles in the selected scene
    // face; this viewport does not synthesize an unavailable monospace font.
   }
  }
  p_=end+1;
  return true;
 }
public:
 explicit RichReader(const char*p):p_(p){}
 bool next(RichChar&out){
  for(;;){
  while(*p_&&markup()){}if(!*p_)return false;
  if(!literal_tag_&&(!literal_end_||p_>=literal_end_)&&*p_=='['&&
     (p_[1]=='l'||p_[1]=='r')&&p_[2]=='b'&&p_[3]==']'){
   const uint32_t cp=p_[1]=='l'?'[':']';p_+=4;emit(out,cp);return true;
  }
  uint32_t cp=(uint8_t)*p_++;
  if(cp>=128){int n=cp<224?1:cp<240?2:3;cp&=(1u<<(6-n))-1;while(n--){uint8_t b=(uint8_t)*p_;if((b&192)!=128){cp='?';break;}++p_;cp=(cp<<6)|(b&63);}}
  if(text_format_control(cp))continue;
  if(literal_tag_&&cp==']')literal_tag_=false;emit(out,cp);return true;
  }
 }
};
}
