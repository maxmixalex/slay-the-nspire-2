#include "light_quad_rows.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"light row:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 uint32_t random=17;
 auto next=[&](){random=random*1664525u+1013904223u;return random;};
 for(unsigned trial=0;trial<10000;++trial){
  int dx=int(next()%1000001)-500000,dy=int(next()%1000001)-500000;
  int sine=int(next()%2049)-1024,cosine=int(next()%2049)-1024;
  int width=int(next()%150000)+1,height=int(next()%150000)+1;
  int shear=int(next()%200001)-100000;
  spire::LightQuadRows rows(dx,dy,sine,cosine,width,height,shear);
  for(unsigned y=0;y<240;++y,dy+=256,rows.next()){
   const int64_t vy=-int64_t(dx)*sine+int64_t(dy)*cosine;
   const int u=32768+int(((int64_t(dx)*cosine+int64_t(dy)*sine)*64-vy*shear/1024)/width);
   const int v=32768+int(vy*64/height);
   CHECK(rows.x()==u);CHECK(rows.y()==v);
  }
 }
 std::printf("Light UV rows: %u exact checks including both signs and zero crossings\n",checks);
}
