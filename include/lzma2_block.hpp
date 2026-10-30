#pragma once
#include <stdint.h>

namespace spire {
// The resource codec always uses a 512 KiB dictionary (LZMA2 property 14).
// The caller's output is also the dictionary; the decoder owns no heap memory.
constexpr uint32_t Lzma2BlockRawLimit=512u*1024u;
constexpr uint32_t Lzma2BlockEncodedLimit=617291u;
constexpr uint32_t Lzma2ProbabilityBytes=28544u;
constexpr uint32_t Lzma2DictionaryProperty=14u;
constexpr uint32_t Lzma2LargeRawLimit=1024u*1024u,Lzma2LargeDictionaryProperty=16u;
constexpr uint32_t lzma2_raw_limit(unsigned property){
    return property==Lzma2DictionaryProperty?Lzma2BlockRawLimit:
        property==Lzma2LargeDictionaryProperty?Lzma2LargeRawLimit:0u;
}

// Rejects partial output, missing end markers, trailing input, an unsupported
// dictionary property and overlapping input/output. Calls are serial, as are the
// existing shared image/encoded caches. A failure may change the output bytes;
// publish a cache entry only after this function returns true.
bool decode_lzma2_block(uint8_t* output,uint32_t capacity,uint32_t expected_size,
    const uint8_t* input,uint32_t input_size,uint32_t property);

// Optional foreground hardware collection only: it must not dispatch input,
// touch decoder/resource state or advance gameplay. Null keeps the ordinary
// synchronous path. A host may bind its existing bounded input FIFO sampler.
void set_lzma2_foreground_poll(void(*poll)());

// Foreground preparation keeps its dictionary and probability bytes in the
// existing retained-picture pool. The small opaque state contains no pixels,
// owns no allocation, and never borrows the shared transfer workspace.
struct Lzma2BlockStream {alignas(uint64_t) uint8_t state[256]{};};
enum class Lzma2StepResult:uint8_t {Pending,Ready,Failed};
bool begin_lzma2_stream(Lzma2BlockStream&,uint8_t* output,uint32_t capacity,
    uint32_t expected_size,uint32_t encoded_size,uint32_t property,
    uint8_t* probabilities,uint32_t probability_capacity);
// Consumes at most the supplied input and emits at most output_budget bytes.
// A complete stream must still have the exact output, end marker and input
// length required by the ordinary synchronous decoder.
Lzma2StepResult step_lzma2_stream(Lzma2BlockStream&,const uint8_t* input,
    uint32_t input_size,uint32_t& consumed,uint32_t output_budget=512);
uint32_t lzma2_stream_produced(const Lzma2BlockStream&);
}
