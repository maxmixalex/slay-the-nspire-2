#include "resource_file_cache.hpp"
#include "storage_source.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"resource files:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Cache=ResourceFileCache<8,4>;
struct FakeHost {
    struct File {FakeHost*host{};unsigned id{},position{};bool open=false;};
    struct Event {char kind;unsigned id;};
    std::array<std::vector<uint8_t>,16> bytes;
    std::array<File,16> handles{};
    std::array<unsigned,16> opens{},seeks{},reads{};
    std::array<bool,16> present{};
    std::vector<Event> events;
    std::vector<bool> captured;
    unsigned limit=7,live=0,peak=0,refused=0,closed=0,polls=0;
    int fail_open=-1,fail_seek=-1,fail_tell=-1,short_read=-1,fail_close=-1;
    bool input=false,edge_on_open=false,edge_on_close=false;
    FakeHost(){
        present.fill(true);
        for(unsigned id=0;id<bytes.size();++id){bytes[id].resize(512+id*17);
            for(unsigned at=0;at<bytes[id].size();++at)bytes[id][at]=uint8_t(id*43+at*7+(at>>3));}
    }
    static void*open(void*context,unsigned id){
        auto&h=*static_cast<FakeHost*>(context);h.events.push_back({'o',id});
        if(h.edge_on_open){h.input=true;h.edge_on_open=false;}
        if(id>=h.bytes.size()||!h.present[id]||h.live==h.limit||h.fail_open==int(id)){
            if(h.fail_open==int(id))h.fail_open=-1;++h.refused;return nullptr;
        }
        // Reuse the first free address, as a FILE implementation may do.
        for(auto&f:h.handles)if(!f.open){f={&h,id,0,true};++h.opens[id];++h.live;h.peak=std::max(h.peak,h.live);return &f;}
        CHECK(false);return nullptr;
    }
    static int close(void*file){
        auto&f=*static_cast<File*>(file);CHECK(f.open);auto&h=*f.host;h.events.push_back({'c',f.id});
        if(h.edge_on_close){h.input=false;h.edge_on_close=false;}
        --h.live;++h.closed;f.open=false;
        if(h.fail_close==int(f.id)){h.fail_close=-1;return -1;}return 0;
    }
    static int seek(void*file,long offset,int origin){
        auto&f=*static_cast<File*>(file);CHECK(f.open);auto&h=*f.host;h.events.push_back({'s',f.id});++h.seeks[f.id];
        if(h.fail_seek==int(f.id)){h.fail_seek=-1;return -1;}
        const long position=(origin==2?long(h.bytes[f.id].size()):origin==1?long(f.position):0)+offset;
        if(position<0||static_cast<unsigned long>(position)>h.bytes[f.id].size())return -1;
        f.position=unsigned(position);return 0;
    }
    static long tell(void*file){
        auto&f=*static_cast<File*>(file);CHECK(f.open);auto&h=*f.host;h.events.push_back({'t',f.id});
        if(h.fail_tell==int(f.id)){h.fail_tell=-1;return -1;}return f.position;
    }
    static unsigned get(void*out,unsigned size,unsigned count,void*file){
        auto&f=*static_cast<File*>(file);CHECK(f.open&&size==1);auto&h=*f.host;h.events.push_back({'r',f.id});++h.reads[f.id];
        unsigned n=std::min(count,unsigned(h.bytes[f.id].size()-f.position));
        if(h.short_read==int(f.id)){h.short_read=-1;if(n)--n;}
        if(n)std::memcpy(out,h.bytes[f.id].data()+f.position,n);f.position+=n;return n;
    }
    static void poll(void*context){auto&h=*static_cast<FakeHost*>(context);++h.polls;h.events.push_back({'p',0});
        if(h.captured.empty()||h.captured.back()!=h.input)h.captured.push_back(h.input);}
    Cache::Host host(){return {this,open,close,seek,tell,get,poll};}
    bool held(unsigned id)const{for(const auto&f:handles)if(f.open&&f.id==id)return true;return false;}
    void polling_edges()const{
        for(unsigned i=0;i<events.size();++i)if((events[i].kind=='o'||events[i].kind=='c')&&events[i].id<8)
            CHECK(i&&i+1<events.size()&&events[i-1].kind=='p'&&events[i+1].kind=='p');
    }
};
static bool get(ResourceSource s,uint32_t at,void*out,uint32_t count){
    if(at>s.length||count>s.length-at||(count&&!out))return false;
    if(s.direct){if(count)std::memcpy(out,s.direct+at,count);return true;}
    return s.read_at&&s.read_at(s.context,at,out,count);
}
static void read(Cache&cache,FakeHost&host,unsigned part,unsigned at,unsigned n,bool expect_seek){
    std::vector<uint8_t> out(n);bool sought=false;CHECK(cache.read(part,at,out.data(),n,sought));CHECK(sought==expect_seek);
    CHECK(std::equal(out.begin(),out.end(),host.bytes[part].begin()+at));
}
static void sequential_probes(){
    FakeHost host;host.limit=1;Cache cache;
    CHECK(cache.open(host.host(),1024));CHECK(!host.live&&host.peak==1&&host.closed==8&&host.polls==32);
    for(unsigned i=0;i<8;++i)CHECK(cache.source(i).length==host.bytes[i].size()&&host.opens[i]==1);
    CHECK(!cache.source(8).length);host.polling_edges();
    cache.close();CHECK(!host.live&&!cache.source(0).length);
    host.present[5]=false;CHECK(!cache.open(host.host(),1024));CHECK(!host.live&&!cache.source(0).length);
    host.present[5]=true;CHECK(cache.open(host.host(),1024));cache.close();
    host.fail_seek=4;CHECK(!cache.open(host.host(),1024));CHECK(!host.live);
    host.fail_tell=2;CHECK(!cache.open(host.host(),1024));CHECK(!host.live);
    host.fail_close=3;CHECK(!cache.open(host.host(),1024));CHECK(!host.live);
    const auto old=host.bytes[1];host.bytes[1].clear();CHECK(!cache.open(host.host(),1024));CHECK(!host.live);
    host.bytes[1]=old;CHECK(!cache.open(host.host(),100));CHECK(!host.live);
    CHECK(cache.open(host.host(),1024));CHECK(!cache.open({},1024));CHECK(!host.live);
}
static void cursor_lru_failures(){
    FakeHost host;Cache cache;CHECK(cache.open(host.host(),1024));
    for(unsigned i=0;i<4;++i)read(cache,host,i,40,12,true);
    CHECK(host.live==4);const unsigned seek0=host.seeks[0];read(cache,host,0,52,9,false);CHECK(host.seeks[0]==seek0);
    read(cache,host,4,40,12,true);CHECK(host.held(0)&&!host.held(1)&&host.held(2)&&host.held(3)&&host.held(4));
    read(cache,host,1,52,9,true);CHECK(host.held(1)&&!host.held(2)); // Same offset and reused FILE address still require seek.
    read(cache,host,3,52,9,false);CHECK(host.live==4&&host.peak==4);
    host.fail_open=7;bool sought=false;uint8_t out[24]{};CHECK(!cache.read(7,20,out,12,sought));CHECK(!sought&&host.live==3);
    read(cache,host,7,20,12,true);CHECK(host.live==4);
    host.short_read=7;CHECK(!cache.read(7,32,out,12,sought));CHECK(!sought&&!host.held(7)&&host.live==3);
    read(cache,host,7,32,12,true); // A short read closes the poisoned stream and retries freshly.
    host.fail_seek=7;CHECK(!cache.read(7,3,out,12,sought));CHECK(sought&&!host.held(7));read(cache,host,7,3,12,true);
    // A close error consumes the handle but aborts that replacement; retry works.
    unsigned victim=0;for(unsigned id:{1u,3u,7u}){read(cache,host,id,70,2,true);}
    for(unsigned id=0;id<8;++id)if(host.held(id)&&id!=1&&id!=3&&id!=7)victim=id;
    host.fail_close=int(victim);CHECK(!cache.read(6,12,out,12,sought));CHECK(!host.held(victim));read(cache,host,6,12,12,true);
    const auto live=host.live,opens=host.opens,old_seeks=host.seeks;
    CHECK(cache.read(6,uint32_t(host.bytes[6].size()),nullptr,0,sought));CHECK(!sought);
    CHECK(!cache.read(6,uint32_t(host.bytes[6].size())+1,nullptr,0,sought));
    CHECK(!cache.read(6,0,nullptr,1,sought));CHECK(!cache.read(8,0,out,1,sought));
    CHECK(!cache.read(6,UINT32_MAX,out,2,sought));CHECK(!cache.read(6,1,out,UINT32_MAX,sought));
    CHECK(host.live==live&&host.opens==opens&&host.seeks==old_seeks);
    const auto stale=cache.source(6);cache.close();CHECK(!host.live&&!get(stale,0,out,1));
    const auto closed=host.closed,polls=host.polls;cache.close();CHECK(host.closed==closed&&host.polls==polls);
    CHECK(cache.open(host.host(),1024));read(cache,host,6,24,12,true);cache.close();host.polling_edges();
}
static void word(std::vector<uint8_t>&out,unsigned at,uint32_t value){for(unsigned i=0;i<4;++i)out[at+i]=uint8_t(value>>(8*i));}
struct MultipartFixture {
    FakeHost host;
    std::vector<uint8_t> index,canonical;
    MultipartFixture(){
        const unsigned order[]={0,1,2,3,4,5,6,7,0,3,6,2,5,1,4,7};
        index.resize(96+16*16);canonical.resize(64);
        for(unsigned i=0;i<8;++i)host.bytes[i].assign(32,0);
        for(unsigned span=0;span<16;++span){const unsigned part=order[span],n=113+span*17,first=unsigned(canonical.size()),physical=unsigned(host.bytes[part].size());
            for(unsigned i=0;i<n;++i){const uint8_t b=uint8_t(part*47+span*29+i*7);canonical.push_back(b);host.bytes[part].push_back(b);}
            word(index,96+span*16,first);word(index,100+span*16,unsigned(canonical.size()));word(index,104+span*16,physical);word(index,108+span*16,part);
        }
        word(canonical,0,0x3143534e);word(canonical,4,1);word(canonical,8,0x41cef1ea);word(canonical,12,64);
        word(canonical,16,unsigned(canonical.size()));word(canonical,20,unsigned(canonical.size())-64);
        word(index,0,0x314d534e);word(index,4,1);word(index,8,0x41cef1ea);word(index,12,96);word(index,16,unsigned(index.size()));
        word(index,20,unsigned(canonical.size()));word(index,24,16);word(index,28,8);std::memcpy(index.data()+32,canonical.data(),64);
        for(unsigned i=0;i<8;++i){auto&p=host.bytes[i];word(p,0,0x3153534e);word(p,4,1);word(p,8,0x41cef1ea);word(p,12,32);word(p,16,unsigned(p.size()));word(p,20,i);}
        host.bytes[11]=index;
    }
};
static void multipart_and_languages(){
    MultipartFixture fixture;auto&host=fixture.host;Cache cache;static MultipartResourceSource common;
    void*manifest=FakeHost::open(&host,11);CHECK(manifest);
    CHECK(cache.open(host.host(),2048));CHECK(host.live==1&&host.peak==2); // Sequential probes plus the index.
    ResourceSource parts[8];for(unsigned i=0;i<8;++i)parts[i]=cache.source(i);
    CHECK(common.open({nullptr,uint32_t(fixture.index.size()),nullptr,fixture.index.data()},parts,8));
    CHECK(host.live==5);CHECK(FakeHost::close(manifest)==0);CHECK(host.live==4);
    // No input/index object is retained by MultipartResourceSource after open.
    std::fill(fixture.index.begin(),fixture.index.end(),0);
    std::vector<uint8_t> out(fixture.canonical.size());CHECK(get(common.source(),0,out.data(),unsigned(out.size())));CHECK(out==fixture.canonical);
    void*english=FakeHost::open(&host,8);CHECK(english);ResourceFileCursor english_cursor;bool sought=false;uint8_t text[16];
    CHECK(english_cursor.read(english,37,text,8,FakeHost::seek,FakeHost::get,sought)&&sought);
    host.fail_open=9;CHECK(!FakeHost::open(&host,9));CHECK(host.held(8)); // Failed language opens preserve the selected stream.
    void*chinese=FakeHost::open(&host,9);CHECK(chinese);ResourceFileCursor chinese_cursor;
    CHECK(chinese_cursor.read(chinese,71,text,8,FakeHost::seek,FakeHost::get,sought)&&sought);
    void*state=FakeHost::open(&host,10);CHECK(state&&host.live==7);
    const unsigned eng_seeks=host.seeks[8],zhs_seeks=host.seeks[9];
    for(unsigned at=0;at<fixture.canonical.size();at+=257){
        const unsigned n=std::min(769u,unsigned(fixture.canonical.size())-at);std::vector<uint8_t> slice(n);
        CHECK(get(common.source(),at,slice.data(),n));CHECK(std::equal(slice.begin(),slice.end(),fixture.canonical.begin()+at));CHECK(host.live<=7);
    }
    CHECK(english_cursor.read(english,45,text,8,FakeHost::seek,FakeHost::get,sought)&&!sought);
    CHECK(std::equal(text,text+8,host.bytes[8].begin()+45));
    CHECK(chinese_cursor.read(chinese,79,text,8,FakeHost::seek,FakeHost::get,sought)&&!sought);
    CHECK(std::equal(text,text+8,host.bytes[9].begin()+79));CHECK(host.seeks[8]==eng_seeks&&host.seeks[9]==zhs_seeks);
    CHECK(host.peak==7&&host.refused==1); // Only the injected language failure, never the host handle limit.
    CHECK(FakeHost::close(state)==0);CHECK(FakeHost::close(english)==0); // Commit the language switch.
    CHECK(get(common.source(),0,out.data(),unsigned(out.size()))&&out==fixture.canonical);
    host.short_read=0;CHECK(!get(common.source(),64,out.data(),100));
    CHECK(get(common.source(),64,out.data(),100));CHECK(std::equal(out.begin(),out.begin()+100,fixture.canonical.begin()+64));
    cache.close();CHECK(host.live==1&&host.held(9)); // Artwork shutdown does not close the locale.
    CHECK(FakeHost::close(chinese)==0);common.close();CHECK(!host.live);
    const unsigned close_count=host.closed,poll_count=host.polls;cache.close();CHECK(host.closed==close_count&&host.polls==poll_count);host.polling_edges();
}
static void capture_and_scope_shutdown(){
    FakeHost host;unsigned closed=0,polls=0;
    {
        Cache cache;CHECK(cache.open(host.host(),1024));host.captured.clear();host.edge_on_open=true;
        read(cache,host,0,20,12,true);CHECK(host.captured.size()==2&&!host.captured[0]&&host.captured[1]);
        host.edge_on_close=true;cache.close();CHECK(host.captured.size()==3&&!host.captured.back()&&!host.live);
        closed=host.closed;polls=host.polls;
    }
    CHECK(host.closed==closed&&host.polls==polls); // No filesystem calls after the explicit release gate.
    {Cache cache;CHECK(cache.open(host.host(),1024));read(cache,host,3,1,1,true);CHECK(host.live==1);}
    CHECK(!host.live);host.polling_edges();
}
int main(){sequential_probes();cursor_lru_failures();multipart_and_languages();capture_and_scope_shutdown();
    std::printf("resource file cache: %u checks passed; 8 parts, 4 slots, 7-stream host ceiling\n",checks);}
