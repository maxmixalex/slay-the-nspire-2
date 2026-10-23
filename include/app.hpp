#pragma once
#include "core.hpp"
#include "keyword_hover.hpp"
#include "event_assets.hpp"
#include "assets.hpp"
#include "ui_assets.hpp"
#include "native_ui_assets.hpp"
#include "locale.hpp"
#include "feedback_assets.hpp"
#include "button_presentation.hpp"
#include "ui_actions.hpp"
#include "pointer.hpp"
#include "map_preview.hpp"
#include "resource_source.hpp"
#include "world_enemy_assets.hpp"
#include "world_map_layout.hpp"
#include "world_map_presentation.hpp"
#include "world_map_transition.hpp"
#include "world_map_window.hpp"
#include "world_map_act_intro.hpp"
#include "health_bar.hpp"
#include "combat_feedback.hpp"
#include "combat_status.hpp"
#include "character_select_presentation.hpp"
#include "turn_banner_presentation.hpp"
#include "combat_preparation.hpp"
namespace spire {
constexpr int ScreenW=320,ScreenH=240,MaxPack=13*1024*1024;
// MaxPack remains the historical in-memory fixture limit. Production hosts
// use bounded range reads and may open larger immutable resource files.
constexpr uint32_t MaxContentFile=256u*1024u*1024u;
// Each appended character/token model adds a title and two descriptions after
// the complete, immutable 0.14 string directory.
// The two storage versions share one catalog. Keep the original v14 constant
// for legacy payloads/fixtures; v15 adds the bounded sparse-tile codec.
constexpr unsigned ContentVersion=14,TileContentVersion=15,NativeUiContentVersion=16,ContentStrings=WorldStringCount;
struct Picture {const uint8_t* pixels;const uint8_t* alpha;const uint8_t* palette;uint16_t w,h;uint8_t format;};
struct Glyph {const uint8_t* pixels;uint8_t w,h;uint16_t advance;uint16_t encoded_size=0;};
struct Clip {uint32_t first;uint16_t count,frame_ms;int16_t pivot_x,pivot_y;};
struct ImageCacheUsage {unsigned entries=0,pages=0;};
class Content {
    const uint8_t* data_{};uint32_t version_{},length_{},images_{},glyphs_{},strings_{},clips_{},cards_{},ui_offset_{},ui_count_{};
    static constexpr uint32_t DirectoryCapacity=2u*1024u*1024u;
    mutable uint8_t directory_[DirectoryCapacity]{};
    const uint8_t* metadata_{};
    const uint8_t* tile_metadata_{};uint32_t directory_bytes_{};
    ResourceReader reader_{};
    mutable bool io_good_=true;
    uint32_t generation_=0;
    // A successful first picture validates all its tiles. The optional bitmap
    // borrows the unused directory tail; without space, every miss does that
    // full check. Structural startup validation never sets these bits.
    uint8_t* tile_validated_{};
    // Exact page costs collected by the existing startup validation. This
    // optional table borrows the resident-directory tail, never another pool.
    uint8_t* image_pages_{};
    const uint8_t* payload(uint32_t offset,uint32_t count,uint8_t* scratch,uint32_t capacity)const;
    const uint8_t* glyph_record(uint32_t cp,int size)const;
public:
    bool open(const uint8_t*,size_t);
    bool open(ResourceSource);
    bool healthy()const{return metadata_&&io_good_;}
    uint32_t generation()const{return generation_;}
    ResourceReadStats read_stats()const{return reader_.stats();}
    Picture image(int id)const;
    // These planning queries never read the source, decode or change LRU.
    // A cost is exact when collected at open; constrained legacy directories
    // fall back to a safe upper bound until an image has been decoded.
    bool image_cached(int id)const;
    bool image_alias(int first,int second)const;
    unsigned image_cache_pages(int id)const;
    ImageCacheUsage image_cache_usage()const;
    static constexpr unsigned image_page_bytes(){return 4096;}
    static constexpr unsigned image_capacity_pages(){return 748;}
    static constexpr unsigned image_capacity_entries(){return 320;}
    Glyph glyph(uint32_t cp,int size)const;
    // Layout uses the resident descriptor only. Measuring a label must not
    // decode/read glyph pixels or evict the glyphs about to be painted.
    Glyph glyph_metrics(uint32_t cp,int size)const;
    const char* string(int id)const;
    Clip clip(int id)const;
    unsigned card_count()const{return cards_;}
    unsigned version()const{return version_;}
    bool current_profile()const{
        if(version_<ContentVersion||version_>MerchantEffectsContentVersion||cards_!=CardKinds)return false;
        const unsigned auxiliary=version_==MerchantEffectsContentVersion?unsigned(MerchantEffectsAuxiliaryCount):version_==FeedbackContentVersion?unsigned(FeedbackAuxiliaryCount):version_==LocaleContentVersion?unsigned(LocaleAuxiliaryCount):version_==NativeUiContentVersion?unsigned(NativeUiAuxiliaryCount):unsigned(U_Count);
        const unsigned animations=version_==MerchantEffectsContentVersion?unsigned(MerchantEffectsAnimationKinds):version_==FeedbackContentVersion?unsigned(FeedbackAnimationKinds):unsigned(AnimationKinds);
        const bool text=version_>=LocaleContentVersion?strings_==LocaleStringCount:strings_==ContentStrings||(version_==NativeUiContentVersion&&strings_==NativeUiStringCount);
        return ui_count_==auxiliary&&text&&clips_==animations;
    }
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
    const char* locale_string(const char* table,const char* key)const;
    const char* ui_text(const char* legacy_utf8)const;
    bool english()const;
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
struct Callbacks {
    bool(*save)(const State&);bool(*load)(State&);bool(*clear_save)()=nullptr;
    // Production hosts supply fresh entropy for automatic new runs. A null
    // callback (or zero sample) keeps deterministic offline replay fixtures.
    uint32_t(*seed_entropy)()=nullptr;
};
// Session-local evidence of a complete canonical checkpoint, not a run UUID
// or a compare-and-swap token supplied by the storage host.
struct SaveCheckpoint {uint64_t hash=0;uint32_t size=0;};
// Only presentation owns delayed map travel. The canonical run remains at
// the departure point until the original transition finishes and is rechecked.
struct MapTravel {
    SaveCheckpoint checkpoint{};
    uint64_t seed=0;
    uint32_t start=0,revision=0,fade_start=0;
    uint8_t act=255,from=reference::NoPoint,to=reference::NoPoint;
    world_map_transition::Speed speed=world_map_transition::Speed::Normal;
    bool pending=false,fading_in=false;
};
struct Motion {int clip=-1;uint32_t start=0;};
// One retained fade per map node lets rapid focus changes finish independently.
// Q12 scale and byte alpha are presentation samples, never saved run state.
struct MapReticleMotion {uint32_t start=0;uint16_t from_scale=4096;uint8_t from_alpha=0,mode=0;};
constexpr unsigned MaxEnemyVisualFacts=64,MaxEnemyVisualCommands=24;
struct EnemyVisualFacts {uint64_t known=0;int values[MaxEnemyVisualFacts]{};};
struct EnemyVisualCommand {const char* action=nullptr;EnemyVisualFacts facts{};int16_t move=-1;uint8_t direct=0;uint32_t earliest=0;};
struct EnemyPresentation {
    uint16_t identity=0;uint8_t model=0,skin=0;
    int state=-1,clip=-1,move=-1,hp=0;
    uint32_t start=0,until=0;
    const char* next_clip=nullptr;
    bool loop=true,dead=false,overflow=false,direct=false;
    bool removing=false;
    uint32_t fade_start=0,hide_at=0;
    EnemyVisualFacts facts{};
    EnemyVisualCommand queue[MaxEnemyVisualCommands]{};uint8_t count=0;
};
struct App {
    // Bind UI-only template values at the final display boundary. A plain
    // string takes the original pointer fast path; rich tags remain intact.
    const char* ui_text(const char* source,char* buffer,size_t capacity)const;
    State game{};Content content{};Callbacks callbacks{};
    ButtonMotion button_motions[MaxButtonMotions]{};
    EndTurnGlowMotion end_turn_glow{};
    CharacterSelectPresentation character_select_presentation{};
    TurnBannerPresentation turn_banner{};
    uint32_t energy_rotation_tick=0,energy_rotation_mdeg=0;
    bool energy_rotation_zero=false;
    uint32_t turn_banner_seed=0;
    int turn_banner_floor=-1,turn_banner_combat=-1,turn_banner_encounter=-1,turn_banner_number=0;
    void sync_turn_banner();
    CombatPreparation combat_preparation{};
    void sync_combat_preparation();
    void step_combat_preparation();
    uint16_t pixels[ScreenW*ScreenH]{};
    bool title=true,paused=false,quit=false,has_save=false,help=false;
    bool settings_open=false,fast_mode=false;
    uint8_t frontend_confirm=0,frontend_choice=0,settings_item=0;
    Character saved_character=Character::Ironclad;
    uint32_t saved_seed=0;int saved_floor=0,saved_hp=0,saved_max_hp=0;
    uint64_t saved_run_seed=0;
    uint8_t saved_ascension=0,saved_region=0;bool saved_world=false,saved_practice=false;
    SaveCheckpoint saved_checkpoint{},run_checkpoint{},confirmed_checkpoint{};
    void forget_run_checkpoint();
    bool refresh_save();
    bool load_saved_run();
    bool save_current_run();
    bool finish_saved_run();
    void return_to_title();
    bool frontend_input(Key);
    void accept_frontend_confirm();
    int title_menu_count()const{return has_save?5:4;}
    bool practice_entry=false;
    bool run_settings=false,run_seed_editing=false;
    bool game_over_summary=false;
    bool game_over_available()const;
    bool game_over_input(Key);
    // Room animation clocks describe presentation only. Reloading an opened
    // chest shows its final pose without repeating any reward operation.
    // 0 restored, 1 awaiting extra rewards, 2 opening, 3 closing, 4 closing
    // a restored chest (which must not replay its Spine opening sequence).
    uint8_t treasure_presentation=0;
    uint32_t treasure_open_tick=0,treasure_banner_tick=0,treasure_close_tick=0;
    uint32_t treasure_collection_start_ms=0;
    bool treasure_collection_started=false,treasure_skip_ready=false;
    uint16_t treasure_chest_from=65535,treasure_banner_from=0;
    int8_t treasure_banner_y=0;
    // An independent presentation stream survives room resets and reloads.
    // Zero means not yet seeded; it never belongs to State or its save codec.
    uint32_t treasure_random=0,treasure_holder_delay_us=0;
    uint64_t treasure_room_seed=0;
    uint16_t treasure_room_point=0xffff;
    uint8_t treasure_room_act=255;
    void reset_treasure_presentation();
    void sync_treasure_presentation();
    unsigned treasure_chest_opacity()const;
    unsigned treasure_collection_opacity()const;
    unsigned treasure_banner_opacity()const;
    int treasure_banner_offset()const;
    unsigned treasure_holder_opacity()const;
    unsigned treasure_holder_brightness()const;
    int treasure_holder_offset()const;
    bool treasure_pointer_can_take()const;
    bool treasure_can_proceed()const;
    bool treasure_empty_waiting()const;
    bool treasure_input(Key);
    void treasure_action(UiAction);
    unsigned camp_options(uint8_t*,unsigned)const;
    bool camp_input(Key);
    void camp_page(int);
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
    HealthBarMotion health_bars[MaxEnemies+2]{};
    uint32_t health_bar_seed=0;
    uint16_t health_bar_combat=0,health_bar_encounter=0;
    uint8_t health_bar_floor=0;
    bool health_bars_active=false;
    void sync_health_bars();
    bool map_legend=false;uint8_t map_legend_item=0;
    bool map_directional=false;
    uint32_t map_clock_ms=0;
    MapPresentationState map_visuals{};
    MapTravel map_travel{};
    MapWindowState map_window{};
    MapActIntroState map_act_intro{};
    uint64_t map_act_seed=0;
    uint32_t map_act_clock_ms=0;
    uint8_t map_act_index=255,map_act_variant=255;
    bool map_act_room_entered=false,map_act_restore_pending=false,map_dragging=false;
    int map_drag_y=0,map_act_scroll=0;
    uint64_t map_window_seed=0;
    uint8_t map_window_act=255;
    uint8_t map_window_point=reference::NoPoint;
    uint32_t map_window_step_ms=0;
    uint32_t map_window_generation=0;
    int32_t map_window_y_q12=0;
    int32_t map_window_camera_base_q12=0;
    int map_room_cursor=0,map_room_scroll=0;
    bool map_window_y_tween_finished=false;
    void reset_map_window();
    void open_map_window(bool from_top_bar=false);
    void close_map_window(bool animate=true);
    void sync_map_window();
    bool map_routes_active()const;
    void proceed_map_window();
    void reset_map_act_intro(bool restored=false);
    void sync_map_act_intro();
    bool map_intro_at_entry()const;
    bool map_can_scroll()const;
    bool cancel_map_act_intro();
    void scroll_map(int rows);
    void snap_map_camera();
    void drag_map(int pixels);
    int32_t map_camera_reference_q12()const;
    int32_t map_camera_target_q12()const;
    WorldMapView map_view(const WorldMapLayout&)const;
    bool map_node_on_screen(unsigned point)const;
    bool begin_map_travel(unsigned point);
    bool begin_map_column(int column);
    void update_map_travel();
    void cancel_map_travel();
    bool map_transition_blocks_keyboard()const;
    bool map_transition_blocks_pointer()const;
    unsigned map_transition_alpha(int y)const;
    uint64_t map_reticle_seed=0;uint8_t map_reticle_act=255,map_reticle_current=reference::NoPoint;
    MapReticleMotion map_reticles[reference::MaxPoints]{};
    void reset_map_reticles();
    void sync_map_reticles();
    unsigned map_reticle_opacity(unsigned point)const;
    unsigned map_reticle_scale(unsigned point)const;
    bool map_input(Key);
    uint32_t animation=0,run_number=0;const char* notice="";
    uint32_t next_run_seed(bool* randomized=nullptr);
    Motion motion[MaxEnemies+1]{};
    EnemyPresentation enemy_presentation[MaxEnemies]{};
    CombatFeedback combat_feedback{};
    CombatStatus combat_status{};
    void begin_combat_feedback();
    void finish_combat_feedback();
    void sync_combat_feedback();
    void observe_combat_effect(const State&,Effect);
    void combat_feedback_move(int slot,int move,uint32_t start,uint32_t action,uint32_t end);
    HealthBarValues combat_health(int actor)const;
    EnemyIntent combat_intent(int slot)const;
    int combat_energy(bool maximum=false)const;
    int combat_status_amount(int actor,int id,bool label=false)const;
    int combat_status_auxiliary(int actor,int power)const;
    EnemyId combat_status_source(int actor,int power)const;
    int combat_status_fact(int fact,int slot=0)const;
    int combat_statuses(int actor,int*out)const;
    bool combat_actions_locked()const;
    bool combat_choice_ready()const;
    void continue_combat_choice();
    uint32_t enemy_visual_clock=0,combat_linger_until=0;
    uint32_t enemy_visual_seed=0;int enemy_visual_floor=-1,enemy_visual_combat=-1,enemy_visual_encounter=-1;
    void sync_enemy_feedback();
    void observe_core(const State&,Log);
    void observe_enemy_animation(const State&,int slot,const char* trigger);
    void enemy_trigger(int,const char*,int move=-1,bool direct=false,const EnemyVisualFacts* facts=nullptr,uint32_t earliest=0);
    bool enemy_frame(int,int& clip,uint32_t& elapsed,bool& loop);
    unsigned enemy_opacity(int slot)const;
    int combat_background()const;
    Motion osty_motion{};int last_osty_hp=0,last_osty_attacks=0;
    void sync_character_feedback();
    PointerState pointer{};
    MapPreview map_preview{};
    HitRegion hits[MaxHits]{}; int hit_count = 0;
    int status_actor=-1, status_selected=0,status_scroll=0,status_scroll_limit=0;
    void open_statuses(int actor);
    void open_status_icon(int encoded);
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
    KeywordHoverState keyword_hover{};
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
    bool world_choice_is_card_reward()const;
    unsigned world_choice_page_size()const;
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
    void reset_presentation(bool keep_map_visuals=false);
};
} // namespace spire
