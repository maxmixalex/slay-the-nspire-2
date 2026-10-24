#include "card_face_layout.hpp"
#include "character_select_presentation.hpp"
#include "fitted_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"card presentation:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static fitted_text::Metric metric(const RichChar&ch,int size){
    const int width=ch.codepoint>=0x2e80?size:ch.codepoint==0x2026?size:(size+1)/2;
    return {width+(ch.style.bold?1:0),width+(ch.style.bold?1:0)};
}
struct Letter {RichChar ch;int x,row,size;};
static std::vector<Letter> layout(const char*text,int width,int height,int preferred=10){
    const auto plan=fitted_text::plan(text,width,height,preferred,8,metric);
    std::vector<Letter> out;
    fitted_text::visit(text,width,plan,metric,[&](RichChar ch,int x,int row,int size){out.push_back({ch,x,row,size});});
    for(const auto&letter:out){CHECK(letter.x>=0&&letter.x+metric(letter.ch,letter.size).width<=width);
        CHECK(letter.row>=0&&letter.row<plan.rows);CHECK(letter.row*plan.step+letter.size+2<=height);}
    if(plan.truncated&&plan.rows){CHECK(!out.empty()&&out.back().ch.codepoint==0x2026);}
    return out;
}
static void text_boundaries(){
    const auto short_text=layout("Deal 6 damage.",68,36);CHECK(!short_text.empty());CHECK(short_text.front().size==10);
    CHECK(short_text.back().ch.codepoint=='.');
    auto chinese=layout("获得[gold]辉星[/gold]，随后获得格挡。",64,32);bool gold=false;
    for(const auto&letter:chinese)if(letter.ch.codepoint==0x8f89){CHECK(letter.ch.style.color==1);gold=true;}CHECK(gold);
    // Twelve Chinese glyphs fit two rows only after switching 10 -> 8px.
    const auto shrink=fitted_text::plan("一二三四五六七八九十一二",48,20,10,8,metric);
    CHECK(shrink.size==8&&shrink.rows==2&&!shrink.truncated);
    const auto overflow=layout("[gold]Gain 2 Stars.[/gold]\n[b]Deal 123 damage.[/b]\nExhaust. More text after the last visible row.",62,29);
    CHECK(overflow.back().ch.codepoint==0x2026&&overflow.back().row==2);
    unsigned dots=0;for(const auto&letter:overflow)dots+=letter.ch.codepoint==0x2026;CHECK(dots==1);
    for(int width:{32,38,56,62,68,95})for(int height:{11,15,21,29,36,53}){
        layout("[gold]辉星[/gold] [b]123[/b] [red]damage[/red]. Very long text ends here.",width,height);
        layout("one\ntwo\nthree\nfour\nfive\nsix",width,height);
    }
    CHECK(layout("",38,21).empty());
    const auto words=layout("Keep words intact",38,32,8);
    std::string rows[3];for(const auto&letter:words){CHECK(letter.row<3);rows[letter.row]+=char(letter.ch.codepoint);}
    CHECK(rows[0]=="Keep "&&rows[1]=="words "&&rows[2]=="intact");
    const auto trailing=layout("ABCDEFGH ",32,11,8);CHECK(trailing.size()==8&&trailing.back().ch.codepoint=='H');
    const auto final_newline=layout("A\nB\nC\n",40,20,8);CHECK(final_newline.back().ch.codepoint==0x2026);
}
static bool inside(card_face::Rect inner,card_face::Rect outer){return inner.x>=outer.x&&inner.y>=outer.y&&inner.x+inner.w<=outer.x+outer.w&&inner.y+inner.h<=outer.y+outer.h;}
static void card_geometry(){
    // Independent authored landmarks: the portrait ends at y233/422 and
    // the body starts at y248/422. These must stay disjoint at every size.
    for(auto size:{card_face::Rect{0,0,45,63},card_face::Rect{0,0,47,66},card_face::Rect{0,0,69,97},card_face::Rect{0,0,80,112},card_face::Rect{0,0,86,121},card_face::Rect{0,0,118,166}}){
        auto box=card_face::layout(13,27,size.w,size.h);CHECK(inside(box.portrait,box.frame));CHECK(inside(box.description,box.frame));
        CHECK(box.portrait.y+box.portrait.h<=box.description.y);CHECK(box.title.y<box.portrait.y);
        CHECK(box.star.y>box.energy.y&&box.star.x<box.energy.x);CHECK(box.type.y<box.description.y);
    }
    auto card=card_face::layout(0,0,80,112);CHECK(card.portrait.x==7&&card.portrait.y==11&&card.portrait.w==67&&card.portrait.h==50);
    CHECK(card.description.y==66&&card.description.h==36);
    CHECK(card_face::material_group(5)==7);CHECK(card_face::material_group(8)==4);CHECK(card_face::material_group(9)==3);CHECK(card_face::material_group(10)==6);
}
static void character_motion(){
    CharacterSelectPresentation motion;motion.observe(true,0,-1,100);
    CHECK(motion.panel_offset(100)==-57&&motion.portrait_scale(0,100)==1024);
    CHECK(motion.opened==100&&motion.value(0,100)==1100&&motion.saturation(0,100)==1000);
    CHECK(motion.value(1,100)==400&&motion.value(1,600)==400&&motion.saturation(1,600)==200);
    CHECK(motion.panel_offset(350)<0&&motion.panel_offset(350)>-5);CHECK(motion.panel_offset(600)==0);
    // OnFocus only enlarges an unselected portrait; selected keyboard focus
    // is already illuminated and returns without changing its scale.
    motion.observe(true,0,1,600);CHECK(motion.panel_start==100&&motion.portrait_scale(0,600)==1024);
    CHECK(motion.portrait_scale(1,600)==1126&&motion.value(1,600)==1100);
    motion.observe(true,0,-2,650);
    CHECK(motion.portrait_scale(1,900)>1024&&motion.portrait_scale(1,900)<1040);
    // Halfway through the source EXPO_OUT recovery, Q12 interpolation of
    // 1.1 -> .8 rounds the displayed thousandths to 810, not below 810.
    CHECK(motion.value(1,900)==810);
    CHECK(motion.portrait_scale(1,1150)==1024&&motion.value(1,1150)==800&&motion.value(2,1150)==400);
    motion.observe(true,0,1,1200);motion.observe(true,1,1,1220);
    CHECK(motion.panel_offset(1220)==-57&&motion.opened==100&&motion.portrait_scale(1,1220)==1126);
    for(unsigned t=1221;t<1720;t+=17)motion.observe(true,1,1,t);
    CHECK(motion.panel_start==1220&&motion.panel_offset(1720)==0&&motion.opened==100);
    motion.observe(true,1,-2,1740);CHECK(motion.portrait_scale(1,2240)==1024&&motion.value(1,2240)==1100);
    // A selection replays only the information panel, not Back/Embark entry.
    motion.observe(true,2,-1,2300);CHECK(motion.portrait_scale(2,2300)==1024&&motion.opened==100);
    motion.observe(true,2,3,2330);motion.observe(true,3,3,2340);CHECK(motion.panel_start==2340);
    CHECK(motion.portrait_scale(2,2340)==1024&&motion.portrait_scale(3,2340)==1126);
    motion.observe(false,3,-1,2400);motion.observe(true,3,-1,2500);
    CHECK(motion.panel_offset(2500)==-57&&motion.opened==2500&&motion.portrait_scale(2,2500)==1024&&motion.value(2,2500)==400);
    motion.reset();CHECK(!motion.active);
    motion.observe(true,0,0,2600);motion.observe(true,0,-2,2650);CHECK(motion.portrait_scale(0,3150)==1024);
    motion.observe(true,0,-1,3150);CHECK(motion.portrait_scale(0,3150)==1024);
    motion.reset();
    motion.observe(true,4,-1,0xffffff00u);CHECK(motion.panel_offset(244)==0);
    CHECK(motion.outline_alpha(0)==255&&motion.outline_alpha(625)==89&&motion.outline_alpha(1250)==255);
}
int main(){text_boundaries();card_geometry();character_motion();std::printf("card presentation: %u checks passed; bounded rich text, six card sizes, selector focus and reentry\n",checks);}
