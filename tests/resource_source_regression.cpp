#include "resource_source.hpp"
#include "loading_screen.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct File {
    struct Progress {ResourceStage stage;uint32_t completed,total;};
    uint32_t readable{},calls{},last_offset{},last_count{},fail_offset=~0u;
    uint8_t salt{};
    std::vector<Progress> progress;
    static uint8_t byte(uint32_t p,uint8_t salt){return (uint8_t)((p*17u)^(p>>8)^salt);}
    static bool read(void* context,uint32_t offset,void* dst,uint32_t count){
        auto& f=*static_cast<File*>(context);++f.calls;f.last_offset=offset;f.last_count=count;
        if(offset>f.readable||count>f.readable-offset||
           (offset<=f.fail_offset&&count>f.fail_offset-offset))return false;
        auto* out=static_cast<uint8_t*>(dst);
        for(uint32_t i=0;i<count;++i)out[i]=byte(offset+i,f.salt);
        return true;
    }
    ResourceSource source(uint32_t advertised=0){return {this,advertised?advertised:readable,read,nullptr};}
    static void report(void*context,ResourceStage stage,uint32_t completed,uint32_t total){
        static_cast<File*>(context)->progress.push_back({stage,completed,total});
    }
};
static void check_bytes(const uint8_t* bytes,uint32_t offset,uint32_t n,uint8_t salt=0){
    for(uint32_t i=0;i<n;++i)CHECK(bytes[i]==File::byte(offset+i,salt));
}
static void bounds(){
    ResourceReader reader;uint32_t value=12345;
    CHECK(!reader.valid());CHECK(!reader.u32(0,value));CHECK(value==12345);
    File file;file.readable=8192+7;CHECK(reader.reset(file.source()));
    CHECK(reader.size()==8199);CHECK(reader.contains(8199,0));CHECK(reader.read(8199,nullptr,0));
    CHECK(!reader.read(8200,nullptr,0));CHECK(!reader.read(8199,&value,1));
    CHECK(!reader.read(8198,&value,2));CHECK(!reader.read(0,nullptr,1));
    CHECK(!reader.read(0xfffffffdu,&value,4));CHECK(file.calls==0);
    uint8_t tail[7]{};CHECK(reader.read(8192,tail,7));check_bytes(tail,8192,7);
    CHECK(file.last_offset==8192&&file.last_count==7);
    CHECK(!reader.u32(8197,value));CHECK(value==12345);
    // Near-uint32 limits use a virtual source, so neither allocation nor
    // pointer arithmetic conceals a wrap in the range checks.
    file.readable=std::numeric_limits<uint32_t>::max();CHECK(reader.reset(file.source()));
    CHECK(reader.u32(0xfffffffbu,value));
    uint8_t expected[4];for(unsigned i=0;i<4;++i)expected[i]=File::byte(0xfffffffbu+i,0);
    CHECK(value==(uint32_t(expected[0])|(uint32_t(expected[1])<<8)|(uint32_t(expected[2])<<16)|(uint32_t(expected[3])<<24)));
    CHECK(file.last_offset==0xfffff000u&&file.last_count==4095);
    CHECK(!reader.u32(0xfffffffcu,value));CHECK(!reader.contains(0xffffffffu,1));
    CHECK(reader.read(0xffffffffu,nullptr,0));
}
static void cache(){
    constexpr unsigned pages=ResourceReader::PageCount;
    File file;file.readable=(pages+4)*ResourceReader::PageSize+37;
    ResourceReader reader(file.source());uint8_t one=0;uint16_t two=0;uint32_t four=0;
    CHECK(reader.u8(17,one));CHECK(one==File::byte(17,0));CHECK(file.calls==1);
    CHECK(reader.u16(21,two));CHECK(file.calls==1);CHECK(reader.stats().cache_hits==1);
    CHECK(reader.u32(4094,four));CHECK(file.calls==2);
    CHECK(four==(uint32_t(File::byte(4094,0))|(uint32_t(File::byte(4095,0))<<8)|(uint32_t(File::byte(4096,0))<<16)|(uint32_t(File::byte(4097,0))<<24)));
    for(uint32_t page=2;page<pages;++page)CHECK(reader.u8(page*4096+3,one));
    CHECK(file.calls==pages);CHECK(reader.u8(0,one));CHECK(file.calls==pages);
    CHECK(reader.u8(pages*4096,one));CHECK(file.calls==pages+1);
    CHECK(reader.u8(0,one));CHECK(file.calls==pages+2); // Capacity+1 evicts page zero.
    std::vector<uint8_t> bulk(4096);const unsigned before=file.calls;
    CHECK(reader.read((pages+2)*4096+11,bulk.data(),4096));check_bytes(bulk.data(),(pages+2)*4096+11,4096);
    CHECK(file.calls==before+1);CHECK(file.last_count==4096);CHECK(reader.stats().bypasses==1);
    CHECK(reader.u8(0,one));CHECK(file.calls==before+1); // Bulk read did not evict a small-read page.
    reader.clear_stats();CHECK(reader.stats().callbacks==0&&reader.stats().cache_hits==0);
    CHECK(reader.u8(0,one));CHECK(reader.stats().cache_hits==1);
    File next;next.readable=file.readable;next.salt=83;
    CHECK(reader.reset(next.source()));CHECK(reader.u8(0,one));CHECK(one==83);CHECK(next.calls==1);
}
static void repeated_text_working_set(){
    // A generated 44-page text working set reproduces the observed capacity
    // pressure without storing private locale text or resource bytes.
    File file;file.readable=101*ResourceReader::PageSize;ResourceReader reader(file.source());
    for(unsigned cycle=0;cycle<4;++cycle){
        for(unsigned i=0;i<44;++i){
            const unsigned offset=((17*i)%101)*ResourceReader::PageSize+31;
            uint8_t bytes[23]{};CHECK(reader.read(offset,bytes,sizeof bytes));check_bytes(bytes,offset,sizeof bytes);
        }
        CHECK(file.calls==44); // All later passes stay in the bounded page cache.
    }
}
static void paired_cold_pages(){
    constexpr unsigned page=ResourceReader::PageSize,pages=ResourceReader::PageCount;
    File file;file.readable=(pages+6)*page+19;ResourceReader reader(file.source());uint8_t bytes[33]{};
    CHECK(reader.read(2*page-17,bytes,sizeof bytes));check_bytes(bytes,2*page-17,sizeof bytes);
    CHECK(file.calls==1&&file.last_offset==page&&file.last_count==2*page);
    CHECK(reader.stats().cache_misses==2&&reader.stats().callback_bytes==2*page);
    uint8_t one=0;CHECK(reader.u8(page,one));CHECK(reader.u8(3*page-1,one));CHECK(file.calls==1);
    // A cached first or second half needs just its missing neighbour, not a
    // new two-page fetch. This also exercises partial copies across slots.
    CHECK(reader.read(3*page-17,bytes,sizeof bytes));check_bytes(bytes,3*page-17,sizeof bytes);
    CHECK(file.calls==2&&file.last_offset==3*page&&file.last_count==page);
    CHECK(reader.read(page-17,bytes,sizeof bytes));check_bytes(bytes,page-17,sizeof bytes);
    CHECK(file.calls==3&&file.last_offset==0&&file.last_count==page);
    const unsigned tail=(pages+6)*page;
    CHECK(reader.read(tail-17,bytes,sizeof bytes));check_bytes(bytes,tail-17,sizeof bytes);
    CHECK(file.calls==4&&file.last_offset==tail-page&&file.last_count==page+19);
    CHECK(reader.u8(file.readable-1,one));CHECK(one==File::byte(file.readable-1,0)&&file.calls==4);

    CHECK(reader.reset(file.source()));file.calls=0;
    for(unsigned i=0;i<pages-1;++i)CHECK(reader.u8(i*page,one));
    const unsigned before=file.calls;
    CHECK(reader.read((pages+1)*page-17,bytes,sizeof bytes));check_bytes(bytes,(pages+1)*page-17,sizeof bytes);
    CHECK(file.calls==before+2); // Wrapped victims preserve the old two-read order.
    CHECK(reader.u8((pages-2)*page,one));CHECK(file.calls==before+2);

    // A failed combined read publishes neither overwritten page; unrelated
    // resident pages survive, and a later actual request can retry normally.
    const unsigned fail=(pages+3)*page;file.fail_offset=fail;
    CHECK(!reader.read(fail+page-17,bytes,sizeof bytes));
    const unsigned failed=file.calls;
    CHECK(reader.u8((pages-2)*page,one));CHECK(file.calls==failed);
    file.fail_offset=~0u;
    CHECK(reader.read(fail+page-17,bytes,sizeof bytes));check_bytes(bytes,fail+page-17,sizeof bytes);
    CHECK(file.calls==failed+1&&file.last_offset==fail&&file.last_count==2*page);
    File replacement;replacement.readable=file.readable;replacement.salt=119;
    CHECK(reader.reset(replacement.source()));
    CHECK(reader.read(fail+page-17,bytes,sizeof bytes));check_bytes(bytes,fail+page-17,sizeof bytes,119);
    CHECK(replacement.calls==1);
}
static void failures(){
    File file;file.readable=4096+2;ResourceReader reader(file.source(8192));
    uint32_t value=0x12345678;CHECK(!reader.u32(4096,value));CHECK(value==0x12345678);
    CHECK(!reader.u32(4096,value));CHECK(file.calls==2); // Failed page must never become cached.
    CHECK(reader.reset(file.source()));uint16_t two=0;
    CHECK(reader.u16(4096,two));CHECK(two==(uint16_t(File::byte(4096,0))|(uint16_t(File::byte(4097,0))<<8)));
    CHECK(file.last_count==2);
    file.readable=3*4096;file.fail_offset=4096;CHECK(reader.reset(file.source()));
    uint8_t bytes[8]{};CHECK(!reader.read(4093,bytes,8));
    CHECK(reader.u32(7,value)); // An unrelated page remains readable after a failed range.
    file.fail_offset=~0u;CHECK(reader.u32(4096,value)); // Provider failure is retryable.
    file.fail_offset=8192;std::vector<uint8_t> bulk(4096);CHECK(!reader.read(8192,bulk.data(),bulk.size()));
    CHECK(!reader.reset({}));CHECK(!reader.u8(0,bytes[0]));
}
static void memory_and_ranges(){
    std::vector<uint8_t> bytes(5*4096+29);
    for(uint32_t i=0;i<bytes.size();++i)bytes[i]=File::byte(i,0);
    ResourceReader memory(ResourceSource::memory(bytes.data(),bytes.size()));
    File file;file.readable=(uint32_t)bytes.size();ResourceReader paged(file.source());
    for(uint32_t start:{0u,1u,4093u,8191u})for(uint32_t count:{0u,1u,31u,4096u,8195u}){
        if(count>bytes.size()-start)continue;
        std::vector<uint8_t> got(count);CHECK(paged.read(start,got.data(),count));
        CHECK(!count||std::memcmp(got.data(),bytes.data()+start,count)==0);
        std::vector<uint8_t> direct(count);CHECK(memory.read(start,direct.data(),count));CHECK(direct==got);
    }
    CHECK(memory.stats().callbacks==0);CHECK(memory.stats().direct_reads>0);
    CHECK(!ResourceReader(ResourceSource::memory(nullptr,1)).valid());
    if(sizeof(size_t)>4)CHECK(!ResourceReader(ResourceSource::memory(bytes.data(),size_t(0xffffffffu)+1u)).valid());
    ResourceSource direct=file.source();direct.direct=bytes.data();file.fail_offset=0;
    ResourceReader preferred(direct);uint8_t byte=0;const unsigned calls=file.calls;
    CHECK(preferred.u8(0,byte));CHECK(file.calls==calls);
}
static void boot_display_progress(){
    constexpr unsigned pixels=320*240;
    std::vector<uint16_t> frame(pixels+2,0x55aa);auto* screen=frame.data()+1;
    const auto gold=[&](){unsigned count=0;for(unsigned x=40;x<280;++x)if(screen[170*320+x]==0xc568)++count;return count;};
    loading_screen(screen,ResourceStage::Directory,0,100,0);CHECK(gold()==0);
    loading_screen(screen,ResourceStage::Directory,50,100,0);CHECK(gold()==120);
    loading_screen(screen,ResourceStage::Directory,50,100,24);CHECK(gold()==120);
    loading_screen(screen,ResourceStage::Directory,UINT32_MAX,UINT32_MAX,0);CHECK(gold()==240);
    loading_screen(screen,ResourceStage::Images,101,100,0);CHECK(gold()==240);
    loading_screen(screen,ResourceStage::Images,0,0,0);CHECK(gold()==0);
    loading_screen(screen,static_cast<ResourceStage>(255),0,0,0);
    CHECK(frame.front()==0x55aa&&frame.back()==0x55aa);
    loading_screen(nullptr,ResourceStage::Header,0,1,0);
}
int main(){bounds();cache();repeated_text_working_set();paired_cold_pages();failures();memory_and_ranges();boot_display_progress();std::printf("resource source: %u checks passed\n",checks);}
