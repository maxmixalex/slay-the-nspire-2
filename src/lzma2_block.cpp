#include "lzma2_block.hpp"
#include "Lzma2Dec.h"
#include "inplace_reset.hpp"

namespace spire {
namespace {
struct ProbabilityArena {
    CLzmaProb values[Lzma2ProbabilityBytes/2]{};
    unsigned allocations=0,frees=0;
    bool occupied=false,fault=false;
};
static_assert(sizeof(CLzmaProb)==2,"The fixed arena requires the SDK UInt16 ABI");
static_assert(sizeof(ProbabilityArena::values)==Lzma2ProbabilityBytes,
              "Probability storage must remain exactly bounded");
ProbabilityArena arena;
void(*foreground_poll)()=nullptr;
void* probability_alloc(ISzAllocPtr,size_t);
void probability_free(ISzAllocPtr,void*);
const ISzAlloc allocator={probability_alloc,probability_free};
void* probability_alloc(ISzAllocPtr source,size_t count){
    if(source!=&allocator||arena.occupied||count!=Lzma2ProbabilityBytes){arena.fault=true;return nullptr;}
    arena.occupied=true;++arena.allocations;return arena.values;
}
void probability_free(ISzAllocPtr source,void* address){
    if(!address)return;
    if(source!=&allocator||address!=arena.values||!arena.occupied){arena.fault=true;return;}
    arena.occupied=false;++arena.frees;
}
}
void set_lzma2_foreground_poll(void(*poll)()){foreground_poll=poll;}
bool decode_lzma2_block(uint8_t* output,uint32_t capacity,uint32_t expected_size,
    const uint8_t* input,uint32_t input_size,uint32_t property){
    if(arena.occupied||!output||!input||!expected_size||expected_size>Lzma2BlockRawLimit||
       capacity<expected_size||!input_size||input_size>Lzma2BlockEncodedLimit||
       property!=Lzma2DictionaryProperty)return false;
    const uintptr_t destination=(uintptr_t)output,source=(uintptr_t)input;
    if(destination<=source?source-destination<expected_size:destination-source<input_size)return false;
    arena.allocations=arena.frees=0;arena.fault=false;
    if(foreground_poll){
        // Reuse the synchronous probability arena and the existing streaming
        // decoder. Each foreground return produces at most4KiB; there is no
        // new pixel/dictionary allocation and no change to stream framing.
        Lzma2BlockStream stream;
        arena.occupied=true;
        bool valid=begin_lzma2_stream(stream,output,capacity,expected_size,input_size,property,
            reinterpret_cast<uint8_t*>(arena.values),Lzma2ProbabilityBytes);
        uint32_t at=0;auto result=Lzma2StepResult::Pending;
        while(valid&&result==Lzma2StepResult::Pending){
            uint32_t consumed=0;
            result=step_lzma2_stream(stream,input+at,input_size-at,consumed,4096);
            at+=consumed;
        }
        arena.occupied=false;
        return valid&&result==Lzma2StepResult::Ready&&at==input_size;
    }
    SizeT produced=expected_size,consumed=input_size;
    ELzmaStatus status=LZMA_STATUS_NOT_SPECIFIED;
    const SRes result=Lzma2Decode(output,&produced,input,&consumed,(Byte)property,
                                  LZMA_FINISH_END,&status,&allocator);
    if(arena.occupied||arena.fault||arena.allocations!=1||arena.frees!=1)return false;
    return result==SZ_OK&&status==LZMA_STATUS_FINISHED_WITH_MARK&&
        consumed==input_size&&produced==expected_size;
}
namespace {
struct StreamState {
    ISzAlloc allocator;
    CLzma2Dec decoder;
    uint8_t* probabilities;
    uint32_t expected,encoded,consumed;
    bool allocated,valid,finished;
};
static_assert(sizeof(StreamState)<=sizeof(Lzma2BlockStream::state),
              "Incremental decoder state must fit its fixed 256-byte envelope");
static_assert(alignof(StreamState)<=alignof(Lzma2BlockStream),"Decoder state alignment");
void* stream_allocate(ISzAllocPtr alloc,size_t count){
    auto&s=*reinterpret_cast<StreamState*>(const_cast<ISzAlloc*>(alloc));
    if(s.allocated||count!=Lzma2ProbabilityBytes)return nullptr;
    s.allocated=true;return s.probabilities;
}
void stream_free(ISzAllocPtr,void*){}
StreamState& stream_state(Lzma2BlockStream&s){return *reinterpret_cast<StreamState*>(s.state);}
const StreamState& stream_state(const Lzma2BlockStream&s){return *reinterpret_cast<const StreamState*>(s.state);}
bool disjoint(const uint8_t*a,uint32_t an,const uint8_t*b,uint32_t bn){
    const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
    return x<=y?y-x>=an:x-y>=bn;
}
}
bool begin_lzma2_stream(Lzma2BlockStream&storage,uint8_t*output,uint32_t capacity,
    uint32_t expected,uint32_t encoded,uint32_t property,uint8_t*probabilities,uint32_t probability_capacity){
    auto&s=*::new(detail::InPlaceAddress{storage.state}) StreamState{};
    if(!output||!probabilities||!expected||expected>Lzma2BlockRawLimit||capacity<expected||
       !encoded||encoded>Lzma2BlockEncodedLimit||property!=Lzma2DictionaryProperty||
       probability_capacity<Lzma2ProbabilityBytes||(reinterpret_cast<uintptr_t>(probabilities)&1u)||
       !disjoint(output,expected,probabilities,Lzma2ProbabilityBytes))return false;
    s.allocator={stream_allocate,stream_free};s.probabilities=probabilities;
    s.expected=expected;s.encoded=encoded;
    Lzma2Dec_Construct(&s.decoder);
    if(Lzma2Dec_AllocateProbs(&s.decoder,Byte(property),&s.allocator)!=SZ_OK||!s.allocated)return false;
    s.decoder.decoder.dic=output;s.decoder.decoder.dicBufSize=expected;
    Lzma2Dec_Init(&s.decoder);s.valid=true;return true;
}
Lzma2StepResult step_lzma2_stream(Lzma2BlockStream&storage,const uint8_t*input,
    uint32_t input_size,uint32_t&consumed,uint32_t output_budget){
    consumed=0;auto&s=stream_state(storage);
    if(!s.valid||s.finished||!input||!output_budget||input_size>s.encoded-s.consumed||
       !disjoint(s.decoder.decoder.dic,s.expected,input,input_size)||
       !disjoint(s.probabilities,Lzma2ProbabilityBytes,input,input_size))return Lzma2StepResult::Failed;
    const uint32_t before=uint32_t(s.decoder.decoder.dicPos),remaining=s.expected-before;
    // Incremental preload also runs on the foreground. Capture at the same
    // bounded decoder boundaries, including calls that consume only headers.
    const auto poll=foreground_poll;
    if(poll){if(output_budget>4096)output_budget=4096;poll();}
    const uint32_t count=remaining<output_budget?remaining:output_budget,limit=before+count;
    SizeT available=input_size;ELzmaStatus status=LZMA_STATUS_NOT_SPECIFIED;
    const SRes code=Lzma2Dec_DecodeToDic(&s.decoder,limit,input,&available,
        limit==s.expected?LZMA_FINISH_END:LZMA_FINISH_ANY,&status);
    if(poll)poll();
    consumed=uint32_t(available);s.consumed+=consumed;
    const uint32_t produced=uint32_t(s.decoder.decoder.dicPos);
    if(code!=SZ_OK||produced<before||produced-before>output_budget){s.valid=false;return Lzma2StepResult::Failed;}
    if(status==LZMA_STATUS_FINISHED_WITH_MARK){
        s.finished=produced==s.expected&&s.consumed==s.encoded;
        if(!s.finished)s.valid=false;
        return s.finished?Lzma2StepResult::Ready:Lzma2StepResult::Failed;
    }
    if((status==LZMA_STATUS_NEEDS_MORE_INPUT&&s.consumed==s.encoded)||
       (!consumed&&produced==before&&input_size)){s.valid=false;return Lzma2StepResult::Failed;}
    return Lzma2StepResult::Pending;
}
uint32_t lzma2_stream_produced(const Lzma2BlockStream&storage){
    return uint32_t(stream_state(storage).decoder.decoder.dicPos);
}
}
