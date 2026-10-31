#include "app.hpp"
#include "content_animation_profile.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> read_file(const char*path){
    FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);
    CHECK(size>40&&uint64_t(size)<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));
    std::vector<uint8_t> data(static_cast<size_t>(size));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());
    CHECK(!std::fclose(f));return data;
}
struct Provider {
    const std::vector<uint8_t>*data;
    static bool read(void*context,uint32_t at,void*out,uint32_t count){
        const auto&data=*static_cast<Provider*>(context)->data;
        if(at>data.size()||count>data.size()-at)return false;
        std::memcpy(out,data.data()+at,count);return true;
    }
    ResourceSource source(){return {this,uint32_t(data->size()),read,nullptr};}
};
int main(int argc,char**argv){
    CHECK(argc==3);const auto before=read_file(argv[1]),after=read_file(argv[2]);
    auto original=std::make_unique<Content>(),reduced=std::make_unique<Content>();
    CHECK(original->open(before.data(),before.size()));CHECK(!original->reduced_animations());
    CHECK(reduced->open(after.data(),after.size()));CHECK(reduced->reduced_animations());
    for(unsigned clip=0;clip<1783;++clip){
        const auto a=original->clip(int(clip)),b=reduced->clip(int(clip));
        CHECK(a.first==b.first&&a.count==b.count&&a.frame_ms==b.frame_ms&&
              a.pivot_x==b.pivot_x&&a.pivot_y==b.pivot_y&&a.sprite_units==b.sprite_units);
    }
    Provider provider{&after};CHECK(reduced->open(provider.source()));CHECK(reduced->reduced_animations());
    // Idle aliases share a decoded allocation across different directory IDs.
    // The ranged reader must perform no further resource read/cache insertion.
    const auto idle=reduced->clip(0);CHECK(idle.count>1);CHECK(reduced->image_alias(idle.first,idle.first+1));
    const auto first=reduced->image(idle.first);CHECK(first.pixels);
    const auto stats=reduced->read_stats();const auto usage=reduced->image_cache_usage();
    const auto second=reduced->image(idle.first+1);CHECK(second.pixels==first.pixels);
    CHECK(reduced->image_cache_usage().entries==usage.entries&&reduced->image_cache_usage().pages==usage.pages);
    CHECK(reduced->read_stats().callbacks==stats.callbacks&&reduced->read_stats().callback_bytes==stats.callback_bytes);
    CHECK(reduced->open(before.data(),before.size()));CHECK(!reduced->reduced_animations());
    CHECK(!reduced->open(nullptr,0));CHECK(!reduced->reduced_animations());
    uint8_t legacy[40]{};std::memcpy(legacy+36,"NAF1",4);
    CHECK(!content_animation_profile::reduced(legacy,45));CHECK(content_animation_profile::reduced(legacy,46));
    std::printf("Reduced poses:1783 stable clocks, automatic profile/reopen and shared alias cache; %u checks passed\n",checks);
}
