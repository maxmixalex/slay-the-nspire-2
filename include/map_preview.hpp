#pragma once
#include "reference_act.hpp"
namespace spire {
inline constexpr char SeedKeys[] = "1234567890QWERTYUIOPASDFGHJKL-ZXCVBNM_";
inline constexpr int SeedKeyCount = sizeof(SeedKeys) - 1;
inline constexpr int SeedButtonCount = SeedKeyCount + 4;
struct MapPreview {
    reference::ReferenceMap map{};
    reference::ActSettings settings{};
    reference::QuotaTrace quota{};
    bool open = false, generated = false, editing = false;
    uint32_t seed_number = 1;
    int first_row = 0, selected = -1, editor_selection = SeedButtonCount - 1;
    char seed[65] = "NSP1RE-1", draft[65]{};
    const char* error = "";
    bool generate();
    bool set_seed(const char*);
    void change_seed(int direction);
    void change_act();
    void begin_edit();
    void edit(int character); // ASCII, 8 backspace, 13 apply, 27 cancel, -1 clear.
};
}
