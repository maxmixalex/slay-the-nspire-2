#pragma once
#include "core.hpp"

namespace spire {
inline bool has_run_modifier(const State&s,RunModifier modifier){
    return (s.run_options.modifiers&run_modifier_bit(modifier))!=0;
}
inline RunModifier current_neow_modifier(const State&s){
    return s.original_events.active&&s.original_events.id==OriginalEventId::Neow&&s.run_options.neow_current&&
        s.run_options.neow_current<=RunModifierKinds?RunModifier(s.run_options.neow_current-1):RunModifier::Count;
}
inline bool neow_modifier_selection(const State&s,RunModifier modifier){
    return (s.run_options.setup_flags&1)&&current_neow_modifier(s)==modifier;
}
inline unsigned run_modifier_count(const RunOptions&o){
    unsigned count=0;for(uint32_t bits=o.modifiers;bits;bits&=bits-1)++count;return count;
}
inline RunModifier run_modifier_at(const RunOptions&o,unsigned ordinal){
    unsigned source=ordinal;
    if(o.mode==RunMode::Daily&&(o.setup_flags&2)&&ordinal<2)source=1-ordinal;
    for(unsigned i=0;i<RunModifierKinds;++i)if(o.modifiers&(uint32_t(1)<<i)){
        if(!source)return RunModifier(i);--source;
    }
    return RunModifier::Count;
}
inline uint8_t run_modifier_character_mask(const State&s){
    const auto bits=s.run_options.modifiers;
    return uint8_t(((bits>>unsigned(RunModifier::IroncladCards))&1u)<<unsigned(Character::Ironclad)|
        ((bits>>unsigned(RunModifier::SilentCards))&1u)<<unsigned(Character::Silent)|
        ((bits>>unsigned(RunModifier::RegentCards))&1u)<<unsigned(Character::Regent)|
        ((bits>>unsigned(RunModifier::NecrobinderCards))&1u)<<unsigned(Character::Necrobinder)|
        ((bits>>unsigned(RunModifier::DefectCards))&1u)<<unsigned(Character::Defect));
}
inline bool configure_run_options(State&s,RunMode mode,uint32_t modifiers,uint32_t daily_day=0){
    RunOptions value{};value.mode=mode;value.modifiers=modifiers;value.daily_day=daily_day;
    if(s.world.active||!valid_run_options(value))return false;
    s.run_options=value;return true;
}
} // namespace spire
