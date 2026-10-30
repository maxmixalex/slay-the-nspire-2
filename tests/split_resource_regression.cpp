#include "resource_source.hpp"
#include "ui12_assets.hpp"
#include "scene_font_assets.hpp"
#include "retained_filter.hpp"
#include "speech_bubble_assets.hpp"
#include "history_body_ui10_assets.hpp"
#include "frontend_assets.hpp"
#include "hud_ui7_assets.hpp"
#include "history_ui8_assets.hpp"
#include "game_over_transition_assets.hpp"
#include "grouped_glyphs.hpp"
#include "grouped_strings.hpp"
#include "bestiary_kaiser_assets.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
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
#include "fixtures/split_directory_lzma2.inc"
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
    unsigned seeks{},position{},first_at{},first_bytes{};bool position_known{};
    void reset_io(){calls=read_bytes=seeks=position=first_at=first_bytes=0;position_known=false;}
    static bool read(void* c,uint32_t at,void*out,uint32_t n){
        auto& f=*static_cast<File*>(c);if(!f.calls){f.first_at=at;f.first_bytes=n;}
        ++f.calls;f.read_bytes+=n;if(!f.position_known||f.position!=at)++f.seeks;f.position_known=false;
        if(at>f.bytes->size()||n>f.bytes->size()-at||(f.fail_at>=int64_t(at)&&f.fail_at<int64_t(at)+n))return false;
        f.position=at+n;f.position_known=true;
        std::memcpy(out,f.bytes->data()+at,n);return true;
    }
    static void progress(void*c,ResourceStage,uint32_t,uint32_t){++static_cast<File*>(c)->progress_calls;}
    ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr,progress};}
};

// Independent NSL2 construction: the directory is synthetic, all literal
// blocks are generated here, and the three compressed streams have a small
// reproducible Python generator. No production pack writer is the oracle.
struct DirectoryFixture {
    static constexpr unsigned Block=65536,Page=4096,DefaultDirectory=3*Block+Page;
    unsigned directory,prefix,art=137,suffix=41,blocks;
    Bytes original,common,locale;
    explicit DirectoryFixture(bool chinese=false,unsigned size=DefaultDirectory,bool first_compressed=true,bool last_compressed=true)
        :directory(size),prefix(std::max(size+321,40u+16+16+6111*8+321)),blocks((size+Block-1)/Block){
        CHECK(directory>=Page&&directory<=2u*1024u*1024u&&directory%Page==0);
        CHECK(!first_compressed||directory==DefaultDirectory);
        original.resize(prefix+art+suffix);
        for(unsigned i=40;i<original.size();++i)original[i]=uint8_t(i*71u+(i>>5));
        uint32_t random=0x12345678;
        for(unsigned i=Block;i<std::min(directory,2*Block);++i){
            random^=random<<13;random^=random>>17;random^=random<<5;original[i]=uint8_t(random>>24);
        }
        for(unsigned i=2*Block;i<directory;++i)original[i]=uint8_t(0xa5+(i%Block)%7);
        std::memcpy(original.data(),"NSP2",4);put(original,4,38);put(original,8,0x41cef1ea);
        put(original,12,unsigned(original.size()));put(original,16,1);put(original,20,1);
        put(original,24,6111);put(original,28,1);put(original,32,1);put(original,36,0);
        const unsigned marker=prefix+art+7,row=40+16+16+6110*8;
        put(original,row,marker);put(original,row+4,3);std::memcpy(original.data()+marker,chinese?"zhs":"eng",4);
        common.resize(64+art);std::memcpy(common.data(),"NSC1",4);put(common,4,1);put(common,8,0x41cef1ea);
        put(common,12,64);put(common,16,unsigned(common.size()));put(common,20,art);
        for(unsigned i=0;i<32;++i)common[32+i]=uint8_t(7*i+3);
        std::memcpy(common.data()+64,original.data()+prefix,art);header_hash(common,true);
        locale.resize(128+blocks*8);std::memcpy(locale.data(),"NSL2",4);put(locale,4,2);put(locale,8,0x41cef1ea);
        put(locale,12,128);put(locale,20,unsigned(original.size()));put(locale,24,prefix);put(locale,28,art);
        put(locale,32,suffix);put(locale,36,chinese?2:1);std::memcpy(locale.data()+48,common.data()+32,32);
        put(locale,96,directory);put(locale,100,Block);put(locale,104,blocks);put(locale,108,128);
        put(locale,112,unsigned(locale.size()));put(locale,120,14);
        for(unsigned index=0;index<blocks;++index){
            const unsigned offset=index*Block,raw=std::min(Block,directory-offset);
            const uint8_t* bytes=original.data()+offset;unsigned stored=raw;bool compressed=false;
            if(index==0&&first_compressed){bytes=split_directory_first;stored=sizeof split_directory_first;compressed=true;}
            else if(index>=2&&(index+1<blocks||last_compressed)){
                CHECK(raw==Block||raw==Page);
                bytes=raw==Block?split_directory_full:split_directory_last;
                stored=raw==Block?sizeof split_directory_full:sizeof split_directory_last;compressed=true;
            }
            put(locale,128+index*8,unsigned(locale.size()));put(locale,132+index*8,stored|(compressed?0u:0x80000000u));
            locale.insert(locale.end(),bytes,bytes+stored);
        }
        put(locale,116,unsigned(locale.size()));
        locale.insert(locale.end(),original.begin()+directory,original.begin()+prefix);
        locale.insert(locale.end(),original.begin()+prefix+art,original.end());put(locale,16,unsigned(locale.size()));
    }
    unsigned offset(unsigned block)const{return get(locale,128+block*8);}
    unsigned stored(unsigned block)const{return get(locale,132+block*8)&0x7fffffffu;}
    void replace_block(unsigned block,const Bytes& bytes){
        const unsigned at=offset(block),old=stored(block);const int change=int(bytes.size())-int(old);
        locale.erase(locale.begin()+at,locale.begin()+at+old);locale.insert(locale.begin()+at,bytes.begin(),bytes.end());
        put(locale,132+block*8,unsigned(bytes.size()));
        for(unsigned next=block+1;next<blocks;++next)put(locale,128+next*8,unsigned(int(offset(next))+change));
        put(locale,116,unsigned(int(get(locale,116))+change));put(locale,16,unsigned(locale.size()));
    }
};

static bool open_directory(SplitResourceSource& joined,const DirectoryFixture& f,bool chinese=false){
    return joined.open(ResourceSource::memory(f.common.data(),f.common.size()),
        ResourceSource::memory(f.locale.data(),f.locale.size()),chinese?ResourceLanguage::Chinese:ResourceLanguage::English);
}
static void same_read(SplitResourceSource& joined,const DirectoryFixture& f,unsigned at,unsigned count){
    Bytes guarded(count+32,0xcd);CHECK(joined.read(at,guarded.data()+16,count));
    CHECK(!count||!std::memcmp(guarded.data()+16,f.original.data()+at,count));
    for(unsigned i=0;i<16;++i)CHECK(guarded[i]==0xcd&&guarded[count+16+i]==0xcd);
}
static void directory_checks_for(bool chinese,bool streamed,bool first_compressed,bool last_compressed){
    DirectoryFixture f(chinese,DirectoryFixture::DefaultDirectory,first_compressed,last_compressed);
    File common{&f.common},locale{&f.locale};SplitResourceSource joined;
    const auto lang=chinese?ResourceLanguage::Chinese:ResourceLanguage::English;
    CHECK(joined.open(streamed?common.source():ResourceSource::memory(f.common.data(),f.common.size()),
        streamed?locale.source():ResourceSource::memory(f.locale.data(),f.locale.size()),lang));
    CHECK(joined.valid()&&joined.language()==lang&&!joined.source().direct);
    CHECK(joined.source().required_cached_prefix==f.directory);
    if(streamed){CHECK(common.read_bytes==64);CHECK(locale.read_bytes<DirectoryFixture::Block);}
    for(unsigned start:{0u,39u,40u,4095u,4096u,65535u,65536u,65537u,131065u,131072u,196607u,196608u,
            f.directory-7,f.directory,f.directory+123,f.prefix-7,f.prefix,f.prefix+f.art-7,f.prefix+f.art,unsigned(f.original.size())}){
        for(unsigned count:{0u,1u,7u,8u,61u,4096u,65536u}){
            if(count<=f.original.size()-start)same_read(joined,f,start,count);
        }
    }
    same_read(joined,f,0,unsigned(f.original.size()));
    CHECK(joined.read(f.original.size(),nullptr,0));CHECK(!joined.read(f.original.size()+1,nullptr,0));
    uint8_t byte=0;CHECK(!joined.read(0,nullptr,1));CHECK(!joined.read(f.original.size(),&byte,1));
    CHECK(!joined.read(0xffffffffu,&byte,1));CHECK(!resource_transfer_uses);
    ResourceReader reader(joined.source());reader.progress(ResourceStage::Directory,1,1);
    if(streamed){
        CHECK(locale.progress_calls==1&&common.progress_calls==0);
        locale.fail_at=f.offset(2)+1;CHECK(!joined.read(2*DirectoryFixture::Block,&byte,1));CHECK(!resource_transfer_uses);
        locale.fail_at=-1;same_read(joined,f,2*DirectoryFixture::Block,31);
        common.fail_at=64+10;CHECK(!joined.read(f.prefix+10,&byte,1));common.fail_at=-1;
        same_read(joined,f,f.prefix+10,1);
    }
    joined.close();CHECK(!joined.valid()&&!joined.source().required_cached_prefix);
    // Switching back to a legacy source must drop the packed mapping, too.
    Fixture old(chinese,38);CHECK(joined.open(ResourceSource::memory(old.common.data(),old.common.size()),
        ResourceSource::memory(old.locale.data(),old.locale.size()),lang));
    CHECK(!joined.source().required_cached_prefix);
    Bytes all(old.original.size());CHECK(joined.read(0,all.data(),all.size()));CHECK(all==old.original);
}

static void malformed_directory(){
    DirectoryFixture f;SplitResourceSource joined;CHECK(open_directory(joined,f));
    const auto header_rejected=[&](unsigned at,uint32_t value){
        const uint32_t previous=get(f.locale,at);put(f.locale,at,value);CHECK(!open_directory(joined,f));
        CHECK(!joined.valid());put(f.locale,at,previous);
    };
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,36u})header_rejected(at,get(f.locale,at)^1u);
    for(uint32_t value:{0u,39u,4095u,4097u,2u*1024u*1024u+4096u,0xffffffffu})header_rejected(96,value);
    for(uint32_t value:{0u,65535u,65537u,0xffffffffu})header_rejected(100,value);
    for(uint32_t value:{0u,3u,5u,33u,0xffffffffu})header_rejected(104,value);
    for(uint32_t value:{0u,96u,127u,129u,0xffffffffu})header_rejected(108,value);
    for(uint32_t value:{0u,128u,get(f.locale,112)-1,get(f.locale,112)+1,0xffffffffu})header_rejected(112,value);
    for(uint32_t value:{0u,get(f.locale,112)-1,get(f.locale,116)-1,get(f.locale,116)+1,0xffffffffu})header_rejected(116,value);
    for(uint32_t value:{0u,13u,15u,40u,0xffffffffu})header_rejected(120,value);
    for(uint32_t value:{1u,0xffffffffu})header_rejected(124,value);
    for(unsigned at:{40u,44u,48u,79u,80u,95u}){
        f.locale[at]^=1;CHECK(open_directory(joined,f));f.locale[at]^=1;
    }
    // Rows are checked when accessed. The first/last blocks exercise both
    // opening-time and deferred validation without demanding a startup scan.
    for(unsigned block:{0u,1u,2u,3u}){
        const unsigned raw=std::min(DirectoryFixture::Block,f.directory-block*DirectoryFixture::Block);
        for(uint32_t value:{0u,raw,raw+1,0x40000001u,0x80000000u,0x80000000u|(raw-1),0x80000000u|(raw+1),0xffffffffu}){
            const unsigned at=132+block*8;const uint32_t previous=get(f.locale,at);put(f.locale,at,value);
            if(open_directory(joined,f)){uint8_t byte=0;CHECK(!joined.read(block*DirectoryFixture::Block,&byte,1));}
            put(f.locale,at,previous);CHECK(!resource_transfer_uses);
        }
        for(uint32_t value:{0u,127u,get(f.locale,112)-1,get(f.locale,116),get(f.locale,116)+1,0xffffffffu}){
            const unsigned at=128+block*8;const uint32_t previous=get(f.locale,at);put(f.locale,at,value);
            if(open_directory(joined,f)){uint8_t byte=0;CHECK(!joined.read(block*DirectoryFixture::Block,&byte,1));}
            put(f.locale,at,previous);
        }
    }
    for(unsigned block:{0u,2u,3u}){
        DirectoryFixture bad;bad.locale[bad.offset(block)]=3;
        if(open_directory(joined,bad)){uint8_t byte=0;CHECK(!joined.read(block*DirectoryFixture::Block,&byte,1));}
        CHECK(!resource_transfer_uses);
    }
    for(unsigned mode=0;mode<4;++mode){
        DirectoryFixture bad;const unsigned at=bad.offset(2),n=bad.stored(2);
        Bytes encoded(bad.locale.begin()+at,bad.locale.begin()+at+n);
        if(mode==0)encoded.pop_back(); // Missing explicit LZMA2 end marker.
        if(mode==1)encoded.push_back(0); // Trailing bytes must not be accepted.
        if(mode==2)encoded.assign(split_directory_last,split_directory_last+sizeof split_directory_last); // Wrong decoded length.
        if(mode==3){CHECK(encoded.size()>6&&encoded[0]>=0xe0);encoded[5]=0xff;} // Invalid literal-context properties.
        bad.replace_block(2,encoded);CHECK(open_directory(joined,bad));
        uint8_t byte=0;CHECK(!joined.read(2*DirectoryFixture::Block,&byte,1));CHECK(!resource_transfer_uses);
    }
    {DirectoryFixture bad;bad.locale.pop_back();put(bad.locale,16,unsigned(bad.locale.size()));CHECK(!open_directory(joined,bad));}
    {DirectoryFixture bad;CHECK(!joined.open(ResourceSource::memory(bad.common.data(),bad.common.size()),
        ResourceSource::memory(bad.locale.data(),bad.locale.size()-1),ResourceLanguage::English));}
    for(unsigned length:{95u,96u,127u,128u,get(f.locale,112)-1}){
        DirectoryFixture bad;bad.locale.resize(length);put(bad.locale,16,length);CHECK(!open_directory(joined,bad));
    }
    // Relabeling the NSL2 header cannot disguise an actual marker mismatch.
    put(f.locale,36,2);CHECK(!open_directory(joined,f,true));put(f.locale,36,1);
    const unsigned marker=get(f.locale,116)+f.prefix-f.directory+7;
    f.locale[marker]='z';CHECK(!open_directory(joined,f));f.locale[marker]='e';
    CHECK(open_directory(joined,f));same_read(joined,f,0,unsigned(f.original.size()));
}

static Bytes workspace_snapshot(){
    const auto* p=reinterpret_cast<const uint8_t*>(&foreground_workspace);
    return {p,p+sizeof foreground_workspace};
}
static void packed_lease_conflicts(){
    DirectoryFixture f;SplitResourceSource joined;CHECK(open_directory(joined,f));
    for(auto use:{ResourceTransferUse::Bytes,ResourceTransferUse::Save,ResourceTransferUse::Codec,ResourceTransferUse::Map}){
        CHECK(!resource_transfer_uses);
        {ResourceTransferLease owner(use);CHECK(owner);std::memset(&foreground_workspace,0x6d,sizeof foreground_workspace);
            const Bytes before=workspace_snapshot();uint8_t byte=0xa7;
            CHECK(!joined.read(0,&byte,1));CHECK(byte==0xa7);CHECK(resource_transfer_uses==unsigned(use));
            CHECK(!std::memcmp(before.data(),&foreground_workspace,before.size()));
            // Literal blocks, raw directory tail, artwork and locale text do
            // not need the transfer workspace and remain readable by owners.
            for(unsigned at:{DirectoryFixture::Block+123,f.directory+123,f.prefix+13,f.prefix+f.art+7})
                same_read(joined,f,at,4);
            CHECK(resource_transfer_uses==unsigned(use));CHECK(!std::memcmp(before.data(),&foreground_workspace,before.size()));
            SplitResourceSource blocked;CHECK(!open_directory(blocked,f));CHECK(!blocked.valid());
            CHECK(resource_transfer_uses==unsigned(use));CHECK(!std::memcmp(before.data(),&foreground_workspace,before.size()));
        }
        CHECK(!resource_transfer_uses);same_read(joined,f,0,41);
    }
    // More than 64 distinct pages evicts the initially warm descriptor page.
    // Its actual offset is deliberately not page aligned, as in both packs.
    DirectoryFixture large(false,6*DirectoryFixture::Block+DirectoryFixture::Page,false);
    CHECK(open_directory(joined,large));ResourceReader reader(joined.source());
    const unsigned descriptor=large.directory+123;uint8_t got[16];CHECK(reader.read(descriptor,got,sizeof got));
    for(unsigned page=0;page<96;++page){uint8_t byte;CHECK(reader.u8(page*ResourceReader::PageSize,byte));}
    reader.clear_stats();
    {ResourceTransferLease owner(ResourceTransferUse::Bytes);CHECK(owner);std::memset(owner.bytes(),0x9d,ResourceTransferCapacity);
        const Bytes before=workspace_snapshot();CHECK(reader.read(descriptor,got,sizeof got));
        CHECK(!std::memcmp(got,large.original.data()+descriptor,sizeof got));CHECK(reader.stats().cache_misses==1);
        CHECK(resource_transfer_uses==unsigned(ResourceTransferUse::Bytes));
        CHECK(!std::memcmp(before.data(),&foreground_workspace,before.size()));
    }
    CHECK(!resource_transfer_uses);
}

struct ReentrantFile {
    File file;SplitResourceSource* joined{};unsigned trigger{};bool attempted{},nested_result{},preserved{};
    static bool read(void* context,uint32_t at,void* dst,uint32_t count){
        auto& self=*static_cast<ReentrantFile*>(context);
        if(!File::read(&self.file,at,dst,count))return false;
        if(self.joined&&!self.attempted&&at<=self.trigger&&self.trigger-at<count){
            self.attempted=true;const unsigned owner=resource_transfer_uses;const Bytes before=workspace_snapshot();uint8_t byte=0x6b;
            self.nested_result=self.joined->read(0,&byte,1);
            self.preserved=byte==0x6b&&owner==unsigned(ResourceTransferUse::Bytes)&&resource_transfer_uses==owner&&
                !std::memcmp(before.data(),&foreground_workspace,before.size());
        }
        return true;
    }
    ResourceSource source(){return {this,unsigned(file.bytes->size()),read};}
};
static void packed_reentrant_read(){
    DirectoryFixture f;ReentrantFile locale;locale.file.bytes=&f.locale;File common{&f.common};SplitResourceSource joined;
    CHECK(joined.open(common.source(),locale.source(),ResourceLanguage::English));
    locale.joined=&joined;locale.trigger=f.offset(2)+1;
    same_read(joined,f,2*DirectoryFixture::Block+17,129);
    CHECK(locale.attempted&&!locale.nested_result&&locale.preserved);CHECK(!resource_transfer_uses);
    same_read(joined,f,0,79);
}

static void packed_requested_table_io(){
    DirectoryFixture f;File common{&f.common},locale{&f.locale};SplitResourceSource joined;
    CHECK(joined.open(common.source(),locale.source(),ResourceLanguage::English));
    auto reset=[&](){common.reset_io();locale.reset_io();};
    reset();same_read(joined,f,0,f.directory);
    unsigned encoded=0;for(unsigned block=0;block<f.blocks;++block)encoded+=f.stored(block);
    CHECK(common.calls==0);CHECK(locale.calls==f.blocks+1);
    CHECK(locale.first_at==128&&locale.first_bytes==f.blocks*8);
    CHECK(locale.read_bytes==f.blocks*8+encoded);
    CHECK(locale.seeks==1); // One table read followed by contiguous group streams.

    // Start inside the literal block and finish inside the last compressed
    // block. Only rows1..3 are fetched, and a literal copies just its slice.
    reset();same_read(joined,f,DirectoryFixture::Block+13,2*DirectoryFixture::Block+3072);
    CHECK(common.calls==0&&locale.calls==4);CHECK(locale.first_at==136&&locale.first_bytes==24);
    CHECK(locale.read_bytes==24+DirectoryFixture::Block-13+f.stored(2)+f.stored(3));
    CHECK(locale.seeks==2);

    // A boundary-spanning request leaves the compressed directory, crosses
    // the raw descriptor tail and then enters common artwork unchanged.
    reset();same_read(joined,f,f.directory-23,23+321+5);
    CHECK(locale.calls==3&&common.calls==1);CHECK(locale.first_at==152&&locale.first_bytes==8);
    CHECK(locale.read_bytes==8+f.stored(3)+321&&common.read_bytes==5);
    CHECK(locale.seeks==2&&common.seeks==1);CHECK(!resource_transfer_uses);

    Bytes output(f.directory+32,0xcd);
    locale.fail_at=136;reset();CHECK(!joined.read(0,output.data()+16,f.directory));
    CHECK(locale.calls==1&&locale.first_bytes==f.blocks*8&&common.calls==0);
    CHECK(std::all_of(output.begin(),output.end(),[](uint8_t value){return value==0xcd;}));
    CHECK(!resource_transfer_uses);locale.fail_at=-1;

    const unsigned original=f.offset(1);put(f.locale,136,get(f.locale,116)+1);
    reset();CHECK(!joined.read(0,output.data()+16,f.directory));
    CHECK(locale.calls==2&&locale.first_bytes==f.blocks*8&&common.calls==0);
    CHECK(locale.read_bytes==f.blocks*8+f.stored(0));CHECK(!resource_transfer_uses);
    for(unsigned i=0;i<16;++i)CHECK(output[i]==0xcd&&output[f.directory+16+i]==0xcd);
    put(f.locale,136,original);same_read(joined,f,0,f.directory);
}

static void directory_limits(){
    for(unsigned size:{4096u,65536u,2u*1024u*1024u})for(bool chinese:{false,true}){
        DirectoryFixture f(chinese,size,false);SplitResourceSource joined;CHECK(open_directory(joined,f,chinese));
        CHECK(joined.source().required_cached_prefix==size);
        same_read(joined,f,0,unsigned(f.original.size()));same_read(joined,f,size-1,2);
        CHECK(!resource_transfer_uses);
    }
}
static void checks_for(bool chinese,bool streamed,unsigned version){
    Fixture f(chinese,version);File common{&f.common},locale{&f.locale};SplitResourceSource joined;
    const auto lang=chinese?ResourceLanguage::Chinese:ResourceLanguage::English;
    CHECK(joined.open(streamed?common.source():ResourceSource::memory(f.common.data(),f.common.size()),
                      streamed?locale.source():ResourceSource::memory(f.locale.data(),f.locale.size()),lang));
    CHECK(joined.valid()&&joined.language()==lang&&joined.source().direct==nullptr);
    CHECK(!joined.source().required_cached_prefix);
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
    {Fixture future(false,MaximumContentVersion+1);CHECK(!joined.open(ResourceSource::memory(future.common.data(),future.common.size()),ResourceSource::memory(future.locale.data(),future.locale.size()),ResourceLanguage::English));}
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
    for(unsigned version=17;version<=MaximumContentVersion;++version)for(bool chinese:{false,true})for(bool streamed:{false,true})checks_for(chinese,streamed,version);
    malformed();
    for(bool chinese:{false,true})for(bool streamed:{false,true})for(bool first:{false,true})for(bool last:{false,true})
        directory_checks_for(chinese,streamed,first,last);
    malformed_directory();packed_lease_conflicts();packed_reentrant_read();packed_requested_table_io();directory_limits();
    std::printf("split resources: %u checks passed; adapter=%zu bytes\n",checks,sizeof(SplitResourceSource));
}
