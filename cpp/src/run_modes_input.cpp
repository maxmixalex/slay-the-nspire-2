#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "world_card_factory.hpp"
#include "ui_core_observer.hpp"
#include "settings_layout.hpp"
#include "menu_overlay_layout.hpp"

namespace spire {
void App::open_run_menu(RunMenu screen) {
    if (!title || has_save)
        return;
    restore_progress();
    if ((screen == RunMenu::Daily && !run_mode_available(RunMode::Daily)) ||
        (screen == RunMenu::Custom && !run_mode_available(RunMode::Custom)))
        return;
    run_mode.screen = screen;
    run_mode.started = map_clock_ms;
    run_mode.focus = -1;
    character_select = false;
    character_random = false;
    practice_entry = false;
    progress_page_scroll.reset();
    pointer_cancel();
    character_select_presentation.reset();
    if (screen == RunMenu::Singleplayer) {
        compendium_selection = 0;
        for (auto& motion : compendium_motion)
            motion = {};
        return;
    }
    run_mode.pending = {};
    selected_character = Character::Ironclad;
    selected_ascension = 0;
    selected_region = reference::ActKind::Overgrowth;
    selected_seed[0] = 0;
    selected_seed_imported = false;
    if (screen == RunMenu::Custom)
        run_mode.pending.mode = RunMode::Custom;
    else if (screen == RunMenu::Daily)
        refresh_daily_run();
}
void App::refresh_daily_run() {
    if (run_mode.screen != RunMenu::Daily)
        return;
    const uint32_t now = callbacks.wall_clock ? callbacks.wall_clock() : 0;
    if (!now)
        return;
    const uint32_t day = now / 86400;
    if (run_mode.pending.mode == RunMode::Daily && run_mode.pending.daily_day == day)
        return;
    selected_seed_imported = false;
    prepare_daily_run(day, selected_character, run_mode.pending, selected_character,
                      selected_ascension, selected_seed);
}
void App::run_modifier_select(unsigned index) {
    if (run_mode.screen != RunMenu::Custom || index >= RunModifierKinds)
        return;
    toggle_run_modifier(run_mode.pending, RunModifier(index));
    run_mode.focus = int16_t(index + 9);
}
bool App::start_selected_run(const RunOptions& options, bool animated) {
    if (character_departure.active() || !valid_run_options(options))
        return false;
    restore_progress();
    if (!practice_entry &&
        (!run_mode_available(options.mode) ||
         (options.mode != RunMode::Daily &&
          !character_available(character_random ? CharacterKinds : unsigned(selected_character)))))
        return false;
    if (!practice_entry && !seen_ftue(FtueId::AcceptTutorials) &&
        open_ftue(FtueId::AcceptTutorials)) {
        ftue.embark = options;
        character_departure.animated_ftue = animated;
        return false;
    }
    uint32_t seed = 0;
    char canonical[65]{};
    bool string_seed = !practice_entry && *selected_seed;
    const bool imported = string_seed && selected_seed_imported;
    if (string_seed) {
        if (!imported &&
            !reference::canonicalize_seed(selected_seed, canonical, sizeof canonical)) {
            message("种子无效");
            return false;
        }
        ++run_number;
    } else {
        bool randomized = false;
        seed = next_run_seed(&randomized);
        if (randomized && !practice_entry) {
            static const char digits[] = "0123456789ABCDEF";
            for (unsigned i = 0; i < 8; ++i)
                canonical[i] = digits[(seed >> ((7 - i) * 4)) & 15];
            string_seed = true;
        }
    }
    uint64_t run_hash = seed;
    if (string_seed) {
        if (!(imported ? reference::parse_seed_code(selected_seed, run_hash)
                       : reference::seed_hash(canonical, run_hash))) {
            message("种子无效");
            return false;
        }
        seed = uint32_t(run_hash) ^ uint32_t(run_hash >> 32);
        if (!seed)
            seed = 1;
    }
    const bool random = character_random;
    if (!practice_entry) {
        reference::Random rng;
        rng.named(run_hash, "act_selection");
        selected_region = reference::ActKind(
            select_start_region(options.mode == RunMode::Standard && first_standard_run(),
                                progress.discovered_acts, rng));
        if (random) {
            selected_character = RunModeCharacters[rng.below(CharacterKinds)];
            character_random = false;
        }
        if (options.mode == RunMode::Standard && selected_ascension > character_max_ascension())
            selected_ascension = uint8_t(character_max_ascension());
    }
    forget_run_checkpoint();
    new_run(game, seed, practice_entry, selected_character);
    if (!practice_entry) {
        game.run_options = options;
        prepare_profile_run(game);
        if (string_seed && !(imported ? world_import_seed(game, selected_seed)
                                      : world_set_seed_raw(game, canonical))) {
            message("种子无效");
            return false;
        }
        uint8_t seen[(EncounterKinds + 7) / 8]{};
        for (unsigned i = 0; i < EncounterKinds; ++i)
            if (progress.encounters[i][0].seen)
                seen[i / 8] |= uint8_t(1u << (i % 8));
        if (!world_begin_run(game, selected_region, selected_ascension,
                             options.mode == RunMode::Standard ? seen : nullptr)) {
            message("Could not start the run.");
            return false;
        }
    }
    if (animated) {
        character_departure.phase = 1;
        character_departure.character = uint8_t(selected_character);
        character_departure.started = map_clock_ms;
        character_departure.reveal_ms = random ? CharacterDeparturePresentation::RandomRevealMs : 0;
        character_departure.continuing = false;
        const int mask = content.ui_image(I12_CharacterTransitionFirst +
                                          (selected_character == Character::Silent        ? 1
                                           : selected_character == Character::Necrobinder ? 2
                                                                                          : 0));
        content.prepare_images(&mask, 1);
        pointer_cancel();
        hit_count = 0;
        return true;
    }
    finish_selected_run();
    return true;
}
void App::finish_selected_run() {
    const auto entry_heal = reward_presentation.heal;
    character_select = run_settings = false;
    run_mode = {};
    title = false;
    reset_presentation();
    turn_banner_suppress_intro = false;
    begin_progress();
    // The initial room was built behind the character transition. Its
    // actual Neow heal retains NAncientEventLayout's200ms delay after this
    // new room becomes visible behind its fade-in.
    if (entry_heal.active && game.world.active &&
        game.original_events.id == OriginalEventId::Neow) {
        reward_presentation.heal = entry_heal;
        reward_presentation.heal.start = map_clock_ms + 200u;
    }
    if (game.world.active) {
        map_visuals.bind(game, map_clock_ms);
        if (const auto* map = world_current_map(game))
            map_visuals.initial_marker(map->root, map_clock_ms);
    }
}
void App::advance_character_departure() {
    auto& m = character_departure;
    if (!m.active())
        return;
    if (m.phase == 3 && map_clock_ms - m.started >= CharacterDeparturePresentation::FadeMs) {
        return_to_title();
        m.phase = 2;
        m.started = map_clock_ms;
    } else if (m.phase == 1 &&
               map_clock_ms - m.started >= m.reveal_ms + CharacterDeparturePresentation::FadeMs) {
        const auto character = m.character;
        if (m.continuing) {
            title = character_select = run_settings = false;
            run_mode = {};
            m.continuing = false;
        } else
            finish_selected_run();
        m.phase = 2;
        m.character = character;
        m.started = map_clock_ms;
    } else if (m.phase == 2 && map_clock_ms - m.started >= CharacterDeparturePresentation::FadeMs)
        m = {};
}
bool App::run_modes_action(UiAction action) {
    if (action == UiAction::OpenStandardRun) {
        if (title && run_mode.screen == RunMenu::Singleplayer) {
            run_mode.screen = RunMenu::Closed;
            run_mode.pending = {};
            practice_entry = false;
            character_select = true;
            selected_character = Character::Ironclad;
            selected_ascension = 0;
            selected_seed[0] = 0;
            selected_seed_imported = false;
            selected_region = reference::ActKind::Overgrowth;
        }
        return true;
    }
    if (action == UiAction::OpenCustomRun || action == UiAction::OpenDailyRun) {
        if (title && run_mode.screen == RunMenu::Singleplayer)
            open_run_menu(action == UiAction::OpenCustomRun ? RunMenu::Custom : RunMenu::Daily);
        return true;
    }
    if (action == UiAction::StartConfiguredRun) {
        if (run_mode.screen == RunMenu::Daily) {
            refresh_daily_run();
            if (run_mode.pending.mode != RunMode::Daily)
                return true;
        }
        if (run_mode.screen == RunMenu::Custom || run_mode.screen == RunMenu::Daily)
            start_selected_run(run_mode.pending, true);
        return true;
    }
    if (action == UiAction::RandomizeRun) {
        if (run_mode.screen == RunMenu::Custom) {
            reference::Random rng;
            rng.reset(next_run_seed());
            Character allowed[CharacterKinds];
            unsigned count = 0;
            for (auto c : RunModeCharacters)
                if (character_available(unsigned(c)))
                    allowed[count++] = c;
            choose_character(allowed[rng.below(count)]);
            pick_run_modifiers(rng, selected_character, run_mode.pending);
        }
        return true;
    }
    if (run_mode.screen != RunMenu::Custom)
        return false;
    if (action == UiAction::RunAscensionDown) {
        if (selected_ascension)
            --selected_ascension;
        return true;
    }
    if (action == UiAction::RunAscensionUp) {
        if (selected_ascension < 10)
            ++selected_ascension;
        return true;
    }
    if (action == UiAction::RunSeed || action == UiAction::RunSeedImport) {
        if (action == UiAction::RunSeedImport && run_seed_editing) {
            map_preview.importing = !map_preview.importing;
            map_preview.draft[0] = 0;
            map_preview.overflow = false;
            map_preview.error = "";
            return true;
        }
        map_preview.open = true;
        run_seed_editing = true;
        map_preview.begin_edit();
        map_preview.importing = action == UiAction::RunSeedImport || selected_seed_imported;
        unsigned n = 0;
        do {
            map_preview.draft[n] = selected_seed[n];
        } while (selected_seed[n++]);
        if (map_preview.importing && !selected_seed_imported)
            map_preview.draft[0] = 0;
        return true;
    }
    if (action == UiAction::RunSeedClear) {
        selected_seed[0] = 0;
        selected_seed_imported = false;
        return true;
    }
    switch (action) {
    case UiAction::SelectIronclad:
        choose_character(Character::Ironclad);
        return true;
    case UiAction::SelectSilent:
        choose_character(Character::Silent);
        return true;
    case UiAction::SelectRegent:
        choose_character(Character::Regent);
        return true;
    case UiAction::SelectNecrobinder:
        choose_character(Character::Necrobinder);
        return true;
    case UiAction::SelectDefect:
        choose_character(Character::Defect);
        return true;
    default:
        return false;
    }
}
bool App::run_modes_input(Key key) {
    if (run_mode.screen == RunMenu::Closed || map_preview.open)
        return false;
    if (key == Key::Quit) {
        frontend_confirm = 3;
        frontend_choice = 0;
        return true;
    }
    if (key == Key::Back) {
        if (run_mode.screen == RunMenu::Singleplayer)
            run_mode = {};
        else
            open_run_menu(RunMenu::Singleplayer);
        pointer_cancel();
        return true;
    }
    if (run_mode.screen == RunMenu::Singleplayer) {
        if (key == Key::Left) {
            if (compendium_selection)
                --compendium_selection;
        } else if (key == Key::Right) {
            if (compendium_selection < 2)
                ++compendium_selection;
        } else if (key == Key::Confirm) {
            const UiAction actions[] = {UiAction::OpenStandardRun, UiAction::OpenDailyRun,
                                        UiAction::OpenCustomRun};
            ui_action(actions[compendium_selection % 3]);
        }
        return true;
    }
    if (key == Key::EndTurn || (run_mode.screen == RunMenu::Daily && key == Key::Confirm)) {
        ui_action(UiAction::StartConfiguredRun);
        return true;
    }
    if (run_mode.screen == RunMenu::Daily) {
        if (key == Key::Up || key == Key::Down)
            progress_page_scroll.wheel(key == Key::Up ? -4 : 4);
        return true;
    }
    auto& focus = run_mode.focus;
    if (focus < 0)
        focus = 0;
    const int previous = focus;
    if (key == Key::Map)
        focus = 9;
    else if (key == Key::Right) {
        if (focus < 4)
            ++focus;
        else if (focus == 5)
            focus = 6;
        else if (focus >= 6 && focus < 9)
            focus = 9;
    } else if (key == Key::Left) {
        if (focus >= 8)
            focus = 7;
        else if (focus == 6)
            focus = 5;
        else if (focus > 0 && focus < 5)
            --focus;
    } else if (key == Key::Up) {
        if (focus >= 10)
            --focus;
        else if (focus == 9 || focus == 7)
            focus = 8;
        else if (focus == 5 || focus == 6 || focus < 5)
            focus = 7;
    } else if (key == Key::Down) {
        if (focus >= 9) {
            if (focus < int(9 + RunModifierKinds) - 1)
                ++focus;
        } else if (focus == 8)
            focus = 9;
        else if (focus == 7)
            focus = 5;
        else if (focus == 5 || focus == 6) {
            for (unsigned i = 0; i < CharacterKinds; ++i)
                if (RunModeCharacters[i] == selected_character)
                    focus = int16_t(i);
        }
    } else if (key == Key::Confirm) {
        const UiAction actions[] = {
            UiAction::SelectIronclad,    UiAction::SelectSilent, UiAction::SelectRegent,
            UiAction::SelectNecrobinder, UiAction::SelectDefect, UiAction::RunAscensionDown,
            UiAction::RunAscensionUp,    UiAction::RunSeed,      UiAction::RandomizeRun};
        if (focus < 9)
            ui_action(actions[focus]);
        else
            run_modifier_select(unsigned(focus - 9));
    }
    // NCharacterSelectButton connects FocusEntered to Select; navigating the
    // custom portrait row immediately changes the selected character.
    if (focus != previous && focus >= 0 && focus < int(CharacterKinds))
        choose_character(RunModeCharacters[unsigned(focus)]);
    return true;
}

} // namespace spire
