#include "storage_source.hpp"
#include "lz4_block.hpp"

namespace spire {
namespace {
uint32_t word(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void copy(void*d,const uint8_t*s,uint32_t n){auto*out=static_cast<uint8_t*>(d);for(uint32_t i=0;i<n;++i)out[i]=s[i];}
bool get(const ResourceSource&s,uint32_t at,void*out,uint32_t n){
    if(at>s.length||n>s.length-at||(n&&!out))return false;
    if(!n)return true;
    if(s.direct){copy(out,s.direct+at,n);return true;}
    return s.read_at&&s.read_at(s.context,at,out,n);
}
constexpr uint32_t Mask=0x7fffffffu,Literal=0x80000000u,Limit=256u*1024u*1024u;
}

void BlockedResourceSource::close(){input_={};length_=blocks_=data_=0;cached_=UINT32_MAX;}
bool BlockedResourceSource::open(ResourceSource input){
    close();uint8_t h[32];
    if(input.context==this||!get(input,0,h,sizeof h)||word(h)!=0x334c534eu||
       word(h+4)!=3||word(h+8)!=0x41cef1eau||word(h+12)!=input.length||
       word(h+20)!=BlockBytes)return false;
    const uint32_t length=word(h+16),blocks=word(h+24),data=word(h+28);
    if(!length||length>BlockBytes*MaxBlocks||blocks!=(length+BlockBytes-1)/BlockBytes||
       data!=32+(blocks+1)*4||data>=input.length||input.length>Limit)return false;
    // The table is functional addressing metadata, not an integrity pass.
    if(!get(input,32,offsets_,(blocks+1)*4))return false;
    for(uint32_t i=0;i<=blocks;++i){
        const uint8_t*p=reinterpret_cast<const uint8_t*>(offsets_)+i*4;
        offsets_[i]=word(p);
    }
    if((offsets_[0]&Mask)!=data||offsets_[blocks]!=input.length)return false;
    for(uint32_t i=0;i<blocks;++i){
        const uint32_t at=offsets_[i]&Mask,end=offsets_[i+1]&Mask;
        const uint32_t raw=length-i*BlockBytes<BlockBytes?length-i*BlockBytes:BlockBytes;
        if(end<=at||end>input.length||((offsets_[i]&Literal)?end-at!=raw:end-at>=raw))return false;
    }
    input_=input;length_=length;blocks_=blocks;data_=data;return true;
}
bool BlockedResourceSource::read(uint32_t at,void*destination,uint32_t count)const{
    if(!length_||at>length_||count>length_-at||(count&&!destination))return false;
    auto*out=static_cast<uint8_t*>(destination);
    while(count){
        const uint32_t block=at/BlockBytes,inside=at%BlockBytes;
        const uint32_t raw=length_-block*BlockBytes<BlockBytes?length_-block*BlockBytes:BlockBytes;
        uint32_t n=raw-inside;if(n>count)n=count;
        const uint32_t physical=offsets_[block]&Mask;
        if(offsets_[block]&Literal){
            // Coalesce neighboring literal pages: existing 64 KiB directory
            // and glyph-stream reads retain their single sequential file read.
            while(n<count&&inside+n>=raw){
                const uint32_t next=(at+n)/BlockBytes;
                if(next>=blocks_||!(offsets_[next]&Literal)||
                   (offsets_[next]&Mask)!=physical+inside+n)break;
                const uint32_t remain=length_-next*BlockBytes;
                uint32_t take=remain<BlockBytes?remain:BlockBytes;
                if(take>count-n)take=count-n;n+=take;
                if(take<BlockBytes)break;
            }
            if(!get(input_,physical+inside,out,n))return false;
        }else{
            if(cached_!=block){
                cached_=UINT32_MAX;const uint32_t bytes=(offsets_[block+1]&Mask)-physical;
                if(!get(input_,physical,encoded_,bytes)||!decode_lz4(encoded_,bytes,decoded_,raw))return false;
                cached_=block;
            }
            copy(out,decoded_+inside,n);
        }
        at+=n;out+=n;count-=n;
    }
    return true;
}
bool BlockedResourceSource::read_at(void*p,uint32_t at,void*out,uint32_t n){return static_cast<BlockedResourceSource*>(p)->read(at,out,n);}
void BlockedResourceSource::progress_at(void*p,ResourceStage s,uint32_t n,uint32_t total){
    auto&self=*static_cast<BlockedResourceSource*>(p);if(self.input_.progress)self.input_.progress(self.input_.context,s,n,total);
}
ResourceSource BlockedResourceSource::source(){return length_?ResourceSource{this,length_,read_at,nullptr,progress_at}:ResourceSource{};}

void MultipartResourceSource::close(){length_=span_count_=part_count_=last_=0;for(auto&p:parts_)p={};}
bool MultipartResourceSource::open(ResourceSource index,const ResourceSource*parts,unsigned count){
    close();uint8_t h[96];
    if(!parts||count!=MaxParts||index.context==this||!get(index,0,h,sizeof h)||
       word(h)!=0x314d534eu||word(h+4)!=1||word(h+8)!=0x41cef1eau||word(h+12)!=96||
       word(h+16)!=index.length||word(h+28)!=count)return false;
    const uint32_t length=word(h+20),spans=word(h+24);
    if(length<=64||length>Limit||!spans||spans>MaxSpans||index.length!=96+spans*16||
       word(h+32)!=0x3143534eu||word(h+36)!=1||word(h+40)!=0x41cef1eau||
       word(h+44)!=64||word(h+48)!=length||word(h+52)!=length-64)return false;
    for(unsigned i=0;i<count;++i){
        uint8_t p[32];
        if(parts[i].context==this||!get(parts[i],0,p,sizeof p)||word(p)!=0x3153534eu||
           word(p+4)!=1||word(p+8)!=0x41cef1eau||word(p+12)!=32||
           word(p+16)!=parts[i].length||word(p+20)!=i||word(p+24)||word(p+28))return false;
    }
    if(!get(index,96,spans_,spans*16))return false;
    uint32_t end=64;
    for(uint32_t i=0;i<spans;++i){
        uint8_t*p=reinterpret_cast<uint8_t*>(spans_+i);
        const Span row{word(p),word(p+4),word(p+8),word(p+12)};
        if(row.first!=end||row.end<=row.first||row.end>length||row.part>=count||
           row.physical<32||row.physical>parts[row.part].length||
           row.end-row.first>parts[row.part].length-row.physical)return false;
        spans_[i]=row;end=row.end;
    }
    if(end!=length)return false;
    for(unsigned i=0;i<count;++i)parts_[i]=parts[i];
    copy(header_,h+32,64);length_=length;span_count_=spans;part_count_=count;return true;
}
bool MultipartResourceSource::read(uint32_t at,void*destination,uint32_t count)const{
    if(!length_||at>length_||count>length_-at||(count&&!destination))return false;
    if(!count)return true;
    auto*out=static_cast<uint8_t*>(destination);
    if(at<64){uint32_t n=64-at;if(n>count)n=count;copy(out,header_+at,n);at+=n;out+=n;count-=n;}
    while(count){
        uint32_t i=last_;
        if(i>=span_count_||at<spans_[i].first||at>=spans_[i].end){
            uint32_t low=0,high=span_count_;
            while(low<high){const uint32_t mid=low+(high-low)/2;if(spans_[mid].end<=at)low=mid+1;else high=mid;}
            i=low;if(i>=span_count_||at<spans_[i].first)return false;last_=i;
        }
        const Span&s=spans_[i];uint32_t n=s.end-at;if(n>count)n=count;
        if(!get(parts_[s.part],s.physical+at-s.first,out,n))return false;
        at+=n;out+=n;count-=n;
    }
    return true;
}
bool MultipartResourceSource::read_at(void*p,uint32_t at,void*out,uint32_t n){return static_cast<MultipartResourceSource*>(p)->read(at,out,n);}
ResourceSource MultipartResourceSource::source(){return length_?ResourceSource{this,length_,read_at}:ResourceSource{};}
}
