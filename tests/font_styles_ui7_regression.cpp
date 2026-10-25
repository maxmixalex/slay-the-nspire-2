// Exercise the actual private Canvas without adding a production test API.
#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"font styles:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t v,unsigned n=4){CHECK(at+n<=b.size());for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(v>>(i*8));}
static Bytes fixture(bool modern=true,bool styles=true,bool small=false,bool scene=false,bool readable_ancient=false){
 const unsigned ni=A_Count,ng=scene?8:modern&&styles?6:3,ns=modern?Ui7StringCount:LocaleStringCount;
 const unsigned nc=scene?PhobiaAnimationKinds:modern?Ui7AnimationKinds:MerchantEffectsAnimationKinds;
 const unsigned nu=scene?unsigned(HistoryUi8AuxiliaryCount):modern?unsigned(Ui7AuxiliaryCount):unsigned(Ui6AuxiliaryCount);
 const unsigned ga=40+ni*16,sa=ga+ng*16,ca=sa+ns*8,base=ca+nc*16+CardKinds*16+nu*4;
 const unsigned table=base+64,costs=table+16,descriptors=costs+((ni+1)&~1u);
 Bytes b(descriptors+16);put(b,0,0x3250534e);put(b,4,scene?HistoryUi8ContentVersion:modern?Ui7ContentVersion:Ui6ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,ni);put(b,20,ng);put(b,24,ns);put(b,28,nc);put(b,32,CardKinds);
 const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,descriptors,table,770,1,524288,617323,costs,ni};
 for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
 const unsigned image=unsigned(b.size());b.push_back(0);b.push_back(0);
 for(unsigned i=0;i<ni;++i){put(b,40+i*16,image);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 // Keep the cold glyph page separate from the directory, image header and
 // language marker read during open. A tiny adjacent fixture is already
 // warm before glyph() and cannot exercise the provider cache miss.
 b.resize((b.size()+ResourceReader::PageSize-1)&~size_t(ResourceReader::PageSize-1));
 const unsigned cp[]={63,65,66,65,65,65,65,65},size[]={8,8,8,72,136,200,0,0},advance[]={1,2,3,5,4,6,0,0};
 const unsigned scene_size[]={6,6,6,70,102,134,166,198},scene_advance[]={1,2,3,5,7,4,6,6};
 const Bytes alpha[]={{127},{255},{127},{255,0,255},{0,255},{0,255,255,0},{},{}};
 const Bytes scene_alpha[]={{127},{255},{127},{255,0,255},{255,0,0,255},{0,255},{0,255,255},{0,255,255,0}};
 for(unsigned i=0;i<ng;++i){const unsigned at=ga+i*16;put(b,at,cp[i]);put(b,at+4,unsigned(b.size()));put(b,at+8,small&&i==0?5:size[i],2);
  if(scene)put(b,at+8,scene_size[i]+(readable_ancient?4:0),2);const auto&mask=scene?scene_alpha[i]:alpha[i];
  b[at+10]=uint8_t(mask.size());b[at+11]=1;put(b,at+12,scene?scene_advance[i]:advance[i],2);b.insert(b.end(),mask.begin(),mask.end());}
 b.resize((b.size()+ResourceReader::PageSize-1)&~size_t(ResourceReader::PageSize-1));
 const unsigned empty=unsigned(b.size());b.push_back(0);const unsigned marker=unsigned(b.size());b.insert(b.end(),{'e','n','g',0});
 for(unsigned i=0;i<ns;++i)put(b,sa+i*8,empty);put(b,sa+LocaleMarkerString*8,marker);put(b,sa+LocaleMarkerString*8+4,3);
 for(unsigned i=0;i<nc;++i){put(b,ca+i*16+4,1,2);put(b,ca+i*16+6,33,2);}put(b,12,unsigned(b.size()));return b;
}
static void clear(){std::fill(std::begin(app.pixels),std::end(app.pixels),0);}
static uint16_t pixel(unsigned x,unsigned y){return app.pixels[y*320+x];}
static void direct(){
 auto bytes=fixture();CHECK(app.content.open(bytes.data(),bytes.size()));Canvas c{app};
 for(int requested:{5,6,7,9,11,14,17}){
  CHECK(app.content.font_size(requested)==8&&c.width("AB",requested)==5);
  clear();const auto selected=c.fitted("AB",10,10,40,14,requested,5,0xffff,false,true);
  CHECK(selected.size==8&&!selected.truncated&&pixel(10,10)==0xffff&&pixel(12,10)!=0);
 }
 CHECK(app.content.glyph_metrics('A',8).advance==2);
 CHECK(app.content.glyph_metrics('A',8,1).advance==5);
 CHECK(app.content.glyph_metrics('A',8,2).advance==4);
 CHECK(app.content.glyph_metrics('A',8,3).advance==6);
 CHECK(app.content.glyph_metrics('B',8,2).advance==3); // unbaked style uses same regular codepoint
 CHECK(c.width(u8"A\ufe0fB",8)==5&&c.width(u8"A\U000e0100B",8)==5);
 for(unsigned cp:{0xfe0fu,0xfe0eu,0x200du,0xe0100u}){
  CHECK(!app.content.glyph(cp,8).pixels&&app.content.glyph(cp,8).advance==0);
  CHECK(app.content.glyph_metrics(cp,8).w==0&&app.content.glyph_metrics(cp,8).advance==0);
 }
 RichReader invisible(u8"A\ufe0f\u200dB");RichChar visible{};
 CHECK(invisible.next(visible)&&visible.codepoint=='A'&&visible.index==0);
 CHECK(invisible.next(visible)&&visible.codepoint=='B'&&visible.index==1);CHECK(!invisible.next(visible));
 CHECK(app.content.glyph_metrics('Z',8,1).advance==1); // then ordinary ? fallback
 CHECK(c.width("A[b]A[/b][i]A[/i][b][i]A[/i][/b]",8)==17);
 CHECK(c.width("[b]A[/b]B",8)==8);
 clear();CHECK(c.text("[b]A[/b]B",10,10,8,0xffff)==18);
 CHECK(pixel(10,10)==0xffff&&pixel(12,10)==0xffff);
 CHECK(pixel(11,10)==0&&pixel(13,10)==0); // no displaced duplicate to fake bold
 CHECK(pixel(15,10)!=0&&pixel(14,10)==0&&pixel(16,10)==0);
 clear();CHECK(c.paragraph("[b]A[/b]B",10,20,6,3,0,8,0xffff)==2);
 CHECK(pixel(10,20)==0xffff&&pixel(12,20)==0xffff&&pixel(10,31)!=0);
 clear();c.centered("[b]A[/b]B",10,40,20,8,0xffff);
 CHECK(pixel(16,40)==0xffff&&pixel(18,40)==0xffff&&pixel(21,40)!=0);
 clear();auto plan=c.fitted("[b]A[/b]B",10,50,8,10,8,8,0xffff,false,true);
 CHECK(!plan.truncated&&plan.rows==1);CHECK(pixel(10,50)==0xffff&&pixel(15,50)!=0);
 auto legacy=fixture(false);CHECK(app.content.open(legacy.data(),legacy.size()));
 CHECK(app.content.glyph_metrics('A',8,1).advance==2);CHECK(c.width("[b]A[/b]B",8)==5);
 // An actual UI7 locale can intentionally supply only Regular. It still
 // resolves each requested style to the same codepoint, not to '?' and not
 // to a nonexistent high-bit descriptor beyond the resident directory.
 auto regular_only=fixture(true,false);CHECK(app.content.open(regular_only.data(),regular_only.size()));
 for(unsigned face=1;face<4;++face){CHECK(app.content.glyph_metrics('A',8,face).advance==2);CHECK(app.content.glyph_metrics('B',8,face).advance==3);}
 CHECK(c.width("[b]A[/b][i]B[/i]",8)==5);
 clear();c.text("[b]A[/b][i]B[/i]",10,60,8,0xffff);
 CHECK(pixel(10,60)==0xffff&&pixel(12,60)!=0&&pixel(11,60)==0&&pixel(13,60)==0);
 auto small=fixture(true,true,true);CHECK(app.content.open(small.data(),small.size()));
 CHECK(app.content.font_size(5)==5&&app.content.font_size(6)==5&&app.content.font_size(7)==8);
 CHECK(app.content.font_size(7,0,true)==5&&app.content.font_size(11)==8);
 CHECK(app.content.glyph('?',5).pixels); // Reopening invalidates cached size bands.
}
static void ancient_families(){
 auto bytes=fixture(true,true,false,true,true);CHECK(app.content.open(bytes.data(),bytes.size()));Canvas c{app};
 RichChar bold{'A',{},0},italic{'A',{},0};bold.style.bold=true;italic.style.italic=true;
 // Distinct masks and advances expose either a metrics-only or paint-only
 // fix. Other scenes must still use generic faces 1/2, not the ancient faces.
 CHECK(c.font_style(bold)==1&&c.font_style(italic)==2);
 CHECK(app.content.glyph_metrics('A',10,1).advance==5&&app.content.glyph_metrics('A',10,2).advance==4);
 CHECK(ancient_text_metric(c,bold,10).advance==7&&ancient_text_metric(c,italic,10).advance==6);
 const char*sample="[b]A[/b][i]A[/i]B";const uint16_t ink=rgb(255,246,226),shadow=0x0821,half_shadow=0x0020;
 CHECK(ancient_text_rows(c,sample,16)==1&&ancient_text_rows(c,sample,15)==2);
 clear();ancient_text(c,sample,10,10,16,1);
 // Original +1px, 64/255-opacity dark-ink shadows occupy the mask holes
 // directly to the right; those pixels are intentionally not the black clear.
 CHECK(pixel(10,10)==ink&&pixel(13,10)==ink&&pixel(11,10)==shadow&&pixel(12,10)==0);
 CHECK(pixel(18,10)==ink&&pixel(19,10)==ink&&pixel(17,10)==0&&pixel(20,10)==shadow);
 CHECK(pixel(23,10)!=0&&pixel(22,10)==0&&pixel(24,10)==half_shadow);
 clear();ancient_text(c,sample,10,20,12,2);
 CHECK(pixel(10,20)==ink&&pixel(13,20)==ink&&pixel(18,20)==0);
 CHECK(pixel(11,31)==ink&&pixel(12,31)==ink&&pixel(16,31)!=0);
 CHECK(pixel(10,31)==0&&pixel(13,31)==shadow&&pixel(17,31)==half_shadow&&app.content.healthy());
}
static void sparse_inspection_size(){
 for(unsigned sparse:{7u,9u}){
 auto bytes=fixture(true,true,false,true);
 // Only the scene-title alphabet has this source face. B is deliberately
 // absent there, but has its complete existing regular6px representation.
 put(bytes,40+A_Count*16+4*16+8,sparse|96,2);
 CHECK(app.content.open(bytes.data(),bytes.size()));Canvas c{app};c.font_face=5;
 CHECK(app.content.font_size(sparse,5,false,true)==int(sparse)&&app.content.font_size(sparse,5)==6);
 CHECK(app.content.font_size(sparse,5,true)==6&&app.content.font_size(8,5)==6);
 clear();auto ordinary=c.fitted("AB",10,10,8,14,8,5,White,false,true);
 CHECK(ordinary.size==6&&!ordinary.truncated&&pixel(10,10)!=0&&pixel(15,10)!=0);
 clear();c.sparse_scene_sizes=true;auto inspection=c.fitted("A",10,10,20,14,sparse,5,White,false,true);
 CHECK(inspection.size==int(sparse)&&!inspection.truncated&&pixel(10,10)==White&&pixel(13,10)==White);
 CHECK(app.content.healthy());
 }
}
struct Provider {
 const Bytes*bytes;unsigned calls=0;
 static bool read(void*context,uint32_t off,void*out,uint32_t count){auto&p=*static_cast<Provider*>(context);++p.calls;
  if(off>p.bytes->size()||count>p.bytes->size()-off)return false;std::memcpy(out,p.bytes->data()+off,count);return true;}
 ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr};}
};
static void provider(){
 auto bytes=fixture();Provider p{&bytes};CHECK(app.content.open(p.source()));const auto opened=p.calls;
 for(unsigned face=0;face<8;++face){CHECK(app.content.glyph_metrics(0xfe0f,8,face).advance==0);CHECK(app.content.glyph(0xfe0f,8,face).advance==0);}
 CHECK(p.calls==opened);
 for(unsigned face=0;face<8;++face)for(int requested:{5,7,9,11,14,20})CHECK(app.content.font_size(requested,face)==8);
 for(unsigned face=0;face<4;++face)CHECK(app.content.glyph_metrics('A',8,face).advance>0);
 CHECK(p.calls==opened);for(unsigned face=0;face<4;++face)CHECK(app.content.glyph('A',8,face).pixels);
 const auto warm=p.calls;CHECK(warm>opened);
 for(unsigned repeat=0;repeat<20;++repeat)for(unsigned face=0;face<4;++face)CHECK(app.content.glyph('A',8,face).pixels);
 CHECK(p.calls==warm&&app.content.healthy());
}
static void ancient_actual_oracle(Canvas&c){
 // The source dialogue's families are an independent oracle: ordinary Kreon,
 // explicit Kreon Bold and Bitter Medium Italic, with the declared 6px floor
 // for source font_size=22. Draw the same glyphs directly, bypassing both the
 // ancient selector and its layout callback, including a real wrapped case.
 const char*sample="A[b]A[/b][i][font_size=22]A[/font_size][/i]B";
 const unsigned cps[]={'A','A','A','B'},faces[]={0,5,6,0};
 for(int width:{121,11,7}){
  clear();int at=0,row=0;
  for(unsigned i=0;i<4;++i){const auto g=app.content.glyph(cps[i],6,faces[i]);CHECK(g.pixels&&g.advance);
   if(at&&at+std::max(int(g.advance),int(g.w))>width){at=0;++row;}
   Canvas shadow=c;shadow.opacity=64;shadow.character(cps[i],g,20+at+1,40+row*7,Ink,true);
   c.character(cps[i],g,20+at,40+row*7,rgb(255,246,226));at+=g.advance;
  }
  const std::vector<uint16_t> expected(app.pixels,app.pixels+ScreenW*ScreenH);
  CHECK(ancient_text_rows(c,sample,width)==row+1);
  clear();ancient_text(c,sample,20,40,width,row+1);
  CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
 }
}
static void character_information_actual(Canvas&c){
 using namespace character_select_layout;
 // Independently projected source VBox / NinePatch geometry. Text may change
 // its natural height, but may not move or stretch the background panel.
 CHECK(InfoX==31&&InfoY==79&&InfoWidth==110&&InfoHeight==72);
 CHECK(InfoPanelX==17&&InfoPanelY==78&&InfoPanelWidth==133&&InfoPanelHeight==77);
 CHECK(NameHeight==27&&StatsHeight==4&&RelicHeight==19);
 Canvas body=c;body.bold_face=5;Canvas name=c;name.font_face=4;Canvas bold=c;bold.font_face=5;
 for(unsigned character=0;character<CharacterKinds;++character){
  const auto&definition=character_definitions[character];char key[96]{},description[1024]{},relic_text[1024]{};
  TextBuffer b{key,sizeof key,0};b.text(definition.key);b.text(".description");app.localized_text("characters",key,description,sizeof description);
  const auto*relic=relic_definition(definition.starting_relic);CHECK(relic&&*description);
  const auto relic_id=RelicId(relic-relic_definitions);app.relic_description(relic_id,relic_text,sizeof relic_text);CHECK(*relic_text);
  const char*title=app.content.world_title("characters",definition.key);CHECK(*title);
  const int title_size=character_name_size(name,title);CHECK(title_size>=10&&title_size<=14&&app.content.font_size(title_size,4)==title_size);
  CHECK(character_name_width(name,title,title_size)<=InfoWidth);
  const auto body_plan=character_info_plan(body,description,InfoWidth),relic_plan=character_info_plan(body,relic_text,RelicTextWidth);
  CHECK(body_plan.size==5&&relic_plan.size==5&&!body_plan.truncated&&!relic_plan.truncated);
  const int body_height=body_plan.rows*body_plan.step+2,top=info_top(body_height),body_y=top+NameHeight+StatsHeight;
  CHECK(top>=InfoY&&body_y+body_height+RelicDescriptionY+relic_plan.rows*relic_plan.step+2<AscensionY);
  // Walk the actual render plans and require every non-whitespace codepoint,
  // including rich colored values, to be emitted rather than ellipsized.
  for(unsigned which=0;which<2;++which){const char*text=which?relic_text:description;const auto&plan=which?relic_plan:body_plan;const int width=which?RelicTextWidth:InfoWidth;
   unsigned expected=0,emitted=0;RichReader reader(text);RichChar ch{};while(reader.next(ch))if(ch.codepoint>32)++expected;
   auto metric=[&](const RichChar&value,int px){const auto g=app.content.glyph_metrics(value.codepoint,px,body.font_style(value));return fitted_text::Metric{g.advance,std::max(int(g.w),int(g.advance))};};
   fitted_text::visit(text,width,plan,metric,[&](const RichChar&value,int x,int row,int px){
    CHECK(value.codepoint!=0x2026&&row<plan.rows&&px==5);if(value.codepoint>32)++emitted;
    CHECK(x+metric(value,px).width<=width);
   });CHECK(expected==emitted);
  }
  const auto relic_title=character_info_plan(bold,app.relic_title(relic_id),RelicTextWidth);
  CHECK(relic_title.size==5&&relic_title.rows==1&&!relic_title.truncated);
  clear();character_info_name(name,title,InfoX,top+(NameHeight-title_size-3)/2,title_size);
  character_info_text(body,description,InfoX,body_y,InfoWidth,body_plan,rgb(255,246,226));
  character_info_text(body,relic_text,InfoX+RelicTextX,body_y+body_height+RelicDescriptionY,RelicTextWidth,relic_plan,rgb(255,246,226));
  CHECK(app.content.healthy());
 }
}
static void actual(const char*path,const char*capture){
 FILE*f=std::fopen(path,"rb");CHECK(f&&std::fseek(f,0,SEEK_END)==0);const long size=std::ftell(f);
 CHECK(size>=40&&static_cast<unsigned long>(size)<=MaxContentFile&&std::fseek(f,0,SEEK_SET)==0);
 Bytes bytes(static_cast<size_t>(size));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));
 Provider p{&bytes};CHECK(app.content.open(p.source())&&app.content.version()>=Ui7ContentVersion);Canvas c{app};
 if(app.content.version()>=HistoryUi8ContentVersion){ancient_actual_oracle(c);character_information_actual(c);}
 const char*sample=app.content.english()?"Upgrade one [gold]card[/gold] in your [gold]deck[/gold].\n[b]Strength[/b] [i]Dexterity[/i]\nA [b]B[/b] [i]C[/i]":
     "升级你牌组中的一张牌。\n[gold]升级[/gold]你[gold]牌组[/gold]中的一张牌。\n[b]力量[/b] [i]敏捷[/i]\nA [b]B[/b] [i]C[/i]";
 const auto draw=[&](){clear();c.paragraph(sample,16,30,288,8,0,12,0xffff);
  c.text("[b]AB[/b] [i]ABC[/i] ABC",16,160,8,0xffff);};
 draw();const unsigned cold=p.calls;const std::vector<uint16_t> first(app.pixels,app.pixels+ScreenW*ScreenH);
 for(unsigned i=0;i<64;++i){draw();CHECK(!std::memcmp(first.data(),app.pixels,sizeof app.pixels));}
 CHECK(p.calls==cold&&app.content.healthy());
 if(capture){FILE*out=std::fopen(capture,"wb");CHECK(out);std::fprintf(out,"P6\n320 240\n255\n");
  for(auto pixel:app.pixels){const uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,out)==3);}CHECK(!std::fclose(out));}
 std::printf("actual %s font draw: %u cold provider calls; 64 warm frames add 0 calls\n",app.content.english()?"eng":"zhs",cold);
}
}
int main(int argc,char**argv){direct();ancient_families();sparse_inspection_size();provider();if(argc>1)actual(argv[1],argc>2?argv[2]:nullptr);std::printf("font styles: %u checks\n",checks);return 0;}
