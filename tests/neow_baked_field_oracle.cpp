// Private-data oracle: compare the offline integer bake with the production
// Neow Shader and exact per-node compositor at clock zero. No game data here.
#include "neow_fire_ui23.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iterator>
using namespace spire;
static std::vector<unsigned char> read(const char*path){std::ifstream f(path,std::ios::binary);if(!f){std::fprintf(stderr,"Cannot read %s\n",path);std::exit(2);}return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){
 if(argc!=7){std::fprintf(stderr,"Usage: neow_baked_field_oracle PARAM FIELD0 FIELD1 FIELD2 FIELD3 RADIANCE\n");return 2;}
 using namespace neow_fire_ui23;
 auto parameter=read(argv[1]);std::vector<unsigned char> raw[4];ancient_fire_ui23::Field fields[4];
 const unsigned dimensions[][3]={{600,636,1},{250,250,3},{256,256,3},{100,100,2}};
 for(unsigned i=0;i<4;++i){raw[i]=read(argv[i+2]);auto&f=fields[i];f.width=dimensions[i][0];f.height=dimensions[i][1];f.channels=dimensions[i][2];
  if(raw[i].size()!=f.width*f.height*f.channels)return 3;
  for(unsigned at=0;at<raw[i].size();at+=131072)f.pages[at/131072]=raw[i].data()+at;
 }
 struct Picture{const unsigned char*pixels;unsigned w,h,format;const void*alpha=nullptr;const void*palette=nullptr;};
 Table table;if(!table.read(Picture{parameter.data(),256,unsigned(parameter.size()/512),0},word(parameter.data()+6)==2))return 4;
 const auto expected=read(argv[6]);if(expected.size()!=MaskBytes*2)return 5;
 Shader shaders[2];for(unsigned i=0;i<2;++i)shaders[i].bind(fields,table.data+ModelsAt+i*128,0);
 unsigned green[MaskBytes]{},blue[MaskBytes]{};
 for(unsigned node=0;node<4;++node){const auto*r=table.node(node);const auto&shader=shaders[r[28]];
  for(unsigned y=0;y<Height;++y){unsigned pixel=table.row_pixel(y);for(unsigned span=table.row_span(y);span<table.row_span(y+1);++span){
   const unsigned x=table.x(span),count=table.count(span);int u=ancient_fire_ui23::integer(r)+int(y)*ancient_fire_ui23::integer(r+16)+int(x)*ancient_fire_ui23::integer(r+8);
   int v=ancient_fire_ui23::integer(r+4)+int(y)*ancient_fire_ui23::integer(r+20)+int(x)*ancient_fire_ui23::integer(r+12);
   for(unsigned i=0;i<count;++i,++pixel,u+=ancient_fire_ui23::integer(r+8),v+=ancient_fire_ui23::integer(r+12)){
    if(!table.active(node,pixel)||unsigned(u)>=268435456u||unsigned(v)>=268435456u)continue;
    const unsigned alpha=(shader.alpha(u>>8,v>>8)*255u+524288u)>>20;
    green[pixel]+=word(r+24)*alpha;blue[pixel]+=word(r+26)*alpha;
   }
  }}
 }
 const auto div255=[](unsigned x){x+=128;return (x+(x>>8))>>8;};unsigned nonzero=0;
 for(unsigned i=0;i<MaskBytes;++i){const unsigned actual=(div255(green[i])<<8)|div255(blue[i]);
  if(actual!=word(expected.data()+i*2)){std::fprintf(stderr,"Neow bake pixel %u: original runtime %04x, offline %04x\n",i,actual,word(expected.data()+i*2));return 1;}nonzero+=actual!=0;
 }
 std::printf("Neow held field: %u exact production-shader pixels (%u nonzero), clock0, %u bytes.\n",MaskBytes,nonzero,MaskBytes*2);
}
