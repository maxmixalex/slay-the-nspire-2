#include "app.hpp"
#include "loading_screen.hpp"
#include "resource_runtime_guard.hpp"
#include "resource_transfer.hpp"
#include "inplace_reset.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks,opens,writes,renames,unlinks,settings_attempts;
static const char*scenario="setup";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"resource failure [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static App app;
static ResourceRuntimeGuard resource_runtime;
static bool resource_mutation_allowed(){return resource_runtime.allow(app.content.healthy());}
static std::map<std::string,Bytes> files;
struct File {std::string path;size_t offset=0;};
static void*nd_open(const char*path,const char*mode){
 ++opens;if(*mode=='w')files[path]={};else if(!files.count(path))return nullptr;return new File{path};
}
static unsigned nd_read(void*out,unsigned size,unsigned count,void*handle){
 auto&f=*static_cast<File*>(handle);const auto&b=files.at(f.path);
 const size_t n=std::min(size_t(size)*count,b.size()-f.offset);
 if(n)std::memcpy(out,b.data()+f.offset,n);f.offset+=n;return unsigned(n/size);
}
static unsigned nd_write(const void*in,unsigned size,unsigned count,void*handle){
 ++writes;auto&f=*static_cast<File*>(handle);const auto*p=static_cast<const uint8_t*>(in);
 auto&b=files.at(f.path);b.insert(b.end(),p,p+size*count);f.offset+=size*count;return count;
}
static int nd_close(void*handle){delete static_cast<File*>(handle);return 0;}
static int nd_seek(void*handle,long offset,int origin){
 auto&f=*static_cast<File*>(handle);const long base=origin==2?long(files.at(f.path).size()):origin==1?long(f.offset):0;
 if(offset< -base||static_cast<unsigned long>(base+offset)>files.at(f.path).size())return -1;
 f.offset=size_t(base+offset);return 0;
}
static long nd_tell(void*handle){return long(static_cast<File*>(handle)->offset);}
static int nd_rename(const char*from,const char*to){
 ++renames;auto it=files.find(from);if(it==files.end()||files.count(to))return -1;
 files.emplace(to,std::move(it->second));files.erase(it);return 0;
}
static int nd_unlink(const char*path){++unlinks;return files.erase(path)?0:-1;}
static char save_path[512],temp_path[512],backup_path[512],progress_path[512],progress_temp[512],progress_backup[512];
static char settings_path[512],settings_temp[512],settings_backup[512],language_path[512];
static unsigned installed_languages=3,pending_language=0;
// These are the actual calculator functions, with only OS file primitives
// replaced. Failed writes must not even open a temporary state file.
#include "../platform/ndless/profile_storage.inc"
#include "../platform/ndless/run_storage.inc"
#include "../platform/ndless/progress_storage.inc"
#include "../platform/ndless/preferences_storage.inc"
#include "../platform/ndless/feedback_storage.inc"
#include "../platform/ndless/language_storage.inc"
static SaveData canonical;
static bool tracked_settings(const UiPreferences&value){++settings_attempts;return save_preferences(value);}
static void put(Bytes&b,unsigned at,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(v>>(8*i));}
struct Resource {
 Bytes bytes;bool fail=false;unsigned failures=0,reads=0;
 Resource(){
  // A valid synthetic current profile, with separate cold image/text pages.
  // All pictures are one invented pixel; no original text/art is embedded.
  constexpr unsigned images=A_Count;
  const unsigned text=40+images*16,clips=text+ContentStrings*8;
  const unsigned cards=clips+AnimationKinds*16,ui=cards+CardKinds*16;
  const unsigned end=ui+U_Count*4,pixels=((end+4095)/4096+1)*4096,empty=pixels+4096;
  bytes.resize(empty+4096);put(bytes,0,0x3250534e);put(bytes,4,ContentVersion);put(bytes,8,0x41cef1ea);
  put(bytes,12,unsigned(bytes.size()));put(bytes,16,images);put(bytes,24,ContentStrings);
  put(bytes,28,AnimationKinds);put(bytes,32,CardKinds);
  for(unsigned i=0;i<images;++i){put(bytes,40+i*16,pixels);put(bytes,44+i*16,1,2);put(bytes,46+i*16,1,2);put(bytes,52+i*16,2);}
  for(unsigned i=0;i<ContentStrings;++i)put(bytes,text+i*8,empty);
  for(unsigned i=0;i<AnimationKinds;++i){put(bytes,clips+i*16+4,1,2);put(bytes,clips+i*16+6,33,2);}
  bytes[pixels]=0x34;bytes[pixels+1]=0x12;
 }
 static bool read(void*context,uint32_t at,void*out,uint32_t count){
  auto&r=*static_cast<Resource*>(context);++r.reads;
  if(r.fail||at>r.bytes.size()||count>r.bytes.size()-at){++r.failures;return false;}
  std::memcpy(out,r.bytes.data()+at,count);return true;
 }
 ResourceSource source(){return {this,uint32_t(bytes.size()),read};}
};
static void reset_counts(){opens=writes=renames=unlinks=settings_attempts=0;}
static Callbacks callbacks(){
 Callbacks cb{};cb.save=save_game;cb.load=load_game;cb.clear_save=clear_save;
 cb.load_preferences=load_preferences;cb.save_preferences=tracked_settings;
 cb.load_progress=load_progress;cb.save_progress=save_progress;cb.current_profile=current_profile;
 cb.profile_info=profile_info;cb.request_profile=request_profile;cb.available_languages=available_languages;
 cb.request_language=request_language;cb.write_feedback=write_feedback;return cb;
}
static void setup(Resource&r,bool arm=true){
 reset_in_place(app);resource_runtime={};files.clear();reset_counts();pending_profile=pending_language=0;pending_profile_delete=false;
 selected_profile=1;profile_launcher="/game/nSpire2.tns";CHECK(configure_profile_paths(1));
 std::strcpy(selector_path,"/game/state/nSpire2.slots.tns");std::strcpy(selector_temp,"/game/state/nSpire2.slots.tmp.tns");
 std::strcpy(selector_backup,"/game/state/nSpire2.slots.bak.tns");std::strcpy(language_path,"/game/state/nSpire2.lang.tns");
 new_run(app.game,91,true);CHECK(save(app.game,canonical));files[save_path]=Bytes(canonical.bytes,canonical.bytes+canonical.size);
 files[backup_path]={4,3,2,1};files[progress_path]={8,7,6,5};files[settings_path]={2,1};files[language_path]={9};
 const auto before=files;r.fail=false;r.failures=r.reads=0;CHECK(app.init(r.source(),callbacks()));
 CHECK(app.content.healthy()&&files==before&&!writes&&!renames&&!unlinks);
 app.progress.tutorial_flags=0;app.progress_dirty=false;app.title=false;app.quit=false;
 app.reset_presentation();if(arm)CHECK(resource_runtime.start(app.content.healthy()));reset_counts();
}
static void preparation(){
 app.sync_combat_preparation();CHECK(app.combat_preparation.active&&app.combat_preparation.count);
 app.combat_preparation.shown=true;app.progress_dirty=true;
}
static void no_mutation(const std::map<std::string,Bytes>&before){
 CHECK(files==before&&!opens&&!writes&&!renames&&!unlinks&&!resource_transfer_uses);
}
static void rejected_callbacks(const std::map<std::string,Bytes>&before){
 reset_counts();CHECK(!save_game(app.game));CHECK(!save_progress(app.progress));CHECK(!clear_save());
 CHECK(!save_preferences({}));CHECK(!write_feedback("synthetic",9));CHECK(!store_profile_selector(2));
 CHECK(!erase_profile_files(1));CHECK(!request_profile(2,false));CHECK(!request_profile(2,true));
 store_language(BootLanguage::Chinese);CHECK(!request_language(2));
 pending_profile=2;pending_profile_delete=true;service_profile();CHECK(!pending_profile&&!pending_profile_delete);
 no_mutation(before);
}
static void startup_and_normal_paths(){
 scenario="startup candidates remain read-only until one complete initialization";Resource r;setup(r,false);
 const auto before=files;rejected_callbacks(before);CHECK(!resource_runtime.failed());
 CHECK(!resource_runtime.start(false)&&!resource_runtime.failed());
 CHECK(resource_runtime.start(app.content.healthy())&&!resource_runtime.start(true));
 scenario="normal save/preferences/progress/language/profile/quit still work";
 CHECK(save_game(app.game));CHECK(save_progress(app.progress));CHECK(save_preferences({}));
 store_language(BootLanguage::English);CHECK(language_preference()==BootLanguage::English);
 CHECK(request_language(2)&&pending_language==2);pending_language=0;
 CHECK(write_feedback("synthetic",9));CHECK(clear_save()&&!files.count(save_path)&&!files.count(backup_path));
 CHECK(save_game(app.game));app.title=true;CHECK(request_profile(2,false));service_profile();CHECK(current_profile()==2);
 CHECK(request_profile(1,false));service_profile();CHECK(current_profile()==1);
 app.frontend_confirm=3;app.frontend_choice=1;app.progress_dirty=true;
 CHECK(resource_runtime.call(app,[](){app.input(Key::Confirm);}));CHECK(app.quit&&!resource_runtime.failed()&&writes);
}
static void failed_advance_pending_input(){
 for(bool confirm_quit:{false,true}){
  scenario=confirm_quit?"advance fails before pending ConfirmQuit":"advance fails before pending Save";
  Resource r;setup(r);preparation();
  if(confirm_quit){app.frontend_confirm=3;app.frontend_choice=1;}
  const Key pending[]={confirm_quit?Key::Confirm:Key::Save,Key::Quit};unsigned dispatched=0;
  const auto before=files;reset_counts();r.fail=true;
  const bool advanced=resource_runtime.call(app,[](){app.advance(33);});
  if(advanced)for(Key key:pending){if(!resource_runtime.call(app,[&](){++dispatched;app.input(key);}))break;}
  CHECK(!advanced&&r.failures&&resource_runtime.failed()&&!app.content.healthy());
  CHECK(!dispatched&&!app.quit);no_mutation(before);rejected_callbacks(before);
 }
}
static void same_input_and_pending_batch(){
 scenario="one real settings action reads a bad string then attempts preferences";
 Resource r;setup(r);app.title=app.settings_open=true;app.progress_dirty=true;
 const auto before=files;r.fail=true;unsigned dispatched=0;
 // This App action performs a localized string read followed by its actual
 // settings callback before the host can regain control to check the fault.
 CHECK(!resource_runtime.call(app,[&](){++dispatched;app.settings_action(UiAction::SettingsSkipIntroLogo);}));
 CHECK(r.failures&&settings_attempts==1&&resource_runtime.failed());no_mutation(before);
 // An already captured later Save/ConfirmQuit sample cannot run after it.
 CHECK(!resource_runtime.call(app,[&](){++dispatched;app.input(Key::Save);}));
 app.frontend_confirm=3;app.frontend_choice=1;
 CHECK(!resource_runtime.call(app,[&](){++dispatched;app.input(Key::Confirm);}));
 CHECK(dispatched==1&&!app.quit);no_mutation(before);
}
static void callback_latch_and_recovery(){
 for(unsigned first=0;first<3;++first){
  scenario="save/progress/clear are the first observer inside a live App operation";
  Resource r;setup(r);const auto before=files;r.fail=true;CHECK(!app.content.image(0).pixels&&!app.content.healthy());
  CHECK(!resource_runtime.failed());
  CHECK(!(first==0?save_game(app.game):first==1?save_progress(app.progress):clear_save()));
  CHECK(resource_runtime.failed());no_mutation(before);
  scenario="reopened locale and refreshed profile cannot clear the latched failure";
  r.fail=false;app.title=true;CHECK(app.reload_language(r.source())&&app.content.healthy());
  app.profile_pending=true;app.profile_target=1;app.profile_delete=false;app.finish_profile_change(true);
  CHECK(app.content.healthy()&&!resource_runtime.start(true));rejected_callbacks(before);
 }
}
static void optional_language_rollback(){
 scenario="failed optional locale rolls back fully before the host observes success";
 Resource r;setup(r);app.title=true;const auto before=files;
 CHECK(resource_runtime.call(app,[&](){
  CHECK(!app.reload_language({})&&!app.content.healthy());
  CHECK(app.reload_language(r.source())&&app.content.healthy());
 }));
 CHECK(!resource_runtime.failed()&&files==before&&!writes&&!renames&&!unlinks);
 CHECK(save_preferences({}));store_language(BootLanguage::English);CHECK(writes&&language_preference()==BootLanguage::English);
}
int main(){
 startup_and_normal_paths();failed_advance_pending_input();same_input_and_pending_batch();
 callback_latch_and_recovery();optional_language_rollback();
 std::printf("resource failure: %u checks passed; real App preparation/input, latched state writes and rollback\n",checks);
}
