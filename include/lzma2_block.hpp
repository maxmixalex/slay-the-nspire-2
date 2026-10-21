#pragma once
#include <stdint.h>

namespace spire {
// The resource codec always uses a 512 KiB dictionary (LZMA2 property 14).
// The caller's output is also the dictionary; the decoder owns no heap memory.
constexpr uint32_t Lzma2BlockRawLimit=512u*1024u;
constexpr uint32_t Lzma2BlockEncodedLimit=617291u;
constexpr uint32_t Lzma2ProbabilityBytes=28544u;
constexpr uint32_t Lzma2DictionaryProperty=14u;

uint32_t resource_crc32(const uint8_t* bytes,uint32_t count);
// Rejects partial output, missing end markers, trailing input, an unsupported
// dictionary property, overlapping input/output and a checksum mismatch.
// Calls are serial, as are the
// existing shared image/encoded caches. A failure may change the output bytes;
// publish a cache entry only after this function returns true.
bool decode_lzma2_block(uint8_t* output,uint32_t capacity,uint32_t expected_size,
    const uint8_t* input,uint32_t input_size,uint32_t property,uint32_t expected_crc);
}
