#include "ui_progress.hpp"
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
template<class Codec>void run(Codec&c,UiRunHistory&v){
    c.integer(v.serial);if(!v.serial)return;
    c.integer(v.run_seed);c.integer(v.seed);c.integer(v.started_at);c.integer(v.seconds);c.integer(v.gold_gained);
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
template<class Codec>void progress(Codec&c,UiProgress&v){
    c.integer(v.next_serial);c.integer(v.count);c.integer(v.next_history);c.integer(v.discovered_acts);
    c.integer(v.characters_started);c.integer(v.current_streak);c.integer(v.best_streak);
    if(!v.next_serial||v.count>UiHistoryCapacity||v.next_history>=UiHistoryCapacity){c.good=false;return;}
    for(auto&b:v.discovered_cards)c.integer(b);for(auto&b:v.discovered_relics)c.integer(b);
    for(auto&b:v.discovered_potions)c.integer(b);for(auto&b:v.discovered_events)c.integer(b);
    for(auto&s:v.characters){c.integer(s.wins);c.integer(s.losses);c.integer(s.current_streak);c.integer(s.best_streak);
        c.integer(s.play_seconds);c.integer(s.fastest_win_seconds);c.integer(s.gold_gained);c.integer(s.floors_climbed);c.integer(s.highest_ascension);}
    for(auto&model:v.enemies)for(auto&s:model){c.integer(s.seen);c.integer(s.wins);c.integer(s.losses);}
    for(auto&model:v.encounters)for(auto&s:model){c.integer(s.seen);c.integer(s.wins);c.integer(s.losses);}
    run(c,v.current);
    for(unsigned i=0;i<v.count&&c.good;++i)run(c,v.history[i]);
}
}
void clear_ui_progress(UiProgress&out){auto*bytes=reinterpret_cast<uint8_t*>(&out);for(size_t i=0;i<sizeof out;++i)bytes[i]=0;out.next_serial=1;}
size_t encode_ui_progress(const UiProgress&value,uint8_t*out,size_t capacity){
    if(!out||capacity<8)return 0;
    out[0]='N';out[1]='S';out[2]='H';out[3]='1';for(unsigned i=4;i<8;++i)out[i]=0;
    // Encoder never changes a field. The shared schema keeps its reader and
    // writer in the same order without depending on C++ padding/endianness.
    Encoder c{out,capacity,8};progress(c,const_cast<UiProgress&>(value));
    if(!c.good||c.at>UiProgressCapacity)return 0;
    for(unsigned i=0;i<4;++i)out[4+i]=uint8_t(c.at>>(i*8));return c.at;
}
bool decode_ui_progress(const uint8_t*data,size_t size,UiProgress&out){
    if(!data||size<8||size>UiProgressCapacity||data[0]!='N'||data[1]!='S'||data[2]!='H'||data[3]!='1')return false;
    uint32_t stored=0;for(unsigned i=0;i<4;++i)stored|=uint32_t(data[4+i])<<(i*8);if(stored!=size)return false;
    clear_ui_progress(out);Decoder c{data,size,8};progress(c,out);return c.good&&c.at==size;
}
} // namespace spire
