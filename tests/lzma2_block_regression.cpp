#include "lzma2_block.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"LZMA2 failure at %u: %s\n",unsigned(__LINE__),#x);std::exit(1);}}while(0)
#include "fixtures/lzma2_blocks.inc"

static uint32_t crc_oracle(const uint8_t*bytes,size_t count){
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<count;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
    return crc^0xffffffffu;
}
static void fixture(const std::vector<uint8_t>&raw,const std::vector<uint8_t>&packed){
    const auto crc=crc_oracle(raw.data(),raw.size());CHECK(resource_crc32(raw.data(),raw.size())==crc);
    std::vector<uint8_t> guarded(raw.size()+64,0xcd);
    auto decode=[&](const std::vector<uint8_t>&input,unsigned expected,unsigned property,unsigned checksum,unsigned capacity){
        std::fill(guarded.begin(),guarded.end(),0xcd);
        const bool result=decode_lzma2_block(guarded.data()+32,capacity,expected,input.data(),input.size(),property,checksum);
        for(unsigned i=0;i<32;++i)CHECK(guarded[i]==0xcd&&guarded[raw.size()+32+i]==0xcd);
        if(expected<raw.size())CHECK(guarded[expected+32]==0xcd);
        return result;
    };
    auto good=[&](){CHECK(decode(packed,raw.size(),14,crc,raw.size()));CHECK(!std::memcmp(raw.data(),guarded.data()+32,raw.size()));};
    good();
    CHECK(!decode(packed,raw.size(),16,crc,raw.size()));
    CHECK(!decode(packed,raw.size(),40,crc,raw.size()));
    CHECK(!decode(packed,raw.size(),14,crc^1,raw.size()));
    CHECK(!decode(packed,0,14,crc,raw.size()));
    CHECK(!decode(packed,raw.size(),14,crc,raw.size()-1));
    CHECK(!decode(packed,raw.size()-1,14,crc,raw.size()));
    if(raw.size()<Lzma2BlockRawLimit)CHECK(!decode(packed,raw.size()+1,14,crc,raw.size()));
    for(size_t n:{size_t(0),size_t(1),packed.size()/2,packed.size()-1}){
        const std::vector<uint8_t> partial(packed.begin(),packed.begin()+n);
        CHECK(!decode(partial,raw.size(),14,crc,raw.size()));good();
    }
    for(uint8_t extra:{uint8_t(0),uint8_t(0x5a)}){
        auto appended=packed;appended.push_back(extra);CHECK(!decode(appended,raw.size(),14,crc,raw.size()));
    }
    for(uint8_t control:{uint8_t(0),uint8_t(3),uint8_t(0x40)}){
        auto corrupt=packed;corrupt[0]=control;CHECK(!decode(corrupt,raw.size(),14,crc,raw.size()));good();
    }
    // Exercise a damaged literal/range-coding region, with the checksum as an
    // independent oracle if the stream remains structurally decodable.
    for(size_t at=1;at<packed.size();at+=packed.size()/17+1){
        auto corrupt=packed;corrupt[at]^=0x80;
        const bool accepted=decode(corrupt,raw.size(),14,crc,raw.size());
        if(accepted)CHECK(!std::memcmp(raw.data(),guarded.data()+32,raw.size()));
    }
    good();
}
static std::vector<uint8_t> uncompressed(const std::vector<uint8_t>&raw){
    std::vector<uint8_t> result;
    for(size_t at=0;at<raw.size();){
        const size_t size=raw.size()-at>65536?65536:raw.size()-at;
        result.push_back(at?2:1);result.push_back((size-1)>>8);result.push_back(size-1);
        result.insert(result.end(),raw.begin()+at,raw.begin()+at+size);at+=size;
    }
    result.push_back(0);return result;
}
int main(){
    CHECK(resource_crc32(reinterpret_cast<const uint8_t*>("123456789"),9)==0xcbf43926u);
    CHECK(resource_crc32(nullptr,0)==0);
    std::vector<uint8_t> raw(4097);
    for(size_t i=0;i<raw.size();++i)raw[i]=i%251;
    fixture(raw,{compressed_ramp,compressed_ramp+sizeof compressed_ramp});
    raw.assign(Lzma2BlockRawLimit,0);fixture(raw,{compressed_zeros,compressed_zeros+sizeof compressed_zeros});
    for(size_t i=0;i<raw.size();++i)raw[i]=((i/97)^(i*17))&255u;
    fixture(raw,{compressed_mixed,compressed_mixed+sizeof compressed_mixed});
    uint32_t random=0x12345678;
    for(auto&v:raw){random^=random<<13;random^=random>>17;random^=random<<5;v=random>>24;}
    fixture(raw,uncompressed(raw));
    raw.assign(1,0x5a);fixture(raw,uncompressed(raw));
    uint8_t input[]={1,0,0,0x5a,0},output[32]{};
    CHECK(!decode_lzma2_block(nullptr,1,1,input,sizeof input,14,0));
    CHECK(!decode_lzma2_block(output,sizeof output,1,nullptr,1,14,0));
    CHECK(!decode_lzma2_block(output,sizeof output,1,input,Lzma2BlockEncodedLimit+1,14,0));
    CHECK(!decode_lzma2_block(output,Lzma2BlockRawLimit+1,Lzma2BlockRawLimit+1,input,sizeof input,14,0));
    CHECK(!decode_lzma2_block(input,sizeof input,1,input,sizeof input,14,0));
    CHECK(!decode_lzma2_block(input+1,sizeof input-1,1,input,sizeof input,14,0));
    CHECK(!decode_lzma2_block(input,sizeof input,2,input+1,sizeof input-1,14,0));
    CHECK(decode_lzma2_block(output,sizeof output,1,input,sizeof input,14,resource_crc32(input+3,1))&&output[0]==0x5a);
    std::printf("Bounded LZMA2: %u checks, 5 synthetic fixtures, 512 KiB maximum output, 28,544-byte probability arena\n",checks);
}
