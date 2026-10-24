#pragma once
#include "reference_rng.hpp"

namespace spire::reference {
constexpr int Columns = 7, MaxRows = 20, MaxPoints = Columns * MaxRows + 3;
constexpr uint8_t NoPoint = 255;
enum class PointType : uint8_t { Unassigned, Unknown, Shop, Treasure, Rest, Monster, Elite, Boss, Start };
enum class MapError : uint8_t { None, Configuration, Capacity, IterationLimit, InvalidGraph };

// Enumeration retains holes after removals, matching the source HashSet's slot
// order. Path traversal and duplicate-segment pruning depend on that order.
struct PointSet {
    uint8_t slots[Columns]{};
    uint8_t used = 0;
    unsigned count() const;
    bool contains(uint8_t id) const;
    bool add(uint8_t id);
    void remove(uint8_t id);
};
struct MapPoint {
    uint8_t col = 0, row = 0;
    PointType type = PointType::Unassigned;
    bool active = false, modifiable = true;
    PointSet parents{}, children{};
};
struct MapConfig {
    uint8_t rows = 16, rests = 7, shops = 3, elites = 5, unknowns = 12;
    bool replace_treasure = false, second_boss = false, prune = true;
    uint8_t variant = 0; // 0 StandardActMap, 1 GoldenPathActMap, 2 SpoilsActMap.
    bool ignore_elite_rules = false; // Constructor-only BigGameHunter override.
};
struct ReferenceMap {
    MapPoint points[MaxPoints]{};
    uint8_t grid[Columns][MaxRows]{};
    uint8_t count = 0, root = NoPoint, boss = NoPoint, second_boss = NoPoint;
    MapConfig config{};
    MapError error = MapError::None;
    uint32_t stage_calls[4]{}; // paths, types, pruning, layout
    Random final_rng{};
    bool valid() const;
};

// Fixed storage; generation never allocates per path. The limits fail explicitly
// instead of dropping segments and silently producing a different map.
struct MapWorkspace {
    static constexpr int MaxGroups = 32768, MaxSegments = 32768, GroupBuckets = 4096;
    // Every edge advances one row and every row has seven columns. Keep the
    // columns in three bits rather than repeating a byte-sized point id for
    // all 32,768 possible segments. The stable row/column table below retains
    // point identity even after pruning clears a cell in ReferenceMap::grid.
    struct Segment {
        uint8_t columns[((MaxRows + 2) * 3 + 7) / 8]{};
        uint8_t first_row = 0, length = 0;
        int16_t next = 0;
        unsigned column(unsigned index) const {
            const unsigned bit = index * 3, byte = bit >> 3, shift = bit & 7;
            unsigned value = columns[byte];
            if (shift > 5) value |= unsigned(columns[byte + 1]) << 8;
            return (value >> shift) & 7u;
        }
        void set_column(unsigned index, unsigned column) {
            const unsigned bit = index * 3, byte = bit >> 3, shift = bit & 7;
            columns[byte] = uint8_t((columns[byte] & ~(7u << shift)) | (column << shift));
            if (shift > 5) columns[byte + 1] = uint8_t(
                (columns[byte + 1] & ~(7u >> (8 - shift))) | (column >> (8 - shift)));
        }
    };
    struct Group { int16_t first = 0, last = 0; uint16_t count = 0, next = 0; };
    Segment segments[MaxSegments]{};
    Group groups[MaxGroups]{};
    uint16_t order[MaxGroups]{};
    uint16_t group_buckets[GroupBuckets]{};
    uint8_t point_ids[MaxRows + 2][Columns]{};
    uint16_t group_count = 0, segment_count = 0;
    uint32_t paths = 0;
};
static_assert(sizeof(MapWorkspace::Segment) == 14 && sizeof(MapWorkspace) <= 794792,
              "Map work storage must keep the full segment capacity within its compact budget");

// Ports the StandardActMap constructor's explicit-count path. The caller supplies
// room quotas and the RNG after any quota draws. Gaussian room-pool construction
// and complete run/encounter seed parity are separate, not implied by this API.
bool generate_map(ReferenceMap&, Random rng, const MapConfig&, MapWorkspace&);
bool generate_golden_map(ReferenceMap&,Random rng,MapWorkspace&);
bool generate_spoils_map(ReferenceMap&,Random rng,const MapConfig&,MapWorkspace&);
} // namespace spire::reference
