#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire {

// Progress reports completed work within one named phase, not an estimated
// fraction of total elapsed time. A sink must not re-enter the resource reader.
enum class ResourceStage : uint8_t { Header,Integrity,Directory,Images,TileIndex,Glyphs,Strings,Ready,Count };
using ResourceProgress=void(*)(void*,ResourceStage,uint32_t completed,uint32_t total);

// A single immutable resource file. read_at must fill the entire requested
// range and return true, or return false. It must not retain dst. The provider
// and context must outlive every reader using them. direct is optional and
// must address length immutable bytes when supplied.
struct ResourceSource {
    void* context{};
    uint32_t length{};
    bool (*read_at)(void*,uint32_t offset,void* dst,uint32_t count){};
    const uint8_t* direct{};
    ResourceProgress progress{};
    static ResourceSource memory(const uint8_t*,size_t);
};

enum class ResourceLanguage : uint32_t { English=1,Chinese=2 };

// An NSC1 artwork file plus an NSL1 language file presents the exact original
// NSP2 byte stream. Only the directory and glyph/text sections are localized;
// compressed animation groups and image pixels live once in the common file.
// Both inputs and this adapter must outlive the returned source. Opening checks
// the container mapping bounds and actual NSP2 locale marker. Historical
// digest fields remain in the container layout but are not runtime gates.
class SplitResourceSource {
public:
    bool open(ResourceSource common,ResourceSource language,ResourceLanguage expected);
    void close();
    bool valid()const{return valid_;}
    ResourceLanguage language()const{return language_id_;}
    ResourceSource source();
    bool read(uint32_t offset,void* dst,uint32_t count)const;
private:
    ResourceSource common_{},localized_{};
    uint32_t prefix_{},artwork_{},length_{};
    ResourceLanguage language_id_{ResourceLanguage::English};
    bool valid_{};
    static bool read_at(void*,uint32_t,void*,uint32_t);
    static void progress_at(void*,ResourceStage,uint32_t,uint32_t);
};
static_assert(sizeof(SplitResourceSource)<=256,"Split resources must not require a resident pack or mapping table");

struct ResourceReadStats {
    uint32_t callbacks{},cache_hits{},cache_misses{},bypasses{},direct_reads{};
    uint64_t callback_bytes{};
};

// Fixed storage: sixty-four 4 KiB pages, no allocation or retained caller buffers.
// Observed Neow text misses span 44 pages: the former eight-page reader
// refetched glyph/string pages each frame even with no animation group reads.
// Small reads use a round-robin page cache. Reads of at least one full page
// bypass it, so decoding a large picture does not evict directory/string data.
// A small range crossing two cold pages fills both with one source call. The
// same sixty-four pages are contiguous; this adds no read-ahead or pixel pool.
// Source reset invalidates every page. No API returns a pointer into the cache.
class ResourceReader {
public:
    static constexpr uint32_t PageSize=4096,PageCount=64;
    ResourceReader()=default;
    explicit ResourceReader(ResourceSource source){reset(source);}
    bool reset(ResourceSource);
    bool valid()const{return valid_;}
    uint32_t size()const{return source_.length;}
    ResourceReadStats stats()const{return stats_;}
    void clear_stats()const{stats_={};}
    bool contains(uint32_t offset,uint32_t count)const;
    // On failure a bulk-read destination is unspecified. Integer
    // outputs are changed only on success. Zero bytes at EOF is a valid read.
    bool read(uint32_t offset,void* dst,uint32_t count)const;
    bool u8(uint32_t offset,uint8_t&)const;
    bool u16(uint32_t offset,uint16_t&)const;
    bool u32(uint32_t offset,uint32_t&)const;
    void progress(ResourceStage stage,uint32_t completed,uint32_t total)const{
        if(source_.progress)source_.progress(source_.context,stage,completed,total);
    }
private:
    struct Page {uint32_t offset{};uint16_t used{};bool valid{};};
    ResourceSource source_{};
    bool valid_{};
    mutable Page pages_[PageCount]{};
    mutable uint8_t page_bytes_[PageCount*PageSize]{};
    mutable uint32_t next_page_{};
    mutable ResourceReadStats stats_{};
    bool fetch(uint32_t offset,void* dst,uint32_t count)const;
    const Page* page(uint32_t offset)const;
    bool read_cold_pair(uint32_t slot,uint32_t offset,void* dst,uint32_t count)const;
};
static_assert(sizeof(ResourceReader)<=257*1024,"Resource reader must stay within its fixed 257 KiB budget");
} // namespace spire
