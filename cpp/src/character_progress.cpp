#include "app.hpp"

namespace spire {
void App::choose_character(Character value) {
    if (unsigned(value) >= CharacterKinds)
        return;
    // Locked portraits remain inspectable, with their original prerequisite.
    // The embark command is the authority that enforces the playable mask.
    selected_character = value;
    character_random = false;
    if (character_select && selected_ascension > character_max_ascension())
        selected_ascension = uint8_t(character_max_ascension());
}
bool App::character_available(unsigned character) const {
    if (practice_entry)
        return character <= CharacterKinds;
    const unsigned mask = profile_character_unlocks(progress);
    return character < CharacterKinds ? bool(mask & (1u << character))
                                      : character == CharacterKinds && mask == 31;
}
unsigned App::character_max_ascension() const {
    if (practice_entry || (progress.timeline.flags & TimelineLegacyAccess) ||
        (!progress.character_unlocks && !(progress.timeline.flags & TimelineInitialized)))
        return 10;
    if (character_random) {
        unsigned n = 0;
        for (const auto& c : progress.characters)
            if (c.highest_ascension > n)
                n = c.highest_ascension;
        return n;
    }
    return character_available(unsigned(selected_character))
               ? progress.characters[unsigned(selected_character)].highest_ascension
               : 0;
}
bool App::run_mode_available(RunMode mode) const {
    return unsigned(mode) <= unsigned(RunMode::Daily);
}
bool App::first_standard_run() const {
    if ((progress.timeline.flags & TimelineLegacyAccess) ||
        (!progress.character_unlocks && !(progress.timeline.flags & TimelineInitialized)))
        return false;
    for (const auto& c : progress.characters)
        if (c.wins || c.losses)
            return false;
    return true;
}

void App::prepare_profile_run(State& s) {
    restore_progress();
    s.world.unlocks = TimelineUnlocks{};
    s.world.first_run = uint8_t(s.run_options.mode == RunMode::Standard && first_standard_run());
}
void App::finish_character_progress(const State&) {
    const auto& r = progress.current;
    if (r.mode != unsigned(RunMode::Standard) || !r.outcome)
        return;
    unsigned mask = profile_character_unlocks(progress) | 1u | (1u << unsigned(Character::Silent));
    // Pinned characters.*.unlockText: play one run with the predecessor.
    // Save/return/Continue never enters this settlement boundary.
    if (r.character == unsigned(Character::Silent))
        mask |= 1u << unsigned(Character::Regent);
    if (r.character == unsigned(Character::Regent))
        mask |= 1u << unsigned(Character::Necrobinder);
    if (r.character == unsigned(Character::Necrobinder))
        mask |= 1u << unsigned(Character::Defect);
    progress.character_unlocks = uint8_t(mask);
}

} // namespace spire
