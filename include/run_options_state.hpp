#pragma once
#include <stdint.h>

namespace spire {
// ModelDb.GoodModifiers followed by BadModifiers. CharacterCards has one
// selectable instance per character, in the original character enumeration.
enum class RunMode : uint8_t { Standard, Custom, Daily };
enum class RunModifier : uint8_t {
    Draft, SealedDeck, Hoarder, Specialized, Insanity, AllStar, Flight, Vintage,
    IroncladCards, SilentCards, RegentCards, NecrobinderCards, DefectCards,
    DeadlyEvents, CursedRun, BigGameHunter, Midas, Murderous, NightTerrors, Terminal,
    Count
};
constexpr unsigned RunModifierKinds=unsigned(RunModifier::Count);
constexpr uint32_t RunModifierMask=(uint32_t(1)<<RunModifierKinds)-1;
constexpr uint32_t run_modifier_bit(RunModifier value){return uint32_t(1)<<unsigned(value);}
constexpr uint32_t RunExclusiveDeckModifiers=run_modifier_bit(RunModifier::Draft)|
    run_modifier_bit(RunModifier::SealedDeck)|run_modifier_bit(RunModifier::Insanity);

// Persisted simulation options, never optional UI metadata. Standard runs
// keep this structure zero. The v17 writer appends it after the frozen v15
// payload; historical v15 Standard and v16 Custom/Daily saves remain readable.
// Neow's ordered modifier selections can suspend in an ordinary card chooser.
struct RunOptions {
    uint32_t modifiers=0,daily_day=0,neow_completed=0;
    RunMode mode=RunMode::Standard;
    // Bit 0: pending Neow selection. Bit 1: Daily's two good modifiers were
    // drawn in reverse enum order; its bad modifier is always last.
    uint8_t neow_current=0,draft_remaining=0,setup_flags=0;
};
static_assert(sizeof(RunOptions)==16,"Keep custom run state bounded");
inline bool valid_run_options(const RunOptions&o){
    if(unsigned(o.mode)>unsigned(RunMode::Daily)||(o.modifiers&~RunModifierMask)||
       (o.neow_completed&~o.modifiers)||o.neow_current>RunModifierKinds||
       o.draft_remaining>10||(o.setup_flags&~3u))return false;
    const uint32_t exclusive=o.modifiers&RunExclusiveDeckModifiers;
    if(exclusive&&(exclusive&(exclusive-1)))return false;
    if(o.mode==RunMode::Standard)return !o.modifiers&&!o.daily_day&&!o.neow_completed&&
        !o.neow_current&&!o.draft_remaining&&!o.setup_flags;
    return o.mode==RunMode::Daily||(!o.daily_day&&!(o.setup_flags&2));
}
} // namespace spire
