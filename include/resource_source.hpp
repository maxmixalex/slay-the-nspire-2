#pragma once
#include <stddef.h>
#include <stdint.h>
#include "locale_languages.hpp"
#include "resource_source_base.hpp"
#include "storage_source.hpp"

namespace spire {


enum class ResourceLanguage : uint32_t { English=1,Chinese=2,TraditionalChinese=3,German=4,LatinAmericanSpanish=5,French=6,Indonesian=7,Italian=8,Japanese=9,Korean=10,Polish=11,Portuguese=12,Russian=13,Spanish=14,Thai=15,Turkish=16 };

// An NSC1 artwork file plus an NSL1/NSL2 language file presents the exact original
// NSP2 byte stream. Only the directory and glyph/text sections are localized;
// compressed animation groups and image pixels live once in the common file.
// Both inputs and this adapter must outlive the returned source. Opening checks
// the container mapping bounds and actual NSP2 locale marker. Historical
// digest fields remain in the container layout but are not runtime gates.
// NSL2 stores only the initial directory in independent64KiB LZMA2 blocks.
// Its end is4KiB aligned, so cold frame-descriptor pages never decode a prefix
// block while an image already holds the shared transfer workspace.
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
    BlockedResourceSource blocked_locale_;
    uint32_t prefix_{},artwork_{},length_{};
    uint32_t packed_directory_{},packed_data_{},packed_tail_{};
    ResourceLanguage language_id_{ResourceLanguage::English};
    bool valid_{};
    static bool read_at(void*,uint32_t,void*,uint32_t);
    static void progress_at(void*,ResourceStage,uint32_t,uint32_t);
    bool read_directory(uint32_t,void*,uint32_t)const;
};
static_assert(sizeof(SplitResourceSource)<=45*1024,"Locale storage keeps bounded 32 KiB encoded and 4 KiB decoded buffers");

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
    // Preparation reuses resident pages. Small cold reads may fill one page;
    // two cold pages and full-page streams use an exact slice instead. At
    // most one provider call and 4 KiB are requested, including read-ahead.
    // Read-ahead never enters a source's protected compressed prefix.
    bool read_bounded(uint32_t offset,void* dst,uint32_t count)const;
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
    const Page* cached_page(uint32_t offset)const;
    const Page* page(uint32_t offset)const;
    bool read_cold_pair(uint32_t slot,uint32_t offset,void* dst,uint32_t count)const;
};
static_assert(sizeof(ResourceReader)<=257*1024,"Resource reader must stay within its fixed 257 KiB budget");
} // namespace spire
