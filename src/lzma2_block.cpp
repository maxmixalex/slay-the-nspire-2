#include "lzma2_block.hpp"
#include "Lzma2Dec.h"

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
// A constant 1 KiB table avoids eight bit steps for every reconstructed byte.
// Its C++17 constant initialization needs no startup constructor or RAM copy.
struct CrcTable {
    uint32_t values[256]{};
    constexpr CrcTable(){
        for(unsigned i=0;i<256;++i){
            uint32_t value=i;
            for(unsigned bit=0;bit<8;++bit)value=(value>>1)^(0xedb88320u&(0u-(value&1u)));
            values[i]=value;
        }
    }
};
constexpr CrcTable crc_table{};
}
uint32_t resource_crc32(const uint8_t* bytes,uint32_t count){
    uint32_t crc=0xffffffffu;
    for(uint32_t i=0;i<count;++i)crc=(crc>>8)^crc_table.values[(crc^bytes[i])&255u];
    return crc^0xffffffffu;
}
bool decode_lzma2_block(uint8_t* output,uint32_t capacity,uint32_t expected_size,
    const uint8_t* input,uint32_t input_size,uint32_t property,uint32_t expected_crc){
    if(arena.occupied||!output||!input||!expected_size||expected_size>Lzma2BlockRawLimit||
       capacity<expected_size||!input_size||input_size>Lzma2BlockEncodedLimit||
       property!=Lzma2DictionaryProperty)return false;
    const uintptr_t destination=(uintptr_t)output,source=(uintptr_t)input;
    if(destination<=source?source-destination<expected_size:destination-source<input_size)return false;
    arena.allocations=arena.frees=0;arena.fault=false;
    SizeT produced=expected_size,consumed=input_size;
    ELzmaStatus status=LZMA_STATUS_NOT_SPECIFIED;
    const SRes result=Lzma2Decode(output,&produced,input,&consumed,(Byte)property,
                                  LZMA_FINISH_END,&status,&allocator);
    if(arena.occupied||arena.fault||arena.allocations!=1||arena.frees!=1)return false;
    return result==SZ_OK&&status==LZMA_STATUS_FINISHED_WITH_MARK&&
        consumed==input_size&&produced==expected_size&&resource_crc32(output,expected_size)==expected_crc;
}
}
