#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)

// Test callers also keep complete states and save buffers off the stack, so
// an ARM/limited-stack harness can reuse these operations without hiding the
// production fix behind a second oversized local object.
static State source{},target{};
static SaveData encoded{},again{},bad{};
static unsigned char source_before[sizeof(State)],target_before[sizeof(State)];

static uint32_t word(const uint8_t*bytes,size_t at){
    return uint32_t(bytes[at])|(uint32_t(bytes[at+1])<<8)|
           (uint32_t(bytes[at+2])<<16)|(uint32_t(bytes[at+3])<<24);
}
static void put_word(uint8_t*bytes,size_t at,uint32_t value){
    for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(value>>(8*i));
}
static void same_encoded(const SaveData&a,const SaveData&b){
    CHECK(a.size==b.size);CHECK(!std::memcmp(a.bytes,b.bytes,a.size));
}
static void save_immutable(const State&s,SaveData&out){
    std::memcpy(source_before,&s,sizeof(s));CHECK(save(s,out));
    CHECK(!std::memcmp(source_before,&s,sizeof(s)));
}
static void reject(const uint8_t*bytes,size_t len){
    std::memcpy(target_before,&target,sizeof(target));
    CHECK(!load(target,bytes,len));
    CHECK(!std::memcmp(target_before,&target,sizeof(target)));
}
static void current_states(){
    for(unsigned character=0;character<CharacterKinds;++character){
        for(unsigned practice=0;practice<2;++practice){
            new_run(source,0x41cef1eaU+character*19+practice,practice,Character(character));
            CHECK(validate(source));save_immutable(source,encoded);
            CHECK(word(encoded.bytes,0)==0x3156534eU);
            CHECK(word(encoded.bytes,4)==SaveVersion);
            CHECK(word(encoded.bytes,8)==0x41cef1eaU);
            save_immutable(source,again);same_encoded(encoded,again);

            // Replace an unrelated live state only after complete validation.
            new_run(target,character+311,false,Character((character+1)%CharacterKinds));
            CHECK(load(target,encoded.bytes,encoded.size));CHECK(validate(target));
            save_immutable(target,again);same_encoded(encoded,again);
            CHECK(target.character==source.character&&target.seed==source.seed);

            reject(nullptr,encoded.size);reject(encoded.bytes,0);
            reject(encoded.bytes,12);reject(encoded.bytes,encoded.size-1);
            reject(encoded.bytes,sizeof(SaveData::bytes)+4);
            reject(encoded.bytes,encoded.size-4);

            bad=encoded;put_word(bad.bytes,0,0);reject(bad.bytes,bad.size);
            bad=encoded;put_word(bad.bytes,4,SaveVersion+1);reject(bad.bytes,bad.size);
            bad=encoded;put_word(bad.bytes,8,0);reject(bad.bytes,bad.size);
            bad=encoded;put_word(bad.bytes,12,255);reject(bad.bytes,bad.size);
            // Prefix widths are frozen; deck_count immediately follows the
            // two power-order entries and four RNG/seed/revision words.
            CHECK(word(encoded.bytes,88)==source.deck_count);
            bad=encoded;put_word(bad.bytes,88,MaxDeck+1);reject(bad.bytes,bad.size);
            // The historical footer is consumed for format compatibility,
            // not hashed. New saves write zero; old nonzero footers still load.
            CHECK(word(encoded.bytes,encoded.size-4)==0);
            bad=encoded;put_word(bad.bytes,bad.size-4,0x81234567u);
            CHECK(load(target,bad.bytes,bad.size));save_immutable(target,again);same_encoded(encoded,again);

            // Failure must release the scratch lease; subsequent saves and
            // loads must still produce the exact original v15 payload.
            CHECK(load(target,encoded.bytes,encoded.size));
            save_immutable(target,again);same_encoded(encoded,again);

            source.phase=Phase::Error;bad=encoded;
            std::memcpy(source_before,&source,sizeof(source));
            CHECK(!save(source,bad));same_encoded(encoded,bad);
            CHECK(!std::memcmp(source_before,&source,sizeof(source)));
            source.phase=target.phase;
        }
    }
}
static std::vector<uint8_t> read_hex(const std::string&path){
    FILE*f=std::fopen(path.c_str(),"r");CHECK(f);
    std::vector<uint8_t> bytes;unsigned value;
    while(std::fscanf(f,"%2x",&value)==1)bytes.push_back(uint8_t(value));
    CHECK(!std::ferror(f));CHECK(!std::fclose(f));return bytes;
}
static void legacy_defaults(){
    // These nonzero defaults were never present in historical payloads.
    // Resetting shared scratch to zero or keeping its previous v15 suffix
    // would change them, even if the early legacy fields decoded correctly.
    CHECK(target.potion_slots==LegacyPotionSlots);
    CHECK(!target.world.active&&target.world.current_point==reference::NoPoint);
    CHECK(target.world.unknown_monster_bits==0x3dcccccdU);
    CHECK(target.world.unknown_elite_bits==0xbf800000U);
    CHECK(target.world.unknown_shop_bits==0x3cf5c28fU);
    CHECK(target.world.unknown_treasure_bits==0x3ca3d70aU);
    CHECK(target.world.potion_drop_bits==0x3ecccccdU);
    CHECK(target.world.reward_relic==NoWorldRelic&&target.world.treasure_relic==NoWorldRelic);
    CHECK(target.world.shop_pending_item==255);
    for(auto id:target.world.shop_relics)CHECK(id==NoWorldRelic);
    CHECK(target.original_events.reward_current==NoOriginalEventReward);
    CHECK(target.original_events.crystal_tool==1&&!target.original_events.active);
    CHECK(target.enemy_world.actor==255&&target.enemy_world.next_creature_id==1);
    CHECK(target.enemy_world.choice_enemy==255&&!target.enemy_world.active);
    for(const auto&model:target.enemy_world.models)CHECK(model.owner==255);
}
static void historical_reuse(const char*directory){
    const char*fixtures[]={"v3-suspended.hex","v4-shop.hex","v5-practice.hex",
        "v6-silent-choice.hex","v7-silent-choice.hex","v8-relic-choice.hex",
        "v9-grove-choice.hex","v11-entropy-boulder.hex","v12-entropy-choice.hex",
        "v13-nightmare-havoc-choice.hex"};
    for(const char*name:fixtures){
        const auto bytes=read_hex(std::string(directory)+"/"+name);
        CHECK(bytes.size()>16&&word(bytes.data(),4)<15);
        // Fill scratch with a modern character state, then leave another
        // decoded v15 state behind a deliberately invalid game phase.
        new_run(source,917,true,Character::Necrobinder);save_immutable(source,encoded);
        bad=encoded;put_word(bad.bytes,12,255);reject(bad.bytes,bad.size);
        CHECK(load(target,bytes.data(),bytes.size()));CHECK(validate(target));legacy_defaults();
        save_immutable(target,encoded);
        CHECK(load(target,encoded.bytes,encoded.size));
        save_immutable(target,again);same_encoded(encoded,again);
        // Alternate versions again after the writer has used the workspace.
        CHECK(load(target,bytes.data(),bytes.size()));legacy_defaults();
        save_immutable(target,again);same_encoded(encoded,again);
    }
}
int main(int argc,char**argv){
    CHECK(argc==2);current_states();historical_reuse(argv[1]);
    std::printf("state codec: %u checks passed; State=%zu bytes, reusable workspace and atomic failures\n",checks,sizeof(State));
}
