#include "core.hpp"
#include "inplace_reset.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#endif

using namespace spire;
static unsigned checks=0;
static void require(bool value,const char*message){
    ++checks;
    if(!value){std::fprintf(stderr,"in-place reset: %s\n",message);std::exit(1);}
}

// All large test fixtures deliberately live outside the worker stack. The
// regression exercises production call frames, not copies in the test harness.
static State clean,dirty;
static SaveData clean_bytes,dirty_bytes;
static constexpr uint64_t guard=0x71d39a5264feb8c0ULL;
struct GuardedState { uint64_t before=guard;State value{};uint64_t after=guard; };
static GuardedState guarded;

struct ChildDefaults { int sentinel=-17;bool enabled=true;uint8_t bytes[5]{}; };
struct MixedDefaults { unsigned zero;ChildDefaults child[3]{};int last=29; };
alignas(MixedDefaults) static unsigned char raw_defaults[sizeof(MixedDefaults)];

static void default_semantics(){
    std::memset(raw_defaults,0xa5,sizeof raw_defaults);
    auto*created=value_construct_at<MixedDefaults>(raw_defaults);
    require(static_cast<void*>(created)==raw_defaults,"construction returns the original storage");
    require(created->zero==0&&created->last==29,"unspecified scalars zero; explicit defaults survive");
    for(auto&child:created->child){
        require(child.sentinel==-17&&child.enabled,"nested nonzero defaults survive");
        for(auto byte:child.bytes)require(byte==0,"nested arrays value initialize");
    }
    MixedDefaults&alias=*created;int&member_alias=created->child[2].sentinel;
    created->zero=99;created->child[2].sentinel=43;created->last=-3;
    reset_in_place(*created);
    require(&alias==created&&alias.zero==0&&member_alias==-17&&alias.last==29,
            "same-type replacement preserves complete-object and member references");

    std::memset(&guarded.value,0xa5,sizeof guarded.value);
    reset_in_place(guarded.value);
    const auto&s=guarded.value;
    require(guarded.before==guard&&guarded.after==guard,"full State reset stays within object storage");
    require(s.hp==0&&s.seed==0&&s.potion_slots==LegacyPotionSlots,"State scalars preserve value defaults");
    require(s.expansion.target==-1&&s.expansion.last_drawn==-1,"expansion sentinels survive");
    for(const auto&frame:s.expansion.frames)
        require(frame.playing==-1&&frame.target==-1&&frame.remaining==0,"all frame sentinels survive");
    require(s.original_events.reward_current==NoOriginalEventReward&&s.original_events.crystal_tool==1,
            "event reward and crystal defaults survive");
    require(s.enemy_world.actor==255&&s.enemy_world.next_creature_id==1&&s.enemy_world.choice_enemy==255,
            "enemy-world identity defaults survive");
    for(const auto&model:s.enemy_world.models)
        require(model.owner==255&&!model.rng.good&&!model.flags,"nested enemy identity defaults survive");
    require(s.world.current_point==reference::NoPoint&&s.world.shop_pending_item==255,
            "map and shop sentinels survive");
    require(s.world.unknown_monster_bits==0x3dcccccdU&&s.world.unknown_elite_bits==0xbf800000U&&
            s.world.unknown_shop_bits==0x3cf5c28fU&&s.world.unknown_treasure_bits==0x3ca3d70aU&&
            s.world.potion_drop_bits==0x3ecccccdU,"original binary32 probability defaults survive");
}

static EventRunFacts history(){
    EventRunFacts facts{};facts.wongo_total=4123;facts.wongo_earned=71;facts.wins=12;
    for(unsigned i=0;i<CharacterKinds;i++)facts.character_wins[i]=i+2;
    facts.freed_repy=facts.completion_recorded=1;return facts;
}
static void same_saved_state(const char*message){
    require(validate(clean)&&validate(dirty),"both independently prepared states are valid");
    require(save(clean,clean_bytes)&&save(dirty,dirty_bytes),"both states encode successfully");
    require(clean_bytes.size==dirty_bytes.size&&!std::memcmp(clean_bytes.bytes,dirty_bytes.bytes,clean_bytes.size),message);
    require(load(dirty,clean_bytes.bytes,clean_bytes.size),"state decodes on the bounded production stack");
    require(save(dirty,dirty_bytes)&&clean_bytes.size==dirty_bytes.size&&
            !std::memcmp(clean_bytes.bytes,dirty_bytes.bytes,clean_bytes.size),
            "bounded-stack save/load preserves exact canonical bytes");
}
static void new_run_pair(Character character){
    // Compare canonical serialized state, not unspecified C++ padding bytes.
    reset_in_place(clean);clean.event_run=history();
    std::memset(&dirty,0xa5,sizeof dirty);dirty.event_run=history();
    new_run(clean,0x4e535052,false,character);new_run(dirty,0x4e535052,false,character);
    require(dirty.character==character&&dirty.hp==character_definitions[unsigned(character)].starting_hp,
            "new run creates the requested character");
    require(dirty.event_run.wongo_total==4123&&dirty.event_run.wongo_earned==71&&dirty.event_run.wins==12&&
            dirty.event_run.character_wins[4]==6&&dirty.event_run.freed_repy&&dirty.event_run.completion_recorded,
            "new_run preserves all previous run facts before world setup");
    same_saved_state("new run clears stale state exactly across every serialized member");
}
static void production_resets(){
    for(unsigned c=0;c<CharacterKinds;c++){
        const auto character=Character(c);new_run_pair(character);

        // Original events have another roughly 40 KiB aggregate, including
        // nonzero defaults. Stale entries must disappear before event dispatch.
        std::memset(&dirty.original_events,0xa5,sizeof dirty.original_events);
        original_event_new_run(clean);original_event_new_run(dirty);
        require(dirty.event_run.wongo_total==4123&&dirty.event_run.wins==12&&
                dirty.event_run.character_wins[4]==6&&!dirty.event_run.wongo_earned&&
                !dirty.event_run.freed_repy&&!dirty.event_run.completion_recorded,
                "event new-run reset preserves lifetime history and clears run-local facts");
        same_saved_state("event reset clears every serialized event member");
        require(original_event_begin(clean,OriginalEventId::AbyssalBaths,0x4e535052,0)&&
                original_event_begin(dirty,OriginalEventId::AbyssalBaths,0x4e535052,0),
                "ordinary event enters on the bounded stack");
        same_saved_state("event entry retains deterministic event RNG and continuation");

        new_run_pair(character);
        dirty.expansion.target=7;dirty.expansion.last_drawn=82;dirty.expansion.frames[MaxAutoFrames-1].playing=41;
        dirty.expansion.powers[0]=19;dirty.ironclad.powers[0]=13;dirty.regent.powers[0]=17;
        dirty.enemy_world.actor=3;dirty.enemy_world.next_creature_id=99;dirty.enemy_world.models[7].owner=2;
        enter_combat(clean);enter_combat(dirty);
        same_saved_state("combat entry clears aggregate state without stale character/enemy facts");
        require(dirty.expansion.frames[MaxAutoFrames-1].playing==-1&&dirty.enemy_world.actor==255&&
                dirty.enemy_world.next_creature_id==1,"combat reset preserves inactive identity sentinels");
    }
}

#if defined(__unix__) || defined(__APPLE__)
static void* bounded_worker(void*){production_resets();return nullptr;}
#endif
int main(){
    default_semantics();
#if defined(__unix__) || defined(__APPLE__)
    pthread_attr_t attributes;pthread_t worker;
    require(pthread_attr_init(&attributes)==0,"create bounded-stack attributes");
    require(pthread_attr_setstacksize(&attributes,64*1024)==0,"request a 64 KiB production stack");
    const int started=pthread_create(&worker,&attributes,bounded_worker,nullptr);
    if(started){pthread_attr_destroy(&attributes);require(false,"start bounded-stack worker");}
    // Only the worker touches the shared assertion counter until join returns.
    const int attributes_released=pthread_attr_destroy(&attributes);
    const int joined=pthread_join(worker,nullptr);
    require(started==0,"start bounded-stack worker");
    require(attributes_released==0,"release worker attributes");
    require(joined==0,"all production reset paths finish on 64 KiB stack");
    std::printf("in-place reset: %u checks; production paths passed on a 64 KiB stack\n",checks);
#else
    production_resets();
    std::printf("in-place reset: %u checks; stack bound unavailable on this host\n",checks);
#endif
    return 0;
}
