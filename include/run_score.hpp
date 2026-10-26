#pragma once
#include "ui_progress.hpp"
#include "world_run.hpp"
#include "world_card_factory.hpp"
namespace spire {
struct UiRunScore {
    uint32_t floor_points=0,gold_points=0,total=0;
    unsigned elites=0,bosses=0;bool complete=false;
};
inline bool ui_run_facts_complete(const UiRunHistory&r){return r.serial&&!r.partial&&r.facts.version&&r.facts.complete;}
inline unsigned ui_run_elites(const UiRunHistory&r){return r.facts.elite_rooms-unsigned(r.facts.elite_rooms&&r.facts.last_room==unsigned(WorldRoom::Elite));}
inline unsigned ui_run_bosses(const UiRunHistory&r){return r.facts.boss_rooms-unsigned(r.facts.boss_rooms&&r.outcome!=1&&r.facts.last_room==unsigned(WorldRoom::Boss));}
inline UiRunScore ui_run_score(const UiRunHistory&r,bool victorious=false){
    UiRunScore score;score.complete=ui_run_facts_complete(r);
    for(unsigned i=0;i<r.floor_count;++i)score.floor_points+=10u*(r.floors[i].act+1u);
    score.gold_points=r.gold_gained/100u;
    // ScoreUtility excludes the final Elite room unconditionally. A final
    // Boss is excluded only for a loss. Preserve this pinned-build behavior.
    score.elites=ui_run_elites(r);score.bosses=victorious?r.facts.boss_rooms:ui_run_bosses(r);
    const uint32_t base=score.floor_points+score.gold_points+50u*score.elites+100u*score.bosses;
    // Original casts the binary64 multiplication to int. Rational arithmetic
    // is observably different at a few integer boundaries (e.g. 45 * 1.4).
    using reference::math::Real;
    const auto multiplier=Real::integer(1)+Real::integer(r.ascension)*Real::raw(UINT64_C(0x3fb999999999999a));
    score.total=uint32_t(world_float_truncate(Real::integer(int32_t(base))*multiplier));return score;
}
// BadgePool order, with the four multiplayer-only models removed.
enum class UiRunBadge:uint8_t {
    Combo,Curses,DoubleSnecko,Elite,Famished,Glutton,Highlander,Honed,BigDeck,
    ILikeShiny,KaChing,MoneyMoney,MysteryMachine,Perfect,Restful,Restless,Speedy,Tablet,TinyDeck,Count
};
constexpr unsigned UiRunBadgeCount=unsigned(UiRunBadge::Count);
inline unsigned ui_badge_tier(unsigned n,unsigned bronze,unsigned silver,unsigned gold){return n>=gold?3:n>=silver?2:n>=bronze?1:0;}
inline unsigned ui_run_badge_rarity(const UiRunHistory&r,UiRunBadge badge){
    const bool won=r.outcome==1,known=r.facts.version&&r.facts.complete&&!r.partial;
    const unsigned count=r.deck_count,starting=r.character<CharacterKinds?unsigned(character_definitions[r.character].starting_hp):0;
    const auto has_relic=[&](RelicId id){for(unsigned i=0;i<r.relic_count;++i)if(r.relics[i]==unsigned(id))return true;return false;};
    switch(badge){
    case UiRunBadge::Combo:return r.facts.combo?1:0;
    case UiRunBadge::Curses:{if(!won)return 0;unsigned n=0;for(unsigned i=0;i<count;++i)n+=definitions[r.deck[i].card.kind].type==CardType::Curse;return n>=5;}
    case UiRunBadge::DoubleSnecko:return has_relic(WorldRelicSneckoEye)&&has_relic(WorldRelicFakeSneckoEye);
    case UiRunBadge::Elite:{unsigned elites=r.facts.elite_rooms;if(elites&&r.facts.last_room==unsigned(WorldRoom::Elite))--elites;return known?ui_badge_tier(elites,3,6,9):0;}
    case UiRunBadge::Famished:return won&&r.max_hp<(starting+1u)/2u;
    case UiRunBadge::Glutton:return won&&r.max_hp>starting?ui_badge_tier(r.max_hp-starting,15,30,50):0;
    case UiRunBadge::Highlander:case UiRunBadge::Honed:{
        if(!won)return 0;bool distinct=true,five=false;uint8_t copies[CardKinds]{};
        for(unsigned i=0;i<count;++i){const auto kind=r.deck[i].card.kind;if(card_rarity(kind)==1)continue;
            if(copies[kind]<5)++copies[kind];if(copies[kind]>1)distinct=false;if(copies[kind]==5)five=true;}
        return badge==UiRunBadge::Highlander?distinct:five;
    }
    case UiRunBadge::BigDeck:return won?ui_badge_tier(count,40,60,100):0;
    case UiRunBadge::ILikeShiny:return r.relic_count>=25;
    case UiRunBadge::KaChing:return r.facts.shop_spent>=1000;
    case UiRunBadge::MoneyMoney:return won?ui_badge_tier(r.gold,200,400,600):0;
    case UiRunBadge::MysteryMachine:{if(!known)return 0;unsigned n=0;for(unsigned i=0;i<r.floor_count;++i)n+=r.floors[i].map_type==unsigned(reference::PointType::Unknown);return n>=15;}
    case UiRunBadge::Perfect:return known?ui_badge_tier(r.facts.perfect_bosses,1,2,3):0;
    case UiRunBadge::Restful:case UiRunBadge::Restless:{
        if(!won||!known)return 0;unsigned rooms=0,heals=0;
        for(unsigned i=0;i<r.floor_count;++i)if(r.floors[i].room==unsigned(WorldRoom::Rest)){++rooms;heals+=(r.floors[i].rest_choices&1u)!=0;}
        return rooms&&(badge==UiRunBadge::Restful?heals==rooms:!heals);
    }
    case UiRunBadge::Speedy:return won&&!r.partial?(r.seconds<=1800?3:r.seconds<=2400?2:r.seconds<=3000?1:0):0;
    case UiRunBadge::Tablet:return won&&r.max_hp==1;
    case UiRunBadge::TinyDeck:return won?(count<=5?3:count<=10?2:count<=20?1:0):0;
    default:return 0;
    }
}
inline uint32_t ui_daily_score(const UiRunHistory&r){
    if(!ui_run_facts_complete(r))return 0;unsigned badges=0;
    for(unsigned i=0;i<UiRunBadgeCount;++i)badges+=ui_run_badge_rarity(r,UiRunBadge(i))!=0;
    return (r.outcome==1?200000000u:100000000u)+(r.floor>99?99u:r.floor)*1000000u+badges*10000u+(9999u-(r.seconds>9999u?9999u:r.seconds));
}
} // namespace spire
