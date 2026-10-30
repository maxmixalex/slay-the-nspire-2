// Native CPU comparison only; calculator acceptance is a separate device run.
// Both paths compile in one translation unit against identical Canvas state.
#include "../src/app.cpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>

using namespace spire;
static App app;
static volatile uint32_t sink;
__attribute__((noinline)) static void previous(Canvas&c,unsigned alpha){
 for(int y=0;y<240;++y){c.poll_row(unsigned(y));
  for(int x=0;x<320;++x)c.paint(y*c.pitch+x,Ink,alpha);}
}
__attribute__((noinline)) static void current(Canvas&c,unsigned alpha){c.dim(0,0,320,240,alpha);}
struct Result {double milliseconds;uint32_t hash;};
static Result measure(void(*draw)(Canvas&,unsigned),unsigned style){
 Canvas c{app};c.opacity=style?201:255;c.brightness=style?154:255;
 c.tint_red=style?39:255;c.tint_green=style?107:255;c.tint_blue=style?219:255;
 for(unsigned i=0;i<76800;++i)c.pixels[i]=uint16_t((i*4793u+0x185fu)^(i*17u));
 uint32_t hash=2166136261u;
 const auto begin=std::chrono::steady_clock::now();
 for(unsigned frame=0;frame<240;++frame){
  draw(c,(frame*37u)%256u);
  // Consume visible output every frame, not only a converged final backstop.
  for(unsigned i=frame;i<76800;i+=997)hash=(hash^c.pixels[i])*16777619u;
 }
 const auto end=std::chrono::steady_clock::now();sink=hash;
 return {std::chrono::duration<double,std::milli>(end-begin).count(),hash};
}
int main(){
 for(unsigned style=0;style<2;++style){
  double before[7],after[7];uint32_t hash=0;
  for(unsigned run=0;run<7;++run){
   Result a,b;if(run&1){b=measure(current,style);a=measure(previous,style);}
   else{a=measure(previous,style);b=measure(current,style);}
   if(a.hash!=b.hash){std::fprintf(stderr,"dim output mismatch\n");return 1;}
   before[run]=a.milliseconds;after[run]=b.milliseconds;hash=a.hash;
  }
  std::sort(before,before+7);std::sort(after,after+7);
  std::printf("{\"style\":%u,\"frames\":240,\"runs\":7,\"previous_median_ms\":%.6f,\"current_median_ms\":%.6f,\"hash\":\"%08x\"}\n",style,before[3],after[3],hash);
 }
}
