// The renderer remains private: this TU supplies app.cpp definitions and the
// archive supplies only its other objects, as in canvas_fast_path_regression.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
namespace {
static App app;static unsigned checks=0,cases=0;
static uint16_t expected[ScreenW*ScreenH];
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"crystal shader line %d case %u: %s\n",__LINE__,cases,#x);std::exit(1);}}while(0)
#include "crystal_shader_reference.inc"
template<unsigned Size> void axis_oracle(const crystal_shader::Axis<Size>&axis){
 for(unsigned i=0;i<Size;++i){
  const double value=double((2*i+1)*1419)-double(348*Size),denominator=200.*Size;
  const int cell=int(std::floor(value/denominator));
  const int fraction=int((value-double(cell)*denominator)*1000./denominator);
  const auto&p=axis.pixels[i];CHECK(int(p.cell)==cell);
  CHECK(p.sample==unsigned(std::floor((2.*i+1)*160./(2.*Size))));
  CHECK(p.low==unsigned(fraction>=50?0:std::min(255,(50-fraction)*255/30)));
  CHECK(p.high==unsigned(fraction<=950?0:std::min(255,(fraction-950)*255/30)));
 }
}
void phase_oracle(uint32_t now){
 const auto p=crystal_shader::phase(now);
 CHECK(p.one==uint32_t((static_cast<unsigned __int128>(now)*425984)/1000000));
 CHECK(p.two==uint32_t((static_cast<unsigned __int128>(now)*720896)/10000000));
}
void fixture(uint32_t now,unsigned mode){
 app.map_clock_ms=now;app.crystal_presentation={};auto&m=app.crystal_presentation;
 for(unsigned i=0;i<CrystalCells;++i){
  m.cells[i]=mode==1||mode==2?(i%3!=0):mode==3;
  m.cell_start[i]=now-(mode==1?500:mode==2?i*37%501:0);
 }
}
void surface(uint16_t*p){for(unsigned i=0;i<ScreenW*ScreenH;++i)p[i]=uint16_t((i*3917u)^(i>>2)^0x8410);}
void compare(uint32_t now,unsigned mode,unsigned opacity){
 ++cases;fixture(now,mode);surface(app.pixels);surface(expected);
 Canvas candidate{app},reference{app};reference.pixels=expected;
 candidate.opacity=reference.opacity=uint8_t(opacity);
 const auto phase=crystal_shader::phase(now);
 for(unsigned fog=0;fog<2;++fog){
  crystal_sphere_reference(reference,fog!=0);crystal_sphere_layer(candidate,fog!=0,phase);
  CHECK(app.content.healthy());
  if(std::memcmp(app.pixels,expected,sizeof expected)){
   for(unsigned i=0;i<ScreenW*ScreenH;++i)if(app.pixels[i]!=expected[i]){
    std::fprintf(stderr,"time=%u mode=%u opacity=%u fog=%u pixel=%u got=%04x expected=%04x\n",now,mode,opacity,fog,i,app.pixels[i],expected[i]);break;}
   CHECK(false);
  }
  ++checks;
 }
}
double timed(bool reference,unsigned frames){
 fixture(10000,2);surface(app.pixels);Canvas c{app};
 const auto start=std::chrono::steady_clock::now();
 for(unsigned i=0;i<frames;++i){app.map_clock_ms=10000+i*33;
  if(reference){crystal_sphere_reference(c,false);crystal_sphere_reference(c,true);}
  else{const auto phase=crystal_shader::phase(app.map_clock_ms);crystal_sphere_layer(c,false,phase);crystal_sphere_layer(c,true,phase);}}
 return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
}
int main(int argc,char**argv){
 axis_oracle(crystal_shader::OuterX);axis_oracle(crystal_shader::OuterY);axis_oracle(crystal_shader::FogX);axis_oracle(crystal_shader::FogY);
 const uint32_t times[]={0,1,33,100,499,500,501,799,800,1000,10000,60000,3600000,0xfffff000u,0xfffffffeu,0xffffffffu};
 for(uint32_t now:times)phase_oracle(now);
 uint32_t random=0x41cef1ea;for(unsigned i=0;i<2048;++i){random^=random<<13;random^=random>>17;random^=random<<5;phase_oracle(random);}
 if(argc>1){
  FILE*f=std::fopen(argv[1],"rb");CHECK(f);std::vector<uint8_t>pack(spire_test::pack_size(f));
  CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());CHECK(!std::fclose(f));CHECK(app.content.open(pack.data(),pack.size()));CHECK(app.content.current_profile());
  for(uint32_t now:times)for(unsigned mode=0;mode<4;++mode)for(unsigned opacity:{0u,91u,255u})compare(now,mode,opacity);
  const unsigned frames=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):360;CHECK(frames>=10&&frames<=3600);
  const double before=timed(true,frames);std::memcpy(expected,app.pixels,sizeof expected);
  const double after=timed(false,frames);CHECK(!std::memcmp(expected,app.pixels,sizeof expected));
  std::printf("{\"operation\":\"crystal_shader\",\"cases\":%u,\"frames_each\":%u,\"before_ms\":%.6f,\"after_ms\":%.6f,\"pixels_equal\":true,\"coordinate_bytes\":2536,\"method\":\"Unpaced host CPU; same actual decoded images and private software shader, not Godot or device FPS\"}\n",cases,frames,before,after);
 }
 std::printf("Crystal shader: %u checks, %u real-resource dual-layer pixel cases; exact fixed coordinates and 32-bit clock phases passed.\n",checks,cases);
}
