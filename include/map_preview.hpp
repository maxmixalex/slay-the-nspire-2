#pragma once
#include "reference_map.hpp"
namespace spire {
struct MapPreview {
    reference::ReferenceMap map{};
    bool open = false, generated = false;
    uint32_t seed_number = 1;
    int first_row = 0, selected = -1;
    char seed[24]{};
    bool generate();
    void change_seed(int direction);
};
}
