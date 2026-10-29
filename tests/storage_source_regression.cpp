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
    struct Request {uint32_t at,size;};std::vector<Request> requests;
    bool partial_failure{};
    static bool read(void*p,uint32_t at,void*out,uint32_t n){
        auto&s=*static_cast<Provider*>(p);++s.calls;s.last_at=at;s.last_size=n;
        s.requests.push_back({at,n});
        if(!s.bytes||at>s.bytes->size()||n>s.bytes->size()-at||(n&&!out))return false;
        if(n&&uint64_t(at)+n>s.fail_at){if(s.partial_failure)std::memcpy(out,s.bytes->data()+at,n/2);return false;}
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
static BlockFixture patterned_fixture(const std::string&pattern,unsigned last_size=B,uint8_t salt=0){
    CHECK(!pattern.empty()&&pattern.size()<=BlockedResourceSource::MaxBlocks&&last_size&&last_size<=B);
    BlockFixture f;const unsigned table=32+4*(unsigned(pattern.size())+1);f.stored.resize(table);
    for(unsigned i=0;i<pattern.size();++i){
        const unsigned size=i+1==pattern.size()?last_size:B;const bool literal=pattern[i]=='L';
        const uint8_t value=uint8_t(i+29+salt);Bytes raw(size,value);
        if(literal)for(unsigned j=0;j<size;++j)raw[j]=uint8_t(j*73+i*29+salt);
        Bytes encoded;
        if(literal)encoded=raw;
        else if(pattern[i]=='D'){
            // A valid deliberately dense LZ4 stream: 4,044 literals, a 47-byte
            // repeat, then five ending literals. Encoded size is exactly 4,070.
            CHECK(size==B);encoded.push_back(0xff);unsigned extra=4044-15;
            while(extra>=255){encoded.push_back(255);extra-=255;}encoded.push_back(uint8_t(extra));
            encoded.insert(encoded.end(),4044,value);encoded.push_back(1);encoded.push_back(0);
            encoded.push_back(47-19);encoded.push_back(0x50);encoded.insert(encoded.end(),5,value);
            CHECK(encoded.size()==4070);
        }else {CHECK(pattern[i]=='C');encoded=repeated_lz4(value,size);}
        put(f.stored,32+i*4,uint32_t(f.stored.size())|(literal?Literal:0));
        f.stored.insert(f.stored.end(),encoded.begin(),encoded.end());f.raw.insert(f.raw.end(),raw.begin(),raw.end());
    }
    put(f.stored,32+unsigned(pattern.size())*4,uint32_t(f.stored.size()));header(f.stored,"NSL3",uint32_t(f.stored.size()));put(f.stored,4,3);
    put(f.stored,16,uint32_t(f.raw.size()));put(f.stored,20,B);put(f.stored,24,unsigned(pattern.size()));put(f.stored,28,table);return f;
}
static BlockedResourceSource blocked;
static void coalesced_reads(){
    auto f=patterned_fixture("LCLLLLLLCL");Provider p{&f.stored};CHECK(blocked.open(p.source()));
    Bytes out(f.raw.size()+2,0xe7);const unsigned at=B+511,n=8*B-511-17;unsigned calls=p.calls;
    CHECK(blocked.read(at,out.data()+1,n)&&p.calls==calls+1);
    CHECK(out[0]==0xe7&&out[n+1]==0xe7&&std::equal(out.begin()+1,out.begin()+1+n,f.raw.begin()+at));
    CHECK(p.last_at==(word(f.stored,36)&Mask)&&p.last_size==(word(f.stored,68)&Mask)-p.last_at);
    // Both literal edge pages retain exact partial byte ranges; neither an
    // earlier nor a later page is fetched solely to complete a batch.
    f=patterned_fixture("CLCLC");p.bytes=&f.stored;CHECK(blocked.open(p.source()));calls=p.calls;
    CHECK(blocked.read(B+17,out.data(),2*B+14)&&p.calls==calls+1);
    CHECK(p.last_at==(word(f.stored,36)&Mask)+17&&p.last_at+p.last_size==(word(f.stored,44)&Mask)+31);
    CHECK(std::equal(out.begin(),out.begin()+2*B+14,f.raw.begin()+B+17));
    // Preserve a resident compressed page when only literal bytes precede it.
    CHECK(blocked.read(2*B+11,out.data(),23));calls=p.calls;
    CHECK(blocked.read(B+17,out.data(),B+47)&&p.calls==calls+1&&p.last_size==B-17);
    CHECK(std::equal(out.begin(),out.begin()+B+47,f.raw.begin()+B+17));
    calls=p.calls;CHECK(blocked.read(2*B+31,out.data(),19)&&p.calls==calls);
    // A cached first page is copied before the mixed remainder uses the buffer.
    CHECK(blocked.open(p.source())&&blocked.read(21,out.data(),17));calls=p.calls;
    CHECK(blocked.read(29,out.data(),3*B-71)&&p.calls==calls+1);
    CHECK(std::equal(out.begin(),out.begin()+3*B-71,f.raw.begin()+29));
    // The encoded batch ends exactly at 32 KiB. A subsequent literal byte is
    // a separate request, with no overflow or unrequested following bytes.
    f=patterned_fixture("CLLLLLLLDL");p.bytes=&f.stored;CHECK(blocked.open(p.source()));calls=p.calls;
    CHECK(blocked.read(0,out.data(),9*B)&&p.calls==calls+1&&p.last_size==BlockedResourceSource::EncodedBytes);
    CHECK(std::equal(out.begin(),out.begin()+9*B,f.raw.begin()));
    CHECK(blocked.open(p.source()));calls=p.calls;const auto begin=p.requests.size();
    CHECK(blocked.read(0,out.data(),9*B+1)&&p.calls==calls+2);
    CHECK(p.requests[begin].size==BlockedResourceSource::EncodedBytes&&p.requests[begin+1].size==1);
    CHECK(std::equal(out.begin(),out.begin()+9*B+1,f.raw.begin()));
    f=patterned_fixture(std::string(24,'L'));p.bytes=&f.stored;CHECK(blocked.open(p.source()));calls=p.calls;out.resize(f.raw.size());
    CHECK(blocked.read(31,out.data(),uint32_t(f.raw.size()-78))&&p.calls==calls+1);
    CHECK(p.last_size==f.raw.size()-78&&p.last_at==word(f.stored,28)+31);
    CHECK(std::equal(out.begin(),out.begin()+f.raw.size()-78,f.raw.begin()+31));
    // The maximum table also drives a complete multi-batch read, not just its
    // last byte. Every physical range is consecutive and bounded by the file.
    std::string pattern(BlockedResourceSource::MaxBlocks,'L');for(unsigned i=0;i<pattern.size();i+=2)pattern[i]='C';
    f=patterned_fixture(pattern,1);p.bytes=&f.stored;CHECK(blocked.open(p.source()));out.resize(f.raw.size());
    const auto first=p.requests.size();calls=p.calls;CHECK(blocked.read(0,out.data(),uint32_t(out.size()))&&out==f.raw);
    CHECK(p.calls-calls<pattern.size());unsigned physical=word(f.stored,28);
    for(size_t i=first;i<p.requests.size();++i){CHECK(p.requests[i].at==physical&&p.requests[i].size<=BlockedResourceSource::EncodedBytes);physical+=p.requests[i].size;}
    CHECK(physical==f.stored.size());
}
static void coalesced_failures(){
    auto f=patterned_fixture("CLLC");Provider p{&f.stored};CHECK(blocked.open(p.source()));Bytes out(f.raw.size()+2,0xe7);
    CHECK(blocked.read(3*B,out.data()+1,17));p.fail_at=word(f.stored,28);p.partial_failure=true;unsigned calls=p.calls;
    CHECK(!blocked.read(0,out.data()+1,uint32_t(f.raw.size()))&&p.calls==calls+1);
    CHECK(out[0]==0xe7&&out.back()==0xe7);p.fail_at=UINT32_MAX;calls=p.calls;
    CHECK(blocked.read(3*B,out.data()+1,17)&&p.calls==calls+1); // failed batch invalidates old decoded page
    CHECK(blocked.open(p.source()));calls=p.calls;
    CHECK(blocked.read(0,out.data()+1,uint32_t(f.raw.size()))&&p.calls==calls+1);
    CHECK(std::equal(out.begin()+1,out.end()-1,f.raw.begin()));
    // A later compressed stream fails after the first page has decoded.
    const unsigned damaged=word(f.stored,44)&Mask;f.stored[damaged+2]=0;CHECK(blocked.open(p.source()));
    CHECK(!blocked.read(0,out.data()+1,uint32_t(f.raw.size())));calls=p.calls;
    CHECK(blocked.read(0,out.data()+1,17)&&p.calls==calls+1);
    CHECK(out[0]==0xe7&&out.back()==0xe7);
}
static BlockedResourceSource reentrant,other_adapter;
struct ReentrantProvider {
    const BlockFixture&fixture;const BlockFixture&other;bool armed{},nested_read{},nested_open{},other_read{};unsigned attempts{};
    static bool read(void*context,uint32_t at,void*out,uint32_t n){
        auto&p=*static_cast<ReentrantProvider*>(context);
        CHECK(at<=p.fixture.stored.size()&&n<=p.fixture.stored.size()-at);std::memcpy(out,p.fixture.stored.data()+at,n);
        if(p.armed){
            p.armed=false;++p.attempts;uint8_t scratch[19]{};const auto old=reentrant.source();
            p.nested_read=reentrant.read(0,scratch,sizeof scratch);
            p.nested_open=reentrant.open(ResourceSource::memory(p.fixture.stored.data(),p.fixture.stored.size()));
            reentrant.close();CHECK(reentrant.source().length==old.length);
            // Re-enter after filling the outer destination, as a foreground
            // provider's post-read input poll may do. The second adapter uses
            // different bytes across its own complete mixed batch.
            Bytes nested(p.other.raw.size());p.other_read=other_adapter.read(0,nested.data(),uint32_t(nested.size()));
            CHECK(p.other_read&&nested==p.other.raw);
        }
        return true;
    }
    ResourceSource source(){return {this,uint32_t(fixture.stored.size()),read};}
};
static void provider_reentry(){
    const auto f=patterned_fixture("CLLC"),other=patterned_fixture("CLCLC",B,91);
    CHECK(other_adapter.open(ResourceSource::memory(other.stored.data(),other.stored.size())));
    ReentrantProvider p{f,other};p.armed=true;CHECK(reentrant.open(p.source()));CHECK(p.attempts==1&&!p.nested_read&&!p.nested_open&&p.other_read);
    p.armed=true;Bytes out(f.raw.size());CHECK(reentrant.read(0,out.data(),uint32_t(out.size()))&&out==f.raw);
    CHECK(p.attempts==2&&!p.nested_read&&!p.nested_open&&p.other_read);
    CHECK(reentrant.read(3*B+17,out.data(),23)&&std::equal(out.begin(),out.begin()+23,f.raw.begin()+3*B+17));
    reentrant.close();CHECK(!reentrant.source().length);other_adapter.close();
}
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
static_assert(sizeof(BlockedResourceSource)<=45*1024,"Independent locale adapter retains bounded 32 KiB batching");
static_assert(sizeof(MultipartResourceSource)<=3*1024,"Concern mapping retains the 128-span bound");
}
int main(int argc,char**argv){blocked_reads();blocked_failures();coalesced_reads();coalesced_failures();provider_reentry();part_reads();part_failures();active_transfer();CHECK(!resource_transfer_uses);CHECK(argc==1||argc==4);if(argc==4)actual(argv[1],argv[2],argv[3]);std::printf("storage source: %u checks passed\n",checks);}
