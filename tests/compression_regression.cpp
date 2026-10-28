#include "lz4_block.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL line %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(){
    unsigned char out[100]={};unsigned char literals[]={0x50,'h','e','l','l','o'};
    CHECK(spire::decode_lz4(literals,sizeof literals,out,5));CHECK(!memcmp(out,"hello",5));
    unsigned char overlap[]={0x1f,'a',1,0,20,0x50,'b','c','d','e','f'};
    CHECK(spire::decode_lz4(overlap,sizeof overlap,out,45));for(int i=0;i<40;i++)CHECK(out[i]=='a');CHECK(!memcmp(out+40,"bcdef",5));
    for(unsigned n=0;n<sizeof overlap;n++)CHECK(!spire::decode_lz4(overlap,n,out,45));
    overlap[2]=0;CHECK(!spire::decode_lz4(overlap,sizeof overlap,out,45));
    CHECK(!spire::decode_lz4(nullptr,0,out,0));CHECK(!spire::decode_lz4(literals,sizeof literals,nullptr,5));
    std::mt19937 rng(77);
    for(int k=0;k<20000;k++){
        unsigned n=rng()%200,capacity=rng()%1000;
        std::vector<unsigned char> data(n),output(capacity+16,0xda);
        for(auto&b:data)b=(unsigned char)rng();
        spire::decode_lz4(data.data(),n,output.data(),capacity);
        for(unsigned i=capacity;i<output.size();i++)CHECK(output[i]==0xda);
    }
    printf("compression: %u boundary checks passed\n",checks);
}
