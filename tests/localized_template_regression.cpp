#include "localized_template.hpp"
#include "rich_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
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
static LocalizedValue catalog_snapshot(const void*p,const char*key,LocalizedValue value){
 if(!std::strcmp(key,"Snapshot")){value.number+=static_cast<const Context*>(p)->value;return value;}
 auto live=resolve(p,key);return live.known?live:value;
}
static void catalog_context(){
 Context x;char text[256]{};
 LocalizedTemplate(text,sizeof text,resolve,&x,"Energy","Stars",true,catalog_snapshot).format("{Snapshot=13:diff()} / {N=99:diff()} / {Other=4:plural:item|items}");
 CHECK(!std::strcmp(text,"16 / 3 / items"));
 LocalizedTemplate(text,sizeof text,resolve,&x,"Energy","Stars",true,catalog_snapshot).format("{Snapshot:diff()} / {Snapshot=broken:diff()}");
 CHECK(!std::strcmp(text,"{Snapshot:diff()} / {Snapshot=broken:diff()}"));
}
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
static std::string rich_plain(const char*text){RichReader reader(text);RichChar ch{};std::string out;while(reader.next(ch)){CHECK(ch.codepoint<128);out+=char(ch.codepoint);}return out;}
static void source_font_sizes(){
 for(unsigned size:{18u,22u,28u}){
  const auto text="[i][font_size="+std::to_string(size)+"]a[b]b[/b]c[/font_size][/i]d";
  RichReader reader(text.c_str());RichChar ch{};
  CHECK(reader.next(ch)&&ch.codepoint=='a'&&ch.style.italic&&!ch.style.bold&&ch.index==0);
  CHECK(reader.next(ch)&&ch.codepoint=='b'&&ch.style.italic&&ch.style.bold&&ch.index==1);
  CHECK(reader.next(ch)&&ch.codepoint=='c'&&ch.style.italic&&!ch.style.bold&&ch.index==2);
  CHECK(reader.next(ch)&&ch.codepoint=='d'&&!ch.style.italic&&!ch.style.bold&&ch.index==3);CHECK(!reader.next(ch));
 }
 CHECK(rich_plain("[font_size=28][gold]a[font_size=22]b[/font_size]c[/gold][/font_size]d")=="abcd");
 for(const char*text:{"[font_size]x[/font_size]","[font_size=]x[/font_size]","[font_size=-1]x[/font_size]",
     "[font_size=22px]x[/font_size]","[font_size=0]x[/font_size]","[font_size=99999999999999999999]x[/font_size]",
     "[font_size=22","[/font_size]","[/font_size=22]"})CHECK(rich_plain(text)==text);
 CHECK(rich_plain("[font_size=22]a[font_size=bad]b[/font_size]c[/font_size]d")=="a[font_size=bad]b[/font_size]cd");
 std::string deep;for(unsigned i=0;i<17;++i)deep+="[font_size=22]";deep+='x';for(unsigned i=0;i<17;++i)deep+="[/font_size]";
 CHECK(rich_plain(deep.c_str())=="[font_size=22]x[/font_size]");
}
static void brackets_and_code(){
 CHECK(rich_plain("[lb]Unknown[rb]")=="[Unknown]");
 CHECK(rich_plain("[lb]gold[rb]X[lb]/gold[rb]")=="[gold]X[/gold]");
 RichReader literal("[lb]gold[rb]X[lb]/gold[rb]");RichChar ch{};
 while(literal.next(ch))CHECK(!ch.style.color);
 RichReader reader("[i][code]a[gold]b[/gold]c[/code][/i]d");
 CHECK(reader.next(ch)&&ch.codepoint=='a'&&ch.style.italic&&!ch.style.color&&ch.index==0);
 CHECK(reader.next(ch)&&ch.codepoint=='b'&&ch.style.italic&&ch.style.color==1&&ch.index==1);
 CHECK(reader.next(ch)&&ch.codepoint=='c'&&ch.style.italic&&!ch.style.color&&ch.index==2);
 CHECK(reader.next(ch)&&ch.codepoint=='d'&&!ch.style.italic&&!ch.style.color&&ch.index==3);CHECK(!reader.next(ch));
 for(const char*text:{"[lb text]","[/lb]","[rb", "[code x]a[/code]","[code=foo]a[/code]","[code","[/code]"})CHECK(rich_plain(text)==text);
 CHECK(rich_plain("[code]a[code bad]b[/code]c[/code]d")=="a[code bad]b[/code]cd");
 std::string deep;for(unsigned i=0;i<17;++i)deep+="[code]";deep+='x';for(unsigned i=0;i<17;++i)deep+="[/code]";
 CHECK(rich_plain(deep.c_str())=="[code]x[/code]");
}
static void rainbow(){
 RichReader reader("x[sine][rainbow freq=0.3 sat=0.8 val=1]a[gold]b[/gold][rainbow speed=-1 sat=0]c[/rainbow]d[/rainbow][/sine]z");RichChar ch{};
 CHECK(reader.next(ch)&&ch.codepoint=='x'&&!ch.style.rainbow);
 CHECK(reader.next(ch)&&ch.codepoint=='a'&&ch.style.rainbow&&ch.style.wave);
 CHECK(ch.style.rainbow_parameters.frequency==300&&ch.style.rainbow_parameters.saturation==800&&ch.style.rainbow_parameters.value==1000&&ch.style.rainbow_parameters.speed==1000);
 CHECK(reader.next(ch)&&ch.codepoint=='b'&&ch.style.rainbow&&ch.style.color==1&&ch.style.rainbow_parameters.frequency==300);
 CHECK(reader.next(ch)&&ch.codepoint=='c'&&ch.style.rainbow_parameters.frequency==1000&&ch.style.rainbow_parameters.saturation==0&&ch.style.rainbow_parameters.value==800&&ch.style.rainbow_parameters.speed==-1000);
 CHECK(reader.next(ch)&&ch.codepoint=='d'&&ch.style.rainbow_parameters.frequency==300&&ch.style.rainbow_parameters.saturation==800&&ch.style.rainbow_parameters.value==1000);
 CHECK(reader.next(ch)&&ch.codepoint=='z'&&!ch.style.rainbow&&!ch.style.wave);CHECK(!reader.next(ch));
 RichReader defaults("[rainbow]x[/rainbow]");CHECK(defaults.next(ch)&&ch.style.rainbow&&ch.style.rainbow_parameters.frequency==1000&&ch.style.rainbow_parameters.saturation==800&&ch.style.rainbow_parameters.value==800&&ch.style.rainbow_parameters.speed==1000);
 RichReader spaced("[rainbow\tval=1.000   freq=+.300 sat=.800 speed=-10 ]x[/rainbow]");CHECK(spaced.next(ch)&&ch.codepoint=='x'&&ch.style.rainbow&&ch.style.rainbow_parameters.frequency==300&&ch.style.rainbow_parameters.speed==-10000);CHECK(!spaced.next(ch));
 const char*invalid[]={"[rainbow unknown=1]x[/rainbow]","[rainbow freq=0.3 unknown=1]x[/rainbow]","[rainbow freq=1 freq=2]x[/rainbow]","[rainbow freq=]x[/rainbow]","[rainbow freq=1e2]x[/rainbow]","[rainbow freq=nan]x[/rainbow]","[rainbow freq=10.001]x[/rainbow]","[rainbow sat=-0.001]x[/rainbow]","[rainbow val=1.001]x[/rainbow]","[rainbow val=0.1234]x[/rainbow]","[rainbow freq=1.2.3]x[/rainbow]","[rainbow speed=99999999999999999999]x[/rainbow]","[rainbow freq=0.3","[/rainbow]"};
 for(const char*text:invalid)CHECK(rich_plain(text)==text);
 const char*nested="[rainbow freq=.3]a[rainbow future=1]b[/rainbow]c[/rainbow]z";
 CHECK(rich_plain(nested)=="a[rainbow future=1]b[/rainbow]cz");RichReader rejected(nested);while(rejected.next(ch))if(ch.codepoint=='b'||ch.codepoint=='c')CHECK(ch.style.rainbow&&ch.style.rainbow_parameters.frequency==300);else if(ch.codepoint=='z')CHECK(!ch.style.rainbow);
 std::string deep;for(unsigned i=0;i<17;i++)deep+="[rainbow]";deep+='x';for(unsigned i=0;i<17;i++)deep+="[/rainbow]";deep+='z';CHECK(rich_plain(deep.c_str())=="[rainbow]x[/rainbow]z");
 std::string too_long="[rainbow "+std::string(128,'x')+"]a[/rainbow]";CHECK(rich_plain(too_long.c_str())==too_long);
 std::string long_nested="[rainbow freq=.3]a"+too_long+"c[/rainbow]z";CHECK(rich_plain(long_nested.c_str())=="a"+too_long+"cz");RichReader long_scope(long_nested.c_str());while(long_scope.next(ch)){if(ch.codepoint=='c')CHECK(ch.style.rainbow&&ch.style.rainbow_parameters.frequency==300);if(ch.codepoint=='z')CHECK(!ch.style.rainbow);}
 RichRainbow parameters;CHECK(rich_rainbow_hue(parameters,500,0)==32768);CHECK(rich_rainbow_hue(parameters,0,25000)==32768);
 parameters.frequency=300;CHECK(rich_rainbow_hue(parameters,500,0)==9830);CHECK(rich_rainbow_hue(parameters,10000,0)==0);
 CHECK(rich_rainbow_hue(parameters,500,25000)==19660);parameters.speed=-1000;CHECK(rich_rainbow_hue(parameters,1000,50000)==0);CHECK(rich_rainbow_hue(parameters,3000,50000)==39321);
 parameters.frequency=-1000;CHECK(!rich_rainbow_hue(parameters,UINT32_MAX,INT32_MIN));parameters.frequency=0;CHECK(!rich_rainbow_hue(parameters,UINT32_MAX,INT32_MAX));
 parameters.frequency=10000;parameters.speed=-10000;CHECK(rich_rainbow_hue(parameters,UINT32_MAX,INT32_MIN)==15047);CHECK(rich_rainbow_hue(parameters,UINT32_MAX,INT32_MAX)==50502);
 // Independent Godot 4.5.1 okhsv_to_srgb results, rounded to RGB888.
 // This includes both original palette values; native math may differ by
 // one byte before the existing RGB565 output quantization.
 struct ColorOracle{unsigned hue,value,red,green,blue;};
 const ColorOracle colors[]={
  {0,800,201,66,119},{0,1000,255,86,152},{1,800,201,66,119},{1,1000,255,86,152},
  {4096,800,201,66,71},{4096,1000,255,86,92},{8192,800,201,101,52},{8192,1000,255,130,69},
  {11000,800,200,124,52},{11000,1000,255,159,68},{16384,800,199,163,53},{16384,1000,255,209,70},
  {20000,800,198,199,57},{20000,1000,254,255,75},{24576,800,119,199,63},{24576,1000,154,255,83},
  {28672,800,64,199,133},{28672,1000,84,255,171},{32768,800,59,199,177},{32768,1000,77,255,227},
  {36864,800,57,189,199},{36864,1000,75,242,255},{40000,800,56,169,200},{40000,1000,74,217,255},
  {44000,800,55,143,201},{44000,1000,73,182,255},{45000,800,55,134,201},{45000,1000,72,172,255},
  {45500,800,54,130,201},{45500,1000,71,166,255},{46000,800,54,125,201},{46000,1000,71,159,255},
  {47000,800,52,113,202},{47000,1000,68,144,255},{48000,800,43,91,203},{48000,1000,57,116,255},
  {49152,800,55,76,203},{49152,1000,72,98,255},{52000,800,96,73,203},{52000,1000,123,94,255},
  {54000,800,121,72,202},{54000,1000,154,92,255},{57344,800,165,72,201},{57344,1000,209,93,255},
  {61440,800,201,70,172},{61440,1000,255,91,219},{65535,800,201,66,119},{65535,1000,255,86,152},
  {5343,800,201,67,53},{5343,1000,255,87,70},{25928,800,72,199,65},{25928,1000,94,255,86},
  {48013,800,43,90,203},{48013,1000,57,116,255},{48068,800,41,88,203},{48068,1000,55,113,255},
  {48069,800,41,88,203},{48069,1000,55,113,255},{48070,800,30,78,203},{48070,1000,41,100,255},
  {59647,800,200,74,201},{59647,1000,255,96,255}};
 for(const auto&c:colors){auto got=rich_rainbow_sample(c.hue,800,c.value);CHECK(std::abs(int(got.red)-int(c.red))<=1&&std::abs(int(got.green)-int(c.green))<=1&&std::abs(int(got.blue)-int(c.blue))<=1);}
 for(unsigned h:{0u,48069u,65535u}){auto black=rich_rainbow_sample(h,1000,0),white=rich_rainbow_sample(h,0,1000),gray=rich_rainbow_sample(h,0,500);CHECK(!black.red&&!black.green&&!black.blue);CHECK(white.red==255&&white.green==255&&white.blue==255);CHECK(gray.red==gray.green&&gray.green==gray.blue&&gray.red>0&&gray.red<255);}
}
int main(){Context x;expect(x,"{N:diff()}","3");expect(x,"{IfUpgraded:show:up {N}|base}","base");x.upgraded=true;expect(x,"{IfUpgraded:show:up {N}|base}","up 3");
 expect(x,"{Catalog=7:diff()}","7");expect(x,"{Catalog=-2}","-2");expect(x,"{N=19:diff()}","3");
 expect(x,"{Catalog=2:plural:one|{} many}","2 many");expect(x,"{Catalog=x:diff()}","{Catalog=x:diff()}");
 {char text[128]{};LocalizedTemplate(text,sizeof text,resolve,&x,"ENERGY_UNIT","STAR_UNIT",true).format("{N:energyIcons()} / {N:starIcons()}");
  const auto expected=std::string("3 ENERGY_UNIT / ")+InlineStarText+InlineStarText+InlineStarText;CHECK(text==expected);}
 {char text[32]{};LocalizedTemplate(text,sizeof text,resolve,&x).format("{Catalog=0:starIcons()}");CHECK(!*text);
  LocalizedTemplate(text,sizeof text,resolve,&x).format("{Catalog=1:starIcons()}");CHECK(!std::strcmp(text,InlineStarText));
  LocalizedTemplate(text,sizeof text,resolve,&x).format("{WideRatio:starIcons()}");CHECK(!std::strcmp(text,InlineStarText));
  LocalizedTemplate clipped(text,8,resolve,&x);clipped.format("{WideMax:starIcons()}");CHECK(clipped.truncated()&&std::strlen(text)==6);
  RichReader read(text);RichChar ch{};unsigned count=0;while(read.next(ch)){CHECK(ch.codepoint==InlineStarCodepoint);++count;}CHECK(count==2);}
 expect(x,"{N:plural:one|{N} many}","3 many");x.value=1;expect(x,"{N:plural:one|{N} many}","one");
 expect(x,"{Type:choose(Attack|Skill|Power):A {N}|S|P|?}","A 1");x.type="Skill";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","S");x.type="Quest";expect(x,"{Type:choose(Attack|Skill|Power):A|S|P|?}","?");
 x.value=-2;expect(x,"{N:cond:<0?negative|positive}","negative");expect(x,"{N:abs()}","2");expect(x,"{Min:abs()}","2147483648");
 expect(x,"{Low}","-0.25");expect(x,"{Percent:percentMore()}","4");expect(x,"{Even:percentMore()}","2");expect(x,"{Percent:percentLess()}","-4");
 expect(x,"{WideMin}","-9223372036854775808");expect(x,"{WideMin:abs()}","9223372036854775808");expect(x,"{WideMax}","9223372036854775807");
 expect(x,"{WideScale}","0.9999");expect(x,"{WideRatio:percentMore()}","50");expect(x,"{WideMin:choose(-9223372036854775808|0):minimum|zero|other}","minimum");
 expect(x,"x{Empty}y","xy");expect(x,"{Missing:show:bad|also bad}","{Missing:show:bad|also bad}");expect(x,"{Missing}","{Missing}");expect(x,"{unclosed","{unclosed");
 for(unsigned size=0;size<16;size++){char b[24];std::memset(b,0x5a,sizeof b);LocalizedTemplate(b,size,resolve,&x).format("prefix{N:abs()}long suffix");CHECK((unsigned char)b[size]==0x5a);if(size)CHECK(std::memchr(b,0,size));}
 expect(x,"{WideMin:n()}","-9,223,372,036,854,775,808");expect(x,"{WideMax:n()}","9,223,372,036,854,775,807");expect(x,"{WideRatio:n()}","1");expect(x,"{Low:n()}","0");expect(x,"{Missing:n()}","{Missing:n()}");
 catalog_context();conditional_corpus();thinky_dots();source_font_sizes();brackets_and_code();rainbow();std::printf("localized templates: %u checks passed\n",checks);
}
