#pragma once
#include <stddef.h>
#include <stdint.h>
#include "inline_icons.hpp"
namespace spire {
// A bounded interpreter for the formatting forms used by the pinned locale.
// It never reads gameplay RNG. Unknown values remain visible as {name} so a
// missing binding cannot silently display a fabricated zero.
struct LocalizedValue {bool known=false;int64_t number=0;uint32_t scale=1;const char* text=nullptr;};
using LocalizedResolver=LocalizedValue(*)(const void*,const char*);
// Explicit catalog values can carry a detached/permanent card snapshot. The
// callback receives the actual catalog value before applying that context.
using LocalizedDefaultResolver=LocalizedValue(*)(const void*,const char*,LocalizedValue);
class LocalizedTemplate {
 char*out_;size_t capacity_,used_=0;LocalizedResolver resolve_;const void*context_;
 LocalizedDefaultResolver default_resolver_=nullptr;
 bool truncated_=false;unsigned language_=0;
 static bool same(const char*a,size_t n,const char*b){size_t i=0;while(b[i]){if(i>=n||a[i]!=b[i])return false;++i;}return i==n;}
 static size_t length(const char*p){size_t n=0;while(p&&p[n])++n;return n;}
 void ch(char c){if(capacity_&&used_+1<capacity_){out_[used_++]=c;out_[used_]=0;}else truncated_=true;}
 void text(const char*p,size_t n){for(size_t i=0;i<n;i++)ch(p[i]);}
 void text(const char*p){text(p,length(p));}
 // Ndless has no 64-bit division runtime. Bounded bit division also handles
 // INT64_MIN without signed negation overflow; normal game values take the
 // fast 32-bit path. This is formatting only and never touches gameplay RNG.
 static uint64_t magnitude(int64_t n){return n<0?uint64_t(-(n+1))+1:uint64_t(n);}
 static uint64_t divide(uint64_t n,uint32_t d,uint32_t&remainder){
  if(n<=UINT32_MAX){remainder=uint32_t(n)%d;return uint32_t(n)/d;}
  uint64_t q=0,r=0;for(unsigned bit=64;bit;){--bit;r=(r<<1)|((n>>bit)&1);if(r>=d){r-=d;q|=uint64_t(1)<<bit;}}
  remainder=uint32_t(r);return q;
 }
 void unsigned_integer(uint64_t u){char digits[24];unsigned n=0;do{uint32_t rem;u=divide(u,10,rem);digits[n++]=char('0'+rem);}while(u);while(n)ch(digits[--n]);}
 bool decimal_comma()const{return language_==4||language_==6||language_==7||language_==8||language_==11||language_==12||language_==13||language_==14||language_==16;}
 const char*group_separator()const{
  if(language_==6)return "\xe2\x80\xaf";
  if(language_==11||language_==13)return "\xc2\xa0";
  return decimal_comma()?".":",";
 }
 void locale_integer(LocalizedValue v){
  // LocaleNumberFormatter defaults to N0. The pinned sixteen cultures group
  // in threes, with midpoint-to-even rounding and their own separator.
  uint32_t remainder;const uint32_t scale=v.scale?v.scale:1;
  uint64_t number=divide(magnitude(v.number),scale,remainder);
  if(uint64_t(remainder)*2>scale||(uint64_t(remainder)*2==scale&&(number&1)))++number;
  if(v.number<0&&number)ch('-');char digits[24];unsigned n=0;
  do{uint32_t rem;number=divide(number,10,rem);digits[n++]=char('0'+rem);}while(number);
  while(n){ch(digits[--n]);if(n&&n%3==0)text(group_separator());}
 }
 void integer(int64_t v){if(v<0)ch('-');unsigned_integer(magnitude(v));}
 void value(LocalizedValue v,bool absolute=false){
  if(v.text){text(v.text);return;}unsigned scale=v.scale?v.scale:1;
  uint32_t rem;uint64_t whole=divide(magnitude(v.number),scale,rem);if(v.number<0&&!absolute)ch('-');unsigned_integer(whole);
  if(rem){ch(decimal_comma()?',':'.');for(unsigned digits=0;rem&&digits<4;digits++){uint64_t digit=divide(uint64_t(rem)*10,scale,rem);ch(char('0'+digit));}}
 }
 void energy_icons(LocalizedValue v){
  if(v.text){text("{energy-value}");return;}
  uint32_t remainder;const uint64_t amount=divide(magnitude(v.number),v.scale?v.scale:1,remainder);
  // The source repeats one to three sprites; zero, negative and >=4 values
  // show the integer followed by one sprite. At 320x240 each sprite occupies
  // one text cell and uses the existing character-colored card resource art.
  if(v.number>0&&amount>0&&amount<4){for(unsigned i=0;i<unsigned(amount);++i)text(InlineEnergyText);return;}
  if(v.number<0&&amount)ch('-');unsigned_integer(amount);text(InlineEnergyText);
 }
 static size_t split(const char*p,size_t n,char delimiter){
  unsigned braces=0,parens=0;for(size_t i=0;i<n;i++){if(p[i]=='{')++braces;else if(p[i]=='}'&&braces)--braces;else if(!braces&&p[i]=='(')++parens;else if(!braces&&p[i]==')'&&parens)--parens;else if(!braces&&!parens&&p[i]==delimiter)return i;}return n;
 }
 void branch(const char*p,size_t n,unsigned choice,LocalizedValue context,unsigned depth){
  for(unsigned at=0;;at++){size_t end=split(p,n,'|');if(at==choice){render(p,end,context,depth+1);return;}if(end==n)return;p+=end+1;n-=end+1;}
 }
 LocalizedValue named(const char*p,size_t n,LocalizedValue context){
  if(!n)return context;char key[96];if(n>=sizeof key)return {};for(size_t i=0;i<n;i++)key[i]=p[i];key[n]=0;
  // v17 card templates carry their audited catalog fallback beside the key.
  // Live resolution still receives the original key; unknown live/static
  // variables never receive an invented zero.
  size_t end=0;while(end<n&&key[end]!='=')++end;
  bool fallback=end<n,negative=false,valid=false;int64_t number=0;
  if(fallback){key[end]=0;size_t at=end+1;if(at<n&&p[at]=='-'){negative=true;++at;}valid=at<n;
   for(;at<n;at++){if(p[at]<'0'||p[at]>'9'||number>999999){valid=false;break;}number=number*10+p[at]-'0';}}
  if(fallback&&valid&&default_resolver_)return default_resolver_(context_,key,{true,negative?-number:number,1,nullptr});
  auto value=resolve_?resolve_(context_,key):LocalizedValue{};
  return value.known?value:fallback&&valid?LocalizedValue{true,negative?-number:number,1,nullptr}:LocalizedValue{};
 }
 // The pinned locale compares Amount with integer thresholds (including the
 // chained ==1 / >1 Shrink branches). Compare the rational value without
 // multiplying either signed operand, even at the int64/uint32 limits.
 static bool condition(const char*p,size_t n,LocalizedValue v,bool&pass){
  if(!n||v.text)return false;char op=*p++;--n;if(op!='<'&&op!='>'&&op!='='&&op!='!')return false;
  bool equal=n&&*p=='=';if(equal){++p;--n;}bool negative=n&&*p=='-';if(negative){++p;--n;}if(!n)return false;
  uint64_t threshold=0;constexpr uint64_t bound=uint64_t(INT64_MAX)/10;
  for(size_t i=0;i<n;i++){if(p[i]<'0'||p[i]>'9')return false;unsigned digit=unsigned(p[i]-'0');if(threshold>bound||(threshold==bound&&digit>(negative?8u:7u)))return false;threshold=threshold*10+digit;}
  if(!threshold)negative=false;bool value_negative=v.number<0;int comparison;
  if(value_negative!=negative)comparison=value_negative?-1:1;
  else{uint32_t rem;uint64_t whole=divide(magnitude(v.number),v.scale?v.scale:1,rem);comparison=whole<threshold?-1:whole>threshold?1:rem?1:0;if(value_negative)comparison=-comparison;}
  pass=op=='<'?(equal?comparison<=0:comparison<0):op=='>'?(equal?comparison>=0:comparison>0):op=='='?comparison==0:comparison!=0;return true;
 }
 bool conditional(const char*p,size_t n,LocalizedValue v,unsigned depth){
  size_t first=split(p,n,'|');if(first==n)return false;
  // StringVar is empty until its owning offer/instance has been prepared.
  // The remaining non-comparison forms in this locale are boolean values.
  if(v.text||split(p,first,'?')==first){branch(p,n,v.text?(*v.text?0:1):(v.number?0:1),v,depth);return true;}
  while(true){size_t end=split(p,n,'|'),q=split(p,end,'?');
   if(q==end){render(p,end,v,depth+1);return true;}
   bool pass=false;if(!condition(p,q,v,pass))return false;
   if(pass){render(p+q+1,end-q-1,v,depth+1);return true;}
   if(end==n)return true;p+=end+1;n-=end+1;
  }
 }
 unsigned plural_choice(const char*p,size_t n,LocalizedValue v,unsigned language)const{
  unsigned forms=1;for(size_t at=0;at<n;){size_t end=split(p+at,n-at,'|');if(end==n-at)break;++forms;at+=end+1;}
  uint32_t fraction=0;uint64_t value=divide(magnitude(v.number),v.scale?v.scale:1,fraction);
  if(forms<=1)return 0;
  if(forms==2)return !fraction&&(language==6||language==12?value<=1:value==1)?0:1;
  uint32_t last=0,last2=0;divide(value,10,last);divide(value,100,last2);
  if(language==13){
   if(!fraction&&last==1&&last2!=11)return 0;
   if(!fraction&&last>=2&&last<=4&&(last2<12||last2>14))return 1;
   return fraction&&forms>3?3:2;}
  if(language==11){
   if(!fraction&&value==1)return 0;
   if(!fraction&&last>=2&&last<=4&&(last2<12||last2>14))return 1;
   return fraction&&forms>3?3:2;}
  return !fraction&&value==1?0:forms-1;
 }
 void expression(const char*p,size_t n,LocalizedValue context,unsigned depth){
  size_t colon=split(p,n,':'),name_len=colon;LocalizedValue v=named(p,name_len,context);
  if(colon==n){if(v.known)value(v);else{ch('{');text(p,n);ch('}');}return;}
  if(!v.known){ch('{');text(p,n);ch('}');return;}
  const char*fmt=p+colon+1;size_t fn=n-colon-1,second=split(fmt,fn,':');
  const char*body=second<fn?fmt+second+1:nullptr;size_t bn=second<fn?fn-second-1:0;
  // The original Italian Refract chains a preview annotation before plural.
  // diff/inverseDiff do not change the numeric value; retain that value for
  // the following grammatical formatter instead of exposing its braces.
  while(body&&(same(fmt,second,"diff()")||same(fmt,second,"inverseDiff()"))){
   fmt=body;fn=bn;second=split(fmt,fn,':');body=second<fn?fmt+second+1:nullptr;bn=second<fn?fn-second-1:0;
  }
  if(same(fmt,fn,"n()")&&!v.text){locale_integer(v);return;}
  if(same(fmt,second,"show")){branch(body,bn,v.known&&(v.text?*v.text!=0:v.number!=0)?0:1,v,depth);return;}
  if(same(fmt,second,"plural")||(second>8&&same(fmt,7,"plural(")&&fmt[second-1]==')')){
   const unsigned language=same(fmt,second,"plural(ru)")?13:same(fmt,second,"plural(pl)")?11:language_;
   branch(body,bn,plural_choice(body,bn,v,language),v,depth);return;}
  if(same(fmt,second,"choose")){branch(body,bn,v.number==int64_t(v.scale?v.scale:1)?0:1,v,depth);return;}
  if(second>=8&&same(fmt,7,"choose(")&&fmt[second-1]==')'){
   const char*choices=fmt+7;size_t cn=second-8;unsigned selected=0;bool matched=false;
   while(true){size_t end=split(choices,cn,'|');
    if(v.text)matched=same(choices,end,v.text);else{char digits[24];unsigned used=0;uint32_t rem;uint64_t number=divide(magnitude(v.number),v.scale?v.scale:1,rem);bool neg=v.number<0&&number;do{number=divide(number,10,rem);digits[used++]=char('0'+rem);}while(number);if(neg)digits[used++]='-';if(used==end){matched=true;for(unsigned i=0;i<used;i++)if(digits[used-1-i]!=choices[i])matched=false;}}
    if(matched||end==cn){if(!matched)++selected;break;}++selected;choices+=end+1;cn-=end+1;
   }branch(body,bn,selected,v,depth);return;
  }
  if(same(fmt,second,"cond")&&body&&conditional(body,bn,v,depth))return;
  if(same(fmt,fn,"diff()")||same(fmt,fn,"inverseDiff()")){if(v.known)value(v);else{ch('{');text(p,name_len);ch('}');}return;}
  if(same(fmt,fn,"abs()")){if(v.known)value(v,true);return;}
  if(same(fmt,fn,"percentMore()")||same(fmt,fn,"percentLess()")){
   if(v.known){const uint32_t scale=v.scale?v.scale:1;
    // Percent variables in the game are bounded ratios. Preserve an invalid
    // out-of-range template visibly rather than overflow its arithmetic.
    if(v.number>INT64_MAX/100||v.number<INT64_MIN/100+int64_t(scale)){ch('{');text(p,n);ch('}');return;}
    int64_t delta=(v.number-int64_t(scale))*100;bool negative=delta<0;if(same(fmt,fn,"percentLess()"))negative=!negative;
    uint32_t rem;uint64_t rounded=divide(magnitude(delta),scale,rem);
    if(uint64_t(rem)*2>scale||(uint64_t(rem)*2==scale&&(rounded&1)))++rounded;if(negative&&rounded)ch('-');unsigned_integer(rounded);}return;
  }
  if(same(fmt,fn,"starIcons()")){
   // Pinned StarIconsFormatter casts the preview value to an integer and
   // emits that many original sprites, including no sprite for zero.
   // Bound the output work by the destination, not an arbitrary game count.
   if(v.text||v.number<0){ch('{');text(p,n);ch('}');return;}
   uint32_t rem;const uint64_t count=divide(uint64_t(v.number),v.scale?v.scale:1,rem);
   const size_t available=capacity_&&used_<capacity_?(capacity_-used_-1)/3:0;
   const size_t emit=count>available?available:size_t(count);
   for(size_t i=0;i<emit;++i)text(InlineStarText);
   if(count>available)truncated_=true;return;
  }
  if(same(fmt,fn,"energyIcons()")){
   energy_icons(v);return;
  }
  if(fn>13&&same(fmt,12,"energyIcons(")&&fmt[fn-1]==')'){
   unsigned amount=0;bool valid=true;for(size_t i=12;i+1<fn;++i){if(fmt[i]<'0'||fmt[i]>'9'||amount>999999){valid=false;break;}amount=amount*10+unsigned(fmt[i]-'0');}
   if(valid)energy_icons({true,int64_t(amount),1,nullptr});else{ch('{');text(p,n);ch('}');}return;
  }
  // Boolean expressions omit the formatter name in the original locale.
  if(second==fn&&v.known){branch(fmt,fn,v.text?(*v.text?0:1):(v.number?0:1),v,depth);return;}
  ch('{');text(p,n);ch('}');
 }
 void render(const char*p,size_t n,LocalizedValue context,unsigned depth){
  if(depth>12){text("{format-depth}");return;}
  for(size_t at=0;at<n;){if(p[at]!='{'){ch(p[at++]);continue;}size_t end=at+1;unsigned nested=1;
   for(;end<n&&nested;end++){if(p[end]=='{')++nested;else if(p[end]=='}')--nested;}
   if(nested){text(p+at,n-at);return;}expression(p+at+1,end-at-2,context,depth);at=end;
  }
 }
public:
 LocalizedTemplate(char*out,size_t capacity,LocalizedResolver resolver,const void*context,const char* = "能量",const char* = "辉星",bool = false,LocalizedDefaultResolver default_resolver=nullptr):out_(out),capacity_(capacity),resolve_(resolver),context_(context),default_resolver_(default_resolver){if(capacity_)out_[0]=0;}
 bool truncated()const{return truncated_;}
 LocalizedTemplate& for_language(unsigned language){language_=language;return *this;}
 void format(const char*source){render(source,length(source),{},0);}
};
} // namespace spire
