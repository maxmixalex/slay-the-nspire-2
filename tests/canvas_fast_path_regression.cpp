// Compile the production Canvas in this test TU. The spire_core archive's
// app.cpp object is not needed because its definitions are already present.
// This keeps the renderer private instead of adding a test-only public API.
#include "../src/app.cpp"
namespace spire {
#include "fixtures/small_material_legacy.inc"
}
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"canvas fast path:%d case=%u %s\n",__LINE__,cases,#x);std::exit(1);}}while(0)
struct Image {unsigned w,h,format;std::vector<uint16_t>colors;Bytes alpha;};
static std::vector<Image> images;
static void put(Bytes&b,size_t at,uint32_t v,unsigned n=4){CHECK(at+n<=b.size());for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(v>>(i*8));}
static void append(Bytes&b,unsigned v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(uint8_t(v>>(i*8)));}
static uint16_t color(unsigned n){return uint16_t((n*4793u+0x185fu)^(n*17u));}
static unsigned rounded_alpha(unsigned a,unsigned b){return (a*b+127u)/255u;}
static Bytes fixture(){
 // All pixels are generated test data. The v3 envelope avoids requiring an
 // unrelated full gameplay profile just to exercise production Content/image.
 constexpr unsigned count=66,strings=66,clips=11;
 constexpr unsigned string_at=40+count*16,clip_at=string_at+strings*8;
 Bytes b(clip_at+clips*16);put(b,0,0x3250534e);put(b,4,3);put(b,8,0x41cef1ea);
 put(b,16,count);put(b,24,strings);put(b,28,clips);
 const unsigned formats[]={0,0,1,2,3,4,1,0,4};
 constexpr unsigned generated=sizeof formats/sizeof formats[0];
 for(unsigned id=0;id<generated;++id){
  const unsigned w=id>=7?320:7,h=id>=7?240:5,format=formats[id];
  Image image{w,h,format,std::vector<uint16_t>(w*h),Bytes(w*h,255)};
  for(unsigned i=0;i<w*h;++i){image.colors[i]=color(i%256);if(format==1||format==3)image.alpha[i]=uint8_t((i*67)%256);}
  if(id==6)std::fill(image.alpha.begin(),image.alpha.end(),0);
  // On-disk offsets must be even. main() also opens this complete pack from
  // an odd host address, which makes every direct RGB565 view unaligned.
  if(b.size()&1u)b.push_back(0);
  const unsigned off=unsigned(b.size());
  if(format<=1){
   for(auto c:image.colors)append(b,c,2);
   if(format==1)b.insert(b.end(),image.alpha.begin(),image.alpha.end());
  }else if(format<=3){
   for(unsigned i=0;i<256;++i)append(b,color(i),2);
   for(unsigned i=0;i<w*h;++i)b.push_back(uint8_t(i));
   if(format==3)b.insert(b.end(),image.alpha.begin(),image.alpha.end());
  }else if(id==8){
   // Full opaque sparse rows model the original separate title backdrop.
   // Every seven-byte row prefix also exercises unaligned RGB565 loads.
   for(unsigned y=0;y<h;++y){
    append(b,1,2);append(b,0,2);append(b,w,2);b.push_back(0);
    for(unsigned x=0;x<w;++x)append(b,image.colors[y*w+x],2);
   }
  }else{
   std::fill(image.alpha.begin(),image.alpha.end(),0);
   for(unsigned y=0;y<h;++y){
    if(y==2){append(b,0,2);continue;}
    append(b,2,2);
    for(unsigned span=0;span<2;++span){const unsigned x=span?4:0,n=3;
     append(b,x,2);append(b,n,2);b.push_back(uint8_t(span));
     for(unsigned j=0;j<n;++j){const unsigned at=y*w+x+j;append(b,image.colors[at],2);
      image.alpha[at]=span?uint8_t((j*101+y*37)%256):255;
      if(span)b.push_back(image.alpha[at]);
     }
    }
   }
  }
  put(b,40+id*16,off);put(b,44+id*16,w,2);put(b,46+id*16,h,2);
  put(b,48+id*16,format);put(b,52+id*16,unsigned(b.size())-off);images.push_back(image);
 }
 for(unsigned id=generated;id<count;++id)for(unsigned i=0;i<16;++i)b[40+id*16+i]=b[40+i];
 const unsigned empty=unsigned(b.size());b.push_back(0);
 for(unsigned i=0;i<strings;++i)put(b,string_at+i*8,empty);
 for(unsigned i=0;i<clips;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;
 for(size_t i=40;i<b.size();++i)hash=(hash^b[i])*16777619u;put(b,36,hash);return b;
}
struct Surface {int w,h,pitch;};
struct Style {bool coverage=false;uint8_t opacity=255,brightness=255,red=255,green=255,blue=255;};
struct Placement {int x,y,w,h;};
static const Style styles[]={
 {},{true},{false,0},{false,1},{false,128},{false,254},
 {false,255,0},{false,255,1},{false,255,127},{false,255,254},
 {false,255,255,0},{false,255,255,254},{false,255,255,255,0},
 {false,255,255,255,254},{false,255,255,255,255,0},
 {false,255,255,255,255,254},{true,129,173,91,202,13},
 {false,201,154,39,107,219}
};
struct Pair {
 static constexpr unsigned guard=19;
 std::vector<uint16_t> actual,expected;
 Bytes actual_alpha,expected_alpha;
 Canvas got{app},oracle{app};
 Pair(Surface s,Style style):actual(guard+s.pitch*s.h+guard),expected(actual.size()),actual_alpha(actual.size()),expected_alpha(actual.size()){
  ++cases;
  for(unsigned i=0;i<actual.size();++i){actual[i]=expected[i]=color(i*11+97);
   static const uint8_t coverage[]={0,1,127,254,255,64,192};actual_alpha[i]=expected_alpha[i]=coverage[i%7];}
  got.pixels=actual.data()+guard;oracle.pixels=expected.data()+guard;
  got.pitch=oracle.pitch=s.pitch;got.clip_w=oracle.clip_w=s.w;got.clip_h=oracle.clip_h=s.h;
  if(style.coverage){got.coverage=actual_alpha.data()+guard;oracle.coverage=expected_alpha.data()+guard;}
  got.opacity=oracle.opacity=style.opacity;got.brightness=oracle.brightness=style.brightness;
  got.tint_red=oracle.tint_red=style.red;got.tint_green=oracle.tint_green=style.green;got.tint_blue=oracle.tint_blue=style.blue;
 }
 void compare(){
  for(unsigned i=0;i<actual.size();++i)if(actual[i]!=expected[i]){
   std::fprintf(stderr,"pixel index=%u got=%04x expected=%04x\n",i,actual[i],expected[i]);CHECK(false);}
  CHECK(actual==expected);CHECK(actual_alpha==expected_alpha);
 }
};
static void material_icons(){
 for(unsigned id=0;id<images.size();++id)for(const auto style:styles)
  for(const Placement p: {Placement{-9,-7,31,34},Placement{2,3,54,76},Placement{-24,5,135,24},Placement{1,1,0,5}})
   for(unsigned saturation:{0u,400u,1000u,1300u})for(unsigned value:{500u,1000u,1400u})for(bool source:{false,true}){
    Pair pair({57,43,61},style);
    small_material_icon(pair.got,int(id),p.x,p.y,p.w,p.h,saturation,value,source);
    legacy_small_material_icon(pair.oracle,int(id),p.x,p.y,p.w,p.h,saturation,value,source);
    pair.compare();
   }
}
static void rect_case(Surface surface,Style style,Placement p,uint16_t value){
 Pair pair(surface,style);pair.got.rect(p.x,p.y,p.w,p.h,value);
 // Independent containment determines the changed rectangle; paint remains
 // the existing color/alpha/coverage oracle for the optimized write path.
 for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x)
  if(x>=p.x&&x<p.x+p.w&&y>=p.y&&y<p.y+p.h)pair.oracle.paint(y*surface.pitch+x,value);
 pair.compare();
}
static void dim_backstops(){
 static const int16_t matrix[]={1731,213,-57,81,1219,37,-21,97,1103};
 const Surface surface{31,27,39};
 for(auto style:styles)
 for(Placement p:{Placement{0,0,31,27},Placement{-3,-5,34,32},Placement{2,3,16,16},
                   Placement{2,3,15,17},Placement{7,9,6,5},Placement{29,25,7,7},
                   Placement{-20,3,9,7},Placement{0,0,0,27},Placement{0,0,31,-1}})
 for(unsigned alpha:{0u,1u,17u,127u,128u,229u,254u,255u})for(unsigned layer=0;layer<3;++layer){
  Pair pair(surface,style);Bytes occlusion(pair.actual.size());
  if(layer==1)pair.got.color_matrix=pair.oracle.color_matrix=matrix;
  if(layer==2){for(unsigned i=0;i<occlusion.size();++i)occlusion[i]=i%3?254:255;
   pair.got.occlusion=pair.oracle.occlusion=occlusion.data()+Pair::guard;}
  pair.got.dim(p.x,p.y,p.w,p.h,alpha);
  for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x)
   if(x>=p.x&&x<p.x+p.w&&y>=p.y&&y<p.y+p.h)pair.oracle.paint(y*surface.pitch+x,Ink,alpha);
  pair.compare();
 }
 // All 65,536 background colors and all call alpha values cross the table.
 // Independent channel arithmetic, rather than the optimized blend helper,
 // proves that no rounding edge changes during a modal fade.
 for(unsigned alpha=0;alpha<256;++alpha){
  Pair pair({256,256,256},{});
  for(unsigned i=0;i<65536;++i){
   pair.got.pixels[i]=uint16_t(i);
   const auto channel=[&](unsigned shift,unsigned mask){
    return ((((i>>shift)&mask)*(255-alpha)+((Ink>>shift)&mask)*alpha+127)/255)<<shift;};
   pair.oracle.pixels[i]=uint16_t(channel(11,31)|channel(5,63)|channel(0,31));
  }
  pair.got.dim(0,0,256,256,alpha);pair.compare();
 }
 // Foreground collection cadence is retained even for empty/transparent
 // rectangles. The renderer must never turn sampling into an input dispatch.
 static unsigned polls;const auto previous=app.callbacks.foreground_poll;
 app.callbacks.foreground_poll=[](){++polls;};
 for(Placement p:{Placement{0,0,320,240},Placement{-5,-3,25,39},Placement{0,0,0,240},Placement{0,250,320,40}})
 for(unsigned alpha:{0u,1u,255u}){
  Pair pair({320,240,320},{});polls=0;pair.got.dim(p.x,p.y,p.w,p.h,alpha);
  const int rows=std::max(0,std::min(240,p.y+p.h)-std::max(0,p.y));CHECK(polls==unsigned((rows+15)/16));
 }
 app.callbacks.foreground_poll=previous;
 // A transparent draw, including opacity rounded down to zero, never reads
 // an inactive material matrix. paint() makes the same early alpha decision.
 for(unsigned alpha:{0u,1u,255u})for(unsigned opacity:{0u,1u}){
  if(rounded_alpha(alpha,opacity))continue;
  Pair pair({320,240,320},{});pair.got.opacity=uint8_t(opacity);
  pair.got.color_matrix=reinterpret_cast<const int16_t*>(uintptr_t(1));
  pair.got.dim(0,0,320,240,alpha);pair.compare();
 }
}
static void picture_case(Surface surface,Style style,unsigned id,Placement p,unsigned opacity,unsigned mode,bool masked=false){
 Pair pair(surface,style);Bytes mask(pair.actual.size());
 if(masked){static constexpr uint8_t values[]={0,127,254,255,255,1,255};
  for(unsigned i=0;i<mask.size();++i)mask[i]=values[(i+i/19)%7];
  pair.got.occlusion=pair.oracle.occlusion=mask.data()+Pair::guard;}
 pair.got.picture(int(id),p.x,p.y,p.w,p.h,opacity,mode);
 const auto&image=images[id];const int width=p.w?p.w:int(image.w),height=p.h?p.h:int(image.h);
 if(width>0&&height>0&&opacity){
  for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
   const int local_x=x-p.x,local_y=y-p.y;if(local_x<0||local_x>=width||local_y<0||local_y>=height)continue;
   const unsigned sx=unsigned(local_x)*image.w/unsigned(width),sy=unsigned(local_y)*image.h/unsigned(height),at=sy*image.w+sx;
   pair.oracle.paint(y*surface.pitch+x,image.colors[at],rounded_alpha(image.alpha[at],opacity),mode);
  }
 }
 pair.compare();
}
static void masked_pictures(){
 for(Surface surface:{Surface{31,27,39},Surface{320,240,320}})
 for(auto style:{styles[0],styles[1],styles[4],styles[16]})for(unsigned id=0;id<9;++id)
 for(Placement p:{Placement{0,0,0,0},Placement{-3,-2,0,0},Placement{3,5,23,19},Placement{-5,-3,41,33}})
 for(unsigned alpha:{127u,255u})for(unsigned mode:{0u,unsigned(presentation::Add)})
  picture_case(surface,style,id,p,alpha,mode,true);
}
static void nine_panels(){
 const Style panel_styles[]={styles[0],styles[1],styles[2],styles[3],styles[4],styles[6],styles[16],styles[17],
  {false,255,255,0,0,0},{false,128,255,0,0,0},{false,1,255,0,0,0},{false,17,255,0,0,0},
  {false,201,0,91,173,39},{true,128,255,0,0,0}};
 const Placement positions[]={{0,0,23,19},{-3,-2,29,23},{3,2,11,9},{21,17,7,5},
  {-30,-20,7,5},{0,0,1,1},{0,0,0,5},{0,0,7,-1}};
 static const int16_t matrix[]={1731,213,-57,81,1219,37,-21,97,1103};
 for(auto style:panel_styles)for(unsigned id=0;id<7;++id)for(auto position:positions)
 for(unsigned opacity:{0u,1u,17u,127u,128u,254u,255u})
 for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add),2u})
 for(unsigned layer=0;layer<3;++layer){
  const Surface surface{23,19,29};Pair pair(surface,style);Bytes occlusion(pair.actual.size());
  if(layer==1){pair.got.color_matrix=pair.oracle.color_matrix=matrix;}
  if(layer==2){for(unsigned i=0;i<occlusion.size();++i)occlusion[i]=i%3?254:255;
   pair.got.occlusion=pair.oracle.occlusion=occlusion.data()+Pair::guard;}
  const auto&image=images[id];
  const bool valid=position.w>0&&position.h>0&&opacity&&image.format<=3;
  CHECK(pair.got.nine_image(int(id),position.x,position.y,position.w,position.h,2,1,3,2,opacity,mode)==valid);
  if(valid)for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
   const int dx=x-position.x,dy=y-position.y;
   if(dx<0||dx>=position.w||dy<0||dy>=position.h)continue;
   const unsigned sx=nine_patch::coordinate(dx,position.w,int(image.w),2,3);
   const unsigned sy=nine_patch::coordinate(dy,position.h,int(image.h),1,2),at=sy*image.w+sx;
   // The old path applies source/call alpha first, then Canvas::paint applies
   // the layer alpha. Keep both quantizations in the independent oracle.
   pair.oracle.paint(y*surface.pitch+x,image.colors[at],rounded_alpha(image.alpha[at],opacity),mode);
  }
  pair.compare();
 }
 // Every destination RGB565 color crosses the common opaque-source table.
 // These include both almost-transparent and almost-opaque panel shading.
 for(unsigned opacity:{1u,17u,127u,128u,254u}){
  Pair pair({256,256,256},{false,201,255,0,0,0});
  for(unsigned i=0;i<65536;++i)pair.got.pixels[i]=pair.oracle.pixels[i]=uint16_t(i);
  CHECK(pair.got.nine_image(0,0,0,256,256,2,1,3,2,opacity));
  for(unsigned i=0;i<65536;++i)pair.oracle.paint(int(i),0,opacity);
  pair.compare();
 }
 Pair missing({13,9,19},{});CHECK(!missing.got.nine_image(-1,0,0,7,5,2,1,3,2));missing.compare();
}
static void opaque_covers(){
 Canvas c{app};c.clip_w=23;c.clip_h=19;
 const Placement covers[]={{0,0,0,0},{-5,-3,31,25},{2,1,21,17},{3,2,4,3},{-30,-21,62,43}};
 const Placement targets[]={{0,0,1,1},{2,1,2,2},{3,3,7,5},{-3,-2,5,4},{17,14,12,9},{24,20,3,4},{-7,-6,2,2}};
 for(unsigned id=0;id<images.size();++id){
  const auto p=app.content.image(int(id));const auto&im=images[id];
  const auto size=app.content.image_dimensions(int(id));CHECK(size.w==im.w&&size.h==im.h);
  for(auto cover:covers)for(auto target:targets)for(unsigned opacity:{1u,254u,255u}){
   c.opacity=uint8_t(opacity);const int w=cover.w?cover.w:int(im.w),h=cover.h?cover.h:int(im.h);
   bool expected=opacity==255&&im.format<4;
   if(expected)for(int y=0;y<c.clip_h;++y)for(int x=0;x<c.clip_w;++x){
    if(x<target.x||x>=target.x+target.w||y<target.y||y>=target.y+target.h)continue;
    const int dx=x-cover.x,dy=y-cover.y;
    if(dx<0||dy<0||dx>=w||dy>=h){expected=false;continue;}
    if(im.alpha[unsigned(dy)*im.h/unsigned(h)*im.w+unsigned(dx)*im.w/unsigned(w)]!=255)expected=false;
   }
   CHECK(c.opaque_picture_covers(p,cover.x,cover.y,cover.w,cover.h,target.x,target.y,target.w,target.h)==expected);
  }
 }
 CHECK(!app.content.image_dimensions(-1).w&&!app.content.image_dimensions(100000).h);
 // The alpha=254 edge must not hide an underlying pixel, even though its
 // projected control rectangle fully contains the smaller target rectangle.
 uint8_t colors[8]{},alpha[]={255,255,255,254};const Picture edge{colors,alpha,nullptr,2,2,1};
 c.opacity=255;CHECK(c.opaque_picture_covers(edge,0,0,8,8,0,0,4,8));
 CHECK(!c.opaque_picture_covers(edge,0,0,8,8,0,0,8,8));
}
static Bytes glyph_fixture(const Bytes&encoded){
 constexpr unsigned count=66,strings=66,clips=11,cards=26,ui=56;
 constexpr unsigned glyph_at=40+count*16,string_at=glyph_at+16,clip_at=string_at+strings*8;
 constexpr unsigned card_at=clip_at+clips*16,payload=card_at+cards*16+ui*4;
 Bytes b(payload+3);put(b,0,0x3250534e);put(b,4,5);put(b,8,0x41cef1ea);
 put(b,16,count);put(b,20,1);put(b,24,strings);put(b,28,clips);put(b,32,cards);
 for(unsigned i=0;i<count;++i){put(b,40+i*16,payload);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 for(unsigned i=0;i<strings;++i)put(b,string_at+i*8,payload+2);
 for(unsigned i=0;i<clips;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
 put(b,glyph_at,'?');put(b,glyph_at+4,unsigned(b.size()));put(b,glyph_at+8,10,2);
 put(b,glyph_at+10,7,1);put(b,glyph_at+11,5,1);put(b,glyph_at+12,8,2);put(b,glyph_at+14,unsigned(encoded.size()),2);
 b.insert(b.end(),encoded.begin(),encoded.end());put(b,12,unsigned(b.size()));return b;
}
static void glyphs(){
 struct Case {Bytes encoded,alpha;};
 const Case samples[]={
  {{35,179},Bytes(35,179)},{{255,179},Bytes(35,179)},
  {{0,255,35,179},Bytes(35,179)},{{3,100},Bytes(35,0)},
  {{3,100,255,200},Bytes(35,200)}
 };
 for(unsigned i=0;i<sizeof samples/sizeof samples[0];++i){
  const auto&sample=samples[i];Bytes expected=sample.alpha;
  if(i>=3)std::fill(expected.begin(),expected.begin()+3,100);
  static const int16_t matrix[]={1731,213,-57,81,1219,37,-21,97,1103};
  for(Surface surface:{Surface{13,9,19},Surface{1,1,7}})
  for(auto style:{styles[0],styles[2],styles[3],styles[4],styles[16],styles[17]})
  for(Placement p:{Placement{2,2,0,0},Placement{-3,-2,0,0}})
  for(uint16_t ink:{uint16_t(0),uint16_t(0x52b4),uint16_t(0xffff)})for(unsigned layer=0;layer<3;++layer)
  for(bool raw:{false,true}){
   Pair pair(surface,style);Bytes mask(pair.actual.size());
   if(layer==1)pair.got.color_matrix=pair.oracle.color_matrix=matrix;
   if(layer==2){for(unsigned k=0;k<mask.size();++k)mask[k]=k%3?254:255;
    pair.got.occlusion=pair.oracle.occlusion=mask.data()+Pair::guard;}
   const Glyph glyph{raw?expected.data():sample.encoded.data(),7,5,8,uint16_t(raw?0:sample.encoded.size())};
   pair.got.glyph(glyph,p.x,p.y,ink);
   for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
    const int sx=x-p.x,sy=y-p.y;if(sx>=0&&sx<7&&sy>=0&&sy<5)pair.oracle.paint(y*surface.pitch+x,ink,expected[sy*7+sx]);
   }
   pair.compare();
  }
  const auto data=glyph_fixture(sample.encoded);CHECK(app.content.open(data.data(),data.size()));
  for(unsigned scale:{2048u,4096u,8192u}){
   Pair pair({20,15,23},{});turn_banner_text(pair.got,"?",10,2,scale,10,0xffff);
   const int left=10-int(8*scale/4096)/2;
   for(unsigned sy=0;sy<5;++sy)for(unsigned sx=0;sx<7;++sx)
    for(int y=2+int(sy*scale/4096);y<2+int((sy+1)*scale/4096);++y)
    for(int x=left+int(sx*scale/4096);x<left+int((sx+1)*scale/4096);++x)
     if(x>=0&&x<20&&y>=0&&y<15)pair.oracle.paint(y*23+x,0xffff,expected[sy*7+sx]);
   pair.compare();CHECK(app.content.healthy());
  }
 }
 // A caller-provided partial pair and empty logical extent draw nothing.
 const uint8_t partial[]={35};for(Glyph glyph:{Glyph{partial,7,5,8,1},Glyph{partial,0,5,8,1},Glyph{partial,7,0,8,1}}){
  Pair pair({13,9,19},{});pair.got.glyph(glyph,0,0,0xffff);pair.compare();
 }
}
static void rectangles(){
 const Surface surfaces[]={{13,9,13},{13,9,19},{1,1,7}};
 const Placement shapes[]={{0,0,13,9},{3,2,5,4},{-3,-2,8,6},{10,7,9,8},{-5,-4,24,19},
  {-20,1,5,3},{15,1,6,4},{1,-20,4,6},{1,12,4,6},{0,0,0,3},{0,0,3,0},{3,2,-1,4},{3,2,4,-1}};
 for(auto surface:surfaces)for(auto style:styles)for(auto shape:shapes)for(uint16_t value:{uint16_t(0),uint16_t(0xffff),uint16_t(0x29af)})rect_case(surface,style,shape,value);
 // Full-frame clears and deliberately padded destination rows.
 for(Surface surface: {Surface{320,240,320},Surface{320,240,327}}){
  rect_case(surface,{}, {0,0,320,240},0x185f);
  rect_case(surface,{true}, {-5,-3,330,246},0xffff);
 }
}
static void pictures(){
 const Surface surfaces[]={{13,9,13},{13,9,19},{1,1,7}};
 const Placement shapes[]={{0,0,0,0},{3,2,0,0},{-3,-2,0,0},{10,7,0,0},{13,9,0,0},{-10,-8,0,0},
  {0,0,14,10},{0,0,3,2},{-3,3,14,10},{4,-2,8,0},{0,0,-1,5},{0,0,7,-1}};
 for(auto surface:surfaces)for(auto style:styles)for(unsigned id=0;id<images.size();++id)for(auto shape:shapes)
  for(unsigned opacity:{0u,1u,128u,254u,255u})for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add),2u})
   picture_case(surface,style,id,shape,opacity,mode);
 for(Surface surface:{Surface{320,240,320},Surface{320,240,327}})for(unsigned id:{7u,8u}){
  picture_case(surface,{},id,{0,0,0,0},255,presentation::Normal);
  picture_case(surface,{},id,{-4,-3,0,0},255,presentation::Normal);
  picture_case(surface,{true},id,{0,0,0,0},255,presentation::Normal);
  picture_case(surface,{},id,{0,0,0,0},255,presentation::Add);
 }
 Pair missing({13,9,19},{});missing.got.picture(-1,0,0);missing.compare();
}
static void picture_regions(){
 // The title packs one text layer and ten flame crops into a single image;
 // its right selection reticle mirrors the left image. Exercise both paths
 // against an independent destination-to-source mapping for every encoding.
 const Surface surface{13,9,19};
 const Placement crops[]={{0,0,7,5},{1,1,4,3},{-2,-1,6,5},{5,4,7,5}};
 const Placement positions[]={{2,1,0,0},{-2,-1,0,0},{10,7,0,0}};
 for(unsigned id:{0u,2u,3u,4u,5u,6u})for(auto crop:crops)for(auto position:positions)
 for(bool mirror:{false,true})for(auto style:{styles[0],styles[1],styles[16]})
 for(unsigned opacity:{123u,255u})for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add)}){
  Pair pair(surface,style);const auto&image=images[id];
  pair.got.picture_region(app.content.image(int(id)),position.x,position.y,crop.x,crop.y,crop.w,crop.h,mirror,opacity,mode);
  for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
   const int dx=x-position.x,dy=y-position.y;if(dx<0||dy<0||dx>=crop.w||dy>=crop.h)continue;
   const int sx=crop.x+(mirror?crop.w-1-dx:dx),sy=crop.y+dy;
   if(sx<0||sy<0||sx>=int(image.w)||sy>=int(image.h))continue;
   const unsigned at=unsigned(sy)*image.w+unsigned(sx);
   pair.oracle.paint(y*surface.pitch+x,image.colors[at],rounded_alpha(image.alpha[at],opacity),mode);
  }
  pair.compare();
 }
}
static void picture_row_views(){
 // Full-width bands must preserve their source pitch, palette, alpha and
 // sparse row coordinates when the existing renderer scales them.
 const Surface surface{13,9,19};
 for(unsigned id:{0u,2u,3u,4u,5u,6u})for(int first:{-1,0,1,4,5})for(int count:{0,1,3,9})
 for(Placement target:{Placement{2,1,0,0},Placement{-2,-1,14,8},Placement{9,6,3,2}})
 for(auto style:{styles[0],styles[1],styles[16]})for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add)}){
  Pair pair(surface,style);const auto&image=images[id];
  pair.got.picture(Canvas::picture_rows(app.content.image(int(id)),first,count),target.x,target.y,target.w,target.h,123,mode);
  if(first>=0&&first<int(image.h)&&count>0){
   const int rows=std::min(count,int(image.h)-first),width=target.w?target.w:int(image.w),height=target.h?target.h:rows;
   for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
    const int dx=x-target.x,dy=y-target.y;if(dx<0||dx>=width||dy<0||dy>=height)continue;
    const unsigned at=unsigned(first+dy*rows/height)*image.w+unsigned(dx)*image.w/unsigned(width);
    pair.oracle.paint(y*surface.pitch+x,image.colors[at],rounded_alpha(image.alpha[at],123),mode);
   }
  }
  pair.compare();
 }
}
static void fake_merchant_atlas(){
 // Original Regent blend order: Normal outside the eye union, then its
 // BeforeNormal / straight-alpha Add / AfterNormal planes. The eye must
 // remain between the source arms; body alpha238 must not be squared.
 constexpr unsigned w=160,h=196,n=w*h;
 CHECK(FakeMerchantRegentAtlasWidth==w&&FakeMerchantRegentAtlasHeight==h);
 std::vector<uint16_t> colors[2]={std::vector<uint16_t>(n),std::vector<uint16_t>(n)};
 Bytes alpha[2]={Bytes(n),Bytes(n)};
 const auto put_pixel=[&](unsigned plane,unsigned x,unsigned y,uint16_t color,unsigned a){
  colors[plane][y*w+x]=color;alpha[plane][y*w+x]=uint8_t(a);
 };
 for(unsigned y=30;y<151;++y)for(unsigned x=40;x<124;++x)
  if(x<92||x>=100||y<77||y>=83)put_pixel((x+y)&1,x,y,0xe984,238);
 for(unsigned y=0;y<6;++y)for(unsigned x=0;x<8;++x){
  put_pixel((x+y)&1,x,190+y,0xe984,238);
  put_pixel((x+y+1)&1,8+x,190+y,0xff0f,128);
  put_pixel((x+y)&1,16+x,190+y,0x18ff,x%3==0?255:x%3==1?93:0);
 }
 const auto combine=[](uint16_t destination,uint16_t source,unsigned a,bool add){
  const auto channel=[&](unsigned shift,unsigned mask){
   const unsigned d=(destination>>shift)&mask,s=(source>>shift)&mask;
   return add?std::min(mask,d+(s*a+127)/255):(d*(255-a)+s*a+127)/255;
  };
  return uint16_t((channel(11,31)<<11)|(channel(5,63)<<5)|channel(0,31));
 };
 const uint16_t before=combine(0,0xe984,238,false),added=combine(before,0xff0f,128,true);
 CHECK(added!=combine(before,0xff0f,(128*128+127)/255,true));
 CHECK(combine(added,0x18ff,93,false)!=combine(combine(before,0x18ff,93,false),0xff0f,128,true));
 for(unsigned format:{1u,4u}){
  Bytes encoded[2];
  for(unsigned plane=0;plane<2;++plane){
   if(format==1){for(uint16_t color:colors[plane])append(encoded[plane],color,2);encoded[plane].insert(encoded[plane].end(),alpha[plane].begin(),alpha[plane].end());}
   else for(unsigned y=0;y<h;++y){
    const unsigned begin=unsigned(encoded[plane].size());append(encoded[plane],0,2);unsigned spans=0;
    for(unsigned x=0;x<w;){if(!alpha[plane][y*w+x]){++x;continue;}const unsigned left=x;
     while(x<w&&alpha[plane][y*w+x])++x;
     append(encoded[plane],left,2);append(encoded[plane],x-left,2);encoded[plane].push_back(1);++spans;
     for(unsigned col=left;col<x;++col){append(encoded[plane],colors[plane][y*w+col],2);encoded[plane].push_back(alpha[plane][y*w+col]);}
    }put(encoded[plane],begin,spans,2);
   }
  }
  for(uint16_t background:{uint16_t(0),uint16_t(0xffff),uint16_t(0x8410)})
  for(Placement position:{Placement{0,0,0,0},Placement{-95,-79,0,0},Placement{220,165,0,0}}){
   Pair pair({320,240,327},{});std::fill(pair.actual.begin(),pair.actual.end(),background);std::fill(pair.expected.begin(),pair.expected.end(),background);
   Bytes storage(std::max(encoded[0].size(),encoded[1].size()));unsigned fetches=0;
   // A single reused decode slot deliberately invalidates the previous view
   // on every fetch. The production helper must consume each view at once.
   fake_merchant::atlas(pair.got,[&](bool moving){CHECK(moving==bool(fetches++&1));
    const auto&source=encoded[moving];std::copy(source.begin(),source.end(),storage.begin());
    return Picture{storage.data(),format==1?storage.data()+n*2:nullptr,nullptr,w,h,uint8_t(format)};
   },position.x,position.y);
   CHECK(fetches==8);
   for(int y=0;y<240;++y)for(int x=0;x<320;++x){
    const int sx=x-position.x,sy=y-position.y;uint16_t value=background;
    if(sx>=40&&sx<124&&sy>=30&&sy<151&&(sx<92||sx>=100||sy<77||sy>=83))value=combine(value,0xe984,238,false);
    if(sx>=92&&sx<100&&sy>=77&&sy<83){
     value=combine(value,0xe984,238,false);value=combine(value,0xff0f,128,true);
     const unsigned a=(sx-92)%3==0?255:(sx-92)%3==1?93:0;value=combine(value,0x18ff,a,false);
    }
    pair.expected[Pair::guard+y*327+x]=value;
   }
   pair.compare();
  }
 }
}
static void title_particles(){
 // One shared two-column atlas must not bleed a neighboring fog frame or
 // star into its cropped destination. Destination sampling is independent
 // of the production full-width-band trick, for dense and sparse storage.
 constexpr unsigned width=270,height=216;
 Bytes rgb,alpha(width*height),sparse;
 for(unsigned y=0;y<height;++y){
  append(sparse,1,2);append(sparse,0,2);append(sparse,width,2);sparse.push_back(1);
  for(unsigned x=0;x<width;++x){
   const unsigned at=y*width+x;const auto pixel=color(at+37);
   alpha[at]=uint8_t((x*53+y*97)&255);append(rgb,pixel,2);append(sparse,pixel,2);sparse.push_back(alpha[at]);
  }
 }
 const Picture atlases[]={{rgb.data(),alpha.data(),nullptr,width,height,1},{sparse.data(),nullptr,nullptr,width,height,4}};
 struct Crop {int x,y,w,h;};
 const Crop crops[]={{0,0,135,54},{135,0,135,54},{0,54,135,54},{135,108,135,54},{0,162,135,54},{135,162,3,3}};
 const Surface surface{173,97,181};
 for(const auto atlas:atlases)for(auto crop:crops)
 for(Placement target:{Placement{11,5,101,40},Placement{-37,-11,134,53},Placement{149,75,112,44},Placement{8,9,1,1},Placement{-1,10,2,2}})
 for(auto style:{styles[0],styles[16]})for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add)}){
  Pair pair(surface,style);const title_scene::ParticleSample sample{target.x,target.y,target.w,target.h,74,90,99,112,0};
  title_particle(pair.got,Canvas::picture_rows(atlas,crop.y,crop.h),sample,crop.x,crop.w,mode);
  pair.oracle.tint_red=uint8_t(rounded_alpha(style.red,sample.red));
  pair.oracle.tint_green=uint8_t(rounded_alpha(style.green,sample.green));
  pair.oracle.tint_blue=uint8_t(rounded_alpha(style.blue,sample.blue));
  for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
   const int dx=x-target.x,dy=y-target.y;if(dx<0||dy<0||dx>=target.w||dy>=target.h)continue;
   const unsigned sx=unsigned(crop.x+dx*crop.w/target.w),sy=unsigned(crop.y+dy*crop.h/target.h),at=sy*width+sx;
   pair.oracle.paint(y*surface.pitch+x,color(at+37),rounded_alpha(alpha[at],sample.opacity),mode);
  }
  pair.compare();
 }
}
static void scaled_sparse_clipping(){
 // Fragmented 270-column atlas rows expose discarded source columns, empty
 // downsampled rows, repeated upsampled columns and clips inside a span.
 // The oracle samples the generated dense image directly at each destination.
 constexpr unsigned width=270,height=54;
 Bytes sparse,alpha(width*height,0);std::vector<uint16_t> pixels(width*height);
 for(unsigned y=0;y<height;++y){
  const auto count_at=sparse.size();append(sparse,0,2);unsigned spans=0;
  for(unsigned x=0;x<width;++x)pixels[y*width+x]=color(y*width+x+103);
  if(y%9)for(unsigned block=0;block<width;block+=19)for(unsigned mode=0;mode<2;++mode){
   const unsigned x=block+(mode?11:1);if(x>=width)continue;
   const unsigned n=std::min(mode?5u:6u,width-x);
   append(sparse,x,2);append(sparse,n,2);sparse.push_back(uint8_t(mode));++spans;
   for(unsigned j=0;j<n;++j){
    const unsigned at=y*width+x+j;alpha[at]=mode?uint8_t((x+j+y*37)*71):255;
    append(sparse,pixels[at],2);if(mode)sparse.push_back(alpha[at]);
   }
  }
  put(sparse,count_at,spans,2);
 }
 const Picture picture{sparse.data(),nullptr,nullptr,width,height,4};
 const Placement targets[]={{0,0,1,1},{-1,7,2,2},{6,-1,3,2},{-37,-11,133,53},
  {11,5,134,53},{149,75,135,54},{-137,9,269,53},{-137,-1,270,55},
  {-136,-2,271,55},{-267,-13,541,109},{169,-137,7,313}};
 const Surface surface{173,97,181};
 for(auto target:targets)for(auto style:{styles[0],styles[1],styles[16],styles[17]})
 for(unsigned opacity:{1u,123u,255u})for(unsigned mode:{unsigned(presentation::Normal),unsigned(presentation::Add)}){
  Pair pair(surface,style);pair.got.picture(picture,target.x,target.y,target.w,target.h,opacity,mode);
  for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
   const int dx=x-target.x,dy=y-target.y;if(dx<0||dy<0||dx>=target.w||dy>=target.h)continue;
   const unsigned sx=unsigned(dx)*width/unsigned(target.w),sy=unsigned(dy)*height/unsigned(target.h),at=sy*width+sx;
   pair.oracle.paint(y*surface.pitch+x,pixels[at],rounded_alpha(alpha[at],opacity),mode);
  }
  pair.compare();
 }
}
static void map_pictures(){
 // A separate direct inverse-transform oracle retains the pre-optimization
 // pixel-center arithmetic. Test all shaders, odd sizes, angles, clipping,
 // palette/alpha and Canvas modulation on the production map renderer.
 const Placement placements[]={{160,120,320,238},{5,25,7,5},{319,208,8,6},{160,120,3,2},
  {-4,48,7,5},{-20,-30,28,19},{320,210,14,10},{4,23,1,1},{160,120,0,5}};
 struct MapStyle{Style layer;MapShader shader;unsigned opacity;};
 const MapStyle map_styles[]={{styles[0],MapShader::Original,255},{styles[1],MapShader::Tint,255},
  {styles[4],MapShader::Normal,123},{styles[16],MapShader::Boss,255},{styles[0],MapShader::Tint,123},
  {styles[0],MapShader::Normal,255},{styles[0],MapShader::Boss,123},{styles[0],MapShader::Original,0}};
 for(unsigned id:{0u,2u,3u,4u,6u,7u})for(auto placement:placements)for(auto settings:map_styles)
 for(int angle:{0,360,-360,-179,-91,-17,17,63,90,179,180,270}){
  const auto style=settings.layer;const auto shader=settings.shader;const auto opacity=settings.opacity;
  Pair pair({320,240,327},style);const int cx=placement.x,cy=placement.y,w=placement.w,h=placement.h;
  const uint16_t ink=0x2945,paper=0xfeb6;const int clip_top=(id&1)?55:22,clip_bottom=(id&1)?170:207;
  map_picture(pair.got,int(id),cx,cy,w,h,shader,ink,paper,opacity,angle,clip_top,clip_bottom);
  const auto&im=images[id];
  if(w>0&&h>0&&opacity){
   const int sine=map_sin(angle),cosine=map_sin(angle+90),as=sine<0?-sine:sine,ac=cosine<0?-cosine:cosine;
   const int half_w=(w*ac+h*as+2047)/2048,half_h=(h*ac+w*as+2047)/2048;
   for(int y=std::max(clip_top,cy-half_h);y<std::min(clip_bottom,cy+half_h);++y)
   for(int x=std::max(0,cx-half_w);x<std::min(320,cx+half_w);++x){
    const int dx=2*(x-cx)+1,dy=2*(y-cy)+1,u=dx*cosine+dy*sine+w*1024,v=dy*cosine-dx*sine+h*1024;
    if(u<0||v<0||u>=w*2048||v>=h*2048)continue;
    const unsigned at=unsigned(v*int(im.h)/(h*2048))*im.w+unsigned(u*int(im.w)/(w*2048));
    pair.oracle.paint(y*pair.oracle.pitch+x,map_shade(im.colors[at],shader,ink,paper),rounded_alpha(im.alpha[at],opacity));
   }
  }
  pair.compare();
 }
}
}
static void blend_reference(){
 // Check all component values at every alpha, with differing opposite lanes.
 for(unsigned alpha=0;alpha<256;++alpha)for(unsigned left=0;left<64;++left)for(unsigned right=0;right<64;++right){
  const uint16_t a=uint16_t(((left&31)<<11)|(left<<5)|(31-(right&31)));
  const uint16_t b=uint16_t(((right&31)<<11)|(right<<5)|(31-(left&31)));
  const unsigned r=((a>>11)*(255-alpha)+(b>>11)*alpha+127)/255;
  const unsigned g=(((a>>5)&63)*(255-alpha)+((b>>5)&63)*alpha+127)/255;
  const unsigned blue=((a&31)*(255-alpha)+(b&31)*alpha+127)/255;
  CHECK(blend(a,b,alpha)==uint16_t((r<<11)|(g<<5)|blue));
 }
}
static void hidden_picture_io(const Bytes& data){
 struct Source {const Bytes* data;unsigned reads=0;
  static bool read(void*context,uint32_t offset,void*out,uint32_t count){
   auto&s=*static_cast<Source*>(context);++s.reads;
   if(offset>s.data->size()||count>s.data->size()-offset)return false;
   std::memcpy(out,s.data->data()+offset,count);return true;
  }
 } source{&data};
 CHECK(app.content.open(ResourceSource{&source,uint32_t(data.size()),Source::read,nullptr}));
 const unsigned opened=source.reads;uint16_t surface[320*240]{};Canvas c{app,surface};
 c.picture(0,0,-5);c.picture(0,-7,0);c.picture(0,320,0);c.picture(0,0,240);
 c.picture(8,0,-240);c.picture(8,-320,0);
 c.picture(1,0,-84,60,84);c.picture(2,0,0,0,0,0);c.opacity=0;c.picture(3,0,0);
 c.opacity=255;
 for(const auto clip:{Surface{0,0,320},Surface{0,240,320},Surface{320,0,320},Surface{-1,240,320}}){
  c.clip_w=clip.w;c.clip_h=clip.h;
  // Negative icon origins can straddle zero while the viewport has no area.
  // Pure glyph preparation must still avoid every image/resource callback.
  c.picture(7,-10,-10,60,84);c.nine_image(7,-10,-10,60,84,4,4,4,4);
  small_material_icon(c,7,-10,-10,60,84,400,700);
 }
 c.clip_w=320;c.clip_h=240;
 CHECK(source.reads==opened);CHECK(!app.content.image_cached(0)&&!app.content.image_cached(8));
 // The tiny first image shares the metadata reader's cached page. Image8
 // is beyond that prefix, so a visible row must really fetch its payload.
 c.opacity=255;c.picture(8,0,-239);CHECK(source.reads>opened);CHECK(app.content.image_cached(8));
 CHECK(app.content.healthy());
}
static Bytes fitted_glyph_fixture(){
 // Distinct generated glyphs occupy different reader pages. Measuring a
 // line uses metadata; fetching a hidden raster therefore has observable I/O.
 constexpr unsigned count=66,strings=66,clips=11,cards=26,ui=56,glyphs=13;
 constexpr unsigned glyph_at=40+count*16,string_at=glyph_at+glyphs*16,clip_at=string_at+strings*8;
 constexpr unsigned payload=clip_at+clips*16+cards*16+ui*4;
 Bytes b(8192+glyphs*4096);put(b,0,0x3250534e);put(b,4,5);put(b,8,0x41cef1ea);
 put(b,12,unsigned(b.size()));put(b,16,count);put(b,20,glyphs);put(b,24,strings);put(b,28,clips);put(b,32,cards);
 for(unsigned i=0;i<count;++i){put(b,40+i*16,payload);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 for(unsigned i=0;i<strings;++i)put(b,string_at+i*8,payload+2);
 for(unsigned i=0;i<clips;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
 for(unsigned i=0;i<glyphs;++i){const unsigned row=glyph_at+i*16,at=8192+i*4096;
  put(b,row,i?unsigned('A')+i-1:unsigned('?'));put(b,row+4,at);put(b,row+8,10,2);
  put(b,row+10,7,1);put(b,row+11,5,1);put(b,row+12,8,2);put(b,row+14,2,2);
  b[at]=35;b[at+1]=uint8_t(40+i*17);
 }
 return b;
}
static void fitted_clip_pixels(const Bytes& data){
 CHECK(app.content.open(data.data(),data.size()));
 struct Label {const char*text;int w,h;bool centered,single,outline,shadow,fly;};
 const Label labels[]={
  {"A\nB\nC\nD",20,44,false,false,false,false,false},
  {"A\nB\nC\nD",20,8,true,false,true,false,false},
  {"ABCDEFGHIJKL",22,13,false,true,false,true,false},
  {"ABCDEFGHIJKL",22,13,true,true,true,false,false},
  {"[sine][jitter]AB\nCD\nEF[/jitter][/sine]",25,44,true,false,true,false,false},
  {"[sine][jitter]AB\nCD\nEF[/jitter][/sine]",25,44,false,false,false,true,false},
  {"AB\nCD\nEF",25,44,true,false,true,false,true}
 };
 constexpr int width=17,height=13,pitch=19,pad=32,wide=192,tall=128;
 std::vector<uint16_t> small(pitch*height),large(wide*tall);
 const bool effects=app.text_effects;app.text_effects=true;
 for(const auto&label:labels)for(auto position:{Placement{-12,-13,0,0},Placement{0,0,0,0},Placement{13,0,0,0},Placement{0,11,0,0},Placement{-12,11,0,0}}){
  app.overflow_label_clocks.reset();app.rich_label_clocks.reset();
  Canvas clipped{app,small.data()},complete{app,large.data()};
  clipped.pitch=pitch;clipped.clip_w=width;clipped.clip_h=height;
  complete.pitch=wide;complete.clip_w=wide;complete.clip_h=tall;
  clipped.text_effect_identity=complete.text_effect_identity=0xf177ed;
  clipped.text_fly_in_elapsed=complete.text_fly_in_elapsed=label.fly?80:UINT32_MAX;
  for(unsigned age:{0u,1700u,2600u}){
   ++cases;app.map_clock_ms=100+age;app.animation=age/33+7;
   for(int y=0;y<tall;++y)for(int x=0;x<wide;++x)large[y*wide+x]=color(unsigned(x+y*wide));
   for(int y=0;y<height;++y)for(int x=0;x<pitch;++x)small[y*pitch+x]=large[(y+pad)*wide+x+pad];
   clipped.fitted(label.text,position.x,position.y,label.w,label.h,10,10,0xffff,label.centered,label.single,label.outline,label.shadow);
   complete.fitted(label.text,position.x+pad,position.y+pad,label.w,label.h,10,10,0xffff,label.centered,label.single,label.outline,label.shadow);
   // Rendering to a larger viewport and then cropping must give identical
   // pixels, including partial rows, moving glyphs and translated outlines.
   for(int y=0;y<height;++y)for(int x=0;x<pitch;++x)
    CHECK(small[y*pitch+x]==(x<width?large[(y+pad)*wide+x+pad]:color(unsigned(x+pad+(y+pad)*wide))));
  }
 }
 app.text_effects=effects;CHECK(app.content.healthy());
}
static void fitted_hidden_glyph_io(const Bytes&data){
 struct Source {const Bytes*data;unsigned mask=0;
  static bool read(void*context,uint32_t at,void*out,uint32_t count){
   auto&s=*static_cast<Source*>(context);if(at>s.data->size()||count>s.data->size()-at)return false;
   for(unsigned i=0;i<13;++i){const unsigned start=8192+i*4096;if(at<start+2&&at+count>start)s.mask|=1u<<i;}
   std::memcpy(out,s.data->data()+at,count);return true;
  }
 } source{&data};
 const auto cold=[&](){CHECK(app.content.open(ResourceSource{&source,uint32_t(data.size()),Source::read,nullptr}));
  source.mask=0;app.overflow_label_clocks.reset();app.map_clock_ms=100;};
 uint16_t surface[20*12]{};Canvas c{app,surface};c.pitch=20;c.clip_w=20;c.clip_h=6;
 cold();c.fitted("A\nB\nC\nD",0,0,20,6,10,10,0xffff);
 CHECK(source.mask==(1u<<1)); // Only the first row owns any visible pixels.
 source.mask=0;app.map_clock_ms+=2600;c.fitted("A\nB\nC\nD",0,0,20,6,10,10,0xffff);
 CHECK(source.mask==(1u<<2)); // Smooth twelve-pixel scroll reveals B, not C/D.
 cold();c.clip_w=7;c.clip_h=12;c.fitted("ABCD",0,0,7,12,10,10,0xffff,false,true);
 CHECK(source.mask==(1u<<1));
 source.mask=0;app.map_clock_ms+=2600;c.fitted("ABCD",0,0,7,12,10,10,0xffff,false,true);
 CHECK(source.mask==((1u<<3)|(1u<<4))); // Eighteen pixels: partial C and D.
 cold();c.clip_w=20;c.clip_h=6;c.text_fly_in_elapsed=1;
 c.fitted("A\nB\nC\nD",0,0,20,6,10,10,0xffff);
 CHECK(source.mask==((1u<<1)|(1u<<2)|(1u<<3)|(1u<<4))); // Rotated fly-in keeps the original path.
 CHECK(app.content.healthy());
}
static void character_name_outline_before_ui14(Canvas&c,Glyph glyph,int x,int y){
 // Keep the former fixed-stride algorithm as an independent pixel oracle.
 // In particular, it deliberately retains the dynamic division and all
 // transparent right-hand padding that the production renderer now avoids.
 constexpr int Radius=1,Stride=48,Height=40;
 if(!glyph.pixels||glyph.w+2*Radius>Stride||glyph.h+2*Radius>Height)return;
 uint8_t mask[Stride*Height]{};
 constexpr int span[3]={0,1,0};
 const auto pixel=[&](unsigned at,unsigned alpha){
  if(!alpha)return;const unsigned row=at/glyph.w,column=at-row*glyph.w;
  for(int dy=-Radius;dy<=Radius;++dy){const int reach=span[dy+Radius];
   auto*out=mask+(int(row)+Radius+dy)*Stride+int(column)+Radius;
   for(int dx=-reach;dx<=reach;++dx)out[dx]=uint8_t(mx(out[dx],alpha));
  }
 };
 const unsigned length=unsigned(glyph.w)*glyph.h;
 if(glyph.encoded_size){unsigned at=0;for(unsigned i=0;i+1<glyph.encoded_size&&at<length;i+=2){
  const unsigned stop=mn(length,at+glyph.pixels[i]);while(at<stop){pixel(at,glyph.pixels[i+1]);++at;}
 }}else for(unsigned at=0;at<length;++at)pixel(at,glyph.pixels[at]);
 Canvas shade=c;shade.opacity=uint8_t(div255(shade.opacity*64));
 Glyph outline{};outline.pixels=mask;outline.w=Stride;outline.h=uint8_t(glyph.h+2*Radius);
 shade.glyph(outline,x+glyph.bearing_x-Radius,y-Radius,rgb(0,0,0));
}
static void character_name_outlines(){
 const Surface surface{57,43,63};
 const Placement placements[]={{2,2,0,0},{-5,-7,0,0},{53,39,0,0},{-52,0,0,0},{60,0,0,0}};
 const auto verify=[&](Glyph glyph){
  for(auto style:{styles[0],styles[1],styles[2],styles[4],styles[16],styles[17]})
  for(auto position:placements)for(int bearing:{-3,2})for(bool masked:{false,true}){
   Pair pair(surface,style);Bytes occlusion(pair.actual.size());
   if(masked){for(unsigned i=0;i<occlusion.size();++i)occlusion[i]=i%3?254:255;
    pair.got.occlusion=pair.oracle.occlusion=occlusion.data()+Pair::guard;}
   glyph.bearing_x=int8_t(bearing);
   character_name_outline(pair.got,glyph,position.x,position.y);
   character_name_outline_before_ui14(pair.oracle,glyph,position.x,position.y);
   pair.compare();
  }
 };
 for(auto size:{Surface{1,1,0},Surface{1,13,0},Surface{7,5,0},Surface{17,23,0},Surface{46,38,0}}){
  Bytes raw(unsigned(size.w*size.h)),encoded{0,191};
  constexpr uint8_t alpha[]={0,1,127,254,255};
  for(unsigned i=0;i<raw.size();++i)raw[i]=alpha[(i/9+i/73+2)%5];
  // Runs cross glyph rows, including the one-pixel-wide case. A trailing
  // partial pair is ignored by the original decoder as well as production.
  for(unsigned at=0;at<raw.size();){unsigned n=1;
   while(at+n<raw.size()&&n<255&&raw[at+n]==raw[at])++n;
   encoded.push_back(uint8_t(n));encoded.push_back(raw[at]);at+=n;
  }
  encoded.push_back(17);
  verify(Glyph{raw.data(),uint8_t(size.w),uint8_t(size.h),uint16_t(size.w)});
  verify(Glyph{encoded.data(),uint8_t(size.w),uint8_t(size.h),uint16_t(size.w),uint16_t(encoded.size())});
 }
 for(const auto&encoded:{Bytes{0,255,3,100,50,255},Bytes{3,100,4},Bytes{0,255}})
  verify(Glyph{encoded.data(),7,5,8,uint16_t(encoded.size())});
 const uint8_t unused=255;
 for(Glyph glyph:{Glyph{},Glyph{&unused,0,5,8},Glyph{&unused,7,0,8},Glyph{&unused,47,5,8},Glyph{&unused,7,39,8}})
  verify(glyph);
}
static void covered_paint_contract(){
 uint16_t target=0;uint8_t coverage=0;Canvas canvas{app,&target,1,1,1,&coverage};
 // Every RGB565 channel pair and every source alpha cross an opaque cache
 // pixel. This oracle uses the former independent integer normalization.
 for(unsigned source=0;source<64;++source)for(unsigned destination=0;destination<64;++destination)
 for(unsigned alpha=0;alpha<256;++alpha){
  const uint16_t foreground=uint16_t(((source&31)<<11)|(source<<5)|((source*17)&31));
  const uint16_t background=uint16_t(((destination&31)<<11)|(destination<<5)|((destination*19)&31));
  const auto channel=[&](unsigned shift,unsigned mask){return ((((foreground>>shift)&mask)*alpha+((background>>shift)&mask)*(255-alpha)+127)/255)<<shift;};
  const uint16_t expected=uint16_t(channel(11,31)|channel(5,63)|channel(0,31));
  target=background;coverage=255;canvas.paint(0,foreground,alpha);
  CHECK(target==expected&&coverage==255);
 }
 // Partial coverage still normalizes by its accumulated alpha, including
 // zero-alpha no-op and an opaque source replacing translucent storage.
 for(unsigned previous=0;previous<255;++previous)for(unsigned alpha=0;alpha<256;++alpha){
  const uint16_t foreground=color(previous*73+alpha),background=color(previous+alpha*41);
  target=background;coverage=uint8_t(previous);canvas.paint(0,foreground,alpha);
  if(!alpha){CHECK(target==background&&coverage==previous);continue;}
  if(!previous||alpha==255){CHECK(target==foreground&&coverage==alpha);continue;}
  const unsigned back=(previous*(255-alpha)+127)/255,total=alpha+back;
  const auto channel=[&](unsigned shift,unsigned mask){return ((((foreground>>shift)&mask)*alpha+((background>>shift)&mask)*back+total/2)/total)<<shift;};
  CHECK(target==uint16_t(channel(11,31)|channel(5,63)|channel(0,31))&&coverage==total);
 }
}
int main(){
 blend_reference();covered_paint_contract();dim_backstops();
 const auto data=fixture();CHECK(app.content.open(data.data(),data.size()));
 CHECK((uintptr_t(app.content.image(0).pixels)&1u)==0u);
 rectangles();material_icons();pictures();masked_pictures();nine_panels();opaque_covers();picture_regions();picture_row_views();fake_merchant_atlas();title_particles();scaled_sparse_clipping();CHECK(app.content.healthy());
 Bytes unaligned(data.size()+1);std::copy(data.begin(),data.end(),unaligned.begin()+1);
 CHECK(app.content.open(unaligned.data()+1,data.size()));
 CHECK((uintptr_t(app.content.image(0).pixels)&1u)==1u);
 material_icons();pictures();nine_panels();picture_regions();picture_row_views();map_pictures();CHECK(app.content.healthy());glyphs();hidden_picture_io(data);
 const auto text=fitted_glyph_fixture();fitted_clip_pixels(text);fitted_hidden_glyph_io(text);character_name_outlines();
 std::printf("canvas fast path: %u cases, %u checks; production paint oracle, RGB565 alignment, clipping, pitch, coverage, modulation, additive, scaled, atlas regions, sparse, palette and glyph decode bounds\n",cases,checks);
}
