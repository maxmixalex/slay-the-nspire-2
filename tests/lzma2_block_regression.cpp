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
#include "fixtures/lzma2_contexts.inc"

static bool incremental(const std::vector<uint8_t>&raw,const std::vector<uint8_t>&packed,unsigned input_chunk,unsigned budget){
    std::vector<uint8_t> guarded(raw.size()+64,0xcd),workspace(Lzma2ProbabilityBytes+64,0xab);
    Lzma2BlockStream stream;
    if(!begin_lzma2_stream(stream,guarded.data()+32,raw.size(),raw.size(),packed.size(),14,
                           workspace.data()+32,Lzma2ProbabilityBytes))return false;
    uint32_t at=0;Lzma2StepResult result=Lzma2StepResult::Pending;
    unsigned steps=0;
    while(result==Lzma2StepResult::Pending){
        CHECK(++steps<raw.size()+packed.size()+100);
        const unsigned before=lzma2_stream_produced(stream),n=std::min<unsigned>(input_chunk,packed.size()-at);
        uint32_t consumed=0;
        result=step_lzma2_stream(stream,packed.data()+at,n,consumed,budget);
        CHECK(consumed<=n);at+=consumed;
        const unsigned after=lzma2_stream_produced(stream);CHECK(after>=before&&after-before<=budget);
        for(unsigned i=0;i<32;++i)CHECK(guarded[i]==0xcd&&guarded[raw.size()+32+i]==0xcd&&workspace[i]==0xab&&workspace[Lzma2ProbabilityBytes+32+i]==0xab);
        // The synchronous decoder uses its separate existing arena and must
        // not destroy a paused incremental stream's probabilities.
        if(steps==2){uint8_t one=0;const uint8_t literal[]={1,0,0,0x71,0};
            CHECK(decode_lzma2_block(&one,1,1,literal,sizeof literal,14)&&one==0x71);}
    }
    if(result!=Lzma2StepResult::Ready)return false;
    CHECK(at==packed.size()&&lzma2_stream_produced(stream)==raw.size());
    CHECK(!std::memcmp(raw.data(),guarded.data()+32,raw.size()));return true;
}

static void fixture(const std::vector<uint8_t>&raw,const std::vector<uint8_t>&packed){
    std::vector<uint8_t> guarded(raw.size()+64,0xcd);
    auto decode=[&](const std::vector<uint8_t>&input,unsigned expected,unsigned property,unsigned capacity){
        std::fill(guarded.begin(),guarded.end(),0xcd);
        const bool result=decode_lzma2_block(guarded.data()+32,capacity,expected,input.data(),input.size(),property);
        for(unsigned i=0;i<32;++i)CHECK(guarded[i]==0xcd&&guarded[raw.size()+32+i]==0xcd);
        if(expected<raw.size())CHECK(guarded[expected+32]==0xcd);
        return result;
    };
    auto good=[&](){CHECK(decode(packed,raw.size(),14,raw.size()));CHECK(!std::memcmp(raw.data(),guarded.data()+32,raw.size()));};
    good();
    CHECK(incremental(raw,packed,4096,512));
    CHECK(incremental(raw,packed,7,97));
    auto trailing=packed;trailing.push_back(0);CHECK(!incremental(raw,trailing,31,512));
    auto truncated=packed;truncated.pop_back();CHECK(!incremental(raw,truncated,4096,512));
    CHECK(!decode(packed,raw.size(),16,raw.size()));
    CHECK(!decode(packed,raw.size(),40,raw.size()));
    CHECK(!decode(packed,0,14,raw.size()));
    CHECK(!decode(packed,raw.size(),14,raw.size()-1));
    CHECK(!decode(packed,raw.size()-1,14,raw.size()));
    if(raw.size()<Lzma2BlockRawLimit)CHECK(!decode(packed,raw.size()+1,14,raw.size()));
    for(size_t n:{size_t(0),size_t(1),packed.size()/2,packed.size()-1}){
        const std::vector<uint8_t> partial(packed.begin(),packed.begin()+n);
        CHECK(!decode(partial,raw.size(),14,raw.size()));good();
    }
    for(uint8_t extra:{uint8_t(0),uint8_t(0x5a)}){
        auto appended=packed;appended.push_back(extra);CHECK(!decode(appended,raw.size(),14,raw.size()));
    }
    for(uint8_t control:{uint8_t(0),uint8_t(3),uint8_t(0x40)}){
        auto corrupt=packed;corrupt[0]=control;CHECK(!decode(corrupt,raw.size(),14,raw.size()));good();
    }
    // Payload changes may remain valid streams. Decoder framing and output
    // bounds still apply; no independent integrity gate compares their pixels.
    for(size_t at=1;at<packed.size();at+=packed.size()/17+1){
        auto corrupt=packed;corrupt[at]^=0x80;
        decode(corrupt,raw.size(),14,raw.size());
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
    std::vector<uint8_t> raw(4097);
    for(size_t i=0;i<raw.size();++i)raw[i]=(i%31)*5;
    fixture(raw,{context_4_0_1,context_4_0_1+sizeof context_4_0_1});
    fixture(raw,{context_3_1_1,context_3_1_1+sizeof context_3_1_1});
    fixture(raw,{context_3_0_0,context_3_0_0+sizeof context_3_0_0});
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
    CHECK(!decode_lzma2_block(nullptr,1,1,input,sizeof input,14));
    CHECK(!decode_lzma2_block(output,sizeof output,1,nullptr,1,14));
    CHECK(!decode_lzma2_block(output,sizeof output,1,input,Lzma2BlockEncodedLimit+1,14));
    CHECK(!decode_lzma2_block(output,Lzma2BlockRawLimit+1,Lzma2BlockRawLimit+1,input,sizeof input,14));
    CHECK(!decode_lzma2_block(input,sizeof input,1,input,sizeof input,14));
    CHECK(!decode_lzma2_block(input+1,sizeof input-1,1,input,sizeof input,14));
    CHECK(!decode_lzma2_block(input,sizeof input,2,input+1,sizeof input-1,14));
    CHECK(decode_lzma2_block(output,sizeof output,1,input,sizeof input,14)&&output[0]==0x5a);
    input[3]=0x6b;CHECK(decode_lzma2_block(output,sizeof output,1,input,sizeof input,14)&&output[0]==0x6b);
    std::printf("Bounded LZMA2: %u checks, 8 synthetic fixtures, adaptive literal contexts, 512 KiB maximum output, 28,544-byte probability arena\n",checks);
}
