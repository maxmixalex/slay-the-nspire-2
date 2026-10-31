#include "implicit_geometry.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"implicit geometry:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint16_t colors[320];
static uint8_t alpha[320],expected[320*8+2],actual[320*8+2];
static unsigned reference(unsigned width){
    unsigned x=0,position=2,spans=0;
    while(x<width){
        if(!alpha[x]){++x;continue;}const unsigned first=x;
        while(x<width&&alpha[x])++x;
        unsigned cost0=5,cost1=5;uint8_t parent[320][2],modes[320];
        for(unsigned at=first;at<x;++at){
            const unsigned p0=cost0<=cost1+5?0:1,p1=cost1<=cost0+5?1:0;
            parent[at-first][0]=uint8_t(p0);parent[at-first][1]=uint8_t(p1);
            const unsigned next0=(p0?cost1+5:cost0)+2,next1=(p1?cost1:cost0+5)+3;
            cost0=alpha[at]==255?next0:1u<<30;cost1=next1;
        }
        unsigned mode=cost0<=cost1?0:1;
        for(unsigned at=x-first;at;){--at;modes[at]=uint8_t(mode);mode=parent[at][mode];}
        for(unsigned at=first;at<x;){
            unsigned end=at+1;mode=modes[at-first];while(end<x&&modes[end-first]==mode)++end;
            implicit_geometry::word(expected+position,at);implicit_geometry::word(expected+position+2,end-at);expected[position+4]=uint8_t(mode);position+=5;
            for(unsigned i=at;i<end;++i){implicit_geometry::word(expected+position,colors[i]);position+=2;if(mode)expected[position++]=alpha[i];}
            ++spans;at=end;
        }
    }
    implicit_geometry::word(expected,spans);return position;
}
static void compare(unsigned width){
    unsigned written=0,geometry=0;const unsigned size=reference(width);
    CHECK(implicit_geometry::row(colors,alpha,width,actual,sizeof actual,written,geometry));
    CHECK(size==written&&!std::memcmp(expected,actual,size));
    CHECK(geometry==2+5u*(actual[0]|unsigned(actual[1])<<8));
    unsigned a,b;CHECK(!implicit_geometry::row(colors,alpha,width,actual,size-1,a,b));
}
int main(){
    for(unsigned i=0;i<320;++i)colors[i]=uint16_t((i*3917u+12345u)^0xa55au);
    unsigned count=1;
    for(unsigned width=1;width<=12;++width){count*=3;
        for(unsigned code=0;code<count;++code){unsigned value=code;
            for(unsigned x=0;x<width;++x){alpha[x]=(value%3)==0?0:(value%3)==1?91:255;value/=3;}compare(width);}
    }
    for(unsigned leading=0;leading<=30;++leading)for(unsigned middle=0;middle<=30;++middle)for(unsigned trailing=0;trailing<=30;++trailing){
        unsigned at=0;for(unsigned i=0;i<leading;++i)alpha[at++]=255;alpha[at++]=1;
        for(unsigned i=0;i<middle;++i)alpha[at++]=255;alpha[at++]=254;
        for(unsigned i=0;i<trailing;++i)alpha[at++]=255;compare(at);
    }
    uint32_t random=0x41cef1ea;
    for(unsigned row=0;row<20000;++row){
        for(unsigned i=0;i<320;++i){random=random*1664525u+1013904223u;const unsigned n=random>>24;alpha[i]=n<15?0:n<50?uint8_t(n):255;}
        compare(320);
    }
    uint8_t storage[42]{0xfd,0xff},active[300]{};
    storage[2]=5;storage[39]=8;CHECK(implicit_geometry::mask(storage,40,300,active));CHECK(active[0]&&active[2]&&active[299]&&!active[1]);
    storage[39]|=16;CHECK(!implicit_geometry::mask(storage,40,300,active));storage[39]&=15;
    CHECK(!implicit_geometry::mask(storage,39,300,active));CHECK(!implicit_geometry::mask(storage,40,301,active));
    storage[40]=1;CHECK(!implicit_geometry::mask(storage,42,300,active));storage[40]=0;
    CHECK(implicit_geometry::mask(storage,42,300,active));
    std::printf("Implicit geometry: exact source DP ties, all ternary rows through12, long opaque thresholds, alpha0 hidden RGB, bounds; %u checks passed\n",checks);
}
