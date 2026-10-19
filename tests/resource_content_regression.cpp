#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"check %u line %d: %s\n",checks,__LINE__,#x);std::exit(1);}}while(0)
static uint32_t word(const uint8_t*p){return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
static uint16_t half(const uint8_t*p){return p[0]|uint16_t(p[1])<<8;}
struct Provider {const std::vector<uint8_t>* bytes;uint32_t fault=UINT32_MAX;unsigned calls=0;};
static bool read_at(void*context,uint32_t offset,void*out,uint32_t count){
    auto&v=*(Provider*)context;++v.calls;
    if(offset>v.bytes->size()||count>v.bytes->size()-offset||(offset<=v.fault&&count>v.fault-offset))return false;
    std::memcpy(out,v.bytes->data()+offset,count);return true;
}
static std::vector<uint8_t> picture_bytes(Picture p,unsigned size){
    CHECK(p.pixels);const uint8_t*start=p.format>=2&&p.format<4?p.palette:p.pixels;CHECK(start);
    return {start,start+size};
}
int main(int argc,char**argv){
    if(argc!=2){std::fprintf(stderr,"Usage: resource_content_regression REAL_PACK\n");return 2;}
    FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(std::fseek(f,0,SEEK_END)==0);long length=std::ftell(f);CHECK(length>40);CHECK(std::fseek(f,0,SEEK_SET)==0);
    std::vector<uint8_t>b((size_t)length);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(std::fclose(f)==0);
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>();Provider source{&b};
    CHECK(memory->open(b.data(),b.size()));CHECK(ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));
    const unsigned images=word(b.data()+16),glyphs=word(b.data()+20),strings=word(b.data()+24),clips=word(b.data()+28);
    for(unsigned id=0;id<images;id++){
        const uint8_t*r=b.data()+40+id*16;unsigned size=word(r+8)==5?word(b.data()+word(r)+1):word(r+12);
        Picture a=memory->image(id);auto expected=picture_bytes(a,size);Picture c=ranged->image(id);auto actual=picture_bytes(c,size);
        CHECK(a.w==c.w&&a.h==c.h&&a.format==c.format);CHECK(expected==actual);
    }
    for(unsigned id=0;id<glyphs;id++){
        const uint8_t*r=b.data()+40+images*16+id*16;unsigned cp=word(r),size=half(r+8),n=half(r+14)?half(r+14):unsigned(r[10])*r[11];
        Glyph a=memory->glyph(cp,size),c=ranged->glyph(cp,size);CHECK(a.pixels&&c.pixels);CHECK(a.w==c.w&&a.h==c.h&&a.advance==c.advance&&a.encoded_size==c.encoded_size);CHECK(std::memcmp(a.pixels,c.pixels,n)==0);
    }
    for(unsigned id=0;id<strings;id++)CHECK(std::strcmp(memory->string(id),ranged->string(id))==0);
    for(unsigned id=0;id<clips;id++){Clip a=memory->clip(id),c=ranged->clip(id);CHECK(a.first==c.first&&a.count==c.count&&a.frame_ms==c.frame_ms&&a.pivot_x==c.pivot_x&&a.pivot_y==c.pivot_y);}
    CHECK(memory->healthy()&&ranged->healthy());CHECK(ranged->read_stats().callback_bytes>b.size());
    // Alternate two independently backed Content objects through identical UI
    // inputs. Shared bounded picture caches must not leak a different source.
    auto a=std::make_unique<App>(),c=std::make_unique<App>();CHECK(a->init(b.data(),b.size(),{}));CHECK(c->init({&source,(uint32_t)b.size(),read_at,nullptr},{}));
    const Key keys[]={Key::Confirm,Key::Right,Key::Right,Key::Confirm,Key::Confirm,Key::Collection,Key::Right,Key::Right,Key::Detail,Key::Back,Key::Back,Key::Map,Key::Back};
    unsigned frames=0;
    auto compare=[&](){a->render();c->render();CHECK(std::memcmp(a->pixels,c->pixels,sizeof a->pixels)==0);CHECK(a->content.healthy()&&c->content.healthy());++frames;};
    compare();for(Key k:keys){a->input(k);c->input(k);for(int t=0;t<3;t++){a->tick();c->tick();compare();}}
    // A callback refusing an advertised range must reject opening the file;
    // reopening after the provider is restored must discard stale cache data.
    source.fault=(uint32_t)b.size()-1;CHECK(!ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));CHECK(!ranged->healthy());
    source.fault=UINT32_MAX;CHECK(ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));
    CHECK(!ranged->open({&source,(uint32_t)b.size()+1,read_at,nullptr}));CHECK(!ranged->healthy());
    std::printf("%u checks; %u images, %u glyphs, %u strings, %u clips and %u identical memory/range UI frames; callback reads are bounded\n",checks,images,glyphs,strings,clips,frames);
}
