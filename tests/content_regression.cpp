#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,size_t p,uint32_t n,unsigned width=4){for(unsigned i=0;i<width;i++)b[p+i]=(uint8_t)(n>>(i*8));}
static void hash(std::vector<uint8_t>&b){uint32_t h=2166136261u;for(size_t i=40;i<b.size();i++){h^=b[i];h*=16777619u;}put(b,36,h);}
static std::vector<uint8_t> pack(){
    constexpr unsigned images=A_Count+160,strings=3*CardKinds+24,clips=AnimationKinds,cards=CardKinds;
    constexpr unsigned dir=40+images*16+strings*8+clips*16+cards*16+U_Count*4;
    std::vector<uint8_t>b(dir+images*82*60*2+strings*2);
    put(b,0,0x3250534e);put(b,4,9);put(b,8,0x41cef1ea);put(b,12,(uint32_t)b.size());
    put(b,16,images);put(b,24,strings);put(b,28,clips);put(b,32,cards);
    for(unsigned i=0;i<images;i++){
        put(b,40+i*16,dir+i*9840);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,9840);
    }
    unsigned off=40+images*16;
    for(unsigned i=0;i<strings;i++){
        unsigned p=dir+images*9840+i*2;put(b,off+i*8,p);put(b,off+i*8+4,1);b[p]='X';
    }
    off+=strings*8;
    for(unsigned i=0;i<clips;i++){put(b,off+i*16,0);put(b,off+i*16+4,1,2);put(b,off+i*16+6,33,2);}
    off+=clips*16;
    for(unsigned i=0;i<cards;i++)for(unsigned j=0;j<4;j++)put(b,off+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;i++)put(b,off+cards*16+i*4,i==U_CampFlames?A_Count:i);
    hash(b);return b;
}
int main(int argc,char**argv){
    auto original=pack();Content c;CHECK(c.open(original.data(),original.size()));CHECK(c.card_count()==CardKinds);
    for(int i=0;i<CardKinds;i++){CHECK(c.card_image(i)==i);CHECK(!std::strcmp(c.card_string(i),"X"));}
    CHECK(c.card_image(-1)==-1&&c.card_image(CardKinds)==-1);CHECK(!*c.card_string(CardKinds));
    constexpr unsigned carddir=40+(A_Count+160)*16+(3*CardKinds+24)*8+AnimationKinds*16;
    auto b=original;put(b,carddir+23*16,9999);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,carddir+3*4,3*CardKinds+24);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,32,25);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;b.back()^=1;CHECK(!c.open(b.data(),b.size()));
    CHECK(!c.open(nullptr,0));CHECK(c.card_count()==0);
    for(unsigned n=0;n<40;n++)CHECK(!c.open(original.data(),n));
    // Card count alone cannot identify the new relic-art directory. A valid
    // older profile stays readable by Content but must not boot this App.
    b=original;put(b,4,8);hash(b);CHECK(c.open(b.data(),b.size()));
    CHECK(!c.current_profile());static App legacy;CHECK(!legacy.init(b.data(),b.size(),{}));
    if(argc>1){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f);std::vector<uint8_t> data(MaxPack);size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);
        CHECK(c.open(data.data(),n));
        for(int i=0;i<CardKinds;i++){
            CHECK(c.image(c.card_image(i)).pixels);CHECK(c.image(c.card_image(i)).w==82);
            for(int v=0;v<3;v++)CHECK(*c.card_string(i,v));
        }
        static App a;CHECK(a.init(data.data(),n,{}));new_run(a.game,7,true);a.title=false;
        const Card offering{Offering,1};char desc[512];a.card_description(offering,0,desc,sizeof desc);
        CHECK(std::strstr(desc,"6"));CHECK(std::strstr(desc,"5"));CHECK(!std::strchr(desc,'{'));
        a.render();
    }
    std::printf("content: %u checks passed\n",checks);
}
