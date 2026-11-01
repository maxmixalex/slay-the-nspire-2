// Calculator-only illumination simplification must leave body pixels and
// scene-union lifetimes intact. Source-pack appearance is checked separately.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace spire;
static App app;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"ancient effects:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 Canvas c{app};
 for(unsigned i=0;i<ScreenW*ScreenH;++i)app.pixels[i]=uint16_t(i*4793+0x187f);
 {
  SoftAddGrid grid(c);CHECK(grid.sampled());CHECK(grid.canvas().clip_w==81&&grid.canvas().clip_h==61);
  for(int value:{128,1152,4224,8192})CHECK(grid.position(value)==(value+384)/4);
  grid.canvas().rect(0,0,81,61,0x0821);grid.finish();
  for(unsigned i=0;i<ScreenW*ScreenH;++i)CHECK(app.pixels[i]==presentation::add_preweighted(uint16_t(i*4793+0x187f),0x0821));
 }
 // Input collection must never see rescaled geometry; unsupported compositing
 // retains the caller's original canvas, including its source coverage.
 uint8_t coverage=0;c.coverage=&coverage;
 {SoftAddGrid grid(c);CHECK(!grid.sampled()&&&grid.canvas()==&c);CHECK(grid.position(-217)==-217&&grid.size(391)==391);}
 c.coverage=nullptr;
 // Only one retained scene slot exists. Room transitions invalidate the
 // illumination, while changing light lookups does not damage its metadata.
 auto&room=neow_room();room.illumination.owner=&app.content;room.illumination.generation=17;
 room.illumination.valid=true;room.illumination.pixels[17484]=0x7f92;
 CHECK(room.illumination.matches(&app.content,17));CHECK(!room.illumination.matches(&app.content,18));
 character_light_quads();CHECK(neow_room().illumination.pixels[17484]==0x7f92&&neow_room().illumination.valid);
 tez_cinder_room(app);CHECK(!neow_room().illumination.valid);
 neow_room().illumination.valid=true;random_scene_rasters();CHECK(!neow_room().illumination.valid);
 CHECK(sizeof(NeowRoom)<=sizeof(TezCinderRoom));CHECK(sizeof(SceneRasterStorage)==sizeof(MerchantRugRaster));
 std::printf("Ancient calculator effects: %u checks; Neow %zu / Tez %zu; shared scene %zu bytes.\n",checks,sizeof(NeowRoom),sizeof(TezCinderRoom),sizeof(SceneRasterStorage));
}
