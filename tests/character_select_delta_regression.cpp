#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
namespace delta=spire::character_select_delta;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"selection delta:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Source {
    std::vector<std::vector<uint8_t>> images;
    std::vector<std::vector<uint16_t>> full;
    unsigned epoch=1,reads=0,last=0;
    uint32_t generation()const{return epoch;}
    Picture image(int id){
        ++reads;last=unsigned(id);
        if(id<0||unsigned(id)>=images.size())return {};
        return {images[id].data(),nullptr,nullptr,320,240,uint8_t(images[id].size()==320*240*2?0:4)};
    }
};
static void word(std::vector<uint8_t>& data,unsigned value){data.push_back(uint8_t(value));data.push_back(uint8_t(value>>8));}
static Source make_source(){
    Source source;std::vector<uint16_t> color(320*240);
    for(unsigned i=0;i<color.size();++i)color[i]=uint16_t(i*237+19);
    for(unsigned frame=0;frame<32;++frame){
        const unsigned x=(frame*71)%320,y=(frame*37)%240;
        color[y*320+x]^=uint16_t(0xb72d+frame*17);
        std::vector<uint8_t> raw;
        if(!(frame%8)){
            raw.resize(color.size()*2);std::memcpy(raw.data(),color.data(),raw.size());
        }else for(unsigned row=0;row<240;++row){
            word(raw,row==y);
            if(row==y){word(raw,x);word(raw,1);raw.push_back(0);word(raw,color[row*320+x]);}
        }
        source.images.push_back(raw);source.full.push_back(color);
    }
    return source;
}
static void equal(const delta::Raster& raster,const Source& source,unsigned frame){
    CHECK(!std::memcmp(raster.pixels,source.full[frame].data(),sizeof raster.pixels));
}
int main(){
    auto source=make_source();static delta::Raster raster;
    CHECK(sizeof(SceneRasterStorage)==sizeof(MerchantRugRaster));
    for(unsigned frame=0;frame<32;++frame){
        const auto reads=source.reads;CHECK(raster.restore(source,0,frame));
        const unsigned required=frame&&!(frame%8)?2:1;
        CHECK(source.reads==reads+required);equal(raster,source,frame);
        CHECK(raster.restore(source,0,frame));CHECK(source.reads==reads+required);
    }
    for(unsigned frame:{0u,7u,15u,8u,31u,2u,16u,23u,22u}){
        const auto reads=source.reads;CHECK(raster.restore(source,0,frame));
        CHECK(source.reads-reads<=delta::KeyInterval+1);equal(raster,source,frame);
    }
    // A reopened Content at the same address has a new generation. The key
    // must be loaded again before any patch can observe the previous owner.
    ++source.epoch;auto reads=source.reads;CHECK(raster.restore(source,0,22));
    CHECK(source.reads-reads==8);equal(raster,source,22);
    Source other=make_source();reads=other.reads;CHECK(raster.restore(other,0,7));
    CHECK(other.reads-reads==8);equal(raster,other,7);
    reads=other.reads;CHECK(raster.restore(other,8,7));CHECK(other.reads-reads==8);equal(raster,other,15);
    // Failed frame publication cannot leave a partially reconstructed frame
    // marked valid. Re-entering at any time starts with a complete key.
    CHECK(!raster.restore(other,64,3));CHECK(!raster.valid);
    CHECK(raster.restore(other,0,3));equal(raster,other,3);
    Source relative=make_source();
    for(unsigned frame=8;frame<32;frame+=8){
        auto& raw=relative.images[frame];raw.clear();
        for(unsigned y=0;y<240;++y){
            unsigned count=0;for(unsigned x=0;x<320;++x)
                count+=relative.full[frame][y*320+x]!=relative.full[0][y*320+x];
            word(raw,count);
            for(unsigned x=0;x<320;++x)if(relative.full[frame][y*320+x]!=relative.full[0][y*320+x]){
                word(raw,x);word(raw,1);raw.push_back(0);word(raw,relative.full[frame][y*320+x]);
            }
        }
    }
    for(unsigned frame=0;frame<32;++frame){CHECK(raster.restore(relative,0,frame));equal(raster,relative,frame);}
    for(unsigned frame:{31u,0u,23u,8u,19u,16u,15u,4u}){
        reads=relative.reads;CHECK(raster.restore(relative,0,frame));
        CHECK(relative.reads-reads<=9);equal(raster,relative,frame);
    }
    auto& scene=character_selection_raster();CHECK(!scene.valid);
    CHECK(scene.restore(source,0,6));equal(scene,source,6);
    (void)random_gradient_cache();CHECK(scene_raster_owner==SceneRasterOwner::Random);
    CHECK(!character_selection_raster().valid);
    CHECK(character_selection_raster().restore(source,0,6));equal(character_selection_raster(),source,6);
    (void)merchant_rug_raster();CHECK(!character_selection_raster().valid);
    CHECK(character_selection_raster().restore(source,0,31));equal(character_selection_raster(),source,31);
    (void)selected_hand_raster();CHECK(!character_selection_raster().valid);
    CHECK(character_selection_raster().restore(source,0,1));equal(character_selection_raster(),source,1);
    std::printf("selection delta: %u checks; sequential, seek, reopen and scene eviction exact\n",checks);
}
