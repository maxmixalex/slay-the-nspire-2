#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire {

// A single immutable resource file. read_at must fill the entire requested
// range and return true, or return false. It must not retain dst. The provider
// and context must outlive every reader using them. direct is optional and
// must address length immutable bytes when supplied.
struct ResourceSource {
    void* context{};
    uint32_t length{};
    bool (*read_at)(void*,uint32_t offset,void* dst,uint32_t count){};
    const uint8_t* direct{};
    static ResourceSource memory(const uint8_t*,size_t);
};

struct ResourceReadStats {
    uint32_t callbacks{},cache_hits{},cache_misses{},bypasses{},direct_reads{};
    uint64_t callback_bytes{};
};

// Fixed storage: eight 4 KiB pages, no allocation, no retained caller buffers.
// Small reads use a round-robin page cache. Reads of at least one full page
// bypass it, so decoding a large picture does not evict directory/string data.
// Source reset invalidates every page. No API returns a pointer into the cache.
class ResourceReader {
public:
    static constexpr uint32_t PageSize=4096,PageCount=8;
    ResourceReader()=default;
    explicit ResourceReader(ResourceSource source){reset(source);}
    bool reset(ResourceSource);
    bool valid()const{return valid_;}
    uint32_t size()const{return source_.length;}
    ResourceReadStats stats()const{return stats_;}
    void clear_stats()const{stats_={};}
    bool contains(uint32_t offset,uint32_t count)const;
    // On failure a bulk-read destination is unspecified. Integer and checksum
    // outputs are changed only on success. Zero bytes at EOF is a valid read.
    bool read(uint32_t offset,void* dst,uint32_t count)const;
    bool u8(uint32_t offset,uint8_t&)const;
    bool u16(uint32_t offset,uint16_t&)const;
    bool u32(uint32_t offset,uint32_t&)const;
    bool fnv1a(uint32_t offset,uint32_t count,uint32_t& out,
               uint32_t seed=2166136261u)const;
private:
    struct Page {uint32_t offset{};uint16_t used{};bool valid{};uint8_t data[PageSize]{};};
    ResourceSource source_{};
    bool valid_{};
    mutable Page pages_[PageCount]{};
    mutable uint32_t next_page_{};
    mutable ResourceReadStats stats_{};
    bool fetch(uint32_t offset,void* dst,uint32_t count)const;
    const Page* page(uint32_t offset)const;
};
static_assert(sizeof(ResourceReader)<=33*1024,"Resource reader must stay within its fixed 33 KiB budget");
} // namespace spire
