#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "world_card_factory.hpp"
#include "ui_core_observer.hpp"
#include "settings_layout.hpp"
#include "menu_overlay_layout.hpp"

namespace spire {
static bool world_choice_phase(Phase p) {
    return p == Phase::WorldRelicChoice || p == Phase::OriginalEventChoice;
}
// Source CardSelectCmd routes deck, simple-grid and bundle selection to
// different overlays. The pending core choice keeps the original indexes;
// previewing never copies State or consumes any gameplay random stream.
RelicId App::world_selection_owner() const {
    if (game.phase != Phase::WorldRelicChoice)
        return NoWorldRelic;
    const auto& w = game.world_relics;
    for (unsigned i = 0; i < w.count; ++i)
        if (w.entries[i].serial == w.choice_owner)
            return w.entries[i].id;
    return NoWorldRelic;
}
WorldSelectionKind App::world_selection_kind() const {
    if (combat_selection_kind() != CombatSelectionKind::None || world_choice_is_card_reward())
        return WorldSelectionKind::None;
    if (game.phase == Phase::WorldRelicChoice) {
        switch (game.world_relics.choice) {
        case WorldRelicChoice::Remove:
        case WorldRelicChoice::Duplicate:
            return WorldSelectionKind::Deck;
        case WorldRelicChoice::Upgrade:
            return WorldSelectionKind::Upgrade;
        case WorldRelicChoice::Transform:
            return WorldSelectionKind::Transform;
        case WorldRelicChoice::Enchant:
            return WorldSelectionKind::Enchant;
        case WorldRelicChoice::OfferBundle:
            return WorldSelectionKind::Bundle;
        case WorldRelicChoice::OfferCard:
            if (world_selection_owner() == WorldRelicSeaGlass)
                return WorldSelectionKind::Simple;
            break;
        default:
            break;
        }
    } else if (game.phase == Phase::OriginalEventChoice) {
        const auto& e = game.original_events;
        switch (e.selection) {
        case OriginalEventSelection::Remove:
        case OriginalEventSelection::Duplicate:
            return WorldSelectionKind::Deck;
        case OriginalEventSelection::Upgrade:
            return WorldSelectionKind::Upgrade;
        case OriginalEventSelection::Transform:
            return e.id == OriginalEventId::WoodCarvings ? WorldSelectionKind::Deck
                                                         : WorldSelectionKind::Transform;
        case OriginalEventSelection::Enchant:
            return WorldSelectionKind::Enchant;
        case OriginalEventSelection::OfferCard:
            if (neow_modifier_selection(game, RunModifier::SealedDeck) ||
                e.id == OriginalEventId::BrainLeech || e.id == OriginalEventId::RoomFullOfCheese)
                return WorldSelectionKind::Simple;
            break;
        default:
            break;
        }
    }
    return WorldSelectionKind::None;
}
bool App::world_selection_active() const {
    return world_selection_kind() != WorldSelectionKind::None && !title && !paused && !help &&
           !panel && !library && !character_select && !settings_open && !frontend_confirm &&
           !map_preview.open && !map_window.is_open && !event_reader && !ftue.active() &&
           detail_card < 0 && detail_item < 0 && status_actor < 0 && !choice_peeking &&
           combat_choice_ready();
}
bool App::world_selection_cancelable() const {
    if (game.phase == Phase::OriginalEventChoice)
        return game.original_events.select_cancelable != 0;
    // The relic core uses min==0 as an empty-selection continuation flag.
    // Source Cancelable remains false for Claws, Gnarled Hammer and Kifuda.
    const auto& w = game.world;
    return game.phase == Phase::WorldRelicChoice && w.active && w.rest_pending &&
           w.rest_pending <= w.rest_count &&
           (WorldRestOption(w.rest_options[w.rest_pending - 1]) == WorldRestOption::Cook ||
            WorldRestOption(w.rest_options[w.rest_pending - 1]) == WorldRestOption::Smith);
}
unsigned App::world_selection_min() const {
    return world_selection_cancelable() && game.phase == Phase::WorldRelicChoice
               ? world_choice_max()
               : world_choice_min();
}
unsigned App::world_selection_max() const {
    // These source selectors keep their authored MaxSelect even when fewer
    // eligible cards exist. The core's clamped bound remains the commit limit.
    const auto owner = world_selection_owner();
    if (owner == WorldRelicClaws)
        return 6;
    if (owner == WorldRelicGnarledHammer || owner == WorldRelicKifuda)
        return 3;
    return world_choice_max();
}
bool App::world_selection_manual() const {
    if (world_selection_kind() == WorldSelectionKind::Simple) {
        return game.phase == Phase::WorldRelicChoice ||
               neow_modifier_selection(game, RunModifier::SealedDeck) ||
               world_selection_min() != world_choice_max();
    }
    return true; // Every deck overlay opens its original preview at MaxSelect.
}
bool App::world_selection_upgrades() const {
    const auto kind = world_selection_kind();
    return kind == WorldSelectionKind::Upgrade || kind == WorldSelectionKind::Transform;
}
bool App::world_selection_confirmable() const {
    if (!world_selection_active() || choice_selected_count < world_selection_min() ||
        choice_selected_count > world_choice_max() ||
        !world_rest_selection_allowed(game, choice_selected_count))
        return false;
    const auto kind = world_selection_kind();
    // The pinned enchant screen enables its empty-preview Confirm, but
    // ConfirmSelection itself ignores zero cards. Preserve that distinction.
    if (deck_selection.preview)
        return true;
    if (kind == WorldSelectionKind::Simple)
        return world_selection_manual();
    if (kind == WorldSelectionKind::Bundle || kind == WorldSelectionKind::Upgrade)
        return false;
    return world_selection_min() != world_selection_max();
}
CombatSelectionPrompt App::world_selection_prompt() const {
    const auto kind = world_selection_kind();
    if (kind == WorldSelectionKind::Bundle)
        return {"gameplay_ui", "CHOOSE_A_PACK"};
    if (game.phase == Phase::WorldRelicChoice) {
        const auto owner = world_selection_owner();
        if (owner == WorldRelicDollysMirror)
            return {"relics", "DOLLYS_MIRROR.selectionScreenPrompt"};
        if (owner == WorldRelicClaws)
            return {"relics", "CLAWS.selectionScreenPrompt"};
        if (owner == WorldRelicSeaGlass)
            return {"relics", "SEA_GLASS.selectionScreenPrompt"};
        if (game.world_relics.choice == WorldRelicChoice::Remove && game.world_relics.choice_reward)
            return {"gameplay_ui", "COMBAT_REWARD_CARD_REMOVAL.selectionScreenPrompt"};
    } else {
        const auto& e = game.original_events;
        if (neow_modifier_selection(game, RunModifier::SealedDeck))
            return {"modifiers", "SEALED_DECK.selectionPrompt"};
        if (kind == WorldSelectionKind::Simple && e.id == OriginalEventId::BrainLeech)
            return {"events", "BRAIN_LEECH.pages.SHARE_KNOWLEDGE.selectionScreenPrompt"};
        if (kind == WorldSelectionKind::Simple && e.id == OriginalEventId::RoomFullOfCheese)
            return {"events", "ROOM_FULL_OF_CHEESE.pages.GORGE.selectionScreenPrompt"};
        if (e.selection == OriginalEventSelection::Transform)
            return {"card_selection", "TO_TRANSFORM"};
    }
    return {"card_selection", kind == WorldSelectionKind::Upgrade     ? "TO_UPGRADE"
                              : kind == WorldSelectionKind::Transform ? "TO_TRANSFORM"
                              : kind == WorldSelectionKind::Enchant   ? "TO_ENCHANT"
                                                                      : "TO_REMOVE"};
}
EventEnchantment App::world_selection_enchantment(int* amount) const {
    if (amount)
        *amount = game.phase == Phase::WorldRelicChoice ? game.world_relics.choice_amount
                                                        : game.original_events.select_amount;
    return game.phase == Phase::WorldRelicChoice
               ? game.world_relics.choice_enchantment
               : EventEnchantment(game.original_events.select_enchantment);
}
void App::sync_world_selection() {
    if (world_selection_kind() == WorldSelectionKind::None)
        return;
    sync_selection();
    auto& m = deck_selection;
    if (!m.bound || m.revision != game.revision || m.phase != unsigned(game.phase)) {
        m.reset();
        m.bound = true;
        m.revision = game.revision;
        m.phase = uint8_t(game.phase);
        m.opened = map_clock_ms;
    }
    if (!pointer.visible && world_selection_active())
        m.show_upgrades = false;
}
void App::cancel_world_selection() {
    if (!world_selection_active())
        return;
    sync_world_selection();
    if (deck_selection.preview) {
        if (world_selection_kind() == WorldSelectionKind::Bundle && choice_selected_count) {
            auto& motion = card_grids[unsigned(CardGridSurface::WorldChoice)].faces[15];
            motion.index = int16_t(choice_selected[0]);
            motion.start = deck_selection.preview_opened;
            motion.selection_start = map_clock_ms;
            motion.chosen = true;
        }
        if (choice_selected_count) {
            uint16_t ids[MaxWorldUiChoices];
            const unsigned count = world_choice_options(ids, MaxWorldUiChoices);
            for (unsigned i = 0; i < count && i < MaxWorldUiChoices; ++i)
                if (ids[i] == choice_selected[choice_selected_count - 1])
                    cursor = int(i);
        }
        choice_selected_count = 0;
        deck_selection.preview = false;
        pointer_cancel();
        focus_card_grid();
        return;
    }
    if (!world_selection_cancelable())
        return;
    continue_combat_choice();
    const bool done = game.phase == Phase::WorldRelicChoice
                          ? world_relic_select(game, nullptr, 0)
                          : original_event_select(game, nullptr, 0);
    if (done) {
        choice_selected_count = 0;
        choice_revision = game.revision;
        deck_selection.reset();
        cursor = 0;
        busy = 0;
        pointer_cancel();
    }
}
unsigned App::world_selection_preview_cards() const {
    if (!deck_selection.preview)
        return 0;
    const auto kind = world_selection_kind();
    if (kind == WorldSelectionKind::Bundle)
        return choice_selected_count ? 3 : 0;
    if (kind == WorldSelectionKind::Transform)
        return choice_selected_count * 2;
    if ((kind == WorldSelectionKind::Upgrade || kind == WorldSelectionKind::Enchant) &&
        world_selection_max() == 1)
        return choice_selected_count * 2;
    return choice_selected_count;
}
bool App::world_selection_preview_card(unsigned index, WorldSelectionCard& out) const {
    out = {};
    const auto kind = world_selection_kind();
    const unsigned count = choice_selected_count;
    if (index >= world_selection_preview_cards())
        return false;
    using namespace deck_selection_layout;
    const bool paired =
        kind == WorldSelectionKind::Transform ||
        ((kind == WorldSelectionKind::Upgrade || kind == WorldSelectionKind::Enchant) &&
         world_selection_max() == 1);
    const bool after = paired && index >= count;
    const unsigned at = paired ? index % count : index;
    const int deck = kind == WorldSelectionKind::Bundle ? -1 : choice_selected[at];
    if (kind == WorldSelectionKind::Bundle) {
        const unsigned offer = choice_selected[0] * 3 + at;
        if (offer >= game.world_relics.offer_count)
            return false;
        out.card = card_from_token(game.world_relics.offers[offer]);
        out.facts = game.world_relics.offer_facts[offer];
    } else {
        if (deck < 0 || deck >= game.deck_count)
            return false;
        out.deck = int16_t(deck);
        out.card = game.deck[deck];
        out.facts = game.event_cards.deck[deck];
    }
    if (kind == WorldSelectionKind::Upgrade && (after || !paired)) {
        out.card.upgraded = 1;
        out.upgraded = true;
    }
    if (kind == WorldSelectionKind::Enchant && after) {
        int amount = 0;
        const auto enchant = world_selection_enchantment(&amount);
        if (!event_card_apply_enchantment(out.card, out.facts, enchant, amount))
            return false;
    }
    if (kind == WorldSelectionKind::Transform && after) {
        const Card original = out.card;
        CardKind replacement = CardKinds;
        const auto owner = world_selection_owner();
        if (game.phase == Phase::WorldRelicChoice && owner == WorldRelicClaws)
            replacement = EventCardMaul;
        else if (game.phase == Phase::OriginalEventChoice && game.original_events.select_extra > 0)
            replacement = CardKind(game.original_events.select_extra - 1);
        out.hover = replacement < CardKinds;
        out.deck = -1;
        out.facts = {};
        if (replacement >= CardKinds) {
            const unsigned rarity = card_rarity(original.kind);
            WorldCardOptions options;
            options.flags = WorldNoModifications;
            options.pools[0] = definitions[original.kind].type == CardType::Quest || rarity == 5 ||
                                       rarity == 6 || rarity == 7
                                   ? WorldCardPool::Colorless
                                   : world_card_pool(original.kind);
            if (rarity != 8 && rarity != 9)
                options.rarity_mask =
                    world_rarity_mask(2) | world_rarity_mask(3) | world_rarity_mask(4);
            CardKind candidates[CardKinds];
            const unsigned available = world_card_candidates(game, options, candidates, CardKinds);
            unsigned valid = 0;
            for (unsigned i = 0; i < available; ++i)
                if (candidates[i] != original.kind)
                    candidates[valid++] = candidates[i];
            if (!valid)
                return false;
            // The source cycles an independently shuffled preview every .2s.
            // This deterministic presentation order never touches run RNG.
            replacement = candidates[(uint32_t(map_clock_ms - deck_selection.preview_opened) / 200 +
                                      at * 17 + unsigned(original.kind)) %
                                     valid];
        }
        out.card = {replacement, 0};
        if (owner == WorldRelicClaws) {
            out.card.upgraded = uint8_t(original.upgraded && card_upgradeable(out.card));
            const auto& facts = game.event_cards.deck[deck];
            if (facts.enchantment != EventEnchantment::None &&
                event_card_can_enchant(game, out.card, out.facts, facts.enchantment))
                event_card_apply_enchantment(out.card, out.facts, facts.enchantment,
                                             facts.enchantment_amount);
        }
        if (owner == WorldRelicAstrolabe && card_upgradeable(out.card))
            out.card.upgraded = 1;
    }
    int scale = 1000, cx = 160, cy = 120;
    if (kind == WorldSelectionKind::Transform) {
        const int source_width = int(count) * 300 + int(count - 1) * 30;
        scale = source_width > 610 ? 610000 / source_width : 1000;
        const int center = after ? 960 : 710,
                  offset = after ? ((int(at) * 2 + 1) * 150 * scale / 1000 + int(at) * 30)
                                 : (-((int(count - at) * 2 - 1) * 150 * scale / 1000) -
                                    int(count - at - 1) * 30);
        cx = (center + offset) * 4 / 21;
    } else if (paired) {
        cx = after ? 213 : 107;
        if (kind == WorldSelectionKind::Enchant) {
            cx = after ? 156 : 59;
            scale = 900;
        }
    } else if (kind == WorldSelectionKind::Enchant) {
        // Keep every preview left of the description column too, including
        // the three-card Gnarled Hammer/Kifuda selector.
        scale = count <= 2 ? 850 : 600;
        const unsigned columns = count <= 2 ? count : 3;
        cx = 106 + (2 * int(at % columns) - int(columns) + 1) * (count <= 2 ? 98 : 64) / 2;
        if (count > 3)
            cy = at < 3 ? 65 : 153;
    } else {
        scale = count > 6 ? 550 : count > 3 ? 800 : 1000;
        const unsigned displayed = kind == WorldSelectionKind::Bundle ? 3 : count;
        const int slot = kind == WorldSelectionKind::Bundle ? int(displayed - 1 - at) : int(at);
        cx = 160 + (2 * slot - int(displayed) + 1) * 200 * 4 * scale / (21 * 1000);
        if (kind == WorldSelectionKind::Bundle) {
            cx = 160 + (2 * slot - int(displayed) + 1) * 52;
            cy = 139;
        }
    }
    out.w = int16_t((PreviewW * scale + 500) / 1000);
    out.h = int16_t((PreviewH * scale + 500) / 1000);
    out.x = int16_t(cx - out.w / 2);
    out.y = int16_t(cy - out.h / 2);
    return true;
}

unsigned App::world_choice_options(uint16_t* out, unsigned capacity) const {
    if (game.phase == Phase::WorldRelicChoice) {
        const unsigned count = world_relic_eligible(game, out, capacity);
        if (out && capacity >= count && game.world_relics.choice == WorldRelicChoice::Remove)
            for (unsigned i = 1; i < count; ++i) {
                const uint16_t value = out[i];
                unsigned j = i;
                if (card_type(game, game.deck[value], -1, value) == CardType::Curse) {
                    while (j && card_type(game, game.deck[out[j - 1]], -1, out[j - 1]) !=
                                    CardType::Curse) {
                        out[j] = out[j - 1];
                        --j;
                    }
                    out[j] = value;
                }
            }
        return count;
    }
    if (game.phase == Phase::OriginalEventChoice) {
        const unsigned count = original_event_eligible(game, out, capacity);
        if (out && capacity >= count && neow_modifier_selection(game, RunModifier::SealedDeck)) {
            const auto& e = game.original_events;
            for (unsigned i = 1; i < count; ++i) {
                const uint16_t value = out[i];
                const Card card = card_from_token(e.offers[value]);
                unsigned j = i;
                while (j) {
                    const Card prior = card_from_token(e.offers[out[j - 1]]);
                    const int rarity = card_rarity(prior.kind) - card_rarity(card.kind);
                    if (rarity < 0 || (!rarity && localized_card_rank(prior.kind) <=
                                                      localized_card_rank(card.kind)))
                        break;
                    out[j] = out[j - 1];
                    --j;
                }
                out[j] = value;
            }
        }
        if (out && capacity >= count &&
            game.original_events.selection == OriginalEventSelection::Remove)
            for (unsigned i = 1; i < count; ++i) {
                const uint16_t value = out[i];
                unsigned j = i;
                if (card_type(game, game.deck[value], -1, value) == CardType::Curse) {
                    while (j && card_type(game, game.deck[out[j - 1]], -1, out[j - 1]) !=
                                    CardType::Curse) {
                        out[j] = out[j - 1];
                        --j;
                    }
                    out[j] = value;
                }
            }
        return count;
    }
    return 0;
}
unsigned App::world_choice_min() const {
    return game.phase == Phase::WorldRelicChoice ? game.world_relics.choice_min
                                                 : game.original_events.select_min;
}
unsigned App::world_choice_max() const {
    return game.phase == Phase::WorldRelicChoice ? game.world_relics.choice_max
                                                 : game.original_events.select_max;
}
bool App::world_choice_is_potion_reward() const {
    return game.phase == Phase::WorldRelicChoice
               ? game.world_relics.choice == WorldRelicChoice::Potion
               : game.phase == Phase::OriginalEventChoice &&
                     game.original_events.selection == OriginalEventSelection::Potion;
}
Potion App::world_choice_potion(int id) const {
    if (id < 0)
        return Potion::Empty;
    if (game.phase == Phase::WorldRelicChoice) {
        const auto& w = game.world_relics;
        if (w.choice == WorldRelicChoice::Potion && w.offer_count)
            return Potion(w.offers[0]);
        if (w.choice == WorldRelicChoice::OfferReward && id < w.offer_count) {
            const auto& r = w.rewards[w.offers[id]];
            if (r.kind == 3)
                return Potion(r.value);
        }
    } else if (game.phase == Phase::OriginalEventChoice) {
        const auto& e = game.original_events;
        if (e.selection == OriginalEventSelection::Potion && e.offer_count)
            return Potion(e.offers[0]);
        if (e.selection == OriginalEventSelection::Rewards && id < e.reward_count &&
            e.rewards[id].kind == 2)
            return Potion(e.rewards[id].model);
    }
    return Potion::Empty;
}
bool App::world_choice_is_card_reward() const {
    if (game.phase == Phase::OriginalEventChoice &&
        neow_modifier_selection(game, RunModifier::Draft))
        return true;
    if (original_event_has_card_reward(game))
        return true;
    if (game.phase != Phase::WorldRelicChoice)
        return false;
    const auto& w = game.world_relics;
    if (w.choice != WorldRelicChoice::OfferCard || !w.choice_reward ||
        w.choice_reward > w.reward_count)
        return false;
    const auto& r = w.rewards[w.choice_reward - 1];
    return r.group && r.kind == 1 && !(r.flags & 1) && w.choice_max == 1;
}
unsigned App::world_choice_page_size() const {
    if (world_choice_is_card_reward())
        return MaxWorldUiChoices; // One original centered row.
    if (game.phase == Phase::WorldRelicChoice) {
        if (game.world_relics.choice == WorldRelicChoice::OfferReward)
            return MaxWorldUiChoices;
        if (game.world_relics.choice == WorldRelicChoice::OfferBundle)
            return 2;
    }
    if (game.phase == Phase::OriginalEventChoice &&
        game.original_events.selection == OriginalEventSelection::Rewards)
        return MaxWorldUiChoices;
    return 10;
}
void App::toggle_world_choice(int id) {
    if (combat_selection_kind() != CombatSelectionKind::None) {
        toggle_choice(id);
        return;
    }
    if (choice_peeking || !world_choice_phase(game.phase) || id < 0 || !combat_choice_ready())
        return;
    sync_selection();
    const auto native = world_selection_kind();
    if (native != WorldSelectionKind::None) {
        if (!world_selection_active())
            return;
        sync_world_selection();
        if (deck_selection.preview)
            return;
    }
    if (world_choice_is_card_reward() && !card_reward_input_ready())
        return;
    uint16_t ids[MaxWorldUiChoices];
    unsigned n = world_choice_options(ids, MaxWorldUiChoices);
    bool eligible = false;
    for (unsigned i = 0; i < n && i < MaxWorldUiChoices; i++)
        if (ids[i] == id)
            eligible = true;
    if (!eligible)
        return;
    const bool reward = game.phase == Phase::WorldRelicChoice
                            ? game.world_relics.choice == WorldRelicChoice::OfferReward
                            : game.original_events.selection == OriginalEventSelection::Rewards;
    if (reward || world_choice_is_card_reward() || world_choice_is_potion_reward()) {
        const bool potion = world_choice_potion(id) != Potion::Empty;
        continue_combat_choice();
        uint16_t selected = uint16_t(id);
        bool done = game.phase == Phase::WorldRelicChoice
                        ? world_relic_select(game, &selected, 1)
                        : original_event_select(game, &selected, 1);
        if (done) {
            choice_selected_count = 0;
            choice_revision = game.revision;
            cursor = 0;
            busy = 0;
            pointer_cancel();
        } else if (potion)
            potion_obtain_failed();
        return;
    }
    for (unsigned i = 0; i < choice_selected_count; i++)
        if (choice_selected[i] == id) {
            for (unsigned j = i + 1; j < choice_selected_count; j++)
                choice_selected[j - 1] = choice_selected[j];
            --choice_selected_count;
            return;
        }
    unsigned maximum = world_choice_max();
    if (maximum == 1)
        choice_selected_count = 0;
    if (choice_selected_count < maximum && choice_selected_count < MaxDeck)
        choice_selected[choice_selected_count++] = uint16_t(id);
    if (native != WorldSelectionKind::None && choice_selected_count == world_selection_max()) {
        if (native == WorldSelectionKind::Simple) {
            if (!world_selection_manual())
                confirm_world_choice();
        } else {
            deck_selection.preview = true;
            deck_selection.preview_opened = map_clock_ms;
            cursor = 0;
            pointer_cancel();
        }
    }
}
void App::confirm_world_choice() {
    if (combat_selection_kind() != CombatSelectionKind::None) {
        if (combat_selection_kind() != CombatSelectionKind::Offer && combat_selection_confirmable())
            confirm_choice();
        return;
    }
    if (choice_peeking || !world_choice_phase(game.phase) || !combat_choice_ready())
        return;
    sync_selection();
    const auto native = world_selection_kind();
    if (native != WorldSelectionKind::None) {
        if (!world_selection_active())
            return;
        sync_world_selection();
        if (choice_selected_count < world_selection_min() ||
            choice_selected_count > world_choice_max() ||
            !world_rest_selection_allowed(game, choice_selected_count))
            return;
        if (native != WorldSelectionKind::Simple && !deck_selection.preview) {
            if (!world_selection_confirmable())
                return;
            deck_selection.preview = true;
            deck_selection.preview_opened = map_clock_ms;
            cursor = 0;
            pointer_cancel();
            return;
        }
        if (native == WorldSelectionKind::Enchant && !choice_selected_count)
            return;
    }
    if (choice_selected_count < world_choice_min() || choice_selected_count > world_choice_max())
        return;
    continue_combat_choice();
    bool done = game.phase == Phase::WorldRelicChoice
                    ? world_relic_select(game, choice_selected, choice_selected_count)
                    : original_event_select(game, choice_selected, choice_selected_count);
    if (done) {
        if (game.original_events.active &&
            game.original_events.id == OriginalEventId::Amalgamator) {
            screen_effect(ScreenShakePresentation::Trauma, ScreenShakePresentation::Strong);
            screen_effect(ScreenShakePresentation::Trauma, ScreenShakePresentation::Strong, 300, 0,
                          300);
        }
        choice_selected_count = 0;
        choice_revision = game.revision;
        deck_selection.reset();
        cursor = 0;
        busy = 0;
        pointer_cancel();
    }
}
void App::world_choice_detail(int id) {
    if (choice_peeking || id < 0 || !combat_choice_ready())
        return;
    // NPotionReward/NPotionLab provide hover tips, not a potion inspector.
    // Keep the currently focused reward and its canonical pending choice.
    if (world_choice_potion(id) != Potion::Empty)
        return;
    if (game.phase == Phase::WorldRelicChoice) {
        const auto& w = game.world_relics;
        if ((w.choice == WorldRelicChoice::OfferCard ||
             w.choice == WorldRelicChoice::CombatOffer) &&
            id < w.offer_count)
            open_card_offer(card_from_token(w.offers[id]), w.offer_facts[id]);
        else if (w.choice == WorldRelicChoice::OfferBundle && id * 3 < w.offer_count)
            open_card_offer(card_from_token(w.offers[id * 3]), w.offer_facts[id * 3]);
        else if (w.choice == WorldRelicChoice::Hand && id < game.card_count)
            open_choice_detail(id);
        else if (w.choice == WorldRelicChoice::OfferReward && id < w.offer_count) {
            const auto& r = w.rewards[w.offers[id]];
            if (r.kind == 2)
                detail_item = r.value;
            else if (r.kind == 5)
                open_card_offer(card_from_token(r.value), r.facts[0], r.tokens[0], r.tokens[1]);
            detail_owned = -1;
            detail_scroll = 0;
        } else if (w.choice != WorldRelicChoice::OfferReward &&
                   w.choice != WorldRelicChoice::Potion && id < game.deck_count)
            open_card_detail(game.deck[id], id);
    } else if (game.phase == Phase::OriginalEventChoice) {
        const auto& e = game.original_events;
        if (e.selection == OriginalEventSelection::OfferCard && id < e.offer_count)
            open_card_offer(card_from_token(e.offers[id]), e.offer_facts[id]);
        else if (e.selection == OriginalEventSelection::Relic && id < e.offer_count) {
            detail_item = e.offers[id];
            detail_owned = -1;
            detail_scroll = 0;
        } else if (e.selection == OriginalEventSelection::Rewards && id < e.reward_count) {
            const auto& r = e.rewards[id];
            if (r.kind == 3)
                detail_item = r.model;
            detail_owned = -1;
            detail_scroll = 0;
        } else if (e.selection != OriginalEventSelection::Relic &&
                   e.selection != OriginalEventSelection::Potion &&
                   e.selection != OriginalEventSelection::Crystal &&
                   e.selection != OriginalEventSelection::Rewards && id < game.deck_count)
            open_card_detail(game.deck[id], id);
    }
}
bool App::world_choice_input(Key key) {
    if (combat_selection_kind() != CombatSelectionKind::None)
        return selection_input(key);
    if (choice_peeking || title || paused || help || panel || !world_choice_phase(game.phase) ||
        !combat_choice_ready())
        return false;
    sync_selection();
    if (world_selection_kind() != WorldSelectionKind::None) {
        if (!world_selection_active())
            return false;
        sync_world_selection();
        if (key == Key::Back) {
            cancel_world_selection();
            return true;
        }
        if (key == Key::EndTurn) {
            if (world_selection_confirmable())
                confirm_world_choice();
            return true;
        }
        if (deck_selection.preview) {
            if (key == Key::Confirm) {
                if (world_selection_confirmable())
                    confirm_world_choice();
                return true;
            }
            const int count = int(world_selection_preview_cards());
            if ((key == Key::Left || key == Key::Right) && count) {
                cursor = (cursor + count + (key == Key::Left ? -1 : 1)) % count;
                return true;
            }
            if (key == Key::Detail && count) {
                WorldSelectionCard model{};
                if (world_selection_preview_card(unsigned(cursor), model) && model.hover)
                    open_card_offer(model.card, model.facts,
                                    model.deck >= 0 ? game.defect.genetic_deck[model.deck] : 0,
                                    model.deck >= 0 ? game.necro.scythe_deck[model.deck] : 0);
                return true;
            }
            return key == Key::Up || key == Key::Down;
        }
    }
    if (world_choice_is_card_reward())
        sync_card_reward_presentation();
    if (game.phase == Phase::OriginalEventChoice &&
        game.original_events.selection == OriginalEventSelection::Crystal)
        return crystal_input(key);
    uint16_t ids[MaxWorldUiChoices];
    unsigned count = world_choice_options(ids, MaxWorldUiChoices);
    if (count > MaxWorldUiChoices)
        return false;
    if (cursor < 0 || cursor >= int(count))
        cursor = 0;
    const bool rewards = game.phase == Phase::WorldRelicChoice
                             ? game.world_relics.choice == WorldRelicChoice::OfferReward
                             : game.original_events.selection == OriginalEventSelection::Rewards;
    const bool potion_reward = world_choice_is_potion_reward();
    if (potion_reward && (key == Key::EndTurn || key == Key::Back)) {
        choice_selected_count = 0;
        confirm_world_choice();
        return true;
    }
    const bool card_reward = world_choice_is_card_reward();
    const int columns = rewards ? 1 : card_reward ? 0 : 5;
    if (card_reward && key == Key::ExhaustPile) {
        card_reward_alternative(false);
        return true;
    }
    if (key == Key::Left && count)
        cursor = (cursor + int(count) - 1) % int(count);
    else if (key == Key::Right && count)
        cursor = (cursor + 1) % int(count);
    else if (key == Key::Up && count)
        cursor = cursor >= columns ? cursor - columns : cursor;
    else if (key == Key::Down && count)
        cursor = cursor + columns < int(count) ? cursor + columns : cursor;
    else if (key == Key::Confirm && count)
        toggle_world_choice(ids[cursor]);
    else if (key == Key::EndTurn && rewards)
        ui_action(UiAction::LeaveWorldRewards);
    else if (key == Key::EndTurn || (key == Key::Back && card_reward && world_choice_min() == 0)) {
        if (card_reward)
            choice_selected_count = 0;
        confirm_world_choice();
    } else if (key == Key::Detail && count)
        world_choice_detail(ids[cursor]);
    else
        return false;
    return true;
}

} // namespace spire
