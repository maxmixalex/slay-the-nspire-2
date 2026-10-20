#pragma once
#include "core.hpp"
#include "event_assets.hpp"
#include "assets.hpp"
#include "ui_assets.hpp"
#include "ui_actions.hpp"
#include "pointer.hpp"
#include "map_preview.hpp"
#include "resource_source.hpp"
#include "world_enemy_assets.hpp"
namespace spire {
constexpr int ScreenW=320,ScreenH=240,MaxPack=13*1024*1024;
// MaxPack remains the historical in-memory fixture limit. Production hosts
// use bounded range reads and may open larger immutable resource files.
constexpr uint32_t MaxContentFile=256u*1024u*1024u;
// Each appended character/token model adds a title and two descriptions after
// the complete, immutable 0.14 string directory.
constexpr unsigned ContentVersion=14,ContentStrings=WorldStringCount;
struct Picture {const uint8_t* pixels;const uint8_t* alpha;const uint8_t* palette;uint16_t w,h;uint8_t format;};
struct Glyph {const uint8_t* pixels;uint8_t w,h;uint16_t advance;uint16_t encoded_size=0;};
struct Clip {uint32_t first;uint16_t count,frame_ms;int16_t pivot_x,pivot_y;};
class Content {
    const uint8_t* data_{};uint32_t version_{},length_{},images_{},glyphs_{},strings_{},clips_{},cards_{},ui_offset_{},ui_count_{};
    static constexpr uint32_t DirectoryCapacity=2u*1024u*1024u;
    uint8_t directory_[DirectoryCapacity]{};
    const uint8_t* metadata_{};
    ResourceReader reader_{};
    mutable bool io_good_=true;
    const uint8_t* payload(uint32_t offset,uint32_t count,uint8_t* scratch,uint32_t capacity)const;
public:
    bool open(const uint8_t*,size_t);
    bool open(ResourceSource);
    bool healthy()const{return metadata_&&io_good_;}
    ResourceReadStats read_stats()const{return reader_.stats();}
    Picture image(int id)const;
    Glyph glyph(uint32_t cp,int size)const;
    const char* string(int id)const;
    Clip clip(int id)const;
    unsigned card_count()const{return cards_;}
    bool current_profile()const{return version_==ContentVersion&&cards_==CardKinds&&ui_count_==U_Count&&strings_==ContentStrings&&clips_==AnimationKinds;}
    int card_image(int kind)const;
    int ui_image(int key)const;
    int relic_image(int index)const;
    int frame_image(Card)const;
    int energy_image(Card)const;
    int potion_image(Potion)const;
    const char* card_string(int kind,int variant=0)const;
    // Keys and directory indexes are public metadata; localized reference
    // strings and image payloads remain in the owner's private resource pack.
    const char* world_string(const char* table,const char* key)const;
    const char* world_title(const char* table,const char* key)const;
    int world_image(const char* group,const char* key)const;
    int world_clip(const char* model,const char* action)const;
};
inline unsigned ui_owned_relic_count(const State&s){return s.world_relics.active?s.world_relics.count:s.relic_count;}
inline RelicId ui_owned_relic_id(const State&s,unsigned position){return position<ui_owned_relic_count(s)?(s.world_relics.active?s.world_relics.entries[position].id:s.relic_order[position]):NoWorldRelic;}
constexpr unsigned MaxWorldUiChoices=MaxOriginalEventRewards>MaxWorldRelicOffers?MaxOriginalEventRewards:MaxWorldRelicOffers;
inline unsigned ui_reward_actions(const State&s,UiAction*out){
 unsigned n=0;const UiAction base[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion};
 const bool transferred=s.world.active&&s.world.reward_count==0&&(s.reward_claimed&7)==7;
 if(!transferred)for(auto a:base){if(out)out[n]=a;++n;}
 if(s.world.active&&s.world.reward_relic!=NoWorldRelic&&!s.world.reward_relic_claimed){if(out)out[n]=UiAction::RewardRelic;++n;}
 if(s.necro.grimoire_pending){if(out)out[n]=UiAction::RewardGrimoire;++n;}
 if(out)out[n]=UiAction::LeaveRewards;return n+1;
}
enum class Key { Left,Right,Up,Down,Confirm,Back,EndTurn,Inspect,Save,Load,Log,Quit,Map,Potions,Relics,Detail,ReferenceMap,Collection,Statuses };
inline bool choice_free_energy(const State&s){
    return s.phase==Phase::ExtendedChoice&&(s.expansion.choice==XChoice::Free||
        (s.expansion.choice_free&&(s.expansion.choice==XChoice::Offer||s.expansion.choice==XChoice::Discard)));
}
struct Callbacks { bool(*save)(const State&);bool(*load)(State&); };
struct Motion {int clip=-1;uint32_t start=0;};
constexpr unsigned MaxEnemyVisualFacts=64,MaxEnemyVisualCommands=24;
struct EnemyVisualFacts {uint64_t known=0;int values[MaxEnemyVisualFacts]{};};
struct EnemyVisualCommand {const char* action=nullptr;EnemyVisualFacts facts{};int16_t move=-1;uint8_t direct=0;uint32_t earliest=0;};
struct EnemyPresentation {
    uint16_t identity=0;uint8_t model=0,skin=0;
    int state=-1,clip=-1,move=-1,hp=0;
    uint32_t start=0,until=0;
    const char* next_clip=nullptr;
    bool loop=true,dead=false,overflow=false;
    EnemyVisualFacts facts{};
    EnemyVisualCommand queue[MaxEnemyVisualCommands]{};uint8_t count=0;
};
struct App {
    State game{};Content content{};Callbacks callbacks{};
    uint16_t pixels[ScreenW*ScreenH]{};
    bool title=true,paused=false,quit=false,has_save=false,help=false;
    bool practice_entry=false;
    bool run_settings=false,run_seed_editing=false;
    uint8_t selected_ascension=0;
    reference::ActKind selected_region=reference::ActKind::Overgrowth;
    char selected_seed[65]{};
    bool hand_focus=false; // Presentation only, never part of a saved run.
    bool hand_can_focus()const;
    bool shop_open=false,event_reader=false;
    int event_reader_scroll=0;
    bool room_input(Key);
    Character selected_character=Character::Ironclad;
    int library_filter=0,library_sort=0;
    uint16_t choice_selected[MaxDeck]{};unsigned choice_selected_count=0;uint32_t choice_revision=0;
    bool character_select=false,library=false,reward_cards=false,library_upgraded=false;
    bool grimoire_select=false;
    int library_tab=0,library_page=0,library_selected=0;
    int cursor=0,target=0,menu=0,panel=0,scroll=0,pile_tab=0,notice_ticks=0,feedback=0;
    int map_scroll=0,busy=0,card_fly=0,fly_card=-1,last_hit[MaxEnemies+1]={},hit_start[MaxEnemies+1]={};
    uint32_t animation=0,run_number=0;const char* notice="";
    Motion motion[MaxEnemies+1]{};
    EnemyPresentation enemy_presentation[MaxEnemies]{};
    uint32_t enemy_visual_clock=0,combat_linger_until=0;
    uint32_t enemy_visual_seed=0;int enemy_visual_floor=-1,enemy_visual_combat=-1,enemy_visual_encounter=-1;
    void sync_enemy_feedback();
    void observe_core(const State&,Log);
    void enemy_trigger(int,const char*,int move=-1,bool direct=false,const EnemyVisualFacts* facts=nullptr,uint32_t earliest=0);
    bool enemy_frame(int,int& clip,uint32_t& elapsed,bool& loop);
    int combat_background()const;
    Motion osty_motion{};int last_osty_hp=0,last_osty_attacks=0;
    void sync_character_feedback();
    PointerState pointer{};
    MapPreview map_preview{};
    HitRegion hits[MaxHits]{}; int hit_count = 0;
    int status_actor=-1, status_selected=0,status_scroll=0,status_scroll_limit=0;
    void open_statuses(int actor);
    bool status_input(Key);
    int deck_candidate=-1;uint32_t deck_choice_revision=0;Phase deck_choice_phase=Phase::Map;
    bool deck_input(Key);
    bool deck_candidate_valid()const;
    void confirm_deck_choice();
    int detail_item=-1,detail_owned=-1;
    uint32_t seen_relic_revision=0;RelicMask relic_flash_mask=0;
    uint32_t seen_world_relic_revision=0;RelicId world_relic_flash=NoWorldRelic;
    int relic_flash_ticks=0;
    void open_owned_relic(int position);
    void sync_relic_feedback();
    int detail_cost_override=-2,detail_instance=-1,detail_deck_index=-1;
    EventCardFacts detail_facts{};bool detail_has_facts=false;
    uint16_t detail_genetic=0,detail_scythe=0;
    void open_choice_detail(int token);
    int detail_card = -1, detail_scroll = 0, inspect_cursor = 0;
    void add_hit(int x,int y,int w,int h,HitKind,int value=0,bool enabled=true);
    const HitRegion* hit_at(int x,int y) const;
    uint32_t input_context() const;
    void pointer_move(int x,int y);
    void pointer_button(bool pressed);
    void pointer_cancel();
    void pointer_scroll(int rows);
    void pointer_key(Key);
    void open_card_detail(Card,int deck_index=-1);
    void open_card_offer(Card,const EventCardFacts&,uint16_t genetic=0,uint16_t scythe=0);
    int card_face_image(Card,int instance=-1,int deck=-1,const EventCardFacts* facts=nullptr)const;
    CardType card_face_type(Card,int instance=-1,int deck=-1,const EventCardFacts* facts=nullptr)const;
    int card_face_frame(Card,int instance=-1,int deck=-1,const EventCardFacts* facts=nullptr)const;
    int card_offer_cost(Card,const EventCardFacts*)const;
    int relic_face_image(RelicId)const;
    void card_modifier_text(Card,char*,size_t,int instance=-1,int deck=-1,const EventCardFacts* facts=nullptr)const;
    bool reference_input(Key);
    bool screen_input(Key);
    unsigned library_items(CardKind* ids=nullptr)const;
    bool selection_input(Key);
    bool world_choice_input(Key);
    unsigned world_choice_options(uint16_t* out,unsigned capacity)const;
    unsigned world_choice_min()const;
    unsigned world_choice_max()const;
    void toggle_world_choice(int);
    void confirm_world_choice();
    void world_choice_detail(int);
    void sync_selection();
    void toggle_choice(int card_id);
    void confirm_choice();
    void ui_action(UiAction);
    void select_library_item(int);
    void reference_text(int code);

    bool init(const uint8_t*,size_t,Callbacks);
    bool init(ResourceSource,Callbacks);
    void input(Key);
    void tick();
    void advance(uint32_t elapsed_ms);
    void draw_frame();
    uint32_t tick_remainder=0;
    void render();
    void card_description(Card,int,char*,size_t,int instance=-1,int deck_index=-1,const EventCardFacts* facts=nullptr)const;
    void world_card_description(Card,int,char*,size_t,int instance=-1,int deck_index=-1,const EventCardFacts* facts=nullptr)const;
    void localized_text(const char* table,const char* key,char*,size_t,int model=-1,int actor=-1,int amount=0)const;
    void original_event_text(int option,bool description,char*,size_t)const;
    const char* relic_title(RelicId,int position=-1)const;
    void relic_description(RelicId,char*,size_t,int position=-1)const;
    int detail_energy()const;
    int detail_stars()const;
    void message(const char*);
    void animate(int actor,int clip,int delay=0);
    void reset_presentation();
};
} // namespace spire
