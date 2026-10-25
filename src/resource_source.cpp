#include "resource_source.hpp"
#include "game_over_transition_assets.hpp"
#include "grouped_glyphs.hpp"
#include "grouped_strings.hpp"
#include "bestiary_kaiser_assets.hpp"
#include "scene_font_assets.hpp"

namespace spire {
static void resource_copy(void* destination,const uint8_t* source,uint32_t count){
    auto* out=static_cast<uint8_t*>(destination);
    for(uint32_t i=0;i<count;++i)out[i]=source[i];
}
ResourceSource ResourceSource::memory(const uint8_t* bytes,size_t size){
    if(!bytes||size>UINT32_MAX)return {};
    return {nullptr,(uint32_t)size,nullptr,bytes};
}
namespace {
constexpr uint32_t CommonHeader=64,LanguageHeader=96,SplitLimit=256u*1024u*1024u;
static uint32_t split_u32(const uint8_t* p){
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
static bool split_read(const ResourceSource& source,uint32_t offset,void* dst,uint32_t count){
    if(offset>source.length||count>source.length-offset||(count&&!dst))return false;
    if(!count)return true;
    if(source.direct){resource_copy(dst,source.direct+offset,count);return true;}
    return source.read_at&&source.read_at(source.context,offset,dst,count);
}
}
void SplitResourceSource::close(){
    common_={};localized_={};prefix_=artwork_=length_=0;valid_=false;
    language_id_=ResourceLanguage::English;
}
bool SplitResourceSource::open(ResourceSource common,ResourceSource localized,ResourceLanguage expected){
    close();
    if((expected!=ResourceLanguage::English&&expected!=ResourceLanguage::Chinese)||
       common.context==this||localized.context==this)return false;
    uint8_t c[CommonHeader],l[LanguageHeader];
    if(!split_read(common,0,c,sizeof c)||!split_read(localized,0,l,sizeof l))return false;
    if(split_u32(c)!=0x3143534Eu||split_u32(l)!=0x314C534Eu||
       split_u32(c+4)!=1||split_u32(l+4)!=1||
       split_u32(c+8)!=0x41cef1eau||split_u32(l+8)!=0x41cef1eau||
       split_u32(c+12)!=CommonHeader||split_u32(l+12)!=LanguageHeader||
       split_u32(c+16)!=common.length||split_u32(l+16)!=localized.length||
       split_u32(l+36)!=uint32_t(expected))return false;
    const uint32_t total=split_u32(l+20),prefix=split_u32(l+24),art=split_u32(l+28),suffix=split_u32(l+32);
    if(total>SplitLimit||prefix<40||prefix>total||!art||art>total-prefix||
       !suffix||suffix!=total-prefix-art||split_u32(c+20)!=art||
       common.length-CommonHeader!=art||
       localized.length-LanguageHeader!=prefix+suffix)return false;
    common_=common;localized_=localized;prefix_=prefix;artwork_=art;length_=total;
    language_id_=expected;valid_=true;
    uint8_t h[40],row[8],marker[4];
    if(!read(0,h,sizeof h)||split_u32(h)!=0x3250534Eu||
       (split_u32(h+4)<17||split_u32(h+4)>SceneFontContentVersion)||split_u32(h+8)!=0x41cef1eau||
       split_u32(h+12)!=total){
        close();return false;
    }
    const uint32_t ni=split_u32(h+16),ng=split_u32(h+20),ns=split_u32(h+24);
    if(!ni||ni>65535||!ng||ng>scene_font_glyph_limit(split_u32(h+4))||ns<=6110||ns>16384){close();return false;}
    const uint32_t marker_row=40+ni*16+ng*16+6110*8;
    if(marker_row>prefix||8>prefix-marker_row||!read(marker_row,row,sizeof row)){
        close();return false;
    }
    const uint32_t marker_at=split_u32(row);
    if(split_u32(row+4)!=3||marker_at<prefix+art||!read(marker_at,marker,sizeof marker)||
       marker[3]||(expected==ResourceLanguage::English?
         marker[0]!='e'||marker[1]!='n'||marker[2]!='g':
         marker[0]!='z'||marker[1]!='h'||marker[2]!='s')){
        close();return false;
    }
    return true;
}
bool SplitResourceSource::read(uint32_t offset,void* dst,uint32_t count)const{
    if(!valid_||offset>length_||count>length_-offset||(count&&!dst))return false;
    auto* out=static_cast<uint8_t*>(dst);
    while(count){
        const ResourceSource* input=&localized_;
        uint32_t physical=0,available=0;
        if(offset<prefix_){physical=LanguageHeader+offset;available=prefix_-offset;}
        else if(offset-prefix_<artwork_){
            input=&common_;physical=CommonHeader+offset-prefix_;available=artwork_-(offset-prefix_);
        }else {physical=LanguageHeader+offset-artwork_;available=length_-offset;}
        const uint32_t n=count<available?count:available;
        if(!split_read(*input,physical,out,n))return false;
        out+=n;offset+=n;count-=n;
    }
    return true;
}
bool SplitResourceSource::read_at(void* context,uint32_t offset,void* dst,uint32_t count){
    return static_cast<SplitResourceSource*>(context)->read(offset,dst,count);
}
void SplitResourceSource::progress_at(void* context,ResourceStage stage,uint32_t completed,uint32_t total){
    const auto& self=*static_cast<SplitResourceSource*>(context);
    const auto& input=self.localized_.progress?self.localized_:self.common_;
    if(input.progress)input.progress(input.context,stage,completed,total);
}
ResourceSource SplitResourceSource::source(){
    if(!valid_)return {};
    return {this,length_,read_at,nullptr,progress_at};
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
    if(!fetch(base,page_bytes_+slot*PageSize,count))return nullptr;
    p.offset=base;p.used=(uint16_t)count;p.valid=true;
    next_page_=(slot+1u)%PageCount;
    return &p;
}
bool ResourceReader::read_cold_pair(uint32_t slot,uint32_t offset,void* dst,uint32_t count)const{
    // The requested bytes straddle two absent pages. Read exactly the same
    // page ranges as two ordinary misses, into adjacent existing slots.
    const uint32_t base=offset&~(PageSize-1u);
    const uint32_t remaining=source_.length-base;
    const uint32_t bytes=remaining<2*PageSize?remaining:2*PageSize;
    Page& first=pages_[slot];Page& second=pages_[slot+1];
    first.valid=second.valid=false;stats_.cache_misses+=2;
    uint8_t* data=page_bytes_+slot*PageSize;
    if(!fetch(base,data,bytes))return false;
    first.offset=base;first.used=PageSize;first.valid=true;
    second.offset=base+PageSize;second.used=uint16_t(bytes-PageSize);second.valid=true;
    next_page_=(slot+2u)%PageCount;
    resource_copy(dst,data+(offset-base),count);return true;
}
bool ResourceReader::read(uint32_t offset,void* dst,uint32_t count)const{
    if(!contains(offset,count)||(count&&!dst))return false;
    if(!count)return true;
    if(source_.direct){
        ++stats_.direct_reads;resource_copy(dst,source_.direct+offset,count);return true;
    }
    if(count>=PageSize){++stats_.bypasses;return fetch(offset,dst,count);}
    const uint32_t base=offset&~(PageSize-1u);
    if(count>PageSize-(offset-base)){
        bool first=false,second=false;
        for(const auto& p:pages_)if(p.valid){first|=p.offset==base;second|=p.offset==base+PageSize;}
        // A resident half must stay useful: fetching just the missing page
        // avoids rereading bytes or evicting another page unnecessarily.
        if(!first&&!second){
            uint32_t slot=next_page_,next=PageCount;
            for(uint32_t i=0;i<PageCount;++i)if(!pages_[i].valid){slot=i;break;}
            for(uint32_t i=0;i<PageCount;++i)if(i!=slot&&!pages_[i].valid){next=i;break;}
            if(next==PageCount)next=(slot+1u)%PageCount;
            // Preserve the original replacement order, including its wrap.
            // Nonadjacent victims use the ordinary two reads below instead
            // of evicting a different page just to combine this request.
            if(next==slot+1u)return read_cold_pair(slot,offset,dst,count);
        }
    }
    auto* out=static_cast<uint8_t*>(dst);
    while(count){
        const Page* p=page(offset);if(!p)return false;
        const uint32_t in_page=offset-p->offset;
        const uint32_t available=p->used-in_page;
        const uint32_t n=count<available?count:available;
        resource_copy(out,page_bytes_+unsigned(p-pages_)*PageSize+in_page,n);
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
} // namespace spire
