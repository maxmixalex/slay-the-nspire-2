#include "card_keyword_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace spire::card_keyword_text;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"card keyword text:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static Labels labels(){Labels result{};const char*names[]={"Unplayable","Innate","Retain","Sly","Ethereal","Exhaust","Eternal"};
 for(unsigned i=0;i<Count;++i)std::strcpy(result.title[i],names[i]);std::strcpy(result.period,".");return result;}
static std::string line(const Labels&l,unsigned i){return std::string("[gold]")+l.title[i]+"[/gold]"+l.period;}
int main(){
 const auto l=labels();
 // These are complete paragraphs. Identical words within prose remain.
 const char*cases[]={"[gold]Retain.[/gold]\nBody\n[gold]Exhaust.[/gold]","[gold]Retain[/gold].\nBody\n[gold]Exhaust[/gold].",
  " \t[gold]Retain.[/gold]\r\nBody\n[gold]Exhaust[/gold].\t","Body\n[gold]Exhaust.[/gold]\n[gold]Eternal.[/gold]"};
 for(const char*text:cases){char body[512]{};std::strcpy(body,text);strip(body,l);CHECK(!std::strcmp(body,"Body"));}
 const char*semantic[]={"Do not [gold]Exhaust[/gold] this card.","[gold]Retain.[/gold] Next sentence.","\"[gold]Sly.[/gold]\"", "Body\n\nNext paragraph."};
 for(const char*text:semantic){char body[512]{};std::strcpy(body,text);strip(body,l);CHECK(!std::strcmp(body,text));}
 for(unsigned flags=0;flags<256;++flags){std::string expected;const auto append=[&](const std::string&value){if(!expected.empty())expected+='\n';expected+=value;};
  for(unsigned i=0;i<5;++i)if(flags&Flags[i])append(line(l,i));append("Body\n[purple]Modifier.[/purple]\nReplay 2.");
  for(unsigned i=5;i<Count;++i)if(flags&Flags[i])append(line(l,i));
  char text[1024]{};CHECK(compose(text,sizeof text,"Body\n[purple]Modifier.[/purple]\nReplay 2.",flags,l));CHECK(text==expected);
  strip(text,l);CHECK(!std::strcmp(text,"Body\n[purple]Modifier.[/purple]\nReplay 2."));
 }
 auto zh=l;std::strcpy(zh.title[2],"保留");std::strcpy(zh.title[5],"消耗");std::strcpy(zh.period,"。");
 char text[256]="[gold]保留。[/gold]\n原段落中提及[gold]消耗[/gold]。\n[gold]消耗。[/gold]";
 strip(text,zh);CHECK(!std::strcmp(text,"原段落中提及[gold]消耗[/gold]。"));
 char out[256]{};CHECK(compose(out,sizeof out,text,5,zh));CHECK(!std::strcmp(out,"[gold]保留[/gold]。\n原段落中提及[gold]消耗[/gold]。\n[gold]消耗[/gold]。"));
 unsigned char bounded[8];std::memset(bounded,0x7b,sizeof bounded);CHECK(!compose(reinterpret_cast<char*>(bounded+2),4,"Body",4,l));CHECK(bounded[0]==0x7b&&bounded[1]==0x7b&&bounded[5]==0&&bounded[6]==0x7b&&bounded[7]==0x7b);
 CHECK(!compose(nullptr,0,"Body",0,l));char empty[1]{'x'};CHECK(compose(empty,1,"",0,l)&&empty[0]==0);CHECK(!compose(empty,1,"Body",0,l)&&empty[0]==0);
 std::printf("card keyword text: %u checks passed\n",checks);
}
