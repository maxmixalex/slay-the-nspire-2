#pragma once
#include "core.hpp"
#include "assets.hpp"
#include "ui_assets.hpp"
#include "ui_actions.hpp"
#include "pointer.hpp"
#include "map_preview.hpp"
namespace spire {
constexpr int ScreenW=320,ScreenH=240,MaxPack=6*1024*1024;
struct Picture {const uint8_t* pixels;const uint8_t* alpha;const uint8_t* palette;uint16_t w,h;uint8_t format;};
struct Glyph {const uint8_t* pixels;uint8_t w,h;uint16_t advance;uint16_t encoded_size=0;};
struct Clip {uint32_t first;uint16_t count,frame_ms;int16_t pivot_x,pivot_y;};
class Content {
    const uint8_t* data_{};uint32_t length_{},images_{},glyphs_{},strings_{},clips_{},cards_{},ui_offset_{},ui_count_{};
public:
    bool open(const uint8_t*,size_t);
    Picture image(int id)const;
    Glyph glyph(uint32_t cp,int size)const;
    const char* string(int id)const;
    Clip clip(int id)const;
    unsigned card_count()const{return cards_;}
    int card_image(int kind)const;
    int ui_image(int key)const;
    int relic_image(int index)const;
    int frame_image(Card)const;
    int energy_image(Card)const;
    int potion_image(Potion)const;
    const char* card_string(int kind,int variant=0)const;
};
enum class Key { Left,Right,Up,Down,Confirm,Back,EndTurn,Inspect,Save,Load,Log,Quit,Map,Potions,Relics,Detail,ReferenceMap,Collection };
struct Callbacks { bool(*save)(const State&);bool(*load)(State&); };
struct Motion {int clip=-1;uint32_t start=0;};
struct App {
    State game{};Content content{};Callbacks callbacks{};
    uint16_t pixels[ScreenW*ScreenH]{};
    bool title=true,paused=false,quit=false,has_save=false,help=false;
    bool practice_entry=false;
    Character selected_character=Character::Ironclad;
    int library_filter=0,library_sort=0;
    uint16_t choice_selected[MaxHand]{};unsigned choice_selected_count=0;uint32_t choice_revision=0;
    bool character_select=false,library=false,reward_cards=false,library_upgraded=false;
    int library_tab=0,library_page=0,library_selected=0;
    int cursor=0,target=0,menu=0,panel=0,scroll=0,pile_tab=0,notice_ticks=0,feedback=0;
    int map_scroll=0,busy=0,card_fly=0,fly_card=-1,last_hit[3]={},hit_start[3]={};
    uint32_t animation=0,run_number=0;const char* notice="";
    Motion motion[3]{};
    PointerState pointer{};
    MapPreview map_preview{};
    HitRegion hits[MaxHits]{}; int hit_count = 0;
    int deck_candidate=-1;uint32_t deck_choice_revision=0;Phase deck_choice_phase=Phase::Map;
    bool deck_input(Key);
    bool deck_candidate_valid()const;
    void confirm_deck_choice();
    int detail_item=-1;
    int detail_card = -1, detail_scroll = 0, inspect_cursor = 0;
    void add_hit(int x,int y,int w,int h,HitKind,int value=0,bool enabled=true);
    const HitRegion* hit_at(int x,int y) const;
    uint32_t input_context() const;
    void pointer_move(int x,int y);
    void pointer_button(bool pressed);
    void pointer_cancel();
    void pointer_scroll(int rows);
    void pointer_key(Key);
    void open_card_detail(Card);
    bool reference_input(Key);
    bool screen_input(Key);
    unsigned library_items(uint8_t* ids=nullptr)const;
    bool selection_input(Key);
    void sync_selection();
    void toggle_choice(int card_id);
    void confirm_choice();
    void ui_action(UiAction);
    void select_library_item(int);
    void reference_text(int code);

    bool init(const uint8_t*,size_t,Callbacks);
    void input(Key);
    void tick();
    void advance(uint32_t elapsed_ms);
    void draw_frame();
    uint32_t tick_remainder=0;
    void render();
    void card_description(Card,int,char*,size_t)const;
    void message(const char*);
    void animate(int actor,int clip,int delay=0);
    void reset_presentation();
};
} // namespace spire
