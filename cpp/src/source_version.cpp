#include "source_version.hpp"

namespace spire {
namespace {
constexpr char Prefix[] = "NSVR1\n";
constexpr char GamePrefix[] = " (StS II v";
constexpr unsigned MaxVersionLength = 31;
char label[sizeof(PortVersionLabel) + sizeof(GamePrefix) + MaxVersionLength + 1] = "nSpire 0.1.0";
} // namespace

bool set_source_version(const char* payload, size_t size) {
    // The fallback is also used after a bad metadata file or a pack switch.
    for (size_t i = 0; i < sizeof(PortVersionLabel); ++i)
        label[i] = PortVersionLabel[i];
    constexpr size_t prefix_size = sizeof(Prefix) - 1;
    if (!payload || size <= prefix_size + 1 || size > prefix_size + MaxVersionLength + 1)
        return false;
    for (size_t i = 0; i < prefix_size; ++i)
        if (payload[i] != Prefix[i])
            return false;
    if (payload[size - 1] != '\n')
        return false;
    bool digit = false;
    unsigned dots = 0;
    for (size_t i = prefix_size; i + 1 < size; ++i) {
        const char c = payload[i];
        if (c >= '0' && c <= '9') {
            digit = true;
        } else if (c == '.' && digit) {
            digit = false;
            ++dots;
        } else {
            return false;
        }
    }
    if (!digit || dots < 1 || dots > 3)
        return false;
    size_t out = sizeof(PortVersionLabel) - 1;
    for (size_t i = 0; i + 1 < sizeof(GamePrefix); ++i)
        label[out++] = GamePrefix[i];
    for (size_t i = prefix_size; i + 1 < size; ++i)
        label[out++] = payload[i];
    label[out++] = ')';
    label[out] = 0;
    return true;
}

const char* version_label() {
    return label;
}
} // namespace spire
