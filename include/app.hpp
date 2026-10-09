#pragma once
#include "core.hpp"
#include "assets.hpp"
#include "pointer.hpp"
#include "map_preview.hpp"
namespace spire {
constexpr int ScreenW=320,ScreenH=240,MaxPack=6*1024*1024;
struct Picture {const uint8_t* pixels;const uint8_t* alpha;const uint8_t* palette;uint16_t w,h;uint8_t format;};
struct Glyph {const uint8_t* pixels;uint8_t w,h;uint16_t advance;};
struct Clip {uint32_t first;uint16_t count,frame_ms;int16_t pivot_x,pivot_y;};
class Content {
    const uint8_t* data_{};uint32_t length_{},images_{},glyphs_{},strings_{},clips_{},cards_{};
public:
    bool open(const uint8_t*,size_t);
    Picture image(int id)const;
    Glyph glyph(uint32_t cp,int size)const;
    const char* string(int id)const;
    Clip clip(int id)const;
    unsigned card_count()const{return cards_;}
    int card_image(int kind)const;
    const char* card_string(int kind,int variant=0)const;
};
enum class Key { Left,Right,Up,Down,Confirm,Back,EndTurn,Inspect,Save,Load,Log,Quit,Map,Potions,Relics,Detail,ReferenceMap };
struct Callbacks { bool(*save)(const State&);bool(*load)(State&); };
struct Motion {int clip=-1;uint32_t start=0;};
struct App {
    State game{};Content content{};Callbacks callbacks{};
    uint16_t pixels[ScreenW*ScreenH]{};
    bool title=true,paused=false,quit=false,has_save=false,help=false;
    int cursor=0,target=0,menu=0,panel=0,scroll=0,pile_tab=0,notice_ticks=0,feedback=0;
    int map_scroll=0,busy=0,card_fly=0,fly_card=-1,last_hit[3]={},hit_start[3]={};
    uint32_t animation=0,run_number=0;const char* notice="";
    Motion motion[3]{};
    PointerState pointer{};
    MapPreview map_preview{};
    HitRegion hits[MaxHits]{}; int hit_count = 0;
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
