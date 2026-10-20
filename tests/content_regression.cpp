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
// Generated synthetic parser fixtures; historical save fixture bytes are
// unrelated and are never rewritten when a content profile changes.
constexpr unsigned minimum_images=CardKinds>A_Count+160?CardKinds:A_Count+160;
constexpr unsigned fixture_images=minimum_images>U_Count?minimum_images:U_Count;
static std::vector<uint8_t> pack(unsigned version=ContentVersion){
    const unsigned images=fixture_images,strings=version==14?ContentStrings:version==13?1556:version==12?749:version==11?407:EventTextEnd;
    const unsigned clips=version==14?AnimationKinds:version==13?38:17,cards=version==14?CardKinds:version==13?497:version==12?228:version==11?114:68;
    const unsigned ui_count=version==14?U_Count:version==13?339:version==12?233:version==11?185:version==10?105:90;
    const unsigned dir=40+images*16+strings*8+clips*16+cards*16+ui_count*4;
    std::vector<uint8_t>b(dir+images*82*60*2+strings*2);
    put(b,0,0x3250534e);put(b,4,version);put(b,8,0x41cef1ea);put(b,12,(uint32_t)b.size());
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
    for(unsigned i=0;i<ui_count;i++)put(b,off+cards*16+i*4,i==U_CampFlames?A_Count:i);
    hash(b);return b;
}
int main(int argc,char**argv){
    auto original=pack();Content c;CHECK(c.open(original.data(),original.size()));CHECK(c.card_count()==CardKinds);CHECK(c.current_profile());
    for(int i=0;i<CardKinds;i++){CHECK(c.card_image(i)==i);CHECK(!std::strcmp(c.card_string(i),"X"));}
    CHECK(c.card_image(-1)==-1&&c.card_image(CardKinds)==-1);CHECK(!*c.card_string(CardKinds));
    constexpr unsigned carddir=40+fixture_images*16+ContentStrings*8+AnimationKinds*16;
    auto b=original;put(b,carddir+23*16,9999);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,carddir+3*4,ContentStrings);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,32,25);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;b.back()^=1;CHECK(!c.open(b.data(),b.size()));
    CHECK(!c.open(nullptr,0));CHECK(c.card_count()==0);
    for(unsigned n=0;n<40;n++)CHECK(!c.open(original.data(),n));
    // Card count alone cannot identify the new relic-art directory. A valid
    // older profile stays readable by Content but must not boot this App.
    static App legacy;
    for(unsigned version:{8u,10u,11u,12u,13u}){
        b=pack(version);CHECK(c.open(b.data(),b.size()));CHECK(c.card_count()==(version==13?497u:version==12?228u:version==11?114u:68u));
        CHECK(!c.current_profile());CHECK(!legacy.init(b.data(),b.size(),{}));
    }
    // The immutable v11/v12 directories must not expand when the current registry
    // does. Merely relabeling either version leaves an incompatible pack.
    b=original;put(b,4,11);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=pack(11);put(b,4,12);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,4,12);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=pack(12);put(b,4,13);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,28,17);hash(b);CHECK(!c.open(b.data(),b.size()));
    if(argc>1){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long length=std::ftell(f);CHECK(length>0&&length<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t> data((size_t)length);size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);
        CHECK(c.open(data.data(),n));
        for(int i=0;i<CardKinds;i++){
            CHECK(c.image(c.card_image(i)).pixels);CHECK(c.image(c.card_image(i)).w==82);
            for(int v=0;v<3;v++)CHECK(*c.card_string(i,v));
        }
        CHECK(c.current_profile());
        for(int i=233;i<U_Count;i++){auto picture=c.image(c.ui_image(i));CHECK(picture.pixels&&picture.w&&picture.h);}
        for(int i=17;i<AnimationKinds;i++){
            Clip clip=c.clip(i);CHECK(clip.count&&clip.frame_ms);CHECK(c.image(clip.first).pixels);CHECK(c.image(clip.first+clip.count-1).pixels);
        }
        CHECK(c.frame_image(Card{DefectZap,0})==c.ui_image(U_BlueSkill));
        CHECK(c.frame_image(Card{RegentAlignment,0})==c.ui_image(U_OrangeSkill));
        CHECK(c.frame_image(Card{NecroAfterlife,0})==c.ui_image(U_PinkSkill));
        CHECK(c.energy_image(Card{DefectZap,0})==c.ui_image(U_DefectEnergy));
        CHECK(c.energy_image(Card{RegentAlignment,0})==c.ui_image(U_RegentEnergy));
        CHECK(c.energy_image(Card{NecroAfterlife,0})==c.ui_image(U_NecrobinderEnergy));
        for(int i=0;i<WorldRelicKinds;i++)CHECK(c.relic_image(i)==c.world_image("relics",world_relic_key(RelicId(i))));
        static App a;CHECK(a.init(data.data(),n,{}));new_run(a.game,7,true);a.title=false;
        const Card offering{Offering,1};char desc[512];a.card_description(offering,0,desc,sizeof desc);
        CHECK(std::strstr(desc,"6"));CHECK(std::strstr(desc,"5"));CHECK(!std::strchr(desc,'{'));
        a.render();
    }
    std::printf("content: %u checks passed\n",checks);
}
