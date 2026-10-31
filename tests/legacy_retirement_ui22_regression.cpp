#include "app.hpp"
#include "retired_assets_ui22.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t word(const uint8_t*p){return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
static void put(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=uint8_t(v>>(8*i));}
struct Provider{const std::vector<uint8_t>*data;uint32_t fault=UINT32_MAX;};
static bool read_at(void*ctx,uint32_t at,void*out,uint32_t count){
    auto&p=*static_cast<Provider*>(ctx);
    if(at>p.data->size()||count>p.data->size()-at||(at<=p.fault&&count>p.fault-at))return false;
    std::memcpy(out,p.data->data()+at,count);return true;
}
static std::vector<uint8_t>read_file(const char*path){
    FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long n=std::ftell(f);
    CHECK(n>40&&uint64_t(n)<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>v(static_cast<size_t>(n));
    CHECK(std::fread(v.data(),1,v.size(),f)==v.size());CHECK(!std::fclose(f));return v;
}
int main(int argc,char**argv){
    CHECK(argc==3);const auto data=read_file(argv[1]),old=read_file(argv[2]);
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>(),prior=std::make_unique<Content>();
    CHECK(memory->open(data.data(),data.size()));Provider provider{&data};
    CHECK(ranged->open({&provider,uint32_t(data.size()),read_at,nullptr}));CHECK(prior->open(old.data(),old.size()));
    CHECK(memory->current_profile()&&ranged->current_profile()&&prior->current_profile());
    CHECK(word(data.data()+16)==retired_assets_ui22::Images);
    for(int id=0;id<1783;++id){const auto a=memory->clip(id),b=prior->clip(id),c=ranged->clip(id);
        CHECK(a.first==b.first&&a.count==b.count&&a.frame_ms==b.frame_ms&&a.pivot_x==b.pivot_x&&a.pivot_y==b.pivot_y&&a.sprite_units==b.sprite_units);
        CHECK(a.first==c.first&&a.count==c.count&&a.frame_ms==c.frame_ms&&a.pivot_x==c.pivot_x&&a.pivot_y==c.pivot_y&&a.sprite_units==c.sprite_units);}
    const unsigned marker_row=40+retired_assets_ui22::MarkerImage*16,marker_at=word(data.data()+marker_row);
    const unsigned clip_at=40+word(data.data()+16)*16+word(data.data()+20)*16+word(data.data()+24)*8;
    for(unsigned offset: {marker_at,marker_at+8,marker_row+8,clip_at+1696*16+4,40u+45810*16+4,40u+45956*16+4}){
        auto damaged=data;damaged[offset]^=1;
        CHECK(!memory->open(damaged.data(),damaged.size()));CHECK(!memory->healthy());
        Provider source{&damaged};CHECK(!ranged->open({&source,uint32_t(damaged.size()),read_at,nullptr}));CHECK(!ranged->healthy());}
    provider.fault=marker_at;
    CHECK(!ranged->open({&provider,uint32_t(data.size()),read_at,nullptr}));CHECK(!ranged->healthy());
    provider.fault=UINT32_MAX;CHECK(ranged->open({&provider,uint32_t(data.size()),read_at,nullptr}));
    const int backdrop=ranged->ui_image(U_MerchantBackdrop);CHECK(backdrop>=0);
    provider.fault=word(data.data()+40+unsigned(backdrop)*16);
    CHECK(!ranged->image(backdrop).pixels);CHECK(!ranged->healthy());
    // The registered branch depends on resident catalog identities, not a
    // successful pixel read. IO failure cannot select retired clips8/9.
    CHECK(ranged->ui_image(U_MerchantBackdrop)==backdrop&&ranged->clip(AnimMerchantRoomTop).count>0);
    CHECK(prior->healthy());
    std::printf("UI22 retirement: memory/ranged marker, IO failure, old profile and 1783 stable clips; %u checks passed\n",checks);
}
