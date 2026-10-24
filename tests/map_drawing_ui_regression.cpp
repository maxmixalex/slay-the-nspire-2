#include "app.hpp"
#include "map_drawing_raster.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static UiProgress restored;
static SaveData before,after;
static uint8_t encoded[UiProgressCapacity];
static MapDrawingRaster raster;
static const char*captures=nullptr;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map drawing: line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void capture(const char*name){
    if(!captures)return;const bool visible=app.pointer.visible;app.pointer.visible=false;app.render();
    const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
    std::fprintf(f,"P6\n320 240\n255\n");
    for(auto p:app.pixels){const uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}
    CHECK(!std::fclose(f));app.pointer.visible=visible;
}
static void tool(unsigned value){
    app.render();HitRegion hit{};bool found=false;
    for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::MapDrawingTool&&app.hits[i].value==int(value)){hit=app.hits[i];found=true;}
    CHECK(found);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);app.pointer_button(true);app.pointer_button(false);
}
static unsigned ink(unsigned x,unsigned y){return raster.alpha[(y-MapDrawingRaster::Top)*320+x];}
int main(int argc,char**argv){
    std::vector<uint8_t>pack;
    if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());}
    if(argc>2)captures=argv[2];
    // Independent paper-space fixtures: partial erasing, later redraw, round
    // endpoints, viewport clipping and camera projection all remain visible.
    MapDrawingDocument document;document.bind(42,1,0);const MapDrawingTransform transform{458};
    CHECK(document.append(700,500,MapDrawingMode::Draw,true));CHECK(document.append(1200,500,MapDrawingMode::Draw,false));
    raster.rebuild(document,transform,{},{});const int y=transform.screen_y_q8(500)/256;
    CHECK(ink(110,y)>0&&ink(160,y)>0&&ink(200,y)>0);
    CHECK(document.append(960,460,MapDrawingMode::Erase,true));CHECK(document.append(960,540,MapDrawingMode::Erase,false));
    raster.rebuild(document,transform,{},{});CHECK(ink(110,y)>0&&ink(160,y)==0&&ink(200,y)>0);
    CHECK(document.append(940,500,MapDrawingMode::Draw,true));CHECK(document.append(980,500,MapDrawingMode::Draw,false));
    raster.rebuild(document,transform,{},{});CHECK(ink(160,y)>0);
    const MapDrawingTransform moved{475};raster.rebuild(document,moved,{},{});CHECK(ink(160,y)==0&&ink(160,y+17)>0);
    const MapDrawingTransform zoom{458,44,100};raster.rebuild(document,zoom,{},{});CHECK(ink(160,y)==0&&ink(160,y*2)>0);
    // A finite alpha profile exercises compositing rather than a binary cut.
    const uint8_t half=128;MapDrawingDocument alpha_doc;alpha_doc.append(960,500,MapDrawingMode::Draw,true);
    raster.rebuild(alpha_doc,transform,{&half,1,1},{});CHECK(ink(160,y)==128);
    alpha_doc.append(960,500,MapDrawingMode::Erase,true);raster.rebuild(alpha_doc,transform,{&half,1,1},{&half,1,1});CHECK(ink(160,y)==0);
    // Optional UI notes use NSH4 only when present. Old NSH2/3 and gameplay
    // saves retain their original schemas; malformed/truncated notes fail.
    clear_ui_progress(app.progress);size_t size=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(size&&encoded[3]=='2');
    CHECK(decode_ui_progress(encoded,size,restored)&&restored.map_drawing.count==0);
    app.progress.map_drawing=document;size=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(size&&encoded[3]=='4');
    CHECK(decode_ui_progress(encoded,size,restored));CHECK(restored.map_drawing.count==document.count&&restored.map_drawing.matches(42,1,0));
    CHECK(!std::memcmp(restored.map_drawing.points,document.points,document.count*sizeof(MapDrawingPoint)));
    UiProgressSummary summary;CHECK(decode_ui_progress_summary(encoded,size,summary)&&summary.exists);
    CHECK(!decode_ui_progress(encoded,size-1,restored));encoded[size-1]=0x3f;encoded[size-2]=0xff;CHECK(!decode_ui_progress(encoded,size,restored));
    // Real map overlay handlers: a drawing press over a reachable node must
    // neither travel nor create a scroll drag. Release finishes only its line.
    clear_ui_progress(app.progress);app.progress_loaded=true;new_run(app.game,0x7319,false,Character::Ironclad);CHECK(world_begin_run(app.game));
    app.title=false;app.fast_mode=true;app.reset_presentation();app.begin_progress();app.open_map_window(true);
    for(unsigned i=0;i<8;++i)app.advance(250);app.render();CHECK(app.map_drawing_ready());CHECK(save(app.game,before));
    tool(0);CHECK(app.map_drawing.mode==MapDrawingMode::Draw);
    app.pointer_move(100,120);app.pointer_button(true);CHECK(app.map_drawing.pressed&&!app.map_dragging);
    app.pointer_move(150,120);app.pointer_button(false);CHECK(!app.map_drawing.pressed&&app.progress.map_drawing.count==2);
    CHECK(app.map_drawing.mode==MapDrawingMode::Draw);capture("map-drawing-pen");
    const unsigned stored=app.progress.map_drawing.count;
    app.pointer_aux_button(2,true);CHECK(app.map_drawing.mode==MapDrawingMode::None);app.pointer_aux_button(2,false);
    app.pointer_move(122,110);app.pointer_aux_button(3,true);CHECK(app.map_drawing.mode==MapDrawingMode::Erase&&app.map_drawing.pressed);
    app.pointer_move(122,130);app.pointer_aux_button(3,false);CHECK(app.map_drawing.mode==MapDrawingMode::None&&app.progress.map_drawing.count==stored+2);capture("map-drawing-erased");
    // The same Exhaust hotkey focuses the actual toolbar. Held confirmation
    // plus directional motion draws; key release stops without closing it.
    app.input(Key::ExhaustPile);CHECK(app.map_drawing.tool_focus==0);
    app.input(Key::Confirm);app.map_drawing_key_release(Key::Confirm);CHECK(app.map_drawing.keyboard&&app.map_drawing.mode==MapDrawingMode::Draw);
    app.input(Key::Confirm);app.input(Key::Right);app.advance(100);app.map_drawing_key_release(Key::Right);app.map_drawing_key_release(Key::Confirm);
    CHECK(!app.map_drawing.pressed&&app.map_drawing.mode==MapDrawingMode::Draw&&app.progress.map_drawing.count>=stored+4);
    app.input(Key::Back);CHECK(app.map_drawing.mode==MapDrawingMode::None&&app.map_window.is_open);
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    const unsigned count=app.progress.map_drawing.count;app.close_map_window(false);app.open_map_window(true);for(unsigned i=0;i<3;++i)app.advance(250);CHECK(app.progress.map_drawing.count==count);
    tool(2);CHECK(!app.progress.map_drawing.count&&app.map_drawing.mode==MapDrawingMode::None);capture("map-drawing-cleared");
    // Capacity remains explicit and never overwrites an earlier mark.
    auto&bounded=app.progress.map_drawing;for(unsigned i=0;i<MapDrawingCapacity;++i)CHECK(bounded.append(int(i%1921),100,MapDrawingMode::Draw,true));
    const auto first=bounded.points[0];CHECK(!bounded.append(1,1,MapDrawingMode::Draw,true));CHECK(bounded.count==MapDrawingCapacity&&bounded.points[0].x==first.x);
    app.game.world.run_seed++;app.sync_map_drawing();CHECK(!bounded.count&&app.map_drawing.mode==MapDrawingMode::None);
    std::printf("Map drawing: %u checks; paper coordinates, partial erase/redraw, pointer/held-key controls, NSH4 compatibility and bounded storage passed.\n",checks);
}
