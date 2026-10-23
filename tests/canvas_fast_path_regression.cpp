// Compile the production Canvas in this test TU. The spire_core archive's
// app.cpp object is not needed because its definitions are already present.
// This keeps the renderer private instead of adding a test-only public API.
#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
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
static void rect_case(Surface surface,Style style,Placement p,uint16_t value){
 Pair pair(surface,style);pair.got.rect(p.x,p.y,p.w,p.h,value);
 // Independent containment determines the changed rectangle; paint remains
 // the existing color/alpha/coverage oracle for the optimized write path.
 for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x)
  if(x>=p.x&&x<p.x+p.w&&y>=p.y&&y<p.y+p.h)pair.oracle.paint(y*surface.pitch+x,value);
 pair.compare();
}
static void picture_case(Surface surface,Style style,unsigned id,Placement p,unsigned opacity,unsigned mode){
 Pair pair(surface,style);pair.got.picture(int(id),p.x,p.y,p.w,p.h,opacity,mode);
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
  for(Surface surface:{Surface{13,9,19},Surface{1,1,7}})for(auto style:{styles[0],styles[16]})for(Placement p:{Placement{2,2,0,0},Placement{-3,-2,0,0}}){
   Pair pair(surface,style);const Glyph glyph{sample.encoded.data(),7,5,8,uint16_t(sample.encoded.size())};
   pair.got.glyph(glyph,p.x,p.y,0xffff);
   for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
    const int sx=x-p.x,sy=y-p.y;if(sx>=0&&sx<7&&sy>=0&&sy<5)pair.oracle.paint(y*surface.pitch+x,0xffff,expected[sy*7+sx]);
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
 for(int angle:{0,360,-360,17,90}){
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
int main(){
 const auto data=fixture();CHECK(app.content.open(data.data(),data.size()));
 CHECK((uintptr_t(app.content.image(0).pixels)&1u)==0u);
 rectangles();pictures();picture_regions();picture_row_views();title_particles();CHECK(app.content.healthy());
 Bytes unaligned(data.size()+1);std::copy(data.begin(),data.end(),unaligned.begin()+1);
 CHECK(app.content.open(unaligned.data()+1,data.size()));
 CHECK((uintptr_t(app.content.image(0).pixels)&1u)==1u);
 pictures();picture_regions();picture_row_views();map_pictures();CHECK(app.content.healthy());glyphs();
 std::printf("canvas fast path: %u cases, %u checks; production paint oracle, RGB565 alignment, clipping, pitch, coverage, modulation, additive, scaled, atlas regions, sparse, palette and glyph decode bounds\n",cases,checks);
}
