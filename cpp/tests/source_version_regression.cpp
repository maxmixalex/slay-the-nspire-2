#include "source_version.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::fprintf(stderr, "source version:%d: %s\n", __LINE__, #condition);                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

int main() {
    using namespace spire;
    CHECK(!std::strcmp(version_label(), "nSpire 0.1.0"));
    const char current[] = "NSVR1\n0.111.0\n";
    CHECK(set_source_version(current, sizeof(current) - 1));
    CHECK(!std::strcmp(version_label(), "nSpire 0.1.0 (StS II v0.111.0)"));
    const char different[] = "NSVR1\n12.345.678.9\n";
    CHECK(set_source_version(different, sizeof(different) - 1));
    CHECK(!std::strcmp(version_label(), "nSpire 0.1.0 (StS II v12.345.678.9)"));
    const char* invalid[] = {"",
                             "0.111.0",
                             "NSVR2\n0.111.0\n",
                             "NSVR1\nv0.111.0\n",
                             "NSVR1\n0..111\n",
                             "NSVR1\n0.111.\n",
                             "NSVR1\n0.111.0",
                             "NSVR1\n0.111.0\nextra",
                             "NSVR1\n0.1.2.3.4\n",
                             "NSVR1\nversion\n",
                             "NSVR1\n12345678901234567890123456789012.1\n"};
    for (const auto* value : invalid) {
        CHECK(set_source_version(current, sizeof(current) - 1));
        CHECK(!set_source_version(value, std::strlen(value)));
        CHECK(!std::strcmp(version_label(), "nSpire 0.1.0"));
    }
    CHECK(!set_source_version(nullptr, 0));
    CHECK(!std::strcmp(version_label(), "nSpire 0.1.0"));
    std::puts("source version: metadata, actual version, and legacy fallback passed");
}
