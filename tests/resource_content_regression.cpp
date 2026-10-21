#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
#include <string>
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
static std::vector<uint8_t> read_file(const char* path){
    FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(std::fseek(f,0,SEEK_END)==0);
    const long length=std::ftell(f);CHECK(length>40&&uint64_t(length)<=MaxContentFile);
    CHECK(std::fseek(f,0,SEEK_SET)==0);std::vector<uint8_t>b((size_t)length);
    CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(std::fclose(f)==0);return b;
}
static std::vector<uint8_t> picture_bytes(Picture p){
    // Inspect the decoded view, never the encoded directory length: format 6
    // has a 16-byte descriptor even when its reconstructed image is much larger.
    CHECK(p.pixels&&p.w&&p.h&&p.w<=320&&p.h<=240&&p.format<=4);
    const uint8_t*start=p.format>=2&&p.format<4?p.palette:p.pixels;CHECK(start);
    unsigned size=0;
    if(p.format<4)size=p.format<2?p.w*p.h*(p.format?3u:2u):512+p.w*p.h*(p.format==3?2u:1u);
    else {
        constexpr unsigned limit=320u*240u*8u+2u*240u;
        for(unsigned y=0;y<p.h;y++){
            CHECK(size<=limit-2);const unsigned spans=half(start+size);size+=2;CHECK(spans<=p.w);
            unsigned last=0;
            for(unsigned j=0;j<spans;j++){
                CHECK(size<=limit-5);const unsigned x=half(start+size),count=half(start+size+2),mode=start[size+4];size+=5;
                CHECK(count&&mode<=1&&x>=last&&x+count<=p.w);last=x+count;
                const unsigned bytes=count*(mode?3u:2u);CHECK(bytes<=limit-size);size+=bytes;
            }
        }
    }
    return {start,start+size};
}
static std::vector<uint8_t> glyph_bytes(Glyph p){
    CHECK(p.pixels);const unsigned n=p.encoded_size?p.encoded_size:unsigned(p.w)*p.h;
    return {p.pixels,p.pixels+n};
}
int main(int argc,char**argv){
    if(argc!=2&&argc!=3){std::fprintf(stderr,"Usage: resource_content_regression REAL_PACK [EQUIVALENT_REFERENCE_PACK]\n");return 2;}
    const auto b=read_file(argv[1]);std::vector<uint8_t> reference_bytes;
    std::unique_ptr<Content> reference;
    if(argc==3){reference_bytes=read_file(argv[2]);reference=std::make_unique<Content>();CHECK(reference->open(reference_bytes.data(),reference_bytes.size()));}
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>();Provider source{&b};
    CHECK(memory->open(b.data(),b.size()));CHECK(ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));
    const unsigned images=word(b.data()+16),glyphs=word(b.data()+20),strings=word(b.data()+24),clips=word(b.data()+28);
    if(reference){
        // Cross-format comparisons require the complete, identical catalog;
        // offsets and storage versions may differ, semantic IDs may not.
        CHECK(memory->current_profile()&&ranged->current_profile()&&reference->current_profile());
        for(unsigned at=16;at<=32;at+=4)CHECK(word(b.data()+at)==word(reference_bytes.data()+at));
    }
    uint64_t decoded_image_bytes=0;
    for(unsigned id=0;id<images;id++){
        Picture a=memory->image(id);auto expected=picture_bytes(a);Picture c=ranged->image(id);auto actual=picture_bytes(c);
        CHECK(a.w==c.w&&a.h==c.h&&a.format==c.format);CHECK(expected==actual);
        decoded_image_bytes+=expected.size();
        if(reference){Picture r=reference->image(id);auto original=picture_bytes(r);CHECK(a.w==r.w&&a.h==r.h&&a.format==r.format);CHECK(expected==original);}
    }
    for(unsigned id=0;id<glyphs;id++){
        const uint8_t*r=b.data()+40+images*16+id*16;const unsigned cp=word(r),size=half(r+8);
        Glyph a=memory->glyph(cp,size);auto expected=glyph_bytes(a);Glyph c=ranged->glyph(cp,size);auto actual=glyph_bytes(c);
        CHECK(a.w==c.w&&a.h==c.h&&a.advance==c.advance&&a.encoded_size==c.encoded_size);CHECK(expected==actual);
        if(reference){
            const uint8_t*rr=reference_bytes.data()+40+images*16+id*16;CHECK(word(rr)==cp&&half(rr+8)==size);
            Glyph g=reference->glyph(cp,size);auto original=glyph_bytes(g);CHECK(a.w==g.w&&a.h==g.h&&a.advance==g.advance&&a.encoded_size==g.encoded_size);CHECK(expected==original);
        }
    }
    for(unsigned id=0;id<strings;id++){const std::string expected=memory->string(id);CHECK(expected==ranged->string(id));if(reference)CHECK(expected==reference->string(id));}
    for(unsigned id=0;id<clips;id++){
        Clip a=memory->clip(id),c=ranged->clip(id);CHECK(a.first==c.first&&a.count==c.count&&a.frame_ms==c.frame_ms&&a.pivot_x==c.pivot_x&&a.pivot_y==c.pivot_y);
        if(reference){Clip r=reference->clip(id);CHECK(a.first==r.first&&a.count==r.count&&a.frame_ms==r.frame_ms&&a.pivot_x==r.pivot_x&&a.pivot_y==r.pivot_y);}
    }
    CHECK(memory->card_count()==ranged->card_count());
    for(unsigned id=0;id<memory->card_count();id++){
        const int image=memory->card_image(id);CHECK(image==ranged->card_image(id));if(reference)CHECK(image==reference->card_image(id));
        for(int variant=0;variant<3;variant++){const std::string expected=memory->card_string(id,variant);CHECK(expected==ranged->card_string(id,variant));if(reference)CHECK(expected==reference->card_string(id,variant));}
    }
    for(unsigned id=0;id<U_Count;id++){const int image=memory->ui_image(id);CHECK(image==ranged->ui_image(id));if(reference)CHECK(image==reference->ui_image(id));}
    CHECK(memory->healthy()&&ranged->healthy());CHECK(ranged->read_stats().callback_bytes>b.size());
    // Alternate two independently backed Content objects through identical UI
    // inputs. Shared bounded picture caches must not leak a different source.
    auto a=std::make_unique<App>(),c=std::make_unique<App>();CHECK(a->init(b.data(),b.size(),{}));CHECK(c->init({&source,(uint32_t)b.size(),read_at,nullptr},{}));
    std::unique_ptr<App> original;
    if(reference){original=std::make_unique<App>();CHECK(original->init(reference_bytes.data(),reference_bytes.size(),{}));}
    const Key keys[]={Key::Confirm,Key::Right,Key::Right,Key::Confirm,Key::Confirm,Key::Collection,Key::Right,Key::Right,Key::Detail,Key::Back,Key::Back,Key::Map,Key::Back};
    unsigned frames=0;
    auto compare=[&](){a->render();c->render();CHECK(std::memcmp(a->pixels,c->pixels,sizeof a->pixels)==0);CHECK(a->content.healthy()&&c->content.healthy());if(original){original->render();CHECK(std::memcmp(a->pixels,original->pixels,sizeof a->pixels)==0);CHECK(original->content.healthy());}++frames;};
    compare();for(Key k:keys){a->input(k);c->input(k);if(original)original->input(k);for(int t=0;t<3;t++){a->tick();c->tick();if(original)original->tick();compare();}}
    // A callback refusing an advertised range must reject opening the file;
    // reopening after the provider is restored must discard stale cache data.
    source.fault=(uint32_t)b.size()-1;CHECK(!ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));CHECK(!ranged->healthy());
    source.fault=UINT32_MAX;CHECK(ranged->open({&source,(uint32_t)b.size(),read_at,nullptr}));
    CHECK(!ranged->open({&source,(uint32_t)b.size()+1,read_at,nullptr}));CHECK(!ranged->healthy());
    if(reference)CHECK(reference->healthy());
    std::printf("%u checks; %u images (%llu decoded bytes), %u glyphs, %u strings, %u clips and %u identical memory/range UI frames%s; callback reads are bounded\n",checks,images,(unsigned long long)decoded_image_bytes,glyphs,strings,clips,frames,reference?"; all content and UI also match the independent reference pack":"");
}
