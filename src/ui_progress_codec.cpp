#include "ui_progress.hpp"
#include "ui7_assets.hpp"
namespace spire {
namespace {
struct Encoder {
    static constexpr bool reading=false;
    uint8_t*data;size_t size,at=0;bool good=true;
    template<class T>void integer(T&v){
        if(at>size||sizeof(T)>size-at){good=false;return;}
        const uint64_t value=uint64_t(v);
        for(unsigned i=0;i<sizeof(T);++i)data[at++]=uint8_t(value>>(i*8));
    }
};
struct Decoder {
    static constexpr bool reading=true;
    const uint8_t*data;size_t size,at=0;bool good=true;
    template<class T>void integer(T&v){
        if(at>size||sizeof(T)>size-at){good=false;return;}
        uint64_t value=0;for(unsigned i=0;i<sizeof(T);++i)value|=uint64_t(data[at++])<<(i*8);v=T(value);
    }
};
template<class Codec>void card(Codec&c,EventDeckSnapshot&v){
    c.integer(v.card.kind);c.integer(v.card.upgraded);
    uint8_t enchantment=uint8_t(v.facts.enchantment);c.integer(enchantment);
    if constexpr(Codec::reading)v.facts.enchantment=EventEnchantment(enchantment);
    c.integer(v.facts.science_type);c.integer(v.facts.rider);c.integer(v.facts.enchantment_amount);
    c.integer(v.facts.energy_offset);c.integer(v.facts.progress);c.integer(v.genetic);c.integer(v.scythe);
    if(v.card.kind>=CardKinds||v.card.upgraded>1||enchantment>=EventEnchantmentKinds)c.good=false;
}
template<class Codec>void run(Codec&c,UiRunHistory&v,unsigned version){
    c.integer(v.serial);if(!v.serial)return;
    c.integer(v.run_seed);c.integer(v.seed);c.integer(v.started_at);c.integer(v.seconds);c.integer(v.gold_gained);
    if(version>=3){
        c.integer(v.modifiers);c.integer(v.daily_day);c.integer(v.mode);c.integer(v.modifier_order);
        RunOptions options{};options.mode=RunMode(v.mode);options.modifiers=v.modifiers;options.daily_day=v.daily_day;options.setup_flags=v.modifier_order;
        if((v.modifier_order&~2u)||!valid_run_options(options)){c.good=false;return;}
    }
    c.integer(v.floor);c.integer(v.hp);c.integer(v.max_hp);c.integer(v.gold);c.integer(v.deck_count);c.integer(v.relic_count);
    c.integer(v.last_battle);c.integer(v.finished_battle);c.integer(v.encounter);c.integer(v.last_enemy_identity);
    c.integer(v.character);c.integer(v.ascension);c.integer(v.outcome);c.integer(v.start_region);
    for(auto&r:v.regions)c.integer(r);
    c.integer(v.floor_count);c.integer(v.partial);c.integer(v.event);c.integer(v.abandoned);c.integer(v.potion_count);
    if(v.character>=CharacterKinds||v.ascension>10||v.outcome>3||v.deck_count>MaxDeck||v.relic_count>WorldRelicCapacity||v.floor_count>UiHistoryFloors||v.potion_count>MaxPotionSlots){c.good=false;return;}
    for(auto&potion:v.potions){c.integer(potion);if(potion>PotionKinds)c.good=false;}
    for(auto&ch:v.seed_text)c.integer(ch);
    if(v.seed_text[64])c.good=false;
    for(auto&seen:v.battle_seen)c.integer(seen);
    for(unsigned i=0;i<v.deck_count&&c.good;++i)card(c,v.deck[i]);
    for(unsigned i=0;i<v.relic_count&&c.good;++i){c.integer(v.relics[i]);if(v.relics[i]>=WorldRelicKinds)c.good=false;}
    for(unsigned i=0;i<v.floor_count&&c.good;++i){auto&f=v.floors[i];
        c.integer(f.entered_seconds);c.integer(f.seconds);c.integer(f.gold_gained);c.integer(f.floor);c.integer(f.encounter);
        c.integer(f.hp);c.integer(f.max_hp);c.integer(f.gold);c.integer(f.turns);
        c.integer(f.act);c.integer(f.region);c.integer(f.point);c.integer(f.room);c.integer(f.event);
        if(f.act>=3||f.region>3)c.good=false;
    }
}
static const UiRunHistory* journal_run(const UiProgress&v,uint32_t serial){
    if(!serial)return nullptr;if(v.current.serial==serial)return &v.current;
    for(unsigned i=0;i<v.count;++i)if(v.history[i].serial==serial)return &v.history[i];return nullptr;
}
static bool journal_entry_valid(const UiProgress&v,const UiHistoryJournalEntry&e){
    if(e.owner>=UiJournalOwners||e.floor>=UiHistoryFloors||e.kind==RunHistoryEvent::None||e.kind>=RunHistoryEvent::Count)return false;
    const auto&owner=v.journal.owners[e.owner];const auto*r=journal_run(v,owner.serial);
    if(!r||e.floor>=r->floor_count||!owner.observed(e.floor))return false;
    const auto valid_card=[](unsigned token){return card_from_token(int(token)).kind<CardKinds;};
    using K=RunHistoryEvent;
    switch(e.kind){
    case K::CardTransformed:return (e.flags==0||e.flags==1||e.flags==4)&&valid_card(e.model)&&valid_card(e.detail);
    case K::CardEnchanted:return valid_card(e.model)&&e.flags>0&&e.flags<EventEnchantmentKinds&&e.detail<=9999;
    case K::CardObtained:case K::CardRemoved:case K::CardUpgraded:case K::CardDowngraded:
    case K::CardPicked:case K::CardSkipped:case K::CardBought:return !e.flags&&valid_card(e.model);
    case K::AncientSkipped:return !e.flags&&(e.model<WorldRelicKinds||e.model==NoWorldRelic);
    case K::RelicObtained:case K::RelicRemoved:case K::RelicSkipped:case K::RelicBought:case K::AncientPicked:return !e.flags&&e.model<WorldRelicKinds;
    case K::PotionObtained:case K::PotionUsed:case K::PotionDiscarded:case K::PotionSkipped:case K::PotionBought:return !e.flags&&e.model>0&&e.model<=PotionKinds;
    case K::EventChoice:
        if(e.model>=Ui7StringCount||e.flags>unsigned(HistoryChoiceVariable::PotionRarity))return false;
        if(e.flags==unsigned(HistoryChoiceVariable::Relic))return e.detail<WorldRelicKinds;
        if(e.flags==unsigned(HistoryChoiceVariable::Potion))return e.detail>0&&e.detail<=PotionKinds;
        if(e.flags==unsigned(HistoryChoiceVariable::PotionRarity))return e.detail>=1&&e.detail<=5;
        return true;
    case K::ModifierChoice:return !e.flags&&e.model<RunModifierKinds;
    case K::RestChoice:return !e.flags&&e.model<8;
    case K::QuestCompleted:return !e.flags&&e.model<CardKinds;
    case K::FurCoat:return !e.flags&&e.model==WorldRelicFurCoat;
    case K::CombatStarted:return e.model>0&&e.model<EncounterKinds&&e.detail>0&&e.flags>=1&&e.flags<=3;
    default:return !e.flags;
    }
}
template<class Codec>void journal(Codec&c,UiProgress&v){
    auto&j=v.journal;
    for(unsigned i=0;i<UiJournalOwners&&c.good;++i){auto&o=j.owners[i];c.integer(o.serial);if(!o.serial)continue;
        const auto*r=journal_run(v,o.serial);if(!r){c.good=false;return;}
        for(unsigned k=0;k<i;++k)if(j.owners[k].serial==o.serial){c.good=false;return;}
        for(auto&bits:o.known)c.integer(bits);for(auto&bits:o.partial)c.integer(bits);
        for(unsigned f=r->floor_count;f<UiJournalFloors;++f)if((o.known[f/32]|o.partial[f/32])&(uint32_t(1)<<(f%32))){c.good=false;return;}
    }
    c.integer(j.count);if(j.count>UiJournalEntries||j.first>=UiJournalEntries){c.good=false;return;}
    if constexpr(Codec::reading)j.first=0;
    for(unsigned i=0;i<j.count&&c.good;++i){auto&e=j.at(i);c.integer(e.model);c.integer(e.detail);c.integer(e.owner);c.integer(e.floor);
        uint8_t kind=uint8_t(e.kind);c.integer(kind);if constexpr(Codec::reading)e.kind=RunHistoryEvent(kind);c.integer(e.flags);
        if(!journal_entry_valid(v,e))c.good=false;
    }
}
template<class Codec>void progress(Codec&c,UiProgress&v,unsigned version){
    c.integer(v.next_serial);c.integer(v.count);c.integer(v.next_history);c.integer(v.discovered_acts);
    c.integer(v.characters_started);c.integer(v.current_streak);c.integer(v.best_streak);
    if(version>=2)c.integer(v.last_saved_at);
    if(!v.next_serial||v.count>UiHistoryCapacity||v.next_history>=UiHistoryCapacity){c.good=false;return;}
    for(auto&b:v.discovered_cards)c.integer(b);for(auto&b:v.discovered_relics)c.integer(b);
    for(auto&b:v.discovered_potions)c.integer(b);for(auto&b:v.discovered_events)c.integer(b);
    if(version>=2)for(auto&wins:v.card_wins)c.integer(wins);
    for(auto&s:v.characters){c.integer(s.wins);c.integer(s.losses);c.integer(s.current_streak);c.integer(s.best_streak);
        c.integer(s.play_seconds);c.integer(s.fastest_win_seconds);c.integer(s.gold_gained);c.integer(s.floors_climbed);c.integer(s.highest_ascension);}
    for(auto&model:v.enemies)for(auto&s:model){c.integer(s.seen);c.integer(s.wins);c.integer(s.losses);}
    for(auto&model:v.encounters)for(auto&s:model){c.integer(s.seen);c.integer(s.wins);c.integer(s.losses);}
    run(c,v.current,version);
    for(unsigned i=0;i<v.count&&c.good;++i)run(c,v.history[i],version);
    if(version>=4){auto&d=v.map_drawing;c.integer(d.seed);c.integer(d.serial);c.integer(d.act);c.integer(d.count);
        if(d.act>=3||d.count>MapDrawingCapacity){c.good=false;return;}
        for(unsigned i=0;i<d.count&&c.good;++i){auto&p=d.points[i];c.integer(p.x);c.integer(p.y_flags);
            if(p.x>1920||p.y()>3240||(!i&&!p.start())||
               (i&&!p.start()&&p.erase()!=d.points[i-1].erase()))c.good=false;
        }
    }
    if(version>=5)c.integer(v.test_subject_kills);
    if(version>=6){
        const auto facts=[&](UiRunHistory&r){
            if(!r.serial)return;auto&f=r.facts;c.integer(f.version);
            if(f.version>1){c.good=false;return;}if(!f.version)return;
            c.integer(f.complete);c.integer(f.shop_spent);c.integer(f.elite_rooms);c.integer(f.boss_rooms);c.integer(f.perfect_bosses);
            c.integer(f.last_room);c.integer(f.last_battle_room);c.integer(f.combo);
            if(f.complete>1||f.combo>1||f.last_room>8||f.last_battle_room>8||f.perfect_bosses>f.boss_rooms){c.good=false;return;}
            for(unsigned i=0;i<r.floor_count&&c.good;++i){auto&p=r.floors[i];c.integer(p.map_type);c.integer(p.rest_choices);c.integer(p.facts);
                if(p.map_type>8)c.good=false;}
        };
        facts(v.current);for(unsigned i=0;i<v.count&&c.good;++i)facts(v.history[i]);
        auto&d=v.current_discoveries;const unsigned kinds[]={CardKinds,WorldRelicKinds,PotionKinds,EnemyKinds};
        for(unsigned category=0;category<4&&c.good;++category){c.integer(d.count[category]);if(d.count[category]>kinds[category]){c.good=false;return;}
            const unsigned n=d.count[category]<10?d.count[category]:10;
            for(unsigned i=0;i<n&&c.good;++i){c.integer(d.first[category][i]);if(d.first[category][i]>=kinds[category])c.good=false;
                for(unsigned j=0;j<i;++j)if(d.first[category][j]==d.first[category][i])c.good=false;}
        }
        c.integer(v.architect_damage);
    }
    if(version>=7)journal(c,v);
    if(version>=8){c.integer(v.tutorial_flags);if(!ftue_flags_valid(v.tutorial_flags))c.good=false;}
}
}
void clear_ui_progress(UiProgress&out){auto*bytes=reinterpret_cast<uint8_t*>(&out);for(size_t i=0;i<sizeof out;++i)bytes[i]=0;out.next_serial=1;}
size_t encode_ui_progress(const UiProgress&value,uint8_t*out,size_t capacity){
    if(!out||capacity<8)return 0;
    unsigned version=value.current.mode?3:2;
    for(unsigned i=0;i<value.count&&i<UiHistoryCapacity;++i)if(value.history[i].mode)version=3;
    if(value.map_drawing.count)version=4;
    if(value.test_subject_kills)version=5;
    if(value.current.facts.version||value.architect_damage)version=6;
    for(unsigned i=0;i<value.count&&i<UiHistoryCapacity;++i)if(value.history[i].facts.version)version=6;
    const auto*d=reinterpret_cast<const uint8_t*>(&value.current_discoveries);for(unsigned i=0;i<sizeof value.current_discoveries;++i)if(d[i])version=6;
    for(const auto&owner:value.journal.owners)if(owner.serial){version=7;break;}
    if(value.journal.count)version=7;
    if(value.tutorial_flags)version=8;
    out[0]='N';out[1]='S';out[2]='H';out[3]=char('0'+version);for(unsigned i=4;i<8;++i)out[i]=0;
    // Encoder never changes a field. The shared schema keeps its reader and
    // writer in the same order without depending on C++ padding/endianness.
    Encoder c{out,capacity,8};progress(c,const_cast<UiProgress&>(value),version);
    if(!c.good||c.at>UiProgressCapacity)return 0;
    for(unsigned i=0;i<4;++i)out[4+i]=uint8_t(c.at>>(i*8));return c.at;
}
bool decode_ui_progress(const uint8_t*data,size_t size,UiProgress&out){
    if(!data||size<8||size>UiProgressCapacity||data[0]!='N'||data[1]!='S'||data[2]!='H'||data[3]<'1'||data[3]>'8')return false;
    uint32_t stored=0;for(unsigned i=0;i<4;++i)stored|=uint32_t(data[4+i])<<(i*8);if(stored!=size)return false;
    clear_ui_progress(out);Decoder c{data,size,8};progress(c,out,unsigned(data[3]-'0'));return c.good&&c.at==size;
}
bool decode_ui_progress_summary(const uint8_t*data,size_t size,UiProgressSummary&out){
    out={};
    if(!data||size<8||size>UiProgressCapacity||data[0]!='N'||data[1]!='S'||data[2]!='H'||data[3]<'1'||data[3]>'8')return false;
    uint32_t stored=0;for(unsigned i=0;i<4;++i)stored|=uint32_t(data[4+i])<<(i*8);if(stored!=size)return false;
    Decoder c{data,size,8};uint32_t serial=0,ignored=0;uint8_t count=0,next=0,byte=0;
    c.integer(serial);c.integer(count);c.integer(next);c.integer(byte);c.integer(byte);c.integer(ignored);c.integer(ignored);
    if(!c.good||!serial||count>UiHistoryCapacity||next>=UiHistoryCapacity)return false;
    const bool version2=data[3]>='2';if(version2)c.integer(out.last_saved_at);
    const size_t fields=(CardKinds+7)/8+(WorldRelicKinds+7)/8+(PotionKinds+7)/8+(OriginalEventKinds+7)/8+(version2?CardKinds*sizeof(uint32_t):0);
    if(c.at>size||fields>size-c.at)return false;c.at+=fields;
    for(unsigned character=0;character<CharacterKinds;++character){uint32_t seconds=0;
        for(unsigned field=0;field<4;++field)c.integer(ignored);c.integer(seconds);
        for(unsigned field=0;field<3;++field)c.integer(ignored);c.integer(byte);
        if(!c.good||byte>10)return false;
        out.play_seconds=seconds>UINT32_MAX-out.play_seconds?UINT32_MAX:out.play_seconds+seconds;
    }
    // This intentionally does not validate or allocate the history tail.
    // Selecting the profile still uses the complete schema decoder.
    out.exists=true;return true;
}
} // namespace spire
