#include "reference_map.hpp"

namespace spire::reference {
unsigned PointSet::count() const {
    unsigned n = 0; for (unsigned i = 0; i < used; ++i) n += slots[i] != NoPoint; return n;
}
bool PointSet::contains(uint8_t id) const {
    for (unsigned i = 0; i < used; ++i) if (slots[i] == id) return true;
    return false;
}
bool PointSet::add(uint8_t id) {
    if (contains(id)) return true;
    // Construction adds edges before pruning removes any; no hole reuse occurs
    // in the supported constructor. Keep that assumption explicit.
    if (used == Columns) return false;
    slots[used++] = id; return true;
}
void PointSet::remove(uint8_t id) {
    for (unsigned i = 0; i < used; ++i) if (slots[i] == id) slots[i] = NoPoint;
}
namespace {
int absolute(int v) { return v < 0 ? -v : v; }
int compare(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return static_cast<uint8_t>(*a) - static_cast<uint8_t>(*b);
}
struct KeyWriter {
    char* data; unsigned pos = 0;
    void put(char c) { if (pos < 95) data[pos++] = c; data[pos] = 0; }
    void number(unsigned v) { if (v >= 10) number(v / 10); put(char('0' + v % 10)); }
};
struct Builder {
    ReferenceMap& m; Random rng; MapWorkspace& w;
    void fail(MapError e) { if (m.error == MapError::None) m.error = e; }
    bool good() const { return m.error == MapError::None && rng.good; }
    uint8_t make(unsigned col, unsigned row) {
        if (m.count >= MaxPoints) { fail(MapError::Capacity); return NoPoint; }
        uint8_t id = m.count++; auto& p = m.points[id];
        p = MapPoint{}; p.col = uint8_t(col); p.row = uint8_t(row); p.active = true; return id;
    }
    uint8_t point(unsigned col, unsigned row) {
        if (col >= Columns || row >= m.config.rows) { fail(MapError::InvalidGraph); return NoPoint; }
        auto& id = m.grid[col][row]; if (id == NoPoint) id = make(col, row); return id;
    }
    void edge(uint8_t from, uint8_t to) {
        if (from >= m.count || to >= m.count) { fail(MapError::InvalidGraph); return; }
        if (!m.points[from].children.add(to) || !m.points[to].parents.add(from)) fail(MapError::Capacity);
    }
    void unlink(uint8_t from, uint8_t to) {
        m.points[from].children.remove(to); m.points[to].parents.remove(from);
    }
    bool crossover(uint8_t id, unsigned col) const {
        const auto& p = m.points[id]; int direction = int(col) - p.col;
        if (!direction) return false;
        uint8_t other = m.grid[col][p.row]; if (other == NoPoint) return false;
        const auto& q = m.points[other];
        for (unsigned i = 0; i < q.children.used; ++i) {
            uint8_t c = q.children.slots[i];
            if (c != NoPoint && int(m.points[c].col) - q.col == -direction) return true;
        }
        return false;
    }
    void path(uint8_t id) {
        while (good() && m.points[id].row < m.config.rows - 1) {
            int directions[3] = {-1, 0, 1}; shuffle(directions, 3, rng);
            const auto& p = m.points[id]; bool found = false;
            for (int direction : directions) {
                int col = int(p.col) + direction; if (col < 0) col = 0; if (col >= Columns) col = Columns - 1;
                if (crossover(id, unsigned(col))) continue;
                uint8_t next = point(unsigned(col), p.row + 1); if (!good()) return;
                edge(id, next); id = next; found = true; break;
            }
            if (!found) { fail(MapError::InvalidGraph); return; }
        }
    }
    void paths() {
        PointSet starts{};
        for (unsigned i = 0; i < Columns && good(); ++i) {
            uint8_t p = point(rng.below(Columns), 1); unsigned retries = 0;
            if (i == 1) while (starts.contains(p)) {
                if (++retries > 10000) { fail(MapError::IterationLimit); return; }
                p = point(rng.below(Columns), 1);
            }
            if (!starts.add(p)) { fail(MapError::Capacity); return; }
            path(p);
        }
        for (unsigned col = 0; col < Columns; ++col) {
            uint8_t p = m.grid[col][m.config.rows - 1]; if (p != NoPoint) edge(p, m.boss);
        }
        if (m.second_boss != NoPoint) edge(m.boss, m.second_boss);
        for (unsigned col = 0; col < Columns; ++col) {
            uint8_t p = m.grid[col][1]; if (p != NoPoint) edge(m.root, p);
        }
    }
    bool allowed(PointType type, uint8_t id) const {
        const auto& p = m.points[id];
        if (p.row < 6 && (type == PointType::Rest || type == PointType::Elite)) return false;
        if (p.row >= m.config.rows - 3 && type == PointType::Rest) return false;
        if (type == PointType::Elite || type == PointType::Rest || type == PointType::Treasure || type == PointType::Shop) {
            const PointSet* sets[] = {&p.parents, &p.children};
            for (const auto* set : sets) {
                for (unsigned i = 0; i < set->used; ++i) {
                    uint8_t q = set->slots[i]; if (q != NoPoint && m.points[q].type == type) return false;
                }
            }
        }
        if (type == PointType::Rest || type == PointType::Monster || type == PointType::Unknown || type == PointType::Elite || type == PointType::Shop) {
            for (unsigned i = 0; i < p.parents.used; ++i) {
                uint8_t parent = p.parents.slots[i]; if (parent == NoPoint) continue;
                const auto& children = m.points[parent].children;
                for (unsigned j = 0; j < children.used; ++j) {
                    uint8_t q = children.slots[j];
                    if (q != NoPoint && q != id && m.points[q].type == type) return false;
                }
            }
        }
        return true;
    }
    unsigned collect(uint8_t* out, PointType type, bool modifiable = false) const {
        unsigned n = 0;
        // MapPoint.CompareTo and GetAllMapPoints both order by column, then row.
        for (unsigned col = 0; col < Columns; ++col) for (unsigned row = 0; row < m.config.rows; ++row) {
            uint8_t id = m.grid[col][row];
            if (id != NoPoint && m.points[id].type == type && (!modifiable || m.points[id].modifiable)) out[n++] = id;
        }
        return n;
    }
    void types() {
        for (unsigned col = 0; col < Columns; ++col) {
            const unsigned rows[] = {unsigned(m.config.rows - 1), unsigned(m.config.rows - 7), 1};
            const PointType types[] = {PointType::Rest, m.config.replace_treasure ? PointType::Elite : PointType::Treasure, PointType::Monster};
            for (unsigned i = 0; i < 3; ++i) {
                uint8_t p = m.grid[col][rows[i]];
                if (p != NoPoint) { m.points[p].type = types[i]; m.points[p].modifiable = false; }
            }
        }
        PointType queue[MaxPoints]{}; unsigned count = 0, head = 0;
        const unsigned quotas[] = {m.config.rests, m.config.shops, m.config.elites, m.config.unknowns};
        const PointType kinds[] = {PointType::Rest, PointType::Shop, PointType::Elite, PointType::Unknown};
        for (unsigned k = 0; k < 4; ++k) for (unsigned j = 0; j < quotas[k]; ++j) queue[count++] = kinds[k];
        for (unsigned pass = 0; pass < 3 && count; ++pass) {
            uint8_t candidates[MaxPoints]; unsigned n = collect(candidates, PointType::Unassigned); shuffle(candidates, n, rng);
            for (unsigned i = 0; i < n && count; ++i) {
                unsigned attempts = count;
                while (attempts--) {
                    PointType type = queue[head]; head = (head + 1) % MaxPoints; --count;
                    if (allowed(type, candidates[i])) { m.points[candidates[i]].type = type; break; }
                    queue[(head + count) % MaxPoints] = type; ++count;
                }
            }
        }
        for (unsigned i = 0; i < m.count; ++i) if (m.points[i].type == PointType::Unassigned) m.points[i].type = PointType::Monster;
        m.points[m.root].type = PointType::Start; m.points[m.boss].type = PointType::Boss;
        if (m.second_boss != NoPoint) m.points[m.second_boss].type = PointType::Boss;
    }
    bool repair(PointType type, unsigned quota) {
        uint8_t candidates[MaxPoints]; unsigned actual = collect(candidates, type);
        if (actual >= quota) return false;
        unsigned missing = quota - actual, n = collect(candidates, PointType::Monster, true);
        shuffle(candidates, n, rng); bool changed = false;
        for (unsigned i = 0; i < n && missing; ++i) if (allowed(type, candidates[i])) {
            m.points[candidates[i]].type = type; --missing; changed = true;
        }
        return changed;
    }
    void segment_key(const uint8_t* ids, unsigned length, char* key) const {
        KeyWriter s{key}; const auto& a = m.points[ids[0]]; const auto& b = m.points[ids[length - 1]];
        if (a.row != 0) { s.number(a.col); s.put(','); } s.number(a.row); s.put('-');
        s.number(b.col); s.put(','); s.number(b.row); s.put('-');
        for (unsigned i = 0; i < length; ++i) { if (i) s.put(','); s.number(unsigned(m.points[ids[i]].type)); }
    }
    int compare_group(unsigned group, const char* key) const {
        // Reuse the first segment rather than retaining thousands of almost
        // unique strings. The graph does not mutate while groups are assembled.
        const auto& segment = w.segments[w.groups[group].first];
        char candidate[96]{}; segment_key(segment.ids, segment.length, candidate);
        return compare(candidate, key);
    }
    void segment(const uint8_t* ids, unsigned length) {
        if (!good()) return;
        char key[96]{}; segment_key(ids, length, key);
        unsigned lo = 0, hi = w.group_count;
        while (lo < hi) { unsigned mid = (lo + hi) / 2; if (compare_group(w.order[mid], key) < 0) lo = mid + 1; else hi = mid; }
        unsigned group;
        if (lo < w.group_count && compare_group(w.order[lo], key) == 0) {
            group = w.order[lo];
            for (int index = w.groups[group].first; index >= 0; index = w.segments[index].next) {
                const auto& existing = w.segments[index];
                for (unsigned i = 1; i + 1 < length; ++i) if (existing.ids[i] == ids[i]) return;
            }
        } else {
            if (w.group_count == MapWorkspace::MaxGroups) { fail(MapError::Capacity); return; }
            group = w.group_count; auto& g = w.groups[group]; g = MapWorkspace::Group{}; g.first = g.last = -1;
            for (unsigned i = w.group_count; i > lo; --i) w.order[i] = w.order[i - 1];
            w.order[lo] = uint16_t(group); ++w.group_count;
        }
        if (w.segment_count == MapWorkspace::MaxSegments) { fail(MapError::Capacity); return; }
        unsigned index = w.segment_count++; auto& seg = w.segments[index]; seg.length = uint8_t(length); seg.next = -1;
        for (unsigned i = 0; i < length; ++i) seg.ids[i] = ids[i];
        auto& g = w.groups[group]; if (g.last >= 0) w.segments[g.last].next = int16_t(index); else g.first = int16_t(index);
        g.last = int16_t(index); ++g.count;
    }
    void visit(uint8_t id, uint8_t* ids, unsigned length) {
        if (!good()) return;
        if (length >= MaxRows + 2) { fail(MapError::Capacity); return; } ids[length++] = id;
        if (m.points[id].type == PointType::Boss) {
            if (++w.paths > 1000000) { fail(MapError::Capacity); return; }
            for (unsigned start = 0; start + 1 < length; ++start) {
                const auto& p = m.points[ids[start]];
                if (p.children.count() <= 1 && p.row != 0) continue;
                for (unsigned end = start + 2; end < length; ++end)
                    if (m.points[ids[end]].parents.count() >= 2) segment(ids + start, end - start + 1);
            }
            return;
        }
        const auto& children = m.points[id].children;
        for (unsigned i = 0; i < children.used; ++i) if (children.slots[i] != NoPoint) visit(children.slots[i], ids, length);
    }
    bool in_segment(const MapWorkspace::Segment& s, uint8_t id) const {
        for (unsigned i = 0; i < s.length; ++i) if (s.ids[i] == id) return true; return false;
    }
    bool removed(uint8_t id) const {
        const auto& p = m.points[id]; return p.row >= m.config.rows || m.grid[p.col][p.row] == NoPoint;
    }
    void remove(uint8_t id) {
        auto& p = m.points[id]; m.grid[p.col][p.row] = NoPoint; p.active = false;
        for (unsigned i = 0; i < p.children.used; ++i) if (p.children.slots[i] != NoPoint) unlink(id, p.children.slots[i]);
        for (unsigned i = 0; i < p.parents.used; ++i) if (p.parents.slots[i] != NoPoint) unlink(p.parents.slots[i], id);
    }
    bool prune_segment(const MapWorkspace::Segment& s) {
        bool changed = false;
        for (unsigned i = 0; i + 1 < s.length; ++i) {
            uint8_t id = s.ids[i]; auto& p = m.points[id];
            if (removed(id) && p.type != PointType::Start && p.type != PointType::Boss) return true;
            if (p.children.count() > 1 || p.parents.count() > 1) continue;
            bool reject = false;
            for (unsigned j = 0; j < p.parents.used; ++j) {
                uint8_t q = p.parents.slots[j]; if (q != NoPoint && m.points[q].children.count() == 1 && !removed(q)) reject = true;
            }
            if (reject) continue;
            for (unsigned j = i; j < s.length; ++j) {
                const auto& q = m.points[s.ids[j]]; if (q.children.count() > 1 && q.parents.count() == 1) reject = true;
            }
            if (reject) continue;
            if (m.points[s.ids[s.length - 1]].parents.count() == 1) return false;
            for (unsigned j = 0; j < p.children.used; ++j) {
                uint8_t q = p.children.slots[j]; if (q != NoPoint && !in_segment(s, q) && m.points[q].parents.count() == 1) reject = true;
            }
            if (reject) continue;
            remove(id); changed = true;
        }
        return changed;
    }
    bool break_segment(const MapWorkspace::Segment& s) {
        bool changed = false;
        for (unsigned i = 0; i + 1 < s.length; ++i) {
            uint8_t p = s.ids[i], q = s.ids[i + 1];
            if (m.points[p].children.count() >= 2 && m.points[q].parents.count() != 1) { unlink(p, q); changed = true; }
        }
        return changed;
    }
    bool prune_once() {
        w.group_count = w.segment_count = 0; w.paths = 0;
        uint8_t path[MaxRows + 2]; visit(m.root, path, 0); if (!good()) return false;
        for (unsigned i = 0; i < w.group_count; ++i) {
            const auto& g = w.groups[w.order[i]]; if (g.count < 2) continue;
            int16_t segments[MaxPoints]; unsigned n = 0;
            for (int s = g.first; s >= 0; s = w.segments[s].next) {
                if (n == MaxPoints) { fail(MapError::Capacity); return false; } segments[n++] = int16_t(s);
            }
            shuffle(segments, n, rng); unsigned pruned = 0;
            for (unsigned j = 0; j < n && pruned < n - 1; ++j) if (prune_segment(w.segments[segments[j]])) ++pruned;
            if (pruned) return true;
            for (unsigned j = 0; j < n; ++j) if (break_segment(w.segments[segments[j]])) return true;
        }
        return false;
    }
    void prune() {
        for (unsigned pass = 0; pass < 3 && good(); ++pass) {
            unsigned iterations = 0;
            while (prune_once() && good()) if (++iterations > 50) { fail(MapError::IterationLimit); return; }
            if (!good()) return;
            bool changed = repair(PointType::Shop, m.config.shops);
            changed = repair(PointType::Elite, m.config.elites) || changed;
            changed = repair(PointType::Rest, m.config.rests) || changed;
            changed = repair(PointType::Unknown, m.config.unknowns) || changed;
            if (!changed) break;
        }
    }
    bool empty_col(unsigned col) const {
        for (unsigned row = 0; row < m.config.rows; ++row) if (m.grid[col][row] != NoPoint) return false; return true;
    }
    void move(uint8_t id, unsigned col) {
        auto& p = m.points[id]; m.grid[p.col][p.row] = NoPoint; p.col = uint8_t(col); m.grid[col][p.row] = id;
    }
    void center() {
        bool left = empty_col(0) && empty_col(1), right = empty_col(5) && empty_col(6);
        int shift = left && !right ? -1 : right && !left ? 1 : 0; if (!shift) return;
        for (unsigned row = 0; row < m.config.rows; ++row) for (int i = 0; i < Columns; ++i) {
            int col = shift > 0 ? Columns - 1 - i : i; uint8_t p = m.grid[col][row];
            if (p != NoPoint) move(p, unsigned(col + shift));
        }
    }
    bool position_allowed(uint8_t id, int col) const {
        const auto& p = m.points[id];
        const PointSet* sets[] = {&p.parents, &p.children};
        for (const auto* set : sets) for (unsigned i = 0; i < set->used; ++i) {
            uint8_t q = set->slots[i]; if (q != NoPoint && absolute(col - m.points[q].col) > 1) return false;
        }
        return true;
    }
    int gap(unsigned col, const uint8_t* ids, unsigned n, uint8_t skip) const {
        int distance = 0x7fffffff;
        for (unsigned i = 0; i < n; ++i) if (ids[i] != skip) {
            int d = absolute(int(col) - m.points[ids[i]].col); if (d < distance) distance = d;
        }
        return distance;
    }
    void spread() {
        for (unsigned row = 0; row < m.config.rows; ++row) {
            uint8_t ids[Columns]; unsigned n = 0;
            for (unsigned col = 0; col < Columns; ++col) if (m.grid[col][row] != NoPoint) ids[n++] = m.grid[col][row];
            unsigned iterations = 0; bool changed;
            do {
                changed = false;
                for (unsigned i = 0; i < n; ++i) {
                    uint8_t id = ids[i]; unsigned best = m.points[id].col; int best_gap = gap(best, ids, n, id);
                    for (unsigned col = 0; col < Columns; ++col) {
                        if (col == m.points[id].col || (m.grid[col][row] != NoPoint && m.grid[col][row] != id) || !position_allowed(id, int(col))) continue;
                        int value = gap(col, ids, n, id); if (value > best_gap) { best = col; best_gap = value; }
                    }
                    if (best != m.points[id].col) { move(id, best); changed = true; }
                }
                if (++iterations > 1000) { fail(MapError::IterationLimit); return; }
            } while (changed);
        }
    }
    void straighten() {
        for (unsigned row = 0; row < m.config.rows; ++row) for (unsigned col = 0; col < Columns; ++col) {
            uint8_t id = m.grid[col][row]; if (id == NoPoint) continue; auto& p = m.points[id];
            if (p.parents.count() != 1 || p.children.count() != 1) continue;
            uint8_t parent = NoPoint, child = NoPoint;
            for (unsigned i = 0; i < p.parents.used; ++i) if (p.parents.slots[i] != NoPoint) parent = p.parents.slots[i];
            for (unsigned i = 0; i < p.children.used; ++i) if (p.children.slots[i] != NoPoint) child = p.children.slots[i];
            bool left = p.col < m.points[parent].col && p.col < m.points[child].col;
            bool right = p.col > m.points[parent].col && p.col > m.points[child].col;
            if (left && col + 1 < Columns && m.grid[col + 1][row] == NoPoint) move(id, col + 1);
            if (right && col > 0 && m.grid[col - 1][row] == NoPoint) move(id, col - 1);
        }
    }
};
}

bool ReferenceMap::valid() const {
    if (error != MapError::None || root >= count || boss >= count || config.rows < 8 || config.rows > MaxRows) return false;
    bool seen[MaxPoints]{}; seen[root] = true;
    // All edges increase row by exactly one, so row traversal is a topological order.
    for (unsigned row = 0; row <= config.rows + 1u; ++row) for (unsigned id = 0; id < count; ++id) {
        const auto& p = points[id]; if (!p.active || p.row != row) continue;
        if (!seen[id]) return false;
        if (p.type != PointType::Start && p.type != PointType::Boss && (p.col >= Columns || p.row >= config.rows || grid[p.col][p.row] != id)) return false;
        if (p.children.count() == 0 && id != (second_boss != NoPoint ? second_boss : boss)) return false;
        for (unsigned j = 0; j < p.children.used; ++j) {
            uint8_t child = p.children.slots[j]; if (child == NoPoint) continue;
            if (child >= count || !points[child].active || points[child].row != p.row + 1 || !points[child].parents.contains(uint8_t(id))) return false;
            seen[child] = true;
        }
    }
    return seen[boss];
}
bool generate_map(ReferenceMap& map, Random rng, const MapConfig& config, MapWorkspace& work) {
    map = ReferenceMap{}; map.config = config;
    for (auto& column : map.grid) for (auto& cell : column) cell = NoPoint;
    if (!rng.good || config.rows < 10 || config.rows > MaxRows || unsigned(config.rests) + config.shops + config.elites + config.unknowns >= MaxPoints) {
        map.error = MapError::Configuration; return false;
    }
    Builder b{map, rng, work};
    map.boss = b.make(Columns / 2, config.rows); map.root = b.make(Columns / 2, 0);
    if (config.second_boss) map.second_boss = b.make(Columns / 2, config.rows + 1);
    b.paths(); map.stage_calls[0] = b.rng.calls;
    if (b.good()) b.types(); map.stage_calls[1] = b.rng.calls;
    if (b.good() && config.prune) b.prune(); map.stage_calls[2] = b.rng.calls;
    if (b.good()) { b.center(); b.spread(); b.straighten(); } map.stage_calls[3] = b.rng.calls;
    map.final_rng = b.rng;
    if (b.good() && !map.valid()) map.error = MapError::InvalidGraph;
    return b.good();
}
} // namespace spire::reference
