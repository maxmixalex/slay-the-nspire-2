// Storage adapters must preserve bytes without borrowing the foreground image
// workspace. Synthetic headers/blocks are independent of the Python writer.
#include "storage_source.hpp"
#include "resource_source.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"storage source:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
constexpr uint32_t B=BlockedResourceSource::BlockBytes,Mask=0x7fffffffu,Literal=0x80000000u;
static uint32_t word(const Bytes&b,unsigned at){CHECK(at+4<=b.size());return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static void put(Bytes&b,unsigned at,uint32_t n){CHECK(at+4<=b.size());for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(n>>(8*i));}
static void header(Bytes&b,const char*magic,uint32_t bytes){CHECK(b.size()>=32);std::memcpy(b.data(),magic,4);put(b,4,1);put(b,8,0x41cef1eau);put(b,12,bytes);}
struct Provider {
    const Bytes*bytes{};unsigned calls{},progress_calls{};uint32_t fail_at=UINT32_MAX,last_at{},last_size{};
    static bool read(void*p,uint32_t at,void*out,uint32_t n){
        auto&s=*static_cast<Provider*>(p);++s.calls;s.last_at=at;s.last_size=n;
        if(!s.bytes||at>s.bytes->size()||n>s.bytes->size()-at||(n&&(!out||uint64_t(at)+n>s.fail_at)))return false;
        if(n)std::memcpy(out,s.bytes->data()+at,n);return true;
    }
    static void progress(void*p,ResourceStage stage,uint32_t n,uint32_t total){auto&s=*static_cast<Provider*>(p);CHECK(stage==ResourceStage::Directory&&n==7&&total==9);++s.progress_calls;}
    ResourceSource source(){return {this,bytes?uint32_t(bytes->size()):0,read,nullptr,progress};}
};
static Bytes repeated_lz4(uint8_t value,unsigned size){
    CHECK(size>=25);Bytes out{0x1f,value,1,0};unsigned extra=size-6-19;
    while(extra>=255){out.push_back(255);extra-=255;}out.push_back(uint8_t(extra));
    out.push_back(0x50);out.insert(out.end(),5,value);return out;
}
struct BlockFixture {Bytes stored,raw;};
static BlockFixture blocked_fixture(unsigned blocks=4,uint8_t value='A',bool broken_stream=false){
    CHECK(blocks>0&&blocks<=BlockedResourceSource::MaxBlocks);
    const uint32_t table=32+(blocks+1)*4;BlockFixture f;f.stored.resize(table);
    for(unsigned i=0;i<blocks;++i){
        const unsigned size=i+1==blocks?333:B;const bool literal=blocks==4&&(i==1||i==2);
        Bytes raw(size,uint8_t(value+i));if(literal)for(unsigned j=0;j<size;++j)raw[j]=uint8_t(j*73+i*29);
        put(f.stored,32+i*4,uint32_t(f.stored.size())|(literal?Literal:0));
        const auto encoded=literal?raw:broken_stream&&i==0?Bytes{0xf0}:repeated_lz4(uint8_t(value+i),size);
        f.stored.insert(f.stored.end(),encoded.begin(),encoded.end());f.raw.insert(f.raw.end(),raw.begin(),raw.end());
    }
    put(f.stored,32+blocks*4,uint32_t(f.stored.size()));header(f.stored,"NSL3",uint32_t(f.stored.size()));put(f.stored,4,3);
    put(f.stored,16,uint32_t(f.raw.size()));put(f.stored,20,B);put(f.stored,24,blocks);put(f.stored,28,table);return f;
}
static BlockedResourceSource blocked;
static void blocked_reads(){
    auto f=blocked_fixture();Provider p{&f.stored};CHECK(blocked.open(p.source()));CHECK(p.calls==2);
    Bytes out(f.raw.size()+2,0xe7);CHECK(blocked.read(0,out.data()+1,uint32_t(f.raw.size())));
    CHECK(out.front()==0xe7&&out.back()==0xe7&&std::equal(f.raw.begin(),f.raw.end(),out.begin()+1));
    for(unsigned at:{0u,1u,B-1,B,B+7,2*B-1,3*B-1,3*B,uint32_t(f.raw.size()-1)}){
        const unsigned n=std::min(79u,unsigned(f.raw.size()-at));Bytes row(n+2,0xe7);
        CHECK(blocked.read(at,row.data()+1,n));CHECK(row.front()==0xe7&&row.back()==0xe7);
        CHECK(std::equal(row.begin()+1,row.end()-1,f.raw.begin()+at));
    }
    CHECK(blocked.read(0,nullptr,0)&&blocked.read(uint32_t(f.raw.size()),nullptr,0));
    CHECK(!blocked.read(0,nullptr,1)&&!blocked.read(uint32_t(f.raw.size()),out.data(),1));
    CHECK(!blocked.read(UINT32_MAX,out.data(),1)&&!blocked.read(1,out.data(),UINT32_MAX));
    CHECK(blocked.open(p.source()));unsigned calls=p.calls;
    CHECK(blocked.read(10,out.data(),20)&&p.calls==calls+1);calls=p.calls;
    CHECK(blocked.read(30,out.data(),20)&&p.calls==calls); // same decoded page
    CHECK(blocked.read(B+5,out.data(),2*B-10)&&p.calls==calls+1&&p.last_size==2*B-10);
    CHECK(std::equal(out.begin(),out.begin()+2*B-10,f.raw.begin()+B+5));
    calls=p.calls;CHECK(blocked.read(3*B,out.data(),12)&&p.calls==calls+1);calls=p.calls;
    CHECK(blocked.read(10,out.data(),20)&&p.calls==calls+1); // cache changed pages
    auto source=blocked.source();source.progress(source.context,ResourceStage::Directory,7,9);CHECK(p.progress_calls==1);
    auto changed=blocked_fixture(4,'Z');Provider next{&changed.stored};CHECK(blocked.open(next.source()));
    CHECK(blocked.read(10,out.data(),20)&&out[0]=='Z'&&next.calls==3); // reopen invalidates old cache
    const auto self=blocked.source();CHECK(!blocked.open(self)&&!blocked.source().length);
    CHECK(!blocked.read(0,out.data(),0));
}
static void blocked_failures(){
    const auto f=blocked_fixture();
    const std::pair<unsigned,uint32_t> mutations[]={{0,0x315a534eu},{4,2},{8,0},{12,0},{16,0},{16,B*BlockedResourceSource::MaxBlocks+1},
        {20,8192},{24,BlockedResourceSource::MaxBlocks+1},{28,0},{32,0},{36,word(f.stored,32)},{48,uint32_t(f.stored.size()-1)}};
    for(const auto&m:mutations){auto bad=f.stored;put(bad,m.first,m.second);Provider p{&bad};CHECK(!blocked.open(p.source()));CHECK(!blocked.source().length);}
    auto bad=f.stored;put(bad,36,word(bad,36)&Mask);Provider literal_as_packed{&bad};CHECK(!blocked.open(literal_as_packed.source()));
    for(unsigned n:{0u,3u,31u,32u,45u}){Bytes short_file(f.stored.begin(),f.stored.begin()+n);Provider p{&short_file};CHECK(!blocked.open(p.source()));}
    Provider p{&f.stored};p.fail_at=40;CHECK(!blocked.open(p.source()));p.fail_at=UINT32_MAX;CHECK(blocked.open(p.source()));
    p.fail_at=word(f.stored,28);uint8_t out[16];std::memset(out,0x8e,sizeof out);
    CHECK(!blocked.read(0,out,sizeof out)&&out[0]==0x8e);const auto failed=p.calls;
    p.fail_at=UINT32_MAX;CHECK(blocked.read(0,out,sizeof out)&&out[0]=='A'&&p.calls==failed+1);
    auto damaged=blocked_fixture(4,'A',true);Provider d{&damaged.stored};CHECK(blocked.open(d.source()));
    std::memset(out,0x8e,sizeof out);CHECK(!blocked.read(0,out,sizeof out)&&out[0]==0x8e);
    CHECK(blocked.open(p.source())&&blocked.read(0,out,sizeof out)&&out[0]=='A');
    auto maximum=blocked_fixture(BlockedResourceSource::MaxBlocks);Provider max{&maximum.stored};CHECK(blocked.open(max.source()));
    CHECK(blocked.read(uint32_t(maximum.raw.size()-16),out,16));CHECK(std::equal(out,out+16,maximum.raw.end()-16));
    blocked.close();CHECK(!blocked.source().length&&!blocked.read(0,out,1));
}
struct PartFixture {Bytes index,raw;std::array<Bytes,MultipartResourceSource::MaxParts> parts;};
static PartFixture part_fixture(unsigned spans=19,uint8_t salt=0){
    CHECK(spans>0&&spans<=MultipartResourceSource::MaxSpans);PartFixture f;f.index.resize(96+spans*16);f.raw.resize(64);
    for(auto&p:f.parts)p.resize(32);
    for(unsigned i=0;i<spans;++i){
        const unsigned part=i%f.parts.size(),size=31+(i%7)*29;auto&target=f.parts[part];const unsigned row=96+i*16;
        put(f.index,row,uint32_t(f.raw.size()));put(f.index,row+4,uint32_t(f.raw.size()+size));put(f.index,row+8,uint32_t(target.size()));put(f.index,row+12,part);
        for(unsigned j=0;j<size;++j){const auto value=uint8_t(i*47+j*31+salt);target.push_back(value);f.raw.push_back(value);}
    }
    header(f.raw,"NSC1",64);put(f.raw,16,uint32_t(f.raw.size()));put(f.raw,20,uint32_t(f.raw.size()-64));
    header(f.index,"NSM1",96);put(f.index,16,uint32_t(f.index.size()));put(f.index,20,uint32_t(f.raw.size()));put(f.index,24,spans);put(f.index,28,unsigned(f.parts.size()));
    std::copy(f.raw.begin(),f.raw.begin()+64,f.index.begin()+32);
    for(unsigned i=0;i<f.parts.size();++i){auto&p=f.parts[i];header(p,"NSS1",32);put(p,16,uint32_t(p.size()));put(p,20,i);}
    return f;
}
struct PartInputs {
    Provider index;std::array<Provider,MultipartResourceSource::MaxParts> providers;
    std::array<ResourceSource,MultipartResourceSource::MaxParts> parts;
    explicit PartInputs(const PartFixture&f):index{&f.index}{for(unsigned i=0;i<parts.size();++i){providers[i].bytes=&f.parts[i];parts[i]=providers[i].source();}}
};
static MultipartResourceSource multipart;
static void part_reads(){
    auto f=part_fixture();PartInputs input(f);CHECK(multipart.open(input.index.source(),input.parts.data(),input.parts.size()));CHECK(input.index.calls==2);
    Bytes out(f.raw.size()+2,0xd3);CHECK(multipart.read(0,out.data()+1,uint32_t(f.raw.size())));
    CHECK(out.front()==0xd3&&out.back()==0xd3&&std::equal(f.raw.begin(),f.raw.end(),out.begin()+1));
    for(unsigned i=0;i<41;++i){const unsigned at=(i*997)%f.raw.size(),n=std::min(271u,unsigned(f.raw.size()-at));
        CHECK(multipart.read(at,out.data(),n));CHECK(std::equal(out.begin(),out.begin()+n,f.raw.begin()+at));}
    for(unsigned at:{0u,31u,63u,64u,uint32_t(f.raw.size())})CHECK(multipart.read(at,nullptr,0));
    CHECK(!multipart.read(0,nullptr,1)&&!multipart.read(uint32_t(f.raw.size()),out.data(),1));
    CHECK(!multipart.read(UINT32_MAX,out.data(),0)&&!multipart.read(1,out.data(),UINT32_MAX));
    CHECK(multipart.read(63,out.data(),2)&&out[0]==f.raw[63]&&out[1]==f.raw[64]);
    input.providers[0].fail_at=32;std::fill(out.begin(),out.end(),0xd3);CHECK(!multipart.read(64,out.data(),1)&&out[0]==0xd3);
    input.providers[0].fail_at=UINT32_MAX;CHECK(multipart.read(64,out.data(),1)&&out[0]==f.raw[64]);
    auto other=part_fixture(19,91);PartInputs next(other);CHECK(multipart.open(next.index.source(),next.parts.data(),next.parts.size()));
    CHECK(multipart.read(64,out.data(),1)&&out[0]==other.raw[64]);
    auto self=multipart.source();CHECK(!multipart.open(self,next.parts.data(),next.parts.size())&&!multipart.source().length);
}
static void part_failures(){
    const auto f=part_fixture();
    const std::pair<unsigned,uint32_t> mutations[]={{0,0},{4,2},{8,0},{12,0},{16,0},{20,64},{24,0},{24,MultipartResourceSource::MaxSpans+1},
        {28,7},{32,0},{40,0},{48,0},{52,0},{96,65},{100,64},{104,31},{104,UINT32_MAX},{108,8},{112,64}};
    for(const auto&m:mutations){auto bad=f;put(bad.index,m.first,m.second);PartInputs input(bad);
        CHECK(!multipart.open(input.index.source(),input.parts.data(),input.parts.size())&&!multipart.source().length);}
    for(const auto&m:std::array<std::pair<unsigned,uint32_t>,5>{{{0,0},{8,0},{16,31},{20,7},{24,1}}}){
        auto bad=f;put(bad.parts[0],m.first,m.second);PartInputs input(bad);CHECK(!multipart.open(input.index.source(),input.parts.data(),input.parts.size()));}
    PartInputs input(f);CHECK(!multipart.open(input.index.source(),nullptr,8));CHECK(!multipart.open(input.index.source(),input.parts.data(),7));
    input.parts[0]={};CHECK(!multipart.open(input.index.source(),input.parts.data(),8));
    PartInputs failure(f);failure.providers[4].fail_at=0;CHECK(!multipart.open(failure.index.source(),failure.parts.data(),8));
    auto maximum=part_fixture(MultipartResourceSource::MaxSpans);PartInputs max(maximum);CHECK(multipart.open(max.index.source(),max.parts.data(),8));
    uint8_t out[8]{};CHECK(multipart.read(uint32_t(maximum.raw.size()-sizeof out),out,sizeof out));CHECK(std::equal(out,out+sizeof out,maximum.raw.end()-sizeof out));
    multipart.close();CHECK(!multipart.source().length&&!multipart.read(0,out,0));
}
static void active_transfer(){
    const auto f=blocked_fixture();Provider p{&f.stored};const auto parts=part_fixture();PartInputs input(parts);
    ResourceTransferLease lease(ResourceTransferUse::Bytes);CHECK(bool(lease));
    uint8_t*workspace=lease.bytes();for(unsigned i=0;i<ResourceTransferCapacity;++i)workspace[i]=uint8_t(i^(i>>8));
    const Bytes protected_bytes(workspace,workspace+ResourceTransferCapacity);
    CHECK(blocked.open(p.source())&&multipart.open(input.index.source(),input.parts.data(),input.parts.size()));
    Bytes out(f.raw.size());CHECK(blocked.read(0,out.data(),uint32_t(out.size()))&&out==f.raw);
    out.resize(parts.raw.size());CHECK(multipart.read(0,out.data(),uint32_t(out.size()))&&out==parts.raw);
    CHECK(resource_transfer_uses==unsigned(ResourceTransferUse::Bytes));
    CHECK(std::equal(protected_bytes.begin(),protected_bytes.end(),workspace));
    ResourceTransferLease nested(ResourceTransferUse::Bytes);CHECK(!nested);
}
static Bytes file(const std::string&path){
    FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0&&uint64_t(n)<=256u*1024u*1024u);CHECK(!std::fseek(f,0,SEEK_SET));
    Bytes result(size_t(n),0);CHECK(std::fread(result.data(),1,result.size(),f)==result.size());CHECK(!std::fclose(f));return result;
}
static void actual(const char*directory,const char*baseline,const char*code){
    const auto index=file(std::string(directory)+"/nSpire2.dat.tns"),expected=file(baseline);std::array<Bytes,8> data;std::array<ResourceSource,8> parts;
    for(unsigned i=0;i<8;++i){data[i]=file(std::string(directory)+"/"+ArtworkPartNames[i]);parts[i]=ResourceSource::memory(data[i].data(),data[i].size());}
    CHECK(multipart.open(ResourceSource::memory(index.data(),index.size()),parts.data(),8));
    const auto language=file(std::string(directory)+"/nSpire2."+code+".dat.tns");static SplitResourceSource split;
    const unsigned id=locale_language_id(code);CHECK(id);CHECK(split.open(multipart.source(),ResourceSource::memory(language.data(),language.size()),ResourceLanguage(id)));
    CHECK(split.source().length==expected.size());Bytes row(65536);
    for(unsigned at=0;at<expected.size();){const unsigned n=std::min(unsigned(row.size()),unsigned(expected.size()-at));CHECK(split.read(at,row.data(),n));CHECK(std::equal(row.begin(),row.begin()+n,expected.begin()+at));at+=n;}
    // Its mapped artwork and NSL3 outer decoding stay usable while an image
    // transfer owns the shared workspace. NSL2's inner directory is excluded
    // here deliberately: Content pins that prefix before rendering starts.
    ResourceTransferLease lease(ResourceTransferUse::Bytes);CHECK(bool(lease));lease.bytes()[0]=0x9b;
    const unsigned cached=split.source().required_cached_prefix;
    for(unsigned at:{cached,cached+4093u,unsigned(expected.size()-257)}){CHECK(split.read(at,row.data(),257));CHECK(std::equal(row.begin(),row.begin()+257,expected.begin()+at));}
    CHECK(lease.bytes()[0]==0x9b);
    std::printf("actual storage %s: %zu logical bytes compared, active transfer preserved\n",code,expected.size());
}
static_assert(sizeof(BlockedResourceSource)<=25*1024,"Independent locale adapter stays within its bounded cache/table");
static_assert(sizeof(MultipartResourceSource)<=18*1024,"Concern mapping must retain the1024-span bound");
}
int main(int argc,char**argv){blocked_reads();blocked_failures();part_reads();part_failures();active_transfer();CHECK(!resource_transfer_uses);CHECK(argc==1||argc==4);if(argc==4)actual(argv[1],argv[2],argv[3]);std::printf("storage source: %u checks passed\n",checks);}
