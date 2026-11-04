#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "resource_transfer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Bytes = std::vector<uint8_t>;
static App a;
static SaveData disk;
static Bytes profile, content;
static unsigned checks;
static bool exists;
static const char *scenario = "setup", *captures = nullptr, *capture_prefix = "";
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "Campaign [%s]:%d %s phase=%u modal=%u point=%u clock=%u\n",      \
                         scenario, __LINE__, #x, unsigned(a.game.phase), unsigned(a.ftue.id),      \
                         a.game.world.current_point, a.map_clock_ms);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
#include "ftue_fixture.hpp"
static bool load_run(State& s) {
    return exists && load(s, disk.bytes, disk.size);
}
static bool save_run(const State& s) {
    exists = save(s, disk);
    return exists;
}
static bool clear_run() {
    exists = false;
    return true;
}
static bool load_profile(UiProgress& p) {
    return decode_ui_progress(profile.data(), profile.size(), p);
}
static bool save_profile(const UiProgress& p) {
    static uint8_t out[UiProgressCapacity];
    const size_t n = encode_ui_progress(p, out, sizeof out);
    if (!n)
        return false;
    profile.assign(out, out + n);
    return true;
}
static unsigned current_profile() {
    return 1;
}
static bool profile_info(unsigned, UiProgressSummary& s) {
    s = {};
    return false;
}
static uint32_t clock_now() {
    return 1793552400;
}
static Bytes read_file(const char* path) {
    FILE* f = std::fopen(path, "rb");
    CHECK(f);
    CHECK(!std::fseek(f, 0, SEEK_END));
    const long n = std::ftell(f);
    CHECK(n > 0);
    CHECK(!std::fseek(f, 0, SEEK_SET));
    Bytes out(size_t(n), 0);
    CHECK(std::fread(out.data(), 1, out.size(), f) == out.size());
    CHECK(!std::fclose(f));
    return out;
}
static void init() {
    a = App{};
    Callbacks cb;
    cb.save = save_run;
    cb.load = load_run;
    cb.clear_save = clear_run;
    cb.load_progress = load_profile;
    cb.save_progress = save_profile;
    cb.current_profile = current_profile;
    cb.profile_info = profile_info;
    cb.wall_clock = clock_now;
    CHECK(a.init(content.data(), content.size(), cb));
    if (a.startup_logo.active())
        a.input(Key::Confirm);
}
static void step(unsigned ms = 100) {
    a.advance(ms);
    a.render();
    CHECK(a.content.healthy());
}
static void wait(unsigned ms = 2200) {
    for (unsigned left = ms; left;) {
        unsigned d = left > 100 ? 100 : left;
        step(d);
        left -= d;
    }
}
static void capture(const char* name) {
    if (!captures)
        return;
    const auto path = std::string(captures) + "/" + capture_prefix + name + ".ppm";
    FILE* f = std::fopen(path.c_str(), "wb");
    CHECK(f);
    std::fprintf(f, "P6\n320 240\n255\n");
    for (auto p : a.pixels) {
        const unsigned char rgb[] = {uint8_t((p >> 11) * 255 / 31),
                                     uint8_t(((p >> 5) & 63) * 255 / 63),
                                     uint8_t((p & 31) * 255 / 31)};
        CHECK(std::fwrite(rgb, 1, 3, f) == 3);
    }
    CHECK(!std::fclose(f));
}
static void close_lesson() {
    for (unsigned i = 0; i < 8 && a.ftue.active(); ++i) {
        if (a.ftue.id == FtueId::CombatRules)
            a.input(Key::Right);
        else
            a.input(Key::Confirm);
        step();
    }
    CHECK(!a.ftue.active());
}
static void go_map() {
    wait(8500);
    if (a.ftue.active()) {
        CHECK(a.ftue.id == FtueId::MapSelect);
        capture("map-lesson");
        close_lesson();
        wait(500);
    }
    CHECK(a.map_window.is_open && a.map_routes_active());
    const auto* m = world_current_map(a.game);
    CHECK(m && m->count > 10);
    uint8_t choices[reference::Columns];
    CHECK(world_available_points(a.game, choices, reference::Columns) > 0);
    capture("map-routes");
    a.input(Key::Confirm);
    CHECK(a.map_travel.pending);
    wait(2500);
}
static void fight() {
    scenario = "ordinary first combat and three-page lesson";
    CHECK(a.game.phase == Phase::Combat);
    CHECK(a.game.enemy_world.encounter == EncounterId::NibbitsWeak);
    for (unsigned i = 0; i < 200 && (!a.ftue.active()); ++i)
        step();
    CHECK(a.ftue.id == FtueId::CombatRules);
    capture("combat-lesson");
    close_lesson();
    wait(6000);
    capture("combat-play");
    for (unsigned actions = 0; actions < 120 && a.game.phase == Phase::Combat; ++actions) {
        if (a.ftue.active()) {
            close_lesson();
            wait();
            continue;
        }
        if (a.combat_actions_locked()) {
            wait();
            continue;
        }
        int card = -1;
        for (unsigned i = 0; i < a.game.hand.count; ++i)
            if (can_play_card(a.game, a.game.hand.ids[i])) {
                card = int(i);
                if (definitions[a.game.cards[a.game.hand.ids[i]].kind].type == CardType::Attack)
                    break;
            }
        if (card >= 0) {
            for (unsigned i = 0; i < 20 && a.cursor != card; ++i)
                a.input(Key::Right);
            CHECK(a.cursor == card);
            a.input(Key::Confirm);
        } else
            a.input(Key::EndTurn);
        wait(2200);
    }
    CHECK(a.game.phase == Phase::Reward || a.game.phase == Phase::WorldRelicChoice);
    CHECK(validate(a.game));
    capture("combat-rewards");
}
static void leave_rewards() {
    scenario = "reward proceed retains a populated map";
    for (unsigned tries = 0; tries < 16 && !a.map_window.is_open; ++tries) {
        if (a.ftue.active()) {
            close_lesson();
            wait();
            continue;
        }
        a.ui_action(a.game.phase == Phase::Reward ? UiAction::LeaveRewards
                                                  : UiAction::LeaveWorldRewards);
        wait();
    }
    CHECK(a.map_window.is_open && world_can_travel(a.game));
    CHECK(a.map_routes_active());
    CHECK(a.progress.character_unlocks == 1);
    capture("reward-map");
}
static void resume() {
    scenario = "ordinary Save and Continue retain the completed battle and live map";
    a.input(Key::Save);
    CHECK(exists);
    const auto seed = a.game.world.run_seed;
    const auto point = a.game.world.current_point;
    const auto combat = a.game.world.combat_count;
    const auto hp = a.game.hp;
    a.return_to_title();
    init();
    CHECK(a.has_save);
    a.input(Key::Load);
    wait(2600);
    CHECK(!a.title && a.game.world.run_seed == seed && a.game.world.current_point == point &&
          a.game.world.combat_count == combat && a.game.hp == hp);
    a.input(Key::Map);
    wait();
    CHECK(a.map_window.is_open && a.map_routes_active());
    CHECK(a.game.world.rooms[0].ancient == unsigned(OriginalEventId::Neow));
    CHECK(validate(a.game));
    capture("continue-map");
}
static void fresh_flow() {
    scenario = "empty profile title to character and opt-in";
    exists = false;
    profile.clear();
    init();
    CHECK(a.progress.character_unlocks == 1);
    a.input(Key::Confirm);
    CHECK(a.run_mode.screen == RunMenu::Singleplayer);
    a.input(Key::Confirm);
    CHECK(a.character_select);
    a.input(Key::Right);
    CHECK(a.selected_character == Character::Silent && !a.character_available(1));
    wait();
    capture("locked-character");
    const auto serial = a.run_number;
    a.input(Key::Confirm);
    CHECK(a.run_number == serial && a.character_select && !a.ftue.active());
    a.input(Key::Left);
    a.input(Key::Confirm);
    CHECK(a.ftue.id == FtueId::AcceptTutorials);
    capture("accept-tutorial");
    a.input(Key::Confirm);
    wait(2600);
    CHECK(!a.title && !a.character_select && a.game.world.active && a.game.world.first_run);
    CHECK(a.game.original_events.id == OriginalEventId::Neow &&
          original_event_option_count(a.game) == 3);
    CHECK(validate(a.game));
    capture("neow-options");
    scenario = "real Neow choice callbacks and proceed";
    for (unsigned i = 0; i < 24 && !a.map_window.is_open; ++i) {
        if (a.ftue.active()) {
            close_lesson();
            wait();
            continue;
        }
        if (a.game.phase == Phase::Event)
            a.input(Key::Confirm);
        else if (a.game.phase == Phase::WorldRelicChoice ||
                 a.game.phase == Phase::OriginalEventChoice) {
            a.input(Key::Confirm);
            a.input(Key::EndTurn);
            a.ui_action(UiAction::ConfirmWorldChoice);
        } else if (a.game.phase == Phase::Upgrade || a.game.phase == Phase::Transform ||
                   a.game.phase == Phase::Remove) {
            a.input(Key::Confirm);
            a.input(Key::Confirm);
        } else
            CHECK(false);
        wait();
    }
    CHECK(a.map_window.is_open);
    go_map();
    fight();
    leave_rewards();
    resume();
}
static void legacy_flow(const char* run, const char* progress) {
    scenario = "baseline-produced no-Neow save resumes through normal map tutorial";
    capture_prefix = "legacy-";
    const auto raw = read_file(run);
    CHECK(raw.size() <= sizeof disk.bytes);
    CHECK(load(a.game, raw.data(), raw.size()));
    CHECK(save(a.game, disk));
    CHECK(disk.size == raw.size());
    unsigned changed = 0;
    for (unsigned i = 0; i < raw.size(); ++i)
        if (raw[i] != disk.bytes[i]) {
            ++changed;
            CHECK(raw[i] == unsigned(reference::PointType::Start) &&
                  disk.bytes[i] == unsigned(reference::PointType::Monster));
        }
    CHECK(changed == 1); // Every seed/RNG/room/map/progress byte is preserved.
    disk.size = raw.size();
    std::memcpy(disk.bytes, raw.data(), raw.size());
    exists = true;
    profile = read_file(progress);
    init();
    CHECK(a.has_save);
    a.input(Key::Load);
    wait(2600);
    CHECK(a.game.world.rooms[0].ancient == 0 && a.game.phase == Phase::Map &&
          a.game.world.current_point == reference::NoPoint);
    CHECK(a.map_intro_at_entry());
    const auto seed = a.game.world.run_seed;
    const auto root = a.game.world.maps[0].root;
    CHECK(a.game.world.maps[0].points[root].type == reference::PointType::Monster);
    go_map();
    CHECK(a.game.world.run_seed == seed && a.game.world.current_point == root);
    CHECK(a.game.enemy_world.active);
    fight();
    leave_rewards();
    CHECK(validate(a.game));
    capture("recovered-map");
    scenario = "recovered root battle proceeds to its original next route";
    go_map();
    CHECK(a.game.phase == Phase::Combat && a.game.world.current_point != root &&
          a.game.enemy_world.active && validate(a.game));
    capture("next-battle");
}
int main(int argc, char** argv) {
    content = argc > 1 ? read_file(argv[1]) : fixture();
    if (argc > 2)
        captures = argv[2];
    fresh_flow();
    if (argc > 4)
        legacy_flow(argv[3], argv[4]);
    std::printf("Campaign normal input: %u checks; opt-in, actual Neow, map tutorial, normal "
                "Nibbits card play, rewards, save and Continue%s.\n",
                checks, argc > 4 ? ", plus baseline old-save recovery" : "");
}
