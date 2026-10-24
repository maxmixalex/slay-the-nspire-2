#include "resource_source.hpp"
#include "frontend_assets.hpp"
#include "ui6_assets.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Bytes=std::vector<uint8_t>;
static void put(Bytes& b,unsigned at,uint32_t n){for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(n>>(8*i));}
static uint32_t get(const Bytes& b,unsigned at){return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static uint32_t hash(const uint8_t*p,unsigned n,uint32_t v=2166136261u){while(n--){v^=*p++;v*=16777619u;}return v;}
static void header_hash(Bytes& b,bool common){const unsigned at=common?28:44,end=common?64:96;put(b,at,hash(b.data()+at+4,end-at-4,hash(b.data(),at)));}
struct Fixture {
    unsigned prefix=40+16+16+6111*8,art=137,suffix=41;
    Bytes original,common,locale;
    explicit Fixture(bool chinese=false,unsigned version=17){
        original.resize(prefix+art+suffix);
        for(unsigned i=40;i<original.size();++i)original[i]=uint8_t(i*71u+(i>>5));
        std::memcpy(original.data(),"NSP2",4);put(original,4,version);put(original,8,0x41cef1ea);put(original,12,original.size());
        put(original,16,1);put(original,20,1);put(original,24,6111);put(original,28,1);put(original,32,1);
        const unsigned marker=prefix+art+7,row=40+16+16+6110*8;
        put(original,row,marker);put(original,row+4,3);std::memcpy(original.data()+marker,chinese?"zhs":"eng",4);
        put(original,36,hash(original.data()+40,original.size()-40));
        common.resize(64+art);std::memcpy(common.data(),"NSC1",4);put(common,4,1);put(common,8,0x41cef1ea);
        put(common,12,64);put(common,16,common.size());put(common,20,art);
        for(unsigned i=0;i<32;++i)common[32+i]=uint8_t(7*i+3); // Historical optional build identity; no runtime gate.
        std::memcpy(common.data()+64,original.data()+prefix,art);header_hash(common,true);
        locale.resize(96+prefix+suffix);std::memcpy(locale.data(),"NSL1",4);put(locale,4,1);put(locale,8,0x41cef1ea);
        put(locale,12,96);put(locale,16,locale.size());put(locale,20,original.size());put(locale,24,prefix);
        put(locale,28,art);put(locale,32,suffix);put(locale,36,chinese?2:1);put(locale,40,get(original,36));
        std::memcpy(locale.data()+48,common.data()+32,32);
        std::memcpy(locale.data()+96,original.data(),prefix);
        std::memcpy(locale.data()+96+prefix,original.data()+prefix+art,suffix);header_hash(locale,false);
    }
};
struct File {
    const Bytes* bytes{};unsigned calls{},read_bytes{},progress_calls{};int64_t fail_at=-1;
    static bool read(void* c,uint32_t at,void*out,uint32_t n){
        auto& f=*static_cast<File*>(c);++f.calls;f.read_bytes+=n;
        if(at>f.bytes->size()||n>f.bytes->size()-at||(f.fail_at>=int64_t(at)&&f.fail_at<int64_t(at)+n))return false;
        std::memcpy(out,f.bytes->data()+at,n);return true;
    }
    static void progress(void*c,ResourceStage,uint32_t,uint32_t){++static_cast<File*>(c)->progress_calls;}
    ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr,progress};}
};
static void checks_for(bool chinese,bool streamed,unsigned version){
    Fixture f(chinese,version);File common{&f.common},locale{&f.locale};SplitResourceSource joined;
    const auto lang=chinese?ResourceLanguage::Chinese:ResourceLanguage::English;
    CHECK(joined.open(streamed?common.source():ResourceSource::memory(f.common.data(),f.common.size()),
                      streamed?locale.source():ResourceSource::memory(f.locale.data(),f.locale.size()),lang));
    CHECK(joined.valid()&&joined.language()==lang&&joined.source().direct==nullptr);
    if(streamed){CHECK(common.read_bytes==64);CHECK(locale.read_bytes==96+40+8+4);} // No extra full payload scan.
    for(unsigned start:{0u,39u,f.prefix-7,f.prefix,f.prefix+f.art-7,f.prefix+f.art,unsigned(f.original.size())}){
        for(unsigned n:{0u,1u,7u,8u,61u,4096u,unsigned(f.original.size())}){
            if(n>f.original.size()-start)continue;
            Bytes b(n);CHECK(joined.read(start,b.data(),n));CHECK(!n||std::memcmp(b.data(),f.original.data()+start,n)==0);
        }
    }
    Bytes all(f.original.size());CHECK(joined.read(0,all.data(),all.size()));CHECK(all==f.original);
    CHECK(joined.read(f.original.size(),nullptr,0));CHECK(!joined.read(f.original.size()+1,nullptr,0));
    CHECK(!joined.read(f.original.size(),all.data(),1));CHECK(!joined.read(0xffffffffu,all.data(),1));CHECK(!joined.read(0,nullptr,1));
    ResourceReader reader(joined.source());
    reader.progress(ResourceStage::Directory,1,1);
    if(streamed)CHECK(locale.progress_calls==1&&common.progress_calls==0);
    // Payload bytes are read as stored; historical pack checksums do not
    // trigger a full-file traversal in either the adapter or the reader.
    f.common[64+13]^=0x80;CHECK(reader.reset(joined.source()));uint8_t changed=0;
    CHECK(reader.u8(f.prefix+13,changed));CHECK(changed==f.common[64+13]);
    joined.close();CHECK(!joined.valid()&&!joined.source().read_at&&!joined.read(0,nullptr,0));
}
static void malformed(){
    Fixture f;SplitResourceSource joined;
    const auto open=[&](){return joined.open(ResourceSource::memory(f.common.data(),f.common.size()),ResourceSource::memory(f.locale.data(),f.locale.size()),ResourceLanguage::English);};
    CHECK(open());
    for(unsigned at:{0u,4u,8u,12u,16u,20u}){
        f.common[at]^=1;CHECK(!open());CHECK(!joined.valid());f.common[at]^=1;
    }
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,36u}){
        f.locale[at]^=1;CHECK(!open());f.locale[at]^=1;
    }
    // Legacy checksums, identity digests and reserved flags do not affect
    // mapping. This retains compatibility with existing split resources.
    for(unsigned at:{24u,28u,32u,63u}){
        f.common[at]^=1;CHECK(open());f.common[at]^=1;
    }
    for(unsigned at:{40u,44u,48u,79u,80u,95u,132u}){
        f.locale[at]^=1;CHECK(open());f.locale[at]^=1;
    }
    // Mapping overflow and actual language mismatches remain invalid.
    for(unsigned at:{20u,24u,28u,32u}){Fixture bad;put(bad.locale,at,0xffffffffu);
        CHECK(!joined.open(ResourceSource::memory(bad.common.data(),bad.common.size()),ResourceSource::memory(bad.locale.data(),bad.locale.size()),ResourceLanguage::English));}
    const unsigned marker=96+f.prefix+7;f.locale[marker]='z';CHECK(!open());f.locale[marker]='e';
    put(f.locale,36,2);CHECK(!open());
    // A relabeled header must agree with the actual original marker as well.
    CHECK(!joined.open(ResourceSource::memory(f.common.data(),f.common.size()),ResourceSource::memory(f.locale.data(),f.locale.size()),ResourceLanguage::Chinese));
    f=Fixture();CHECK(!joined.open(ResourceSource::memory(f.common.data(),f.common.size()-1),ResourceSource::memory(f.locale.data(),f.locale.size()),ResourceLanguage::English));
    {Fixture future(false,Ui6ContentVersion+1);CHECK(!joined.open(ResourceSource::memory(future.common.data(),future.common.size()),ResourceSource::memory(future.locale.data(),future.locale.size()),ResourceLanguage::English));}
    File c{&f.common},l{&f.locale};CHECK(joined.open(c.source(),l.source(),ResourceLanguage::English));
    c.fail_at=64+10;uint8_t out[64];CHECK(!joined.read(f.prefix-4,out,32));
    c.fail_at=-1;l.fail_at=96+f.prefix+2;CHECK(!joined.read(f.prefix+f.art-4,out,16));
    l.fail_at=-1;CHECK(joined.read(f.prefix+f.art-4,out,16));
    CHECK(!joined.open({},l.source(),ResourceLanguage::English));
}
struct Disk {
    FILE* file{};uint32_t length{};
    explicit Disk(const char* path){file=std::fopen(path,"rb");CHECK(file);CHECK(!std::fseek(file,0,SEEK_END));const long n=std::ftell(file);CHECK(n>0&&uint64_t(n)<=0xffffffffu);length=uint32_t(n);}
    ~Disk(){if(file)std::fclose(file);}
    static bool read(void*c,uint32_t at,void*out,uint32_t n){auto& d=*static_cast<Disk*>(c);return at<=d.length&&n<=d.length-at&&!std::fseek(d.file,long(at),SEEK_SET)&&std::fread(out,1,n,d.file)==n;}
    ResourceSource source(){return {this,length,read};}
};
static void actual(const char* common,const char* locale,const char* original,const char* language){
    CHECK(!std::strcmp(language,"zhs")||!std::strcmp(language,"eng"));
    Disk a(common),b(locale),expected(original);SplitResourceSource joined;
    const auto lang=!std::strcmp(language,"zhs")?ResourceLanguage::Chinese:ResourceLanguage::English;
    CHECK(joined.open(a.source(),b.source(),lang));CHECK(joined.source().length==expected.length);
    Bytes got(65536),want(65536);for(uint32_t at=0;at<expected.length;){const uint32_t n=expected.length-at<got.size()?expected.length-at:uint32_t(got.size());
        CHECK(joined.read(at,got.data(),n));CHECK(Disk::read(&expected,at,want.data(),n));CHECK(!std::memcmp(got.data(),want.data(),n));at+=n;}
    std::printf("actual %s: %u virtual bytes identical to the original full pack\n",language,expected.length);
}
int main(int argc,char**argv){
    if(argc==5)actual(argv[1],argv[2],argv[3],argv[4]);
    else if(argc!=1){std::fprintf(stderr,"usage: split_resource_regression [common locale original eng|zhs]\n");return 2;}
    // The published split format must support the same latest NSP2 catalog
    // as the full-pack runtime, across both languages and source adapters.
    for(unsigned version=17;version<=Ui6ContentVersion;++version)for(bool chinese:{false,true})for(bool streamed:{false,true})checks_for(chinese,streamed,version);
    malformed();
    std::printf("split resources: %u checks passed; adapter=%zu bytes\n",checks,sizeof(SplitResourceSource));
}
