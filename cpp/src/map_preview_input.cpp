#include "app.hpp"
namespace spire {
void App::reference_text(int code) {
    pointer_cancel();
    if (map_preview.open && map_preview.editing) {
        if (code == 13 && map_preview.overflow) {
            map_preview.error = content.language() == 2
                                    ? "输入过长。退格修改，或转换后导入种子码。"
                                    : "Too long. Backspace to edit, or import a code.";
            return;
        }
        if (run_seed_editing && code == 13) {
            char canonical[65]{};
            uint64_t imported = 0;
            if (map_preview.importing && !reference::parse_seed_code(map_preview.draft, imported)) {
                map_preview.error = content.language() == 2 ? "输入 SEED64- 和 16 位十六进制数字。"
                                                            : "Use SEED64- plus 16 hex digits.";
                return;
            }
            if (!map_preview.importing &&
                !reference::canonicalize_seed(map_preview.draft, canonical, sizeof canonical)) {
                map_preview.error = content.language() == 2
                                        ? "种子文本最多 64 字节；长种子请导入。"
                                        : "Text: at most 64 UTF-8 bytes. Import long seeds.";
                return;
            }
            // NCustomRunScreen stores the entered text; StartRunLobby applies
            // SeedHelper only at departure. Whitespace-only is therefore an
            // explicit empty canonical seed, whereas blank means random.
            selected_seed_imported = map_preview.importing;
            if (selected_seed_imported)
                reference::format_seed_code(imported, selected_seed);
            else {
                unsigned n = 0;
                do {
                    selected_seed[n] = map_preview.draft[n];
                } while (map_preview.draft[n++]);
            }
            map_preview.editing = map_preview.open = run_seed_editing = false;
            return;
        }
        map_preview.edit(code);
        if (map_preview.overflow)
            map_preview.error = content.language() == 2
                                    ? "输入过长。退格修改，或转换后导入种子码。"
                                    : "Too long. Backspace to edit, or import a code.";
        if (run_seed_editing && !map_preview.editing) {
            if (code == 13) {
                unsigned n = 0;
                do {
                    selected_seed[n] = map_preview.seed[n];
                } while (map_preview.seed[n++]);
            }
            run_seed_editing = false;
            map_preview.open = false;
        }
    }
}
bool App::reference_input(Key key) {
    auto& view = map_preview;
    if (key == Key::ReferenceMap) {
        pointer_cancel();
        view.editing = false;
        view.open = !view.open;
        if (view.open && !view.generated)
            view.generate();
        return true;
    }
    if (!view.open)
        return false;
    if (view.editing) {
        if (run_seed_editing && run_mode.screen == RunMenu::Custom) {
            if (key == Key::Up || key == Key::Down)
                ui_action(UiAction::RunSeedImport);
            else if (key == Key::Back)
                reference_text(27);
            else if (key == Key::Confirm || key == Key::EndTurn)
                reference_text(13);
            else if (key == Key::Quit)
                quit = true;
            return true;
        }
        if (key == Key::Back)
            reference_text(27);
        else if (key == Key::Left)
            view.editor_selection = (view.editor_selection + SeedButtonCount - 1) % SeedButtonCount;
        else if (key == Key::Right)
            view.editor_selection = (view.editor_selection + 1) % SeedButtonCount;
        else if (key == Key::Up)
            view.editor_selection =
                view.editor_selection >= 10 ? view.editor_selection - 10 : SeedButtonCount - 1;
        else if (key == Key::Down)
            view.editor_selection =
                view.editor_selection < 30 ? view.editor_selection + 10 : SeedButtonCount - 1;
        else if (key == Key::Confirm) {
            int index = view.editor_selection;
            if (index < SeedKeyCount)
                reference_text(SeedKeys[index]);
            else if (index == SeedKeyCount)
                reference_text(8);
            else if (index == SeedKeyCount + 1)
                reference_text(-1);
            else if (index == SeedKeyCount + 2)
                reference_text(27);
            else
                reference_text(13);
        } else if (key == Key::Quit)
            quit = true;
        return true;
    }
    if (key == Key::Back)
        view.open = false;
    else if (key == Key::Up)
        view.first_row =
            (view.first_row < int(view.map.config.rows) - 3 ? view.first_row + 1 : view.first_row);
    else if (key == Key::Down)
        view.first_row = view.first_row > 0 ? view.first_row - 1 : 0;
    else if (key == Key::Left)
        view.change_seed(-1);
    else if (key == Key::Right)
        view.change_seed(1);
    else if (key == Key::Confirm) {
        pointer_cancel();
        view.begin_edit();
    } else if (key == Key::Quit)
        quit = true;
    return true;
}
} // namespace spire
