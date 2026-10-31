// Exercise the App menu lifecycle and the actual Ndless run callbacks together.
// Every phase starts in a new process and shares only ordinary files. Synthetic
// fixture pixels are public test data, never a supplied save or original art.
#include "app.hpp"
#include "debug_commands.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "resource_transfer.hpp"
#include "resource_runtime_guard.hpp"
#include "installation_paths.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

using namespace spire;
using Bytes=std::vector<uint8_t>;
static App app;
static ResourceRuntimeGuard resource_runtime;
static char save_path[512],temp_path[512],backup_path[512];
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Ndless filesystem lifecycle:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
#include "ftue_fixture.hpp"

static void*nd_open(const char*path,const char*mode){return std::fopen(path,mode);}
static unsigned nd_read(void*data,unsigned size,unsigned count,void*file){return unsigned(std::fread(data,size,count,static_cast<FILE*>(file)));}
static unsigned nd_write(const void*data,unsigned size,unsigned count,void*file){return unsigned(std::fwrite(data,size,count,static_cast<FILE*>(file)));}
static int nd_close(void*file){return std::fclose(static_cast<FILE*>(file));}
static int nd_rename(const char*from,const char*to){return std::rename(from,to);}
static int nd_unlink(const char*path){return std::remove(path);}
static bool resource_mutation_allowed(){return resource_runtime.allow(app.content.healthy());}
static int profile_file_present(const char*path){void*file=nd_open(path,"rb");return !file?0:nd_close(file)==0?1:-1;}
static bool remove_profile_file(const char*path){const int present=profile_file_present(path);return present==0||(present>0&&nd_unlink(path)==0);}
#include "../platform/ndless/run_storage.inc"

static void advance(unsigned ms){while(ms){const unsigned step=ms>50?50:ms;app.advance(step);app.render();ms-=step;}}
static bool present(const char*path){return std::filesystem::is_regular_file(path);}
static void removed(){CHECK(!present(save_path)&&!present(backup_path)&&!present(temp_path));}
static constexpr const char*other_files[]={"nSpire2.sav.p2.tns","nSpire2.sav.p3.tns","nSpire2.progress.tns","nSpire2.settings.tns","nSpire2.lang.tns"};
static void markers(const std::filesystem::path&root,bool write){
 for(const char*name:other_files){const auto path=root/"state"/name;FILE*file=std::fopen(path.c_str(),write?"wb":"rb");CHECK(file);
  if(write)CHECK(std::fwrite("KEEP",1,4,file)==4);
  else{char bytes[5]{};CHECK(std::fread(bytes,1,sizeof bytes,file)==4&&!std::memcmp(bytes,"KEEP",4));}
  CHECK(!std::fclose(file));
 }
}
static int phase(const std::string&mode,const std::filesystem::path&root){
 std::filesystem::create_directories(root/"state");const auto launcher=(root/"nSpire2.tns").string();
 CHECK(installation_path(save_path,sizeof save_path,launcher.c_str(),InstallationDirectory::State,"nSpire2.sav.tns"));
 CHECK(installation_path(temp_path,sizeof temp_path,launcher.c_str(),InstallationDirectory::State,"nSpire2.tmp.tns"));
 CHECK(installation_path(backup_path,sizeof backup_path,launcher.c_str(),InstallationDirectory::State,"nSpire2.bak.tns"));
 const auto data=fixture();CHECK(app.init(data.data(),data.size(),{save_game,load_game,clear_save}));CHECK(resource_runtime.start(app.content.healthy()));
 if(mode=="prepare"||mode=="prepare-map2"||mode=="prepare-map3"){
  CHECK(!app.has_save);app.title=false;app.forget_run_checkpoint();new_run(app.game,0x21c467,false,Character::Ironclad);
  CHECK(world_set_seed(app.game,"FILESYSTEM-REGRESSION"));CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,3));CHECK(app.game.original_events.id==OriginalEventId::Neow);
  if(mode!="prepare")CHECK(debug_enter_act(app.game,mode=="prepare-map2"?1:2));
  app.reset_presentation();clear_ui_progress(app.progress);app.progress_loaded=app.progress_dirty=true;app.begin_progress(mode!="prepare");
  CHECK(app.save_current_run()&&app.save_current_run());CHECK(present(save_path)&&present(backup_path));
  std::filesystem::copy_file(save_path,temp_path);CHECK(present(temp_path));markers(root,true);
 }else if(mode=="abandon"||mode=="death"){
  CHECK(app.title&&app.has_save&&present(save_path)&&present(backup_path)&&present(temp_path));
  app.input(Key::Confirm);advance(6000);CHECK(!app.title&&app.run_save_linked);
  if(mode=="abandon"){
   app.input(Key::Back);CHECK(app.paused);
   for(unsigned i=0;i<3;++i)app.input(Key::Down);CHECK(app.menu==3);
   app.input(Key::Confirm);CHECK(app.frontend_confirm==2&&app.frontend_choice==0);
   app.input(Key::Right);app.input(Key::Confirm);
  }else{player_kill(app.game,true);resolve_pending(app.game);advance(50);}
  CHECK(app.game.phase==Phase::Dead&&app.game.hp==0&&!app.paused&&!app.run_save_linked);removed();
  app.input(Key::Save);removed();advance(6000);app.input(Key::Confirm);CHECK(app.game_over_summary);advance(6000);
  app.input(Key::Confirm);CHECK(app.character_departure.phase==3&&!app.title);advance(1600);
  CHECK(app.title&&!app.has_save&&!app.character_departure.active());removed();markers(root,false);
 }else if(mode=="reopen"){
  CHECK(app.title&&!app.has_save&&app.title_menu_count()==4);removed();markers(root,false);
 }else CHECK(false);
 std::printf("Ndless filesystem lifecycle %s: %u checks passed\n",mode.c_str(),checks);return 0;
}
static bool child(const std::string&executable,const char*mode,const std::filesystem::path&root){
 // exec has no inherited App, ResourceTransfer lease, saved-probe workspace,
 // image cache or selected menu. It must discover the persisted result again.
 std::fflush(nullptr);const pid_t pid=fork();CHECK(pid>=0);
 if(!pid){execl(executable.c_str(),executable.c_str(),"--child",mode,root.c_str(),static_cast<char*>(nullptr));_exit(127);}
 int status=0;CHECK(waitpid(pid,&status,0)==pid);return WIFEXITED(status)&&WEXITSTATUS(status)==0;
}
int main(int argc,char**argv){
 if(argc==4&&!std::strcmp(argv[1],"--child"))return phase(argv[2],argv[3]);
 CHECK(argc==1);const auto executable=std::filesystem::absolute(argv[0]).string();
 const auto base=std::filesystem::temp_directory_path()/"nspire-run-lifecycle-XXXXXX";
 std::string directory=base.string();std::vector<char>name(directory.begin(),directory.end());name.push_back(0);CHECK(mkdtemp(name.data()));
 const std::filesystem::path root=name.data();bool ok=true;
 const struct {const char*name,*prepare,*end;} cases[]={
  {"neow","prepare","abandon"},{"death","prepare","death"},
  {"act2","prepare-map2","abandon"},{"act3","prepare-map3","abandon"}};
 for(const auto&test:cases){const auto path=root/test.name;
  if(!child(executable,test.prepare,path)||!child(executable,test.end,path)||!child(executable,"reopen",path)){ok=false;break;}
 }
 if(ok){std::filesystem::remove_all(root);std::puts("Ndless filesystem lifecycle: four scenarios and twelve independent processes passed");}
 else std::fprintf(stderr,"Failed lifecycle files retained at %s\n",root.c_str());
 return ok?0:1;
}
