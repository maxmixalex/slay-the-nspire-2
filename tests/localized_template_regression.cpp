#include "localized_template.hpp"
#include "rich_text.hpp"
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
 if(!std::strcmp(k,"Min"))return {true,INT_MIN};if(!std::strcmp(k,"WideMin"))return {true,INT64_MIN};
 if(!std::strcmp(k,"WideMax"))return {true,INT64_MAX};if(!std::strcmp(k,"WideScale"))return {true,4294967294LL,UINT32_MAX};
 if(!std::strcmp(k,"WideRatio"))return {true,6442450942LL,UINT32_MAX};if(!std::strcmp(k,"Empty"))return {true,0,1,""};return {};
}
static void expect(Context&x,const char*source,const char*expected){char b[512];LocalizedTemplate(b,sizeof b,resolve,&x).format(source);CHECK(!std::strcmp(b,expected));}
struct CorpusContext{int amount=2;bool flag=true;const char*text="REF";};
static LocalizedValue corpus_resolve(const void*p,const char*k){const auto&x=*static_cast<const CorpusContext*>(p);
 if(!std::strcmp(k,"GainsBlock")||!std::strcmp(k,"displayCount"))return {true,x.flag};
 if(!std::strcmp(k,"OwnerName"))return {true,0,1,"OWNER"};if(!std::strcmp(k,"modTitle"))return {true,0,1,"MOD"};
 const char* strings[]={"AncientCard","AncientCard.StringValue","StarterCard","StarterCard.StringValue","StarterRelic","StarterRelic.StringValue","UpgradedRelic","Character.StringValue","CardTitles.StringValue","ApplierName.StringValue"};
 for(const char*name:strings)if(!std::strcmp(k,name))return {true,0,1,x.text};
 const char* numbers[]={"Amount","SlowAmount","DisplayAmount","CalculatedBlock","Cards","modAmount","QuestCards","EventCards","RareCards","UncommonCards","CommonCards","CurseCards","BasicCards","ShopRelics","AncientRelics","EventRelics","RareRelics","UncommonRelics","CommonRelics","StarterRelics"};
 for(const char*name:numbers)if(!std::strcmp(k,name))return {true,x.amount};return {};
}
static void conditional_corpus(){
 // Every cond-bearing entry in the pinned zhs corpus, retaining its operators,
 // nesting, context selectors and tags. Prose is replaced by neutral labels;
 // the owner's original localization data is not distributed with tests.
 struct Case{const char*key,*source,*present,*empty;};
 const Case cases[]={
  {"cards.SOVEREIGN_BLADE.description","{GainsBlock:cond:\n{CalculatedBlock:diff()}[gold]BLOCK[/gold]|}","\n2[gold]BLOCK[/gold]",""},
  {"gameplay_ui.FORMAT_CARD_MOD_TITLE","{modTitle}{displayCount:cond: {modAmount}|}","MOD 2","MOD"},
  {"powers.BARRICADE_POWER.smartDescription","{ApplierName.StringValue:cond:[gold]{OwnerName}[/gold]|YOU}","[gold]OWNER[/gold]","YOU"},
  {"powers.DEXTERITY_POWER.smartDescription","{Amount:cond:<0?MINUS|PLUS}:{Amount:abs()}","PLUS:2","PLUS:0"},
  {"powers.FOCUS_POWER.smartDescription","{Amount:cond:<0?MINUS|PLUS}:{Amount:abs()}","PLUS:2","PLUS:0"},
  {"powers.SHRINK_POWER.smartDescription","{ApplierName.StringValue:cond:{} ACTIVE|[gold]{OwnerName}[/gold]}{Amount:cond:==1?TURN|>1?[blue]{}[/blue] TURNS|}","REF ACTIVE[blue]2[/blue] TURNS","[gold]OWNER[/gold]"},
  {"powers.SLOW_POWER.smartDescription","{SlowAmount:cond:>0?[blue]{DisplayAmount}%[/blue]|}","[blue]2%[/blue]",""},
  {"powers.STRENGTH_POWER.smartDescription","{Amount:cond:<0?MINUS|PLUS}:{Amount:abs()}","PLUS:2","PLUS:0"},
  {"relics.ARCHAIC_TOOTH.description","{StarterCard.StringValue:cond:[gold]{StarterCard}[/gold] TO [gold]{AncientCard}[/gold]|GENERIC}","[gold]REF[/gold] TO [gold]REF[/gold]","GENERIC"},
  {"relics.ARCHAIC_TOOTH.eventDescription","{StarterCard.StringValue:cond:[gold]{StarterCard}[/gold] TO [gold]{AncientCard}[/gold]|GENERIC}","[gold]REF[/gold] TO [gold]REF[/gold]","GENERIC"},
  {"relics.DUSTY_TOME.description","{AncientCard.StringValue:cond:[gold]{}+[/gold]|[gold]GENERIC[/gold]}","[gold]REF+[/gold]","[gold]GENERIC[/gold]"},
  {"relics.DUSTY_TOME.eventDescription","{AncientCard.StringValue:cond:[gold]{}+[/gold]|GENERIC [gold]ANCIENT[/gold]}","[gold]REF+[/gold]","GENERIC [gold]ANCIENT[/gold]"},
  {"relics.PAELS_TOOTH.description","{Cards}{CardTitles.StringValue:cond:\n\nLIST\n{}|}","2\n\nLIST\nREF","0"},
  {"relics.PAELS_TOOTH.eventDescription","{Cards}{CardTitles.StringValue:cond:\n\nLIST\n{}|}","2\n\nLIST\nREF","0"},
  {"relics.SEA_GLASS.description","{Cards}{Character.StringValue:cond:[gold]{}[/gold]|GENERIC}","2[gold]REF[/gold]","0GENERIC"},
  {"relics.TOUCH_OF_OROBAS.description","{StarterRelic.StringValue:cond:[gold]{StarterRelic}[/gold] TO [gold]{UpgradedRelic}[/gold]|GENERIC}","[gold]REF[/gold] TO [gold]REF[/gold]","GENERIC"},
  {"relics.TOUCH_OF_OROBAS.eventDescription","{StarterRelic.StringValue:cond:[gold]{StarterRelic}[/gold] TO [gold]{UpgradedRelic}[/gold]|GENERIC}","[gold]REF[/gold] TO [gold]REF[/gold]","GENERIC"},
  {"run_history.DECK_HISTORY.categories","{QuestCards:cond:>0?{}Q|}{EventCards:cond:>0?{}E|}{RareCards:cond:>0?{}R|}{UncommonCards:cond:>0?{}U|}{CommonCards:cond:>0?{}C|}{CurseCards:cond:>0?{}X|}{BasicCards:cond:>0?{}B|}","2Q2E2R2U2C2X2B",""},
  {"run_history.RELIC_HISTORY.categories","{ShopRelics:cond:>0?{}S|}{AncientRelics:cond:>0?{}A|}{EventRelics:cond:>0?{}E|}{RareRelics:cond:>0?{}R|}{UncommonRelics:cond:>0?{}U|}{CommonRelics:cond:>0?{}C|}{StarterRelics:cond:>0?{}B|}","2S2A2E2R2U2C2B",""},
 };
 CHECK(sizeof cases/sizeof*cases==19);
 for(const auto&c:cases)for(unsigned empty=0;empty<2;empty++){
  CorpusContext x{empty?0:2,empty==0,empty?"":"REF"};char out[512]{};LocalizedTemplate(out,sizeof out,corpus_resolve,&x).format(c.source);
  if(std::strcmp(out,empty?c.empty:c.present)){std::fprintf(stderr,"conditional corpus %s empty=%u got=%s\n",c.key,empty,out);CHECK(false);}CHECK(true);
 }
 Context x;expect(x,"{N:cond:==1?one|>1?{} many|none}","3 many");x.value=1;expect(x,"{N:cond:==1?one|>1?{} many|none}","one");
 x.value=0;expect(x,"{N:cond:==1?one|>1?{} many|none}","none");x.value=-2;expect(x,"{N:cond:==1?one|>1?{} many|none}","none");
 expect(x,"{N:cond:<0?negative|positive}","negative");expect(x,"{N:cond:<=-2?yes|no}","yes");expect(x,"{N:cond:!=-2?no|yes}","yes");
 expect(x,"{Low:cond:>=0?no|<0?yes|no}","yes");expect(x,"{WideRatio:cond:>1?yes|no}","yes");expect(x,"{WideScale:cond:==1?no|<1?yes|no}","yes");
 expect(x,"{WideMin:cond:==-9223372036854775808?yes|no}","yes");expect(x,"{WideMax:cond:>=9223372036854775807?yes|no}","yes");
 expect(x,"{N:cond:>9223372036854775808?bad|also bad}","{N:cond:>9223372036854775808?bad|also bad}");
 expect(x,"{Missing.StringValue:cond:bad|also bad}","{Missing.StringValue:cond:bad|also bad}");
 expect(x,"{Empty:cond:{Missing}|empty}","empty");expect(x,"{Type:cond:{Missing}|empty}","{Missing}");
 expect(x,"{Type:cond:{N:cond:<0?[gold]{}[/gold]|positive}|empty}","[gold]-2[/gold]");
}
static void thinky_dots(){
 RichReader reader("x[thinky_dots]a[aqua]b[/aqua]\n\xe4\xb8\xad[/thinky_dots]z");RichChar ch{};
 CHECK(reader.next(ch)&&ch.codepoint=='x'&&!ch.style.thinky_dots);
 CHECK(reader.next(ch)&&ch.codepoint=='a'&&ch.style.thinky_dots&&ch.style.thinky_start==1&&ch.index==1);
 CHECK(reader.next(ch)&&ch.codepoint=='b'&&ch.style.thinky_dots&&ch.style.color==7&&ch.style.thinky_start==1&&ch.index==2);
 CHECK(reader.next(ch)&&ch.codepoint=='\n'&&ch.style.thinky_dots&&!ch.style.color&&ch.index==3);
 CHECK(reader.next(ch)&&ch.codepoint==0x4e2d&&ch.style.thinky_dots&&ch.index==4);
 CHECK(reader.next(ch)&&ch.codepoint=='z'&&!ch.style.thinky_dots&&ch.index==5);CHECK(!reader.next(ch));
 RichReader unknown("[not_a_tag]x[/not_a_tag]");CHECK(unknown.next(ch)&&ch.codepoint=='[');
 CHECK(rich_thinky_offset(0,0)==0&&rich_thinky_offset(100,2)==0&&rich_thinky_offset(UINT32_MAX,UINT32_MAX)==0);
 CHECK(rich_thinky_offset(44,0)==-1&&rich_thinky_offset(200,0)==-1&&rich_thinky_offset(356,0)==-1);
 CHECK(rich_thinky_offset(400,0)==0&&rich_thinky_offset(4400,0)==0&&rich_thinky_offset(4600,0)==-1);
 CHECK(rich_thinky_offset(300,1)==-1&&rich_thinky_offset(500,1)==0);
}
int main(){Context x;expect(x,"{N:diff()}","3");expect(x,"{IfUpgraded:show:up {N}|base}","base");x.upgraded=true;expect(x,"{IfUpgraded:show:up {N}|base}","up 3");
 expect(x,"{N:plural:one|{N} many}","3 many");x.value=1;expect(x,"{N:plural:one|{N} many}","one");
 expect(x,"{Type:choose(Attack|Skill|Power):A {N}|S|P|?}","A 1");x.type="Skill";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","S");x.type="Quest";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","?");
 x.value=-2;expect(x,"{N:cond:<0?negative|positive}","negative");expect(x,"{N:abs()}","2");expect(x,"{Min:abs()}","2147483648");
 expect(x,"{Low}","-0.25");expect(x,"{Percent:percentMore()}","4");expect(x,"{Even:percentMore()}","2");expect(x,"{Percent:percentLess()}","-4");
 expect(x,"{WideMin}","-9223372036854775808");expect(x,"{WideMin:abs()}","9223372036854775808");expect(x,"{WideMax}","9223372036854775807");
 expect(x,"{WideScale}","0.9999");expect(x,"{WideRatio:percentMore()}","50");expect(x,"{WideMin:choose(-9223372036854775808|0):minimum|zero|other}","minimum");
 expect(x,"x{Empty}y","xy");expect(x,"{Missing:show:bad|also bad}","{Missing:show:bad|also bad}");expect(x,"{Missing}","{Missing}");expect(x,"{unclosed","{unclosed");
 for(unsigned size=0;size<16;size++){char b[24];std::memset(b,0x5a,sizeof b);LocalizedTemplate(b,size,resolve,&x).format("prefix{N:abs()}long suffix");CHECK((unsigned char)b[size]==0x5a);if(size)CHECK(std::memchr(b,0,size));}
 conditional_corpus();thinky_dots();std::printf("localized templates: %u checks passed\n",checks);
}
