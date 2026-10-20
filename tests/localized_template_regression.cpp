#include "localized_template.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Context{int value=3;bool upgraded=false;const char*type="Attack";};
static LocalizedValue resolve(const void*p,const char*k){const auto&x=*static_cast<const Context*>(p);
 if(!std::strcmp(k,"N"))return {true,x.value};if(!std::strcmp(k,"IfUpgraded"))return {true,x.upgraded};
 if(!std::strcmp(k,"Type"))return {true,0,1,x.type};if(!std::strcmp(k,"Percent"))return {true,1035,1000};
 if(!std::strcmp(k,"Even"))return {true,1025,1000};if(!std::strcmp(k,"Low"))return {true,-25,100};
 if(!std::strcmp(k,"Min"))return {true,INT_MIN};if(!std::strcmp(k,"Empty"))return {true,0,1,""};return {};
}
static void expect(Context&x,const char*source,const char*expected){char b[512];LocalizedTemplate(b,sizeof b,resolve,&x).format(source);CHECK(!std::strcmp(b,expected));}
int main(){Context x;expect(x,"{N:diff()}","3");expect(x,"{IfUpgraded:show:up {N}|base}","base");x.upgraded=true;expect(x,"{IfUpgraded:show:up {N}|base}","up 3");
 expect(x,"{N:plural:one|{N} many}","3 many");x.value=1;expect(x,"{N:plural:one|{N} many}","one");
 expect(x,"{Type:choose(Attack|Skill|Power):A {N}|S|P|?}","A 1");x.type="Skill";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","S");x.type="Quest";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","?");
 x.value=-2;expect(x,"{N:cond:<0?negative|positive}","negative");expect(x,"{N:abs()}","2");expect(x,"{Min:abs()}","2147483648");
 expect(x,"{Low}","-0.25");expect(x,"{Percent:percentMore()}","4");expect(x,"{Even:percentMore()}","2");expect(x,"{Percent:percentLess()}","-4");
 expect(x,"x{Empty}y","xy");expect(x,"{Missing:show:bad|also bad}","{Missing:show:bad|also bad}");expect(x,"{Missing}","{Missing}");expect(x,"{unclosed","{unclosed");
 for(unsigned size=0;size<16;size++){char b[24];std::memset(b,0x5a,sizeof b);LocalizedTemplate(b,size,resolve,&x).format("prefix{N:abs()}long suffix");CHECK((unsigned char)b[size]==0x5a);if(size)CHECK(std::memchr(b,0,size));}
 std::printf("localized templates: %u checks passed\n",checks);
}
