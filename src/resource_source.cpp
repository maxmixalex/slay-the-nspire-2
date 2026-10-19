#include "resource_source.hpp"

namespace spire {
static void resource_copy(void* destination,const uint8_t* source,uint32_t count){
    auto* out=static_cast<uint8_t*>(destination);
    for(uint32_t i=0;i<count;++i)out[i]=source[i];
}
ResourceSource ResourceSource::memory(const uint8_t* bytes,size_t size){
    if(!bytes||size>UINT32_MAX)return {};
    return {nullptr,(uint32_t)size,nullptr,bytes};
}
bool ResourceReader::reset(ResourceSource source){
    source_=source;valid_=source.direct||source.read_at;
    next_page_=0;stats_={};
    for(auto& p:pages_){p.valid=false;p.used=0;p.offset=0;}
    if(!valid_)source_={};
    return valid_;
}
bool ResourceReader::contains(uint32_t offset,uint32_t count)const{
    return valid_&&offset<=source_.length&&count<=source_.length-offset;
}
bool ResourceReader::fetch(uint32_t offset,void* dst,uint32_t count)const{
    ++stats_.callbacks;stats_.callback_bytes+=count;
    return source_.read_at&&source_.read_at(source_.context,offset,dst,count);
}
const ResourceReader::Page* ResourceReader::page(uint32_t offset)const{
    const uint32_t base=offset&~(PageSize-1u);
    for(const auto& p:pages_)if(p.valid&&p.offset==base){++stats_.cache_hits;return &p;}
    ++stats_.cache_misses;
    uint32_t slot=next_page_;
    for(uint32_t i=0;i<PageCount;++i)if(!pages_[i].valid){slot=i;break;}
    Page& p=pages_[slot];p.valid=false;
    const uint32_t remaining=source_.length-base;
    const uint32_t count=remaining<PageSize?remaining:PageSize;
    if(!fetch(base,p.data,count))return nullptr;
    p.offset=base;p.used=(uint16_t)count;p.valid=true;
    next_page_=(slot+1u)%PageCount;
    return &p;
}
bool ResourceReader::read(uint32_t offset,void* dst,uint32_t count)const{
    if(!contains(offset,count)||(count&&!dst))return false;
    if(!count)return true;
    if(source_.direct){
        ++stats_.direct_reads;resource_copy(dst,source_.direct+offset,count);return true;
    }
    if(count>=PageSize){++stats_.bypasses;return fetch(offset,dst,count);}
    auto* out=static_cast<uint8_t*>(dst);
    while(count){
        const Page* p=page(offset);if(!p)return false;
        const uint32_t in_page=offset-p->offset;
        const uint32_t available=p->used-in_page;
        const uint32_t n=count<available?count:available;
        resource_copy(out,p->data+in_page,n);
        out+=n;offset+=n;count-=n;
    }
    return true;
}
bool ResourceReader::u8(uint32_t offset,uint8_t& out)const{
    uint8_t n;if(!read(offset,&n,1))return false;out=n;return true;
}
bool ResourceReader::u16(uint32_t offset,uint16_t& out)const{
    uint8_t b[2];if(!read(offset,b,2))return false;
    out=(uint16_t)(uint16_t(b[0])|(uint16_t(b[1])<<8));return true;
}
bool ResourceReader::u32(uint32_t offset,uint32_t& out)const{
    uint8_t b[4];if(!read(offset,b,4))return false;
    out=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);return true;
}
bool ResourceReader::fnv1a(uint32_t offset,uint32_t count,uint32_t& out,uint32_t seed)const{
    if(!contains(offset,count))return false;
    uint8_t bytes[PageSize];uint32_t value=seed;
    while(count){
        const uint32_t n=count<PageSize?count:PageSize;
        if(!read(offset,bytes,n))return false;
        for(uint32_t i=0;i<n;++i){value^=bytes[i];value*=16777619u;}
        offset+=n;count-=n;
    }
    out=value;return true;
}
} // namespace spire
