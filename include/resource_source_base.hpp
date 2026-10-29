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
    // A compressed directory adapter may require this prefix to be retained
    // by Content before image reads can acquire the shared decode workspace.
    uint32_t required_cached_prefix{};
    static ResourceSource memory(const uint8_t*,size_t);
};

}
