#include "loading_screen.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&bytes,unsigned at,uint32_t n){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(n>>(i*8));}
int main(){
    for(unsigned id=0;id<unsigned(BootText::Count);++id)for(auto language:{BootLanguage::Unknown,BootLanguage::English,BootLanguage::Chinese}){
        const char*label=boot_text(static_cast<BootText>(id),language);CHECK(*label);CHECK(BootPaint::width(label)<=304);
        bool chinese=false,english=false;
        for(const char*p=label;*p;){const auto ch=BootPaint::next(p);if(ch<128){english|=ch>='A'&&ch<='Z';continue;}chinese=true;
            bool found=false;for(const auto&glyph:BootGlyphs)if(glyph.codepoint==ch){found=true;uint16_t bits=0;for(auto row:glyph.rows)bits|=row;CHECK(bits!=0);break;}CHECK(found);}
        if(language==BootLanguage::Unknown)CHECK(chinese&&english);
        if(language==BootLanguage::English)CHECK(!chinese&&english);
    }
    std::vector<uint16_t> pixels(320*240+2,0xface);auto*screen=pixels.data()+1;
    for(auto language:{BootLanguage::Unknown,BootLanguage::English,BootLanguage::Chinese})for(unsigned stage=0;stage<unsigned(ResourceStage::Count);++stage){
        loading_screen(screen,static_cast<ResourceStage>(stage),50,100,0,nullptr,false,language);
        unsigned gold=0;for(unsigned x=40;x<280;++x)gold+=screen[169*320+x]==0xc568;CHECK(gold==120);
        CHECK(pixels.front()==0xface&&pixels.back()==0xface);
    }
    boot_language_screen(screen,BootLanguage::Chinese,false);CHECK(pixels.front()==0xface&&pixels.back()==0xface);
    boot_error_screen(screen,BootText::InvalidResources,BootLanguage::Unknown);CHECK(pixels.front()==0xface&&pixels.back()==0xface);
    loading_screen(nullptr,ResourceStage::Header,0,1,0,nullptr,false,BootLanguage::Chinese);
    for(auto language:{BootLanguage::English,BootLanguage::Chinese}){
        const auto*bytes=reinterpret_cast<const uint8_t*>(boot_preference_bytes(language));CHECK(boot_preference(bytes,8)==language);
        CHECK(boot_preference(bytes,7)==BootLanguage::Unknown);CHECK(boot_preference(bytes,9)==BootLanguage::Unknown);
    }
    CHECK(boot_preference(nullptr,8)==BootLanguage::Unknown);CHECK(boot_preference(reinterpret_cast<const uint8_t*>("NSL1zzz\n"),8)==BootLanguage::Unknown);
    std::vector<uint8_t> pack(40+16+16+6111*8+4);std::memcpy(pack.data(),"NSP2",4);
    put(pack,4,18);put(pack,8,0x41cef1ea);put(pack,12,pack.size());put(pack,16,1);put(pack,20,1);put(pack,24,6111);
    const unsigned row=40+16+16+6110*8;put(pack,row,pack.size()-4);put(pack,row+4,3);
    std::memcpy(pack.data()+pack.size()-4,"eng",4);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::English);
    std::memcpy(pack.data()+pack.size()-4,"zhs",4);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Chinese);
    put(pack,row,0xffffffffu);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Unknown);
    put(pack,16,0xffffffffu);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Unknown);
    CHECK(boot_probe_language({})==BootLanguage::Unknown);
    std::printf("%u boot language/coverage/bounds checks passed\n",checks);
}
