#pragma once
#include "resource_source_base.hpp"

namespace spire {

// Small independent LZ4 pages around locale containers. They may be read while
// an image owns ResourceTransferLease: this adapter never borrows that scratch.
// Literal pages are read directly and compressed pages use one 4 KiB cache.
class BlockedResourceSource {
public:
    static constexpr uint32_t BlockBytes=4096,MaxBlocks=4096;
    bool open(ResourceSource);
    void close();
    ResourceSource source();
    bool read(uint32_t,void*,uint32_t)const;
private:
    ResourceSource input_{};
    uint32_t length_{},blocks_{},data_{};
    uint32_t offsets_[MaxBlocks+1]{};
    mutable uint32_t cached_=UINT32_MAX;
    mutable uint8_t decoded_[BlockBytes]{},encoded_[BlockBytes]{};
    static bool read_at(void*,uint32_t,void*,uint32_t);
    static void progress_at(void*,ResourceStage,uint32_t,uint32_t);
};

// An NSM1 table maps the original immutable NSC1 artwork address space onto
// independently replaceable concern files. Rendering performs only a memory
// lookup and the original requested file read; no image is copied at startup.
class MultipartResourceSource {
public:
    static constexpr unsigned MaxParts=8,MaxSpans=8192;
    bool open(ResourceSource index,const ResourceSource* parts,unsigned count);
    void close();
    ResourceSource source();
    bool read(uint32_t,void*,uint32_t)const;
private:
    struct Span {uint32_t first,end,physical,part;};
    ResourceSource parts_[MaxParts]{};
    Span spans_[MaxSpans]{};
    uint8_t header_[64]{};
    uint32_t length_{},span_count_{},part_count_{};
    mutable uint32_t last_=0;
    static bool read_at(void*,uint32_t,void*,uint32_t);
};

constexpr const char* ArtworkPartNames[MultipartResourceSource::MaxParts]={
    "nSpire2.interface.dat.tns","nSpire2.cards.dat.tns",
    "nSpire2.characters.dat.tns","nSpire2.enemies.dat.tns",
    "nSpire2.bosses.dat.tns","nSpire2.rooms.dat.tns",
    "nSpire2.effects.dat.tns","nSpire2.shared.dat.tns"
};
}
