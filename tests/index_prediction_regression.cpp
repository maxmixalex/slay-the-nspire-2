#include "index_prediction.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"index prediction line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
void put(Bytes& b,unsigned at,uint32_t value){CHECK(at<=b.size()&&b.size()-at>=4);for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(value>>(i*8));}
void append(Bytes& b,uint32_t value){const unsigned at=unsigned(b.size());b.resize(at+4);put(b,at,value);}
uint32_t zigzag(int32_t d){return d<0?uint32_t(-int64_t(d)*2-1):uint32_t(d)*2u;}
struct Fixture{Bytes expected,encoded;unsigned frames=0,tiles=65535u*300u;};
Fixture fixture(unsigned frames,bool changing){
    Fixture f;f.frames=frames;std::vector<uint32_t> previous;
    uint32_t rng=0x12345678u;
    for(unsigned frame=0;frame<frames;++frame){
        const unsigned count=changing?(frame%5==0?1:frame%5==1?3:frame%5==2?300:19):300;
        const unsigned geometry=changing?1+frame%17:6900;
        for(Bytes* b:{&f.expected,&f.encoded}){append(*b,614880);append(*b,count*4);append(*b,geometry);}
        std::vector<uint32_t> current;
        for(unsigned i=0;i<count;++i){
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            uint32_t id=rng%f.tiles;
            if(previous.size()==count&&i%4==0)id=previous[i];
            if(i%7==0)id=i&1?0:f.tiles-1;
            const uint32_t spatial=i?current.back():0,temporal=previous.size()==count?previous[i]:0;
            const uint32_t a=zigzag(int32_t(id)-int32_t(spatial)),b=zigzag(int32_t(id)-int32_t(temporal));
            append(f.expected,id);append(f.encoded,b<a?(b<<1)|1u:a<<1);current.push_back(id);
        }
        for(unsigned i=0;i<geometry;++i){const uint8_t v=uint8_t(i*19+frame);f.expected.push_back(v);f.encoded.push_back(v);}
        previous=std::move(current);
    }
    return f;
}
void roundtrip(){
    for(unsigned frames:{1u,2u,17u,64u})for(bool changing:{false,true}){
        Fixture f=fixture(frames,changing);CHECK(f.encoded.size()<=524288);
        for(unsigned alignment=0;alignment<4;++alignment){
            Bytes buffer(32+alignment+f.encoded.size()+32,0xcd);
            std::memcpy(buffer.data()+32+alignment,f.encoded.data(),f.encoded.size());
            CHECK(restore_index_prediction(buffer.data()+32+alignment,unsigned(f.encoded.size()),frames,f.tiles));
            CHECK(!std::memcmp(buffer.data()+32+alignment,f.expected.data(),f.expected.size()));
            for(unsigned i=0;i<32+alignment;++i)CHECK(buffer[i]==0xcd);
            for(unsigned i=unsigned(buffer.size()-32);i<buffer.size();++i)CHECK(buffer[i]==0xcd);
        }
    }
}
void failures(){
    Fixture f=fixture(2,true);
    CHECK(!restore_index_prediction(nullptr,unsigned(f.encoded.size()),f.frames,f.tiles));
    for(unsigned size:{0u,1u,11u,524289u}){Bytes b=f.encoded;CHECK(!restore_index_prediction(b.data(),size,f.frames,f.tiles));}
    for(unsigned count:{0u,65u}){Bytes b=f.encoded;CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),count,f.tiles));}
    for(unsigned tiles:{0u,65535u*300u+1u}){Bytes b=f.encoded;CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),f.frames,tiles));}
    const unsigned values[][2]={{0,0},{0,614881},{4,0},{4,1204},{4,3},{8,0},{8,614881},{8,524288},{12,0xffffffffu},{12,f.tiles*4u}};
    for(const auto& entry:values){Bytes b=f.encoded;put(b,entry[0],entry[1]);CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),f.frames,f.tiles));}
    {Bytes b=f.encoded;b.push_back(0);CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),f.frames,f.tiles));}
    {Bytes b=f.encoded;b.pop_back();CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),f.frames,f.tiles));}
    // A valid-sized difference still cannot restore an out-of-range tile.
    {Bytes b=f.encoded;put(b,12,2u);CHECK(!restore_index_prediction(b.data(),unsigned(b.size()),f.frames,f.tiles));}
}
}
int main(){roundtrip();failures();std::printf("index prediction: %u checks passed; exact in-place frames, unaligned records, limits and guarded failures\n",checks);}
