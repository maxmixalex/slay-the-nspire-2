#pragma once
#include "merchant_layout.hpp"
#include "room_ui7_presentation.hpp"
#include "crystal_presentation.hpp"
#include "ancient_banner_presentation.hpp"
#include "ancient_dialogue_presentation.hpp"
#include "room_ui7_assets.hpp"
#include "phobia_assets.hpp"
#include "character_death_assets.hpp"
#include "game_over_presentation.hpp"
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
#include "combat_card_motion.hpp"
#include "combat_selection.hpp"
#include "combat_osty_presentation.hpp"
#include "combat_orb_presentation.hpp"
#include "card_grid_scroll.hpp"
#include "card_highlight.hpp"
#include "deck_selection_layout.hpp"
#include "card_library.hpp"
#include "collection_assets.hpp"
#include "card_ui7_presentation.hpp"
#include "relic_inspect.hpp"
#include "card_target_presentation.hpp"
#include "combat_status.hpp"
#include "combat_spark_motion.hpp"
#include "character_select_presentation.hpp"
#include "turn_banner_presentation.hpp"
#include "combat_preparation.hpp"
#include "ftue_presentation.hpp"
#include "ftue_assets.hpp"
#include "selection_assets.hpp"
#include "title_scene.hpp"
#include "title_assets.hpp"
#include "frontend_assets.hpp"
#include "ui6_assets.hpp"
#include "ui7_assets.hpp"
#include "reward_presentation.hpp"
#include "title_particles.hpp"
#include "ui_preferences.hpp"
#include "ui_progress.hpp"
#include "ui_profiles.hpp"
#include "profile_assets.hpp"
#include "hud_ui7_assets.hpp"
#include "hud_ui7_presentation.hpp"
#include "ui_history_presentation.hpp"
#include "history_journal_ui8_assets.hpp"
#include "game_over_transition_assets.hpp"
#include "grouped_glyphs.hpp"
#include "grouped_strings.hpp"
#include "card_resource_assets.hpp"
#include "ui_history_layout.hpp"
#include "history_ui8_assets.hpp"
#include "run_modes.hpp"
#include "bestiary_presentation.hpp"
#include "auxiliary_ui.hpp"
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
constexpr unsigned RetainedContentVersion=20;
struct Picture {const uint8_t* pixels;const uint8_t* alpha;const uint8_t* palette;uint16_t w,h;uint8_t format;};
struct Glyph {const uint8_t* pixels;uint8_t w,h;uint16_t advance;uint16_t encoded_size=0;};
struct Clip {uint32_t first;uint16_t count,frame_ms;int16_t pivot_x,pivot_y;uint32_t sprite_units=0;};
struct ImageDimensions {uint16_t w=0,h=0;};
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
    mutable uint32_t font_sizes_[8]{},font_size_generation_=0;
    mutable uint8_t font_sizes_known_=0;
    // Decoded page costs learned on demand or supplied by the v20 directory.
    // The optional table uses the existing directory tail, never another pool.
    uint8_t* image_pages_{};
    const uint8_t* payload(uint32_t offset,uint32_t count,uint8_t* scratch,uint32_t capacity)const;
    const uint8_t* glyph_record(uint32_t cp,int size,unsigned style=0)const;
    Picture image_payload(int id,const uint8_t* supplied,uint32_t supplied_size)const;
public:
    bool open(const uint8_t*,size_t);
    bool open(ResourceSource);
    bool healthy()const{return metadata_&&io_good_;}
    uint32_t generation()const{return generation_;}
    ResourceReadStats read_stats()const{return reader_.stats();}
    Picture image(int id)const;
    // Batch only missing images required by this frame into the existing
    // encoded-image workspace; decoding and bounds checks stay unchanged.
    void prepare_images(const int* ids,unsigned count)const;
    // These planning queries never read the source, decode or change LRU.
    // NSP2/20 uses resident decoded costs; older packs use geometry bounds
    // until first decode, without scanning unrelated image or animation groups.
    ImageDimensions image_dimensions(int id)const;
    bool image_cached(int id)const;
    bool image_alias(int first,int second)const;
    unsigned image_cache_pages(int id)const;
    ImageCacheUsage image_cache_usage()const;
    static constexpr unsigned image_page_bytes(){return 4096;}
    static constexpr unsigned image_capacity_pages(){return 748;}
    static constexpr unsigned image_capacity_entries(){return 320;}
    // Select a real resident font band before layout. This never loads a
    // glyph or scans the resource payload; sparse legacy faces fall back.
    int font_size(int preferred,unsigned style=0,bool at_most=false,bool sparse_scene_sizes=false)const;
    Glyph glyph(uint32_t cp,int size,unsigned style=0)const;
    // Layout uses the resident descriptor only. Measuring a label must not
    // decode/read glyph pixels or evict the glyphs about to be painted.
    Glyph glyph_metrics(uint32_t cp,int size,unsigned style=0)const;
    const char* string(int id)const;
    Clip clip(int id)const;
    unsigned card_count()const{return cards_;}
    unsigned version()const{return version_;}
    bool has_ftue_assets()const{return version_>=FtueContentVersion&&(ui_count_==FtueAuxiliaryCount||has_selection_assets());}
    bool has_selection_assets()const{return version_>=SelectionContentVersion&&(ui_count_==SelectionAuxiliaryCount||has_inspection_assets());}
    bool has_inspection_assets()const{return version_>=SelectionContentVersion&&ui_count_==InspectionAuxiliaryCount;}
    bool current_profile()const{
        if(version_<ContentVersion||version_>GroupedStringContentVersion||cards_!=CardKinds)return false;
        const unsigned auxiliary=version_>=GameOverTransitionContentVersion?unsigned(GameOverTransitionAuxiliaryCount):version_>=HistoryJournalContentVersion?unsigned(HistoryJournalAuxiliaryCount):version_>=CharacterDeathContentVersion?unsigned(CharacterDeathAuxiliaryCount):version_>=HistoryUi8ContentVersion?unsigned(HistoryUi8AuxiliaryCount):version_>=HudUi7ContentVersion?unsigned(HudUi7AuxiliaryCount):version_>=ProfileContentVersion?unsigned(ProfileAuxiliaryCount):version_>=PhobiaContentVersion?unsigned(PhobiaAuxiliaryCount):version_>=Ui7ContentVersion?unsigned(Ui7AuxiliaryCount):version_>=Ui6ContentVersion?unsigned(Ui6AuxiliaryCount):version_>=FrontendContentVersion?unsigned(FrontendAuxiliaryCount):version_>=TitleEffectsContentVersion?unsigned(TitleEffectsAuxiliaryCount):version_>=MerchantEffectsContentVersion?unsigned(MerchantEffectsAuxiliaryCount):version_==FeedbackContentVersion?unsigned(FeedbackAuxiliaryCount):version_==LocaleContentVersion?unsigned(LocaleAuxiliaryCount):version_==NativeUiContentVersion?unsigned(NativeUiAuxiliaryCount):unsigned(U_Count);
        const unsigned animations=version_>=CharacterDeathContentVersion?unsigned(CharacterDeathAnimationKinds):version_>=PhobiaContentVersion?unsigned(PhobiaAnimationKinds):version_>=Ui7ContentVersion?unsigned(Ui7AnimationKinds):version_>=MerchantEffectsContentVersion?unsigned(MerchantEffectsAnimationKinds):version_==FeedbackContentVersion?unsigned(FeedbackAnimationKinds):unsigned(AnimationKinds);
        const bool text=version_>=Ui7ContentVersion?strings_==Ui7StringCount:version_>=LocaleContentVersion?strings_==LocaleStringCount:strings_==ContentStrings||(version_==NativeUiContentVersion&&strings_==NativeUiStringCount);
        const bool ui=ui_count_==auxiliary||(version_>=CardResourceContentVersion&&ui_count_==CardResourceAuxiliaryCount)||has_ftue_assets();
        return ui&&text&&clips_==animations;
    }
    int card_image(int kind)const;
    int ui_image(int key)const;
    int relic_image(int index)const;
    int frame_image(Card)const;
    int energy_image(Card,bool unknown=false)const;
    int potion_image(Potion)const;
    const char* card_string(int kind,int variant=0)const;
    // Keys and directory indexes are public metadata; localized reference
    // strings and image payloads remain in the owner's private resource pack.
    const char* world_string(const char* table,const char* key)const;
    int world_string_id(const char*table,const char*key)const;
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
 unsigned n=0;auto add=[&](UiAction action){if(out)out[n]=action;++n;};
 if(!(s.reward_claimed&1)&&s.last_gold>0)add(UiAction::RewardGold);
 if(!(s.reward_claimed&2)&&(!s.world.active||s.world.reward_count))add(UiAction::RewardCards);
 if(!(s.reward_claimed&4)&&s.reward_potion!=Potion::Empty)add(UiAction::RewardPotion);
 if(s.world.active&&s.world.reward_relic!=NoWorldRelic&&!s.world.reward_relic_claimed){if(out)out[n]=UiAction::RewardRelic;++n;}
 if(s.necro.grimoire_pending){if(out)out[n]=UiAction::RewardGrimoire;++n;}
 if(out)out[n]=UiAction::LeaveRewards;return n+1;
}
enum class Key { Left,Right,Up,Down,Confirm,Back,EndTurn,Inspect,Save,Load,Log,Quit,Map,Potions,Relics,Detail,ReferenceMap,Collection,Statuses,DrawPile,DiscardPile,ExhaustPile,Debug };
inline bool choice_free_energy(const State&s){
    return s.phase==Phase::ExtendedChoice&&(s.expansion.choice==XChoice::Free||
        (s.expansion.choice_free&&(s.expansion.choice==XChoice::Offer||s.expansion.choice==XChoice::Discard)));
}
struct Callbacks {
    bool(*save)(const State&);bool(*load)(State&);bool(*clear_save)()=nullptr;
    // Production hosts supply fresh entropy for automatic new runs. A null
    // callback (or zero sample) keeps deterministic offline replay fixtures.
    uint32_t(*seed_entropy)()=nullptr;
    bool(*load_preferences)(UiPreferences&)=nullptr;
    bool(*save_preferences)(const UiPreferences&)=nullptr;
    // Bits 0/1 identify installed English/Chinese resources. Requests are
    // serviced by the host after the current input callback has returned.
    unsigned(*available_languages)()=nullptr;
    bool(*request_language)(unsigned)=nullptr;
    bool(*load_progress)(UiProgress&)=nullptr;
    bool(*save_progress)(const UiProgress&)=nullptr;
    uint32_t(*wall_clock)()=nullptr;
    unsigned(*current_profile)()=nullptr;
    bool(*profile_info)(unsigned,UiProgressSummary&)=nullptr;
    bool(*request_profile)(unsigned,bool)=nullptr;
    bool(*write_feedback)(const char*,unsigned)=nullptr;
};
// Run parameters used by Continue/abandon, without re-encoding saved state.
struct SavedRunIdentity {
    uint64_t run_seed=0;uint32_t seed=0;
    uint32_t modifiers=0,daily_day=0;
    RunMode mode=RunMode::Standard;
    Character character=Character::Ironclad;
    uint8_t ascension=0,region=0;
    bool world=false,practice=false;
};
// Only presentation owns delayed map travel. The canonical run remains at
// the departure point until the original transition finishes and is rechecked.
struct MapTravel {
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
    CreaturePose pose{};
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
    ProceedMotion proceed_motion{};
    EndTurnGlowMotion end_turn_glow{};
    CharacterSelectPresentation character_select_presentation{};
    TurnBannerPresentation turn_banner{};
    uint32_t energy_rotation_tick=0,energy_rotation_mdeg=0;
    bool energy_rotation_zero=false;
    uint32_t turn_banner_seed=0;
    int turn_banner_floor=-1,turn_banner_combat=-1,turn_banner_encounter=-1,turn_banner_number=0;
    void sync_turn_banner();
    CombatPreparation combat_preparation{};
    FtuePresentation ftue{};
    bool open_ftue(FtueId,int target=-1);
    void sync_ftues();
    void ftue_control(int);
    bool ftue_input(Key);
    bool ftue_before_end_turn();
    bool ftue_before_play(int);
    bool ftue_before_action(UiAction);
    void observe_ftue_log(Log);
    void observe_ftue_history(const RunHistoryNotice&);
    void ftue_manual_played();
    RewardPresentation reward_presentation{};
    RewardAcquisitionSnapshot reward_acquisition_snapshot()const;
    void finish_reward_acquisition(const RewardAcquisitionSnapshot&);
    void observe_deck_removing(const State&,int index);
    void sync_reward_presentation();
    bool card_reward_active()const;
    bool reward_list_interactive()const;
    bool card_reward_interactive()const;
    void sync_card_reward_presentation();
    bool card_reward_input_ready();
    void card_reward_alternative(bool skip);
    bool reward_pickup_hidden(RewardPickupKind,unsigned)const;
    int reward_gold()const;
    void sync_combat_preparation();
    void step_combat_preparation();
    uint16_t pixels[ScreenW*ScreenH]{};
    bool title=true,paused=false,quit=false,has_save=false,help=false;
    bool pause_visible=false;
    uint32_t pause_opened=0;
    bool settings_open=false,fast_mode=false;
    AuxiliaryUi auxiliary{};
    void open_auxiliary(AuxiliaryScreen);
    void close_auxiliary();
    void auxiliary_control(int);
    bool auxiliary_input(Key);
    bool auxiliary_text(int);
    void auxiliary_scroll(int);
    void advance_auxiliary(unsigned);
    unsigned feedback_report(char*,unsigned)const;
    bool show_card_indices=false,text_effects=true,keyboard_only=false,phobia_mode=false,show_run_timer=false;
    bool run_timer_visible()const;
    void run_timer_label(char*,size_t)const;
    uint8_t screen_shake=2;
    bool bestiary_actions_preferred=true;
    InputBindings input_bindings=default_input_bindings();
    bool calculator_keyboard=false;
    int8_t binding_listening=-1;
    uint8_t binding_bank=0;
    struct HeldInput {uint16_t code=0;uint8_t key=255;uint32_t context=0;};
    HeldInput held_inputs[12]{};
    void begin_key_rebind(unsigned command);
    bool physical_key(uint16_t code,bool pressed);
    void physical_keys_cancel();
    void reset_input_bindings();
    const char* input_command_title(unsigned command)const;
    bool settings_language_open=false;
    uint8_t settings_language_item=0,settings_top=0;
    void restore_preferences();
    void set_fast_mode(bool value);
    void persist_preferences();
    unsigned available_languages()const;
    bool request_language(unsigned language);
    bool reload_language(ResourceSource source);
    void settings_action(UiAction);
    bool settings_input(Key);
    UiProgress progress{};
    bool profiles_open=false,profile_pending=false,profile_delete=false;
    uint8_t profile_selection=0,profile_target=1;
    uint32_t profiles_started=0;
    UiProgressSummary profile_summaries[UiProfileCount]{};
    UiProfileMotion profile_motion[UiProfileCount*2+1]{};
    UiProfileLabelMotion profile_labels[UiProfileCount]{};
    unsigned current_profile()const;
    void open_profiles();
    void refresh_profiles();
    bool profiles_input(Key);
    bool request_profile_change(unsigned,bool erase=false);
    void finish_profile_change(bool success);
    bool compendium_open=false,statistics_open=false,history_open=false;
    uint8_t compendium_selection=0,history_index=0;
    int history_selected=-1;
    UiHistoryPresentation history_presentation{};
    UiHistoryTooltip history_tooltip{};
    uint32_t progress_page_started=0;
    UiHistoryScroll progress_page_scroll{};
    UiMenuTransition compendium_motion[6]{};
    void open_compendium();
    void open_statistics();
    void open_history();
    void history_select(int direction);
    bool progress_navigation_input(Key);
    bool progress_pointer_button(bool,const HitRegion&);
    void history_pointer(const HitRegion&);
    bool progress_loaded=false,progress_dirty=false;
    bool ftue_profile_pending=false;
    uint32_t progress_seen_revision=0,progress_remainder_ms=0;
    uint16_t progress_seen_floor=0,progress_seen_battle=0;
    uint8_t progress_seen_phase=255;
    bool progress_seen_ending=false;
    void restore_progress();
    bool persist_progress();
    bool seen_ftue(FtueId)const;
    bool seen_popup(FtueId)const;
    void mark_ftue(FtueId);
    void set_ftues_enabled(bool);
    void reset_ftues();
    void begin_progress(bool resumed=false);
    void sync_progress();
    void progress_time(uint32_t);
    void observe_progress_log(Log);
    void observe_progress_effect(const State&);
    void observe_progress_damage(const State&,const CoreDamageResult&);
    void observe_run_fact(const State&,unsigned,unsigned);
    void observe_history(const State&,const RunHistoryNotice&);
    int bind_history_journal();
    void history_journal_line(const UiHistoryJournalEntry&,char*,unsigned)const;
    void discover_potion(Potion);
    void discover_card(CardKind);
    void discover_relic(RelicId);
    void observe_enemy_spawned(const State&,unsigned model,uint16_t identity);
    void finish_progress(bool abandoned=false,const State*source=nullptr);
    bool bestiary_available()const;
    bool act_discovered(unsigned region)const;
    const UiEnemyStatistics& enemy_statistics(unsigned model,int character=-1)const;
    const UiEnemyStatistics& encounter_statistics(unsigned encounter,int character=-1)const;
    const UiRunHistory* history_at(unsigned newest_first)const;
    const UiFloorHistory* current_floor_history(unsigned act,unsigned point)const;
    BestiaryPresentation bestiary{};
    bool open_bestiary(bool force=false);
    bool bestiary_input(Key);
    bool bestiary_pointer(const HitRegion&);
    void bestiary_scroll(int rows);
    unsigned bestiary_move_count()const;
    unsigned bestiary_entry_model(int row)const;
    uint8_t frontend_confirm=0,frontend_choice=0,settings_item=0,settings_tab=0;
    Character saved_character=Character::Ironclad;
    uint32_t saved_seed=0;int saved_floor=0,saved_hp=0,saved_max_hp=0,saved_gold=0;
    uint64_t saved_run_seed=0;
    uint8_t saved_ascension=0,saved_region=0,saved_current_region=0;bool saved_world=false,saved_practice=false;
    bool run_save_linked=false;
    SavedRunIdentity confirmed_run{};
    void forget_run_checkpoint();
    bool refresh_save();
    bool load_saved_run();
    bool save_current_run();
    bool finish_saved_run();
    void return_to_title();
    bool frontend_input(Key);
    RunModeFrontend run_mode{};
    void open_run_menu(RunMenu);
    void refresh_daily_run();
    bool run_modes_input(Key);
    bool run_modes_action(UiAction);
    void run_modifier_select(unsigned);
    bool start_selected_run(const RunOptions&);
    const char* run_modifier_text(RunModifier,bool description=false)const;
    uint16_t localized_card_rank(CardKind)const;
    void accept_frontend_confirm();
    int title_menu_count()const{return has_save?5:4;}
    bool practice_entry=false;
    bool run_settings=false,run_seed_editing=false;
    uint8_t character_control=0;
    bool game_over_summary=false,history_from_game_over=false;
    GameOverPresentation game_over_presentation{};
    int8_t game_over_focus=-1;
    void sync_game_over_presentation();
    bool game_over_button_ready()const;
    void show_game_over_summary();
    bool game_over_available()const;
    bool game_over_input(Key);
    const UiRunHistory* game_over_run()const;
    void open_game_over_history();
    void game_over_quote(char*,unsigned)const;
    void game_over_detail(int,char*,unsigned,char*,unsigned)const;
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
    void sync_camp_presentation();
    bool camp_proceed_ready()const;
    bool camp_input(Key);
    void camp_page(int);
    uint8_t selected_ascension=0;
    reference::ActKind selected_region=reference::ActKind::Overgrowth;
    char selected_seed[65]{};
    bool hand_focus=false; // Presentation only, never part of a saved run.
    bool hand_can_focus()const;
    bool shop_open=false,event_reader=false;
    RoomUi7Presentation room_presentation{};
    CrystalPresentation crystal_presentation{};
    AncientBannerPresentation ancient_banner{};
    AncientDialoguePresentation ancient_dialogue{};
    void sync_ancient_dialogue();
    bool ancient_dialogue_ready()const;
    bool ancient_dialogue_input(Key);
    void next_ancient_dialogue();
    bool scroll_ancient_dialogue(int);
    void ancient_dialogue_text(unsigned,char*,size_t)const;
    void sync_crystal_presentation();
    bool crystal_input(Key);
    void crystal_click(int);
    int event_reader_scroll=0;
    bool room_input(Key);
    Character selected_character=Character::Ironclad;
    int library_filter=1,library_sort=0;
    CardLibraryPresentation card_library{};CollectionPresentation collection{};
    bool library_search_editing=false;
    void open_library(int tab);
    bool library_input(Key);
    void library_control(int);
    void library_text(int code);
    void commit_library_query();
    bool library_card_seen(CardKind)const;
    bool library_item_seen(int tab,int index)const;
    unsigned library_item_character(int tab,int index)const;
    void prepare_collection();
    void focus_collection();
    bool collection_input(Key);
    uint16_t choice_selected[MaxDeck]{};unsigned choice_selected_count=0;uint32_t choice_revision=0;
    CombatSelectionPresentation combat_selection{};
    NativeConfirmPresentation native_confirm_motion{};
    CombatSelectionKind combat_selection_kind()const;
    CombatSelectionPrompt combat_selection_prompt()const;
    unsigned combat_selection_items(uint16_t* ids=nullptr,unsigned capacity=0)const;
    unsigned combat_selection_min()const;
    unsigned combat_selection_max()const;
    bool combat_selection_manual()const;
    bool combat_selection_interactive()const;
    bool combat_selection_ready();
    bool combat_selection_confirmable()const;
    void sync_combat_selection();
    void skip_combat_selection();
    bool choice_peeking=false;uint16_t choice_peek_from=0;uint32_t choice_peek_at=0,choice_peek_revision=0;
    bool combat_peek_context()const;
    bool combat_peek_available()const;
    unsigned combat_peek_amount()const;
    void sync_combat_peek();
    void toggle_combat_peek();
    bool combat_peek_input(Key);
    uint32_t catalog_choice_revision=UINT32_MAX;uint8_t catalog_choice_phase=255;
    bool character_select=false,library=false,reward_cards=false,library_upgraded=false;
    bool grimoire_select=false;
    int library_tab=0,library_page=0,library_selected=0;
    int cursor=0,target=0,menu=0,panel=0,scroll=0,pile_tab=0,notice_ticks=0,feedback=0;
    uint8_t pile_sort_priority[4]{0,1,2,3},pile_sort_descending=0;
    uint32_t pile_sort_epoch=0;
    bool pile_upgraded=false;
    int pile_control_focus=-1;
    unsigned pile_items(int*out=nullptr)const;
    void sort_pile(unsigned column);
    CardGridScroll card_grids[unsigned(CardGridSurface::Count)]{};
    int card_grid_drag=-1,card_grid_drag_y=0;
    CardGridScroll& configure_card_grid(CardGridSurface,int count,uint32_t identity=0);
    int active_card_grid()const;
    bool card_grid_input(Key);
    void reset_card_grids();
    void focus_card_grid();
    int keyboard_card=-1;
    uint32_t keyboard_card_revision=0;
    bool card_target_input(Key);
    int selected_hand_card()const;
    uint8_t potion_menu_item=0,potion_page=0;
    int hud_focus=-1; // HP, gold, holders, Room/Floor/Boss, then relic positions at 32.
    HudRelicMotion hud_relic_motion{};
    TargetReticleMotion hud_focus_reticle{};
    bool potion_targeting=false,potion_removed_discard=false;
    int potion_popup_slot=-1,potion_removed_slot=-1;
    uint32_t potion_popup_start=0,potion_removed_start=0;
    struct PotionPopupExit {uint32_t start=0;int16_t x=0,y=0;uint8_t alpha=0;bool thrown=false,active=false;} potion_popup_exit;
    Potion potion_removed=Potion::Empty;
    PotionBeltMotion potion_belt_motion{};
    DeckCountPulse deck_count_pulse{};
    TargetReticleMotion target_reticles[MaxEnemies+2]{};
    void potion_obtain_failed();
    void open_potion(int slot);
    void close_potion(Potion bottle=Potion::Empty);
    void begin_potion_popup_exit(Potion);
    void start_potion_use();
    bool finish_potion_use(int target);
    bool potion_input(Key);
    bool potion_needs_target()const;
    bool potion_is_thrown()const;
    bool potion_usable()const;
    void remove_potion_visual(Potion,int slot,bool discarded);
    int map_scroll=0,busy=0,card_fly=0,fly_card=-1,last_hit[MaxEnemies+1]={},hit_start[MaxEnemies+1]={};
    HealthBarMotion health_bars[MaxEnemies+2]{};
    uint32_t health_bar_seed=0;
    uint16_t health_bar_combat=0,health_bar_encounter=0;
    uint8_t health_bar_floor=0;
    bool health_bars_active=false;
    void sync_health_bars();
    bool map_legend=false;uint8_t map_legend_item=0;
    bool map_directional=false;
    MapDrawingInput map_drawing{};
    bool map_drawing_ready()const;
    void sync_map_drawing();
    void map_drawing_tool(unsigned tool,bool keyboard=false);
    bool map_drawing_key(Key);
    void map_drawing_key_release(Key);
    void map_drawing_advance(uint32_t);
    bool map_drawing_pointer_button(bool,const HitRegion&);
    void map_drawing_pointer_motion();
    void pointer_aux_button(unsigned button,bool down);
    bool map_drawing_point(bool begin=false);
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
    CombatSceneLayout creature_layout{};
    CreaturePose player_presentation[2]{};
    CombatOstyPresentation osty_presentation{};
    CombatOrbPresentation orb_presentation{};
    CombatFeedback combat_feedback{};
    CombatStatus combat_status{};
    CombatSparkMotion combat_sparks{};
    CombatCardMotion combat_cards{};
    void begin_combat_feedback(int manual_card=-1);
    void finish_combat_feedback();
    void sync_combat_feedback();
    void before_combat_effect(const State&,Effect);
    void observe_combat_effect(const State&,Effect);
    void observe_combat_damage(const State&,const CoreDamageResult&);
    void sync_combat_damage();
    void combat_scene_offset(int&x,int&y)const;
    void observe_power_flash(const State&,int receiver,const char*key);
    void combat_feedback_move(int slot,int move,uint32_t start,uint32_t action,uint32_t end);
    HealthBarValues combat_health(int actor)const;
    EnemyIntent combat_intent(int slot)const;
    unsigned combat_intent_alpha(int slot)const;
    int combat_energy(bool maximum=false)const;
    CombatHand::Cards combat_hand()const;
    HandPose combat_hand_pose(unsigned index,bool focused=false)const;
    bool combat_hand_settled(unsigned index,bool focused=false)const;
    CombatCardMotion::Counts combat_pile_counts()const;
    void sync_combat_cards();
    int combat_status_amount(int actor,int id,bool label=false)const;
    bool combat_powers_frozen()const;
    bool combat_status_counter(int actor,int id)const;
    int combat_status_auxiliary(int actor,int power)const;
    unsigned combat_power_entries(int actor,uint16_t*out,unsigned capacity)const;
    int combat_power_amount(uint16_t token,bool label=false)const;
    bool combat_power_counter(uint16_t token)const;
    EnemyId combat_status_source(int actor,int power)const;
    int combat_status_fact(int fact,int slot=0)const;
    int combat_statuses(int actor,int*out)const;
    bool combat_actions_locked()const;
    bool combat_choice_ready()const;
    void continue_combat_choice();
    uint32_t enemy_visual_clock=0,combat_linger_until=0;
    uint32_t enemy_visual_seed=0;int enemy_visual_floor=-1,enemy_visual_combat=-1,enemy_visual_encounter=-1;
    void sync_enemy_feedback();
    CreaturePose combat_enemy_pose(int)const;
    CreaturePose combat_player_pose(bool pet=false)const;
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
    DeckSelectionPresentation deck_selection{};
    bool deck_selection_active()const;
    bool deck_selection_cancelable()const;
    unsigned deck_choice_items(uint16_t*,unsigned)const;
    void sync_deck_selection();
    void cancel_deck_choice();
    bool deck_input(Key);
    bool deck_candidate_valid()const;
    void confirm_deck_choice();
    int detail_item=-1,detail_owned=-1;
    RelicInspectPresentation relic_inspect{};
    void open_relic_inspect(RelicId,int owned=-1);
    void sync_relic_inspect();
    void close_relic_inspect();
    void move_relic_inspect(int direction);
    bool relic_inspect_input(Key);
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
    CardInspectPresentation card_inspect{};
    unsigned card_inspect_ids(int*out)const;
    void bind_card_inspect();
    void close_card_inspect();
    void move_card_inspect(int direction);
    KeywordHoverState keyword_hover{};
    void add_hit(int x,int y,int w,int h,HitKind,int value=0,bool enabled=true);
    const HitRegion* hit_at(int x,int y) const;
    uint32_t input_context() const;
    void pointer_move(int x,int y);
    void pointer_button(bool pressed);
    void pointer_cancel(bool keep_keyboard_card=false);
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
    WorldSelectionKind world_selection_kind()const;
    bool world_selection_active()const;
    bool world_selection_cancelable()const;
    bool world_selection_manual()const;
    bool world_selection_confirmable()const;
    bool world_selection_upgrades()const;
    unsigned world_selection_min()const;
    unsigned world_selection_max()const;
    CombatSelectionPrompt world_selection_prompt()const;
    RelicId world_selection_owner()const;
    EventEnchantment world_selection_enchantment(int*amount=nullptr)const;
    unsigned world_selection_preview_cards()const;
    bool world_selection_preview_card(unsigned,WorldSelectionCard&)const;
    void sync_world_selection();
    void cancel_world_selection();
    bool world_choice_is_card_reward()const;
    bool world_choice_is_potion_reward()const;
    Potion world_choice_potion(int id)const;
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
    mutable char monster_title_buffer[128]{};
    const char* monster_title(EnemyId)const;
    void original_event_text(int option,bool description,char*,size_t)const;
    const char* relic_title(RelicId,int position=-1)const;
    void relic_description(RelicId,char*,size_t,int position=-1)const;
    int detail_energy()const;
    int detail_stars()const;
    void message(const char*);
#if SPIRE_DEBUG_COMMANDS
    bool debug_open=false,debug_editing=false;
    int debug_selection=0,debug_variant=0;
    char debug_line[128]{},debug_reply[256]{};
    bool debug_command(const char*);
    bool debug_input(Key);
    void debug_text(int);
    void debug_preset();
#endif
    void animate(int actor,int clip,int delay=0);
    void reset_presentation(bool keep_map_visuals=false);
};
} // namespace spire
