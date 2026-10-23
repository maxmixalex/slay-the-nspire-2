#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "world_relics.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static App a;
static SaveData before,after,disk,expected;
static State candidate;
static bool disk_exists=false,read_ok=true,write_ok=true,delete_ok=true;
static unsigned reads=0,writes=0,deletes=0;
static unsigned checks;
static const char* capture_directory;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world frontend UI: %s:%d %s (event=%s phase=%u)\n",__FILE__,__LINE__,#x,original_event_key(a.game.original_events.id),unsigned(a.game.phase));std::exit(1);}}while(0)

struct TextIndex {const char* key;uint16_t index;};
static const TextIndex world_text[]={
#include "../src/world_text_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=uint8_t(value>>(8*i));}
static bool ends(const std::string&s,const char*tail){size_t n=std::strlen(tail);return s.size()>=n&&s.compare(s.size()-n,n,tail)==0;}
static std::vector<uint8_t> synthetic_profile(){
 // Distinct glyphs and image colors make saved facts and navigation visible.
 // This is an interaction fixture, not replacement artwork.
 constexpr unsigned images=U_Count>CardKinds?U_Count:CardKinds,glyphs=4*95;
 constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16;
 constexpr unsigned clipdir=stringdir+ContentStrings*8,carddir=clipdir+AnimationKinds*16;
 constexpr unsigned uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
 std::vector<uint8_t>b(payload);
 put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,20,glyphs);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 for(unsigned i=0;i<images;i++){
  unsigned w=i==A_Logo?title_scene::AtlasWidth:i?2:82,h=i==A_Logo?title_scene::AtlasHeight:i?2:60,offset=unsigned(b.size()),length=w*h*2;
  put(b,40+i*16,offset);put(b,44+i*16,w,2);put(b,46+i*16,h,2);put(b,52+i*16,length);b.resize(offset+length);
  for(unsigned pixel=0;pixel<w*h;pixel++)put(b,offset+pixel*2,((i*13+pixel*5)&31)<<11|((i*29+pixel*7)&63)<<5|((i*7+pixel*3)&31),2);
 }
 const unsigned sizes[]={8,10,12,16};unsigned glyph=0;
 for(unsigned size:sizes)for(unsigned cp=32;cp<127;cp++,glyph++){
  unsigned at=glyphdir+glyph*16,offset=unsigned(b.size());put(b,at,cp);put(b,at+4,offset);put(b,at+8,size,2);b[at+10]=5;b[at+11]=7;put(b,at+12,6,2);b.resize(offset+35);
  for(unsigned y=0;y<7;y++)for(unsigned x=0;x<5;x++)b[offset+y*5+x]=cp==' '?0:uint8_t((cp>>((x+y*3)%7)&1)?255:0);
 }
 std::vector<std::string>strings(ContentStrings,"SYNTHETIC_UI_TEXT");
 for(unsigned i=0;i<CardKinds;i++){strings[i*3]=definitions[i].key;strings[i*3+1]="CARD_DESCRIPTION";strings[i*3+2]="UPGRADED_DESCRIPTION";}
 for(const auto&row:world_text){
  CHECK(row.index<strings.size());std::string key=row.key;
  strings[row.index]=key;
  if(ends(key,".description")||ends(key,".eventDescription")||key.find(".talk.firstVisitEver.")!=std::string::npos){
   strings[row.index]="EVENT_READER_TEXT ";for(unsigned i=0;i<40;i++)strings[row.index]+="visible rules and saved choices ";
  }
  if(key=="ancients.PROCEED.title"||key=="events.PROCEED.title")strings[row.index]="PROCEED";
  if(key=="game_over_screen.SCORE_LINE.floorsClimbed")strings[row.index]="CLIMBED {FloorCount} floors";
  if(key=="game_over_screen.SCORE_LINE.ascension")strings[row.index]="ASCENSION {AscensionLevel}";
  if(key=="game_over_screen.SCORE_LINE.goldGained")strings[row.index]="TOTAL GOLD {GoldAmount}";
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 for(unsigned i=0;i<CardKinds;i++){put(b,carddir+i*16,0);for(unsigned j=1;j<4;j++)put(b,carddir+i*16+j*4,i*3+j-1);}
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));return b;
}

static bool store_run(const State&s){++writes;if(!write_ok)return false;if(!save(s,disk))return false;disk_exists=true;return true;}
static bool restore_run(State&s){++reads;return read_ok&&disk_exists&&load(s,disk.bytes,disk.size);}
static bool erase_run(){++deletes;if(!delete_ok)return false;disk_exists=false;return true;}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void same_saved(const SaveData&target){CHECK(save(a.game,after));CHECK(after.size==target.size&&!std::memcmp(after.bytes,target.bytes,target.size));}
static void presentation(bool title=false){
 a.title=title;a.paused=a.help=a.library=a.character_select=a.settings_open=a.quit=false;a.frontend_confirm=a.frontend_choice=a.settings_item=0;
 a.run_settings=a.practice_entry=false;a.map_preview.open=false;a.menu=0;a.reset_presentation();a.pointer.visible=false;
}
static void start(Character character=Character::Ironclad,uint32_t seed=0x82179,const char*run_seed=nullptr,unsigned ascension=3,reference::ActKind region=reference::ActKind::Overgrowth){
 a.forget_run_checkpoint();new_run(a.game,seed,false,character);if(run_seed)CHECK(world_set_seed(a.game,run_seed));
 CHECK(world_begin_run(a.game,region,ascension));presentation();CHECK(validate(a.game));
}
static void empty_disk(){disk_exists=false;read_ok=write_ok=delete_ok=true;a.has_save=false;a.forget_run_checkpoint();a.callbacks={store_run,restore_run,erase_run};}
static void put_disk(){CHECK(validate(a.game));CHECK(a.save_current_run());expected=disk;}
static void disk_unchanged(){CHECK(disk_exists&&disk.size==expected.size&&!std::memcmp(disk.bytes,expected.bytes,disk.size));}
static void external_run(Character character,uint32_t seed,const char*run_seed,unsigned ascension,reference::ActKind region){
 new_run(candidate,seed,false,character);CHECK(world_set_seed(candidate,run_seed));CHECK(world_begin_run(candidate,region,ascension));CHECK(store_run(candidate));
}
static void capture(const char*name){
 if(!capture_directory)return;a.pointer.visible=false;a.render();char path[1024]{};int n=std::snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);CHECK(n>0&&unsigned(n)<sizeof path);
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;y++){for(unsigned x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static HitRegion find_hit(HitKind kind,int value){
 a.render();HitRegion found{};unsigned count=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value){found=a.hits[i];++count;}
 CHECK(count==1&&found.enabled&&found.x>=0&&found.y>=0&&found.x+found.w<=ScreenW&&found.y+found.h<=ScreenH);return found;
}
static void press(HitKind kind,int value){const auto h=find_hit(kind,value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;}
static void action(UiAction value){press(HitKind::UiAction,int(value));}
static void menu_rows(unsigned count){
 a.render();unsigned seen=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Menu){CHECK(a.hits[i].value>=0&&unsigned(a.hits[i].value)<count);seen|=1u<<a.hits[i].value;}
 CHECK(seen==((1u<<count)-1));
}
static void confirm_accept(){CHECK(a.frontend_confirm&&a.frontend_choice==0);a.input(Key::Right);CHECK(a.frontend_choice==1);a.input(Key::Confirm);CHECK(!a.frontend_confirm);}

static void title_glow_curve(){
 // Independent floating-point evaluation of the source Curve/Gradient,
 // allowing one output pixel/byte for the runtime's fixed-point rounding.
 for(unsigned ms:{0u,99u,300u,600u,1235u,1508u,2000u,2984u,3000u,0xffffffffu}){
  const double phase=double(ms%3000)/3000.;const bool rising=phase<.502521;
  const double t=rising?phase/.502521:(phase-.502521)/(1.-.502521),weight=t*t*(3.-2.*t);
  const double scale=rising?.417635+(.507939-.417635)*weight:.507939+(.423164-.507939)*weight;
  const double gray=phase<=.411546?89.+166.*phase/.411546:
      phase<.994413?255.-166.*(phase-.411546)/(.994413-.411546):89.;
  const auto sample=title_scene::glow_sample(ms);
  CHECK(std::abs(sample.width-int(std::lround(164.*scale/.507939)))<=1);
  CHECK(std::abs(sample.height-int(std::lround(104.*scale/.507939)))<=1);
  CHECK(std::abs(int(sample.brightness)-int(std::lround(gray)))<=1);
 }
 const auto first=title_scene::glow_sample(0),peak=title_scene::glow_sample(1508);
 CHECK(first.width==135&&first.height==86&&first.brightness==89&&peak.width==164&&peak.height==104);
}
static void title_particle_curves(){
 // Numerical integration of the original Hermite damping/constant gravity
 // is independent of the runtime's fixed-point closed-form antiderivative.
 for(unsigned age:{0u,1000u,100000u,200000u,399999u,400000u})for(unsigned damping:{0u,32768u,65535u}){
  const double seconds=age/1000.,dt=seconds/8000.;double distance=0;
  for(unsigned i=0;i<8000;++i){const double t=(i+.5)*dt,u=t/400.,u2=u*u,u3=u2*u;
   const double curve=(2*u3-3*u2+1)+(-2*u3+3*u2)*100+(u3-u2)*130.445;
   const double acceleration=.1-double(damping)/65535.*.001*curve;
   distance+=(seconds-t)*acceleration*dt;
  }
  CHECK(std::abs(double(title_scene::fog_displacement(age,damping))/65536.-distance)<.5);
 }
 for(unsigned time:{0u,1u,1294u,4999u,5000u,100001u,399999u,400000u,0xffffffffu}){
  for(unsigned i=0;i<title_scene::StarCount;++i){const auto p=title_scene::star_sample(i,time);
   CHECK(p.width>=1&&p.width<=2&&p.width==p.height&&p.opacity<=255);
   CHECK(p.x>=-182&&p.x<=486&&p.y>=71&&p.y<=206);
   CHECK(p.red<=71&&p.green<=214&&p.blue<=224);
  }
  for(unsigned layer=0;layer<2;++layer)for(unsigned i=0;i<title_scene::FogCount;++i){const auto p=title_scene::fog_sample(layer,i,time);
   CHECK(p.width>=101&&p.width<=135&&p.height>=40&&p.height<=54&&p.frame<7);
   CHECK(p.y>=155&&p.y+p.height<=219&&p.red==74&&p.green==90&&p.blue==99);
   CHECK(p.opacity==(layer?255u:112u));
  }
 }
 // Animation speed is zero: one birth chooses a shape, not a flipbook.
 for(unsigned layer=0;layer<2;++layer)for(unsigned i=0;i<title_scene::FogCount;++i){
  const auto first=title_scene::fog_sample(layer,i,1),later=title_scene::fog_sample(layer,i,1001);
  CHECK(first.frame==later.frame&&first.width==later.width&&first.height==later.height);
 }
}
static void title_background_layers(){
 CHECK(a.content.version()==TitleEffectsContentVersion);
 const auto backdrop=a.content.image(a.content.ui_image(U_TitleBackdrop));CHECK(backdrop.pixels&&backdrop.w==320&&backdrop.h==240);
 const auto particles=a.content.image(a.content.ui_image(U_TitleParticles));CHECK(particles.pixels&&particles.w==270&&particles.h==216);
 const auto foreground=a.content.image(a.content.ui_image(U_TitleForeground));CHECK(foreground.pixels&&foreground.w==320&&foreground.h==240);
 std::vector<uint16_t>colors(ScreenW*ScreenH);std::vector<uint8_t>opaque(colors.size());
 if(foreground.format==4){
  const uint8_t*run=foreground.pixels;
  for(unsigned y=0;y<240;++y){const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
   for(unsigned s=0;s<spans;++s){const unsigned x=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,stride=run[4]?3u:2u;run+=5;
    for(unsigned j=0;j<n;++j,run+=stride){const unsigned at=y*320+x+j;colors[at]=uint16_t(run[0]|unsigned(run[1])<<8);opaque[at]=stride==2||run[2]==255;}
   }
  }
 }else for(unsigned at=0;at<colors.size();++at){
  const uint8_t*pixel=foreground.palette?foreground.palette+2*foreground.pixels[at]:foreground.pixels+2*at;
  colors[at]=uint16_t(pixel[0]|unsigned(pixel[1])<<8);opaque[at]=!foreground.alpha||foreground.alpha[at]==255;
 }
 a.map_clock_ms=50;a.render();std::vector<uint16_t>first(a.pixels,a.pixels+ScreenW*ScreenH);capture("frontend-title-particles-50ms");
 unsigned visible_changes=0,covered=0;
 for(unsigned time:{1250u,25050u,100050u}){
  a.map_clock_ms=time;a.render();
  for(unsigned y=0;y<ScreenH;++y)for(unsigned x=0;x<ScreenW;++x){const unsigned at=y*ScreenW+x;
   const bool logo=x>=33&&x<204&&y>=27&&y<132;
   if(a.pixels[at]!=first[at]){CHECK(logo||(y>=70&&y<219));visible_changes+=!logo;}
   // No menu, logo or footer occupies this right-hand foreground region.
   // Fully opaque buildings must remain above both particle emitters.
   if(x>=224&&y>=70&&y<225&&opaque[at]){CHECK(a.pixels[at]==colors[at]);++covered;}
  }
  if(time==1250)capture("frontend-title-glow-bright");
  if(time==25050)capture("frontend-title-particles-25s");
  if(time==100050)capture("frontend-title-particles-100s");
 }
 CHECK(visible_changes>0&&covered>0);CHECK(a.content.healthy());unchanged();
}
static void title_animation_is_bounded_and_repeats(){
 empty_disk();start();snapshot();a.return_to_title();a.menu=0;a.map_clock_ms=50;a.render();
 if(a.content.version()>=TitleEffectsContentVersion){title_background_layers();return;}
 const auto logo=a.content.image(A_Logo);
 const bool glow=logo.h==title_scene::AtlasHeight;
 if(logo.w!=title_scene::AtlasWidth||(!glow&&logo.h!=title_scene::LegacyAtlasHeight))return; // Historical static logo.
 std::vector<uint16_t>first(a.pixels,a.pixels+ScreenW*ScreenH);unsigned different_frames=0;
 auto in_fire=[](unsigned x,unsigned y){return x>=title_scene::FireX&&x<title_scene::FireX+title_scene::FireWidth&&y>=title_scene::FireY&&y<title_scene::FireY+title_scene::FireHeight;};
 auto in_glow=[](unsigned x,unsigned y){return x>=title_scene::GlowCenterX-title_scene::AtlasWidth/2&&x<title_scene::GlowCenterX+(title_scene::AtlasWidth+1)/2&&y>=title_scene::GlowCenterY-title_scene::GlowHeight/2&&y<title_scene::GlowCenterY+(title_scene::GlowHeight+1)/2;};
 for(unsigned frame=1;frame<title_scene::FireFrames;++frame){
  a.map_clock_ms=frame*title_scene::FireFrameMs+50;a.render();bool changed=false;
  for(unsigned y=0;y<ScreenH;++y)for(unsigned x=0;x<ScreenW;++x)if(a.pixels[y*ScreenW+x]!=first[y*ScreenW+x]){
   CHECK(in_fire(x,y)||(glow&&in_glow(x,y)));changed=true;
  }
  different_frames+=changed;
 }
 CHECK(different_frames>=2);
 if(glow){
  // The flame has the same frame after 1s. A changed pixel outside its
  // rectangle proves the independent light has actually been rendered.
  a.map_clock_ms=1050;a.render();unsigned changed=0;
  for(unsigned y=0;y<ScreenH;++y)for(unsigned x=0;x<ScreenW;++x)if(a.pixels[y*ScreenW+x]!=first[y*ScreenW+x]){CHECK(in_fire(x,y)||in_glow(x,y));changed+=!in_fire(x,y);}
  CHECK(changed>0);capture("frontend-title-glow-bright");
 }
 a.map_clock_ms=(glow?title_scene::GlowPeriodMs:title_scene::FireFrames*title_scene::FireFrameMs)+50;a.render();
 CHECK(!std::memcmp(first.data(),a.pixels,sizeof a.pixels));unchanged();
}
static void probe_is_read_only(){
 empty_disk();start(Character::Silent,1001);const uint32_t saved_seed=a.game.seed;const int hp=a.game.hp,max_hp=a.game.max_hp,floor=a.game.world.total_floor,gold=a.game.gold;put_disk();
 start(Character::Regent,1002);snapshot();unsigned r=reads;
 CHECK(a.refresh_save()&&a.has_save&&reads==r+1);CHECK(a.saved_character==Character::Silent&&a.saved_seed==saved_seed&&a.saved_hp==hp&&a.saved_max_hp==max_hp&&a.saved_floor==floor&&a.saved_gold==gold&&a.saved_world&&!a.saved_practice&&a.saved_ascension==3);unchanged();
 a.return_to_title();CHECK(a.title&&a.has_save&&a.title_menu_count()==5);unchanged();
 read_ok=false;CHECK(!a.refresh_save()&&!a.has_save);unchanged();read_ok=true;
 disk.bytes[0]^=1;CHECK(!a.refresh_save()&&!a.has_save);unchanged();disk.bytes[0]^=1;CHECK(a.refresh_save());unchanged();
}
static void main_menu_and_continue(){
 empty_disk();start();snapshot();a.return_to_title();CHECK(a.title_menu_count()==4);menu_rows(4);capture("frontend-title-new");
 a.menu=0;for(unsigned i=0;i<4;i++)a.input(Key::Down);CHECK(a.menu==0);unchanged();
 press(HitKind::Menu,0);CHECK(a.character_select&&a.title&&!a.practice_entry);a.input(Key::Back);CHECK(!a.character_select&&a.title);unchanged();
 press(HitKind::Menu,1);CHECK(a.settings_open);action(UiAction::SettingsBack);CHECK(!a.settings_open);unchanged();
 press(HitKind::Menu,2);CHECK(a.library&&a.title);a.input(Key::Back);CHECK(!a.library&&a.title);unchanged();
 press(HitKind::Menu,3);CHECK(a.frontend_confirm==3&&!a.quit);capture("frontend-quit-confirm");a.input(Key::Confirm);CHECK(!a.frontend_confirm&&!a.quit);unchanged();
 press(HitKind::Menu,3);confirm_accept();CHECK(a.quit);a.quit=false;unchanged();

 start(Character::Silent,2001);put_disk();start(Character::Defect,2002);snapshot();a.return_to_title();CHECK(a.title_menu_count()==5);menu_rows(5);
 a.pointer_move(213,183);CHECK(a.menu==0);a.pointer.visible=false;capture("frontend-title-continue");
 a.menu=0;for(unsigned i=0;i<5;i++)a.input(Key::Down);CHECK(a.menu==0);unchanged();
 press(HitKind::Menu,2);CHECK(a.settings_open);a.input(Key::Back);CHECK(!a.settings_open);unchanged();
 press(HitKind::Menu,3);CHECK(a.library&&a.title);a.input(Key::Back);CHECK(!a.library);unchanged();
 press(HitKind::Menu,4);CHECK(a.frontend_confirm==3);a.input(Key::Back);CHECK(!a.frontend_confirm&&!a.quit);unchanged();
 press(HitKind::Menu,0);CHECK(!a.title&&!a.character_select&&a.game.character==Character::Silent);same_saved(expected);
 // A decodable terminal save is not resumable. The shortcut must not replace
 // the visible run before this admission check has succeeded.
 player_kill(a.game,true);resolve_pending(a.game);CHECK(a.game.phase==Phase::Dead);put_disk();
 start(Character::Regent,2003);snapshot();a.return_to_title();CHECK(!a.has_save);a.input(Key::Load);CHECK(a.title&&!a.has_save);unchanged();
 disk.bytes[0]^=1;a.input(Key::Load);CHECK(a.title&&!a.has_save);unchanged();disk.bytes[0]^=1;
}
static void pause_save_failure_and_success(){
 empty_disk();start(Character::Necrobinder,3001);snapshot();a.input(Key::Back);CHECK(a.paused&&!a.title);menu_rows(5);capture("frontend-pause");
 unsigned w=writes;write_ok=false;press(HitKind::Menu,4);CHECK(a.paused&&!a.title&&!disk_exists&&writes==w+1&&a.notice_ticks);unchanged();
 write_ok=true;press(HitKind::Menu,4);CHECK(a.title&&!a.paused&&a.has_save&&disk_exists);same_saved(disk);unchanged();
 // The saved expedition remains exactly resumable after returning to title.
 a.game.gold+=9;CHECK(validate(a.game));a.menu=0;a.input(Key::Confirm);CHECK(!a.title&&!a.paused);same_saved(disk);
 a.input(Key::Back);CHECK(a.paused);press(HitKind::Menu,1);CHECK(a.settings_open&&a.paused);a.input(Key::Back);CHECK(!a.settings_open&&a.paused);
 press(HitKind::Menu,2);CHECK(a.library&&a.paused);a.input(Key::Back);CHECK(!a.library&&a.paused);press(HitKind::Menu,0);CHECK(!a.paused&&!a.title);same_saved(disk);
}
static void abandon_saved_with_confirmation(){
 empty_disk();start(Character::Regent,4001);put_disk();snapshot();a.return_to_title();unsigned d=deletes;
 press(HitKind::Menu,1);CHECK(a.frontend_confirm==1&&a.frontend_choice==0&&disk_exists&&deletes==d);capture("frontend-abandon-confirm");
 // Cancel is initially selected; one Enter cannot destroy the saved game.
 a.input(Key::Confirm);CHECK(!a.frontend_confirm&&a.has_save&&disk_exists&&deletes==d);unchanged();
 press(HitKind::Menu,1);delete_ok=false;confirm_accept();CHECK(a.title&&a.has_save&&disk_exists&&deletes==d+1&&a.notice_ticks);unchanged();
 delete_ok=true;press(HitKind::Menu,1);action(UiAction::FrontendAccept);CHECK(a.title&&!a.has_save&&!disk_exists&&deletes==d+2&&a.title_menu_count()==4);unchanged();menu_rows(4);
}
static void abandon_current_bypasses_prevention(){
 empty_disk();start(Character::Ironclad,5001);a.game.potion_slots=8;for(auto&p:a.game.potions)p=Potion::Empty;a.game.potions[7]=Potion::FairyInABottle;
 CHECK(gain_world_relic(a.game,WorldRelicLizardTail));resolve_pending(a.game);put_disk();snapshot();unsigned d=deletes;
 a.input(Key::Back);CHECK(a.paused);press(HitKind::Menu,3);CHECK(a.frontend_confirm==2&&deletes==d);action(UiAction::FrontendCancel);CHECK(a.paused&&a.game.hp>0&&disk_exists);unchanged();
 delete_ok=false;press(HitKind::Menu,3);confirm_accept();CHECK(a.paused&&a.game.hp>0&&disk_exists&&deletes==d+1);unchanged();
 delete_ok=true;press(HitKind::Menu,3);confirm_accept();CHECK(!a.paused&&!a.title&&a.game.phase==Phase::Dead&&a.game.world.finished&&a.game.hp==0&&!disk_exists&&deletes==d+2);
 CHECK(a.game.potions[7]==Potion::FairyInABottle&&world_relic_counter(a.game,WorldRelicLizardTail)==0&&a.game.event_run.completion_recorded);snapshot();
 a.input(Key::Back);CHECK(a.paused);a.render();bool disabled=false;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Menu&&a.hits[i].value==3)disabled=!a.hits[i].enabled;CHECK(disabled);
 a.menu=3;a.input(Key::Confirm);CHECK(a.paused&&!a.frontend_confirm);a.menu=0;a.input(Key::Confirm);CHECK(!a.paused);unchanged();
}
static void protected_saved_expeditions(){
 // A different seed must survive a completed practice/expedition's end UI.
 empty_disk();start(Character::Ironclad,6001);put_disk();start(Character::Ironclad,6002);player_kill(a.game,true);resolve_pending(a.game);snapshot();unsigned d=deletes;
 CHECK(a.finish_saved_run()&&disk_exists&&deletes==d);unchanged();a.input(Key::Confirm);CHECK(a.game_over_summary);action(UiAction::GameOverMainMenu);CHECK(a.title&&a.has_save&&disk_exists&&deletes==d);unchanged();
 a.menu=0;a.input(Key::Confirm);CHECK(!a.title);same_saved(expected);
 // Equal seed with a different character is also a distinct saved run.
 empty_disk();start(Character::Silent,6101);put_disk();start(Character::Defect,6101);player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;
 CHECK(a.finish_saved_run()&&disk_exists&&deletes==d);unchanged();
 // Isolate the practice bit with two legacy states, for which world=false
 // and the seed/character are equal. A finished practice cannot erase a run.
 empty_disk();new_run(a.game,6201,false,Character::Ironclad);presentation();put_disk();new_run(a.game,6201,true,Character::Ironclad);presentation();player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;
 CHECK(a.game.practice&&!a.game.world.active&&a.finish_saved_run()&&disk_exists&&deletes==d);unchanged();
}
static void canonical_identity_and_save_association(){
 // A fresh App starts with the same bootstrap seed. Native seed text changes
 // the independent 64-bit original run seed, not that bootstrap identity.
 empty_disk();start(Character::Ironclad,6301,"SAVED_NATIVE_SEED");const uint64_t native_seed=a.game.world.run_seed;put_disk();
 start(Character::Ironclad,6301,"OTHER_NATIVE_SEED");CHECK(a.game.seed==6301&&a.game.world.run_seed!=native_seed);
 player_kill(a.game,true);resolve_pending(a.game);snapshot();unsigned d=deletes;
 CHECK(a.finish_saved_run()&&deletes==d&&a.saved_run_seed==native_seed);disk_unchanged();unchanged();
 // Ascension and initial region are equally material native parameters.
 empty_disk();start(Character::Ironclad,6302,"MATCHED_NATIVE_SEED",3);put_disk();
 start(Character::Ironclad,6302,"MATCHED_NATIVE_SEED",4);player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;
 CHECK(a.finish_saved_run()&&deletes==d&&a.saved_ascension==3);disk_unchanged();unchanged();
 empty_disk();start(Character::Ironclad,6303,"MATCHED_NATIVE_SEED",3,reference::ActKind::Overgrowth);put_disk();
 start(Character::Ironclad,6303,"MATCHED_NATIVE_SEED",3,reference::ActKind::Underdocks);player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;
 CHECK(a.finish_saved_run()&&deletes==d&&a.saved_region==unsigned(reference::ActKind::Overgrowth));disk_unchanged();unchanged();

 // Identical parameters across hosts are not a global run ID. Without a
 // successful Continue/save association, cleanup must preserve the old run.
 empty_disk();start(Character::Silent,6304,"SAME_PARAMETERS");put_disk();start(Character::Silent,6304,"SAME_PARAMETERS");
 CHECK(!a.run_save_linked);player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;unsigned r=reads;
 CHECK(a.finish_saved_run()&&deletes==d&&reads==r);disk_unchanged();unchanged();
 CHECK(a.load_saved_run()&&a.run_save_linked);same_saved(expected);
 player_kill(a.game,true);resolve_pending(a.game);CHECK(a.finish_saved_run()&&!disk_exists&&deletes==d+1);

 // Progress does not change the associated run. Cleanup has no content
 // fingerprint and accepts a normal save with changed gold or HP.
 empty_disk();start(Character::Silent,6305,"SAME_PROGRESS_PARAMETERS");put_disk();CHECK(a.run_save_linked);
 CHECK(load(candidate,disk.bytes,disk.size));candidate.gold+=7;CHECK(validate(candidate));CHECK(store_run(candidate));expected=disk;
 a.game.gold+=11;player_kill(a.game,true);resolve_pending(a.game);snapshot();d=deletes;
 CHECK(a.finish_saved_run()&&deletes==d+1&&!disk_exists&&!a.run_save_linked);unchanged();
}
static void refreshed_continue_metadata(){
 empty_disk();start(Character::Silent,6401,"OLD_CACHED_SAVE",3);put_disk();start(Character::Defect,6402);snapshot();a.return_to_title();
 CHECK(a.saved_character==Character::Silent&&a.saved_seed==6401);unchanged();
 external_run(Character::Regent,6403,"REPLACED_BEFORE_CONTINUE",7,reference::ActKind::Underdocks);expected=disk;
 const uint64_t actual_seed=candidate.world.run_seed;const int actual_hp=candidate.hp,actual_max_hp=candidate.max_hp;
 press(HitKind::Menu,0);CHECK(!a.title&&a.game.character==Character::Regent);same_saved(expected);
 CHECK(a.saved_character==Character::Regent&&a.saved_seed==6403&&a.saved_run_seed==actual_seed&&a.saved_ascension==7&&a.saved_region==unsigned(reference::ActKind::Underdocks));
 CHECK(a.saved_hp==actual_hp&&a.saved_max_hp==actual_max_hp&&a.saved_gold==a.game.gold&&a.saved_floor==a.game.world.total_floor&&a.saved_world&&!a.saved_practice);
 CHECK(a.saved_current_region==unsigned(reference::ActKind::Underdocks));
 CHECK(a.run_save_linked);
 player_kill(a.game,true);resolve_pending(a.game);unsigned d=deletes;CHECK(a.finish_saved_run()&&!disk_exists&&deletes==d+1);
}
static void title_confirmation_tracks_run_identity(){
 empty_disk();start(Character::Ironclad,6501,"TITLE_SAVED_SEED");put_disk();start(Character::Defect,6502);snapshot();a.return_to_title();unsigned d=deletes;
 // Even opening the dialog requires a successful fresh read.
 read_ok=false;press(HitKind::Menu,1);CHECK(!a.frontend_confirm&&!a.has_save&&a.notice_ticks&&deletes==d);disk_unchanged();unchanged();
 read_ok=true;CHECK(a.refresh_save());press(HitKind::Menu,1);CHECK(a.frontend_confirm==1);
 external_run(Character::Regent,6503,"TITLE_REPLACEMENT",6,reference::ActKind::Underdocks);expected=disk;
 confirm_accept();CHECK(a.title&&a.has_save&&deletes==d&&a.saved_character==Character::Regent&&a.saved_seed==6503);disk_unchanged();unchanged();
 press(HitKind::Menu,1);read_ok=false;confirm_accept();CHECK(a.title&&deletes==d&&a.notice_ticks);disk_unchanged();unchanged();
 // The same run remains selected after ordinary progress changes. A content
 // hash must not add another dialog or a redundant post-deletion read.
 read_ok=true;CHECK(a.refresh_save());press(HitKind::Menu,1);CHECK(a.frontend_confirm==1);
 candidate.gold+=13;CHECK(store_run(candidate));expected=disk;unsigned r=reads;
 confirm_accept();CHECK(a.title&&!a.has_save&&!disk_exists&&deletes==d+1&&reads==r+1);unchanged();
}
static void unreadable_save_does_not_abandon(){
 empty_disk();start(Character::Necrobinder,6601,"READ_FAILURE");put_disk();snapshot();unsigned d=deletes;
 a.input(Key::Back);CHECK(a.paused);press(HitKind::Menu,3);read_ok=false;
 // A previous failed menu probe must not erase the known association.
 CHECK(!a.refresh_save()&&!a.has_save&&a.run_save_linked);
 confirm_accept();CHECK(a.paused&&a.game.hp>0&&a.game.phase!=Phase::Dead&&deletes==d&&a.notice_ticks);disk_unchanged();unchanged();
 read_ok=true;press(HitKind::Menu,3);confirm_accept();CHECK(!a.paused&&a.game.phase==Phase::Dead&&!disk_exists&&deletes==d+1);
}
static void optional_clear_uses_successful_terminal_probe(){
 empty_disk();start(Character::Ironclad,6701,"FALLBACK_ACTIVE");a.game.potion_slots=8;for(auto&p:a.game.potions)p=Potion::Empty;a.game.potions[7]=Potion::FairyInABottle;
 CHECK(gain_world_relic(a.game,WorldRelicLizardTail));resolve_pending(a.game);put_disk();snapshot();a.callbacks.clear_save=nullptr;
 unsigned d=deletes,w=writes;a.input(Key::Back);press(HitKind::Menu,3);write_ok=false;confirm_accept();
 CHECK(a.paused&&a.game.hp>0&&a.game.phase!=Phase::Dead&&deletes==d&&writes==w+1);disk_unchanged();unchanged();
 write_ok=true;press(HitKind::Menu,3);confirm_accept();CHECK(!a.paused&&a.game.phase==Phase::Dead&&a.game.hp==0&&a.game.world.finished&&disk_exists&&!a.has_save&&deletes==d&&writes==w+2);
 CHECK(a.game.potions[7]==Potion::FairyInABottle&&world_relic_counter(a.game,WorldRelicLizardTail)==0&&a.game.event_run.completion_recorded);same_saved(disk);
 CHECK(load(candidate,disk.bytes,disk.size)&&candidate.phase==Phase::Dead);snapshot();CHECK(!a.refresh_save()&&!a.load_saved_run());unchanged();

 // Title abandonment writes a terminal version of the selected save while
 // retaining the unrelated visible game byte-for-byte, on failure or success.
 empty_disk();start(Character::Silent,6702,"FALLBACK_TITLE");put_disk();start(Character::Defect,6703);snapshot();a.return_to_title();a.callbacks.clear_save=nullptr;
 d=deletes;w=writes;press(HitKind::Menu,1);write_ok=false;confirm_accept();CHECK(a.title&&a.has_save&&writes==w+1&&deletes==d);disk_unchanged();unchanged();
 write_ok=true;press(HitKind::Menu,1);confirm_accept();CHECK(a.title&&!a.has_save&&disk_exists&&writes==w+2&&deletes==d);unchanged();
 CHECK(load(candidate,disk.bytes,disk.size)&&candidate.phase==Phase::Dead&&candidate.character==Character::Silent);CHECK(!a.load_saved_run());unchanged();

 // Ending an already terminal visible run also requires the fallback write
 // to succeed before forgetting its earlier resumable checkpoint.
 empty_disk();start(Character::Regent,6704,"FALLBACK_ENDING");put_disk();player_kill(a.game,true);resolve_pending(a.game);snapshot();a.callbacks.clear_save=nullptr;
 w=writes;write_ok=false;CHECK(!a.finish_saved_run()&&writes==w+1);disk_unchanged();unchanged();
 write_ok=true;CHECK(a.finish_saved_run()&&!a.has_save&&disk_exists&&writes==w+2);same_saved(disk);unchanged();a.callbacks.clear_save=erase_run;
}
static void ending_clears_only_its_checkpoint(){
 empty_disk();start(Character::Ironclad,7001);put_disk();player_kill(a.game,true);resolve_pending(a.game);presentation();snapshot();unsigned d=deletes;
 a.input(Key::Confirm);CHECK(a.game_over_summary);delete_ok=false;action(UiAction::GameOverMainMenu);CHECK(!a.title&&a.game_over_summary&&disk_exists&&deletes==d+1);unchanged();
 delete_ok=true;action(UiAction::GameOverMainMenu);CHECK(a.title&&!a.has_save&&!disk_exists&&deletes==d+2);unchanged();
 a.input(Key::Load);CHECK(a.title&&!a.has_save&&a.game.phase==Phase::Dead);unchanged();
}
static void modal_isolation_and_fast_mode(){
 empty_disk();start(Character::Silent,8001);snapshot();a.return_to_title();a.menu=0;auto h=find_hit(HitKind::Menu,0);
 a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.frontend_confirm=3;a.frontend_choice=0;a.pointer_button(false);CHECK(!a.character_select&&!a.quit&&a.frontend_confirm==3);unchanged();
 a.character_select=true;unsigned run=a.run_number;a.ui_action(UiAction::StartCharacter);CHECK(a.run_number==run);a.character_select=false;
 a.input(Key::Load);a.input(Key::Save);a.input(Key::EndTurn);CHECK(a.frontend_confirm==3&&!a.quit);unchanged();
 a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind==HitKind::UiAction&&(a.hits[i].value==int(UiAction::FrontendAccept)||a.hits[i].value==int(UiAction::FrontendCancel)));
 action(UiAction::FrontendCancel);CHECK(!a.frontend_confirm);unchanged();
 press(HitKind::Menu,1);CHECK(a.settings_open);a.character_select=true;a.ui_action(UiAction::StartCharacter);CHECK(a.run_number==run);a.character_select=false;unchanged();
 action(UiAction::SettingsControls);CHECK(a.help&&a.settings_open);bool fast=a.fast_mode;a.input(Key::Confirm);CHECK(!a.help&&a.settings_open&&a.fast_mode==fast);unchanged();
 action(UiAction::SettingsFastMode);CHECK(a.fast_mode!=fast);action(UiAction::SettingsFastMode);CHECK(a.fast_mode==fast);unchanged();
 // Fast changes command waits, not the animation clock. No rules, draws,
 // event offers, RNG stream or encoded field changes during these ticks.
 a.animation=1000;a.tick_remainder=0;a.busy=a.card_fly=a.feedback=a.notice_ticks=12;a.fast_mode=false;
 a.advance(66);CHECK(a.animation==1002&&a.busy==10&&a.card_fly==10);unchanged();
 action(UiAction::SettingsFastMode);CHECK(a.fast_mode);a.advance(66);CHECK(a.animation==1004&&a.busy==8&&a.card_fly==8);a.render();unchanged();
 action(UiAction::SettingsBack);CHECK(!a.settings_open);a.fast_mode=false;
}
int main(int argc,char**argv){
 std::vector<uint8_t>data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=synthetic_profile();
 if(argc>2)capture_directory=argv[2];CHECK(a.init(data.data(),data.size(),{store_run,restore_run,erase_run}));CHECK(a.content.current_profile());
 title_glow_curve();title_particle_curves();title_animation_is_bounded_and_repeats();probe_is_read_only();main_menu_and_continue();pause_save_failure_and_success();abandon_saved_with_confirmation();abandon_current_bypasses_prevention();protected_saved_expeditions();
 canonical_identity_and_save_association();refreshed_continue_metadata();title_confirmation_tracks_run_identity();unreadable_save_does_not_abandon();optional_clear_uses_successful_terminal_probe();
 ending_clears_only_its_checkpoint();modal_isolation_and_fast_mode();
 std::printf("world frontend UI: %u checks passed; %s profile; guarded save lifecycle and presentation-only settings\n",checks,argc>1?"actual":"synthetic");
}
