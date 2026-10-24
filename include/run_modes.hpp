#pragma once
#include "run_options.hpp"
#include "reference_rng.hpp"

namespace spire {
// NCustomRunScreen, NSingleplayerSubmenu and NDailyRunScreen presentation.
// The pending options stay outside State until Embark is accepted.
enum class RunMenu : uint8_t { Closed, Singleplayer, Custom, Daily };
struct RunModeFrontend {
    RunOptions pending{};
    RunMenu screen=RunMenu::Closed;
    int16_t focus=-1;
    int16_t last_focus=-2;
    uint32_t started=0;
    uint32_t layout_generation=UINT32_MAX;
    uint16_t row_offsets[RunModifierKinds+1]{};
};
constexpr Character RunModeCharacters[]={Character::Ironclad,Character::Silent,
    Character::Regent,Character::Necrobinder,Character::Defect};
inline const char* run_modifier_key(RunModifier modifier){
    static const char*keys[]={"DRAFT","SEALED_DECK","HOARDER","SPECIALIZED","INSANITY","ALL_STAR","FLIGHT","VINTAGE",
        "IRONCLAD","SILENT","REGENT","NECROBINDER","DEFECT","DEADLY_EVENTS","CURSED_RUN","BIG_GAME_HUNTER","MIDAS","MURDEROUS","NIGHT_TERRORS","TERMINAL"};
    return unsigned(modifier)<RunModifierKinds?keys[unsigned(modifier)]:"";
}
inline bool run_modifier_character(RunModifier modifier){return modifier>=RunModifier::IroncladCards&&modifier<=RunModifier::DefectCards;}
inline unsigned run_modifier_icon(RunModifier modifier){
    return unsigned(modifier)<8?unsigned(modifier):run_modifier_character(modifier)?8:unsigned(modifier)-4;
}
inline void toggle_run_modifier(RunOptions&options,RunModifier modifier){
    if(unsigned(modifier)>=RunModifierKinds)return;
    const uint32_t bit=run_modifier_bit(modifier);
    if(options.modifiers&bit)options.modifiers&=~bit;
    else{if(bit&RunExclusiveDeckModifiers)options.modifiers&=~RunExclusiveDeckModifiers;options.modifiers|=bit;}
}
// ModifierModel.Pick2Good1Bad, including removal of every incompatible
// starting-deck option and the extra draw for CharacterCards' model.
inline void pick_run_modifiers(reference::Random&rng,Character excluded,RunOptions&options){
    RunModifier good[9]={RunModifier::Draft,RunModifier::SealedDeck,RunModifier::Hoarder,RunModifier::Specialized,
        RunModifier::Insanity,RunModifier::AllStar,RunModifier::Flight,RunModifier::Vintage,RunModifier::IroncladCards};
    unsigned count=9;RunModifier picked[2]{};options.modifiers=0;options.setup_flags=0;
    for(unsigned draw=0;draw<2;++draw){
        const unsigned index=rng.below(count);const RunModifier source=good[index];RunModifier chosen=source;
        if(source==RunModifier::IroncladCards){
            Character available[5]{};unsigned n=0;for(Character c:RunModeCharacters)if(c!=excluded)available[n++]=c;
            const Character character=available[rng.below(n)];unsigned ordinal=0;while(RunModeCharacters[ordinal]!=character)++ordinal;
            chosen=RunModifier(unsigned(RunModifier::IroncladCards)+ordinal);
        }
        picked[draw]=chosen;options.modifiers|=run_modifier_bit(chosen);
        const bool exclusive=(run_modifier_bit(source)&RunExclusiveDeckModifiers)!=0;
        unsigned n=0;for(unsigned i=0;i<count;++i)if(i!=index&&(!exclusive||!(run_modifier_bit(good[i])&RunExclusiveDeckModifiers)))good[n++]=good[i];count=n;
    }
    options.modifiers|=run_modifier_bit(RunModifier(unsigned(RunModifier::DeadlyEvents)+rng.below(7)));
    if(options.mode==RunMode::Daily&&picked[0]>picked[1])options.setup_flags=2;
}
struct RunDate {unsigned year=1970,month=1,day=1;};
inline RunDate run_date(uint32_t day){
    RunDate date;const auto leap=[](unsigned y){return y%4==0&&(y%100!=0||y%400==0);};
    while(day>=(leap(date.year)?366u:365u)){day-=leap(date.year)?366:365;++date.year;}
    const unsigned months[]={31,28,31,30,31,30,31,31,30,31,30,31};
    while(date.month<12&&day>=months[date.month-1]+unsigned(date.month==2&&leap(date.year))){day-=months[date.month-1]+unsigned(date.month==2&&leap(date.year));++date.month;}
    date.day=day+1;return date;
}
inline void daily_run_seed(uint32_t day,char(&out)[65]){
    const auto date=run_date(day);const char digits[]="0123456789";
    out[0]=digits[date.day/10];out[1]=digits[date.day%10];out[2]='_';out[3]=digits[date.month/10];out[4]=digits[date.month%10];out[5]='_';
    out[6]=digits[date.year/1000%10];out[7]=digits[date.year/100%10];out[8]=digits[date.year/10%10];out[9]=digits[date.year%10];
    out[10]='_';out[11]='1';out[12]='P';out[13]=0;
}
inline void prepare_daily_run(uint32_t day,Character previous,RunOptions&options,Character&character,uint8_t&ascension,char(&seed)[65]){
    daily_run_seed(day,seed);reference::Random rng;rng.reset(reference::hash64(seed,10));
    reference::Random character_rng,ascension_rng,modifier_rng;
    character_rng.reset(rng.next());ascension_rng.reset(rng.next());modifier_rng.reset(rng.next());
    character=RunModeCharacters[character_rng.below(CharacterKinds)];ascension=uint8_t(ascension_rng.range(0,11));
    options={};options.mode=RunMode::Daily;options.daily_day=day;pick_run_modifiers(modifier_rng,previous,options);
}
} // namespace spire
