#pragma once
#include <stddef.h>
#include <stdint.h>
namespace spire {
// A bounded interpreter for the formatting forms used by the pinned locale.
// It never reads gameplay RNG. Unknown values remain visible as {name} so a
// missing binding cannot silently display a fabricated zero.
struct LocalizedValue {bool known=false;int64_t number=0;uint32_t scale=1;const char* text=nullptr;};
using LocalizedResolver=LocalizedValue(*)(const void*,const char*);
class LocalizedTemplate {
 char*out_;size_t capacity_,used_=0;LocalizedResolver resolve_;const void*context_;
 static bool same(const char*a,size_t n,const char*b){size_t i=0;while(b[i]){if(i>=n||a[i]!=b[i])return false;++i;}return i==n;}
 static size_t length(const char*p){size_t n=0;while(p&&p[n])++n;return n;}
 void ch(char c){if(capacity_&&used_+1<capacity_){out_[used_++]=c;out_[used_]=0;}}
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
 void integer(int64_t v){if(v<0)ch('-');unsigned_integer(magnitude(v));}
 void value(LocalizedValue v,bool absolute=false){
  if(v.text){text(v.text);return;}unsigned scale=v.scale?v.scale:1;
  uint32_t rem;uint64_t whole=divide(magnitude(v.number),scale,rem);if(v.number<0&&!absolute)ch('-');unsigned_integer(whole);
  if(rem){ch('.');for(unsigned digits=0;rem&&digits<4;digits++){uint64_t digit=divide(uint64_t(rem)*10,scale,rem);ch(char('0'+digit));}}
 }
 static size_t split(const char*p,size_t n,char delimiter){
  unsigned braces=0,parens=0;for(size_t i=0;i<n;i++){if(p[i]=='{')++braces;else if(p[i]=='}'&&braces)--braces;else if(!braces&&p[i]=='(')++parens;else if(!braces&&p[i]==')'&&parens)--parens;else if(!braces&&!parens&&p[i]==delimiter)return i;}return n;
 }
 void branch(const char*p,size_t n,unsigned choice,LocalizedValue context,unsigned depth){
  for(unsigned at=0;;at++){size_t end=split(p,n,'|');if(at==choice){render(p,end,context,depth+1);return;}if(end==n)return;p+=end+1;n-=end+1;}
 }
 LocalizedValue named(const char*p,size_t n,LocalizedValue context){
  if(!n)return context;char key[96];if(n>=sizeof key)return {};for(size_t i=0;i<n;i++)key[i]=p[i];key[n]=0;
  return resolve_?resolve_(context_,key):LocalizedValue{};
 }
 void expression(const char*p,size_t n,LocalizedValue context,unsigned depth){
  size_t colon=split(p,n,':'),name_len=colon;LocalizedValue v=named(p,name_len,context);
  if(colon==n){if(v.known)value(v);else{ch('{');text(p,n);ch('}');}return;}
  if(!v.known){ch('{');text(p,n);ch('}');return;}
  const char*fmt=p+colon+1;size_t fn=n-colon-1,second=split(fmt,fn,':');
  const char*body=second<fn?fmt+second+1:nullptr;size_t bn=second<fn?fn-second-1:0;
  if(same(fmt,second,"show")){branch(body,bn,v.known&&(v.text?*v.text!=0:v.number!=0)?0:1,v,depth);return;}
  if(same(fmt,second,"plural")){branch(body,bn,v.number==int64_t(v.scale?v.scale:1)?0:1,v,depth);return;}
  if(second>=8&&same(fmt,7,"choose(")&&fmt[second-1]==')'){
   const char*choices=fmt+7;size_t cn=second-8;unsigned selected=0;bool matched=false;
   while(true){size_t end=split(choices,cn,'|');
    if(v.text)matched=same(choices,end,v.text);else{char digits[24];unsigned used=0;uint32_t rem;uint64_t number=divide(magnitude(v.number),v.scale?v.scale:1,rem);bool neg=v.number<0&&number;do{number=divide(number,10,rem);digits[used++]=char('0'+rem);}while(number);if(neg)digits[used++]='-';if(used==end){matched=true;for(unsigned i=0;i<used;i++)if(digits[used-1-i]!=choices[i])matched=false;}}
    if(matched||end==cn){if(!matched)++selected;break;}++selected;choices+=end+1;cn-=end+1;
   }branch(body,bn,selected,v,depth);return;
  }
  if(same(fmt,second,"cond")&&body){
   size_t q=split(body,bn,'?');bool pass=false;if(q<bn){
    if(same(body,q,"<0"))pass=v.number<0;else if(same(body,q,">0"))pass=v.number>0;
    else if(same(body,q,"=0")||same(body,q,"==0"))pass=v.number==0;
    else if(same(body,q,"<=0"))pass=v.number<=0;else if(same(body,q,">=0"))pass=v.number>=0;
    branch(body+q+1,bn-q-1,pass?0:1,v,depth);return;}
  }
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
  if(same(fmt,fn,"energyIcons()")||same(fmt,fn,"starIcons()")){
   if(v.known)value(v);else{ch('{');text(p,name_len);ch('}');}text(same(fmt,fn,"starIcons()")?" 点[gold]星星[/gold]":" 点能量");return;
  }
  if(fn>13&&same(fmt,12,"energyIcons(")&&fmt[fn-1]==')'){
   bool preceding=used_&&((out_[used_-1]>='0'&&out_[used_-1]<='9')||out_[used_-1]=='X');
   if(!preceding)text(fmt+12,fn-13);text(" 点能量");return;
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
 LocalizedTemplate(char*out,size_t capacity,LocalizedResolver resolver,const void*context):out_(out),capacity_(capacity),resolve_(resolver),context_(context){if(capacity_)out_[0]=0;}
 void format(const char*source){render(source,length(source),{},0);}
};
} // namespace spire
