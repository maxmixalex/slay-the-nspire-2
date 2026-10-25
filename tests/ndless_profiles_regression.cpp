#include "ui_profiles.hpp"
#include "ui_progress.hpp"
#include "ui_preferences.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using Bytes=std::vector<uint8_t>;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Ndless profiles:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::map<std::string,Bytes> files;
static std::string fail_write,fail_rename,fail_unlink;
static unsigned read_bytes=0;
struct File {std::string path;size_t offset=0;};
static void*nd_open(const char*path,const char*mode){
 if(*mode=='w')files[path]={};else if(!files.count(path))return nullptr;
 return new File{path};
}
static unsigned nd_read(void*output,unsigned size,unsigned count,void*handle){
 auto&f=*static_cast<File*>(handle);auto&data=files.at(f.path);const size_t n=std::min<size_t>(size*count,data.size()-f.offset);
 if(n)std::memcpy(output,data.data()+f.offset,n);f.offset+=n;read_bytes+=unsigned(n);return unsigned(n/size);
}
static unsigned nd_write(const void*input,unsigned size,unsigned count,void*handle){
 auto&f=*static_cast<File*>(handle);const unsigned n=count-(f.path==fail_write&&count?1:0);fail_write.clear();
 const auto*bytes=static_cast<const uint8_t*>(input);auto&data=files.at(f.path);data.insert(data.end(),bytes,bytes+size*n);return n;
}
static int nd_close(void*handle){delete static_cast<File*>(handle);return 0;}
static int nd_seek(void*handle,long offset,int origin){
 auto&f=*static_cast<File*>(handle);const long base=origin==2?long(files.at(f.path).size()):origin==1?long(f.offset):0;
 if(offset< -base||static_cast<unsigned long>(base+offset)>files.at(f.path).size())return -1;f.offset=size_t(base+offset);return 0;
}
static long nd_tell(void*handle){return long(static_cast<File*>(handle)->offset);}
static int nd_rename(const char*from,const char*to){
 if(fail_rename==from){fail_rename.clear();return -1;}
 auto found=files.find(from);if(found==files.end()||files.count(to))return -1;
 files.emplace(to,std::move(found->second));files.erase(found);return 0;
}
static int nd_unlink(const char*path){if(fail_unlink==path){fail_unlink.clear();return -1;}return files.erase(path)?0:-1;}
static char save_path[512],temp_path[512],backup_path[512],progress_path[512],progress_temp[512],progress_backup[512];
static char settings_path[512],settings_temp[512],settings_backup[512];
static uint8_t progress_bytes[spire::UiProgressCapacity+1];
static struct AppResult {bool title=true,success=false;unsigned completed=0;void finish_profile_change(bool ok){success=ok;++completed;}} app;
static bool sibling(char*out,const char*launcher,const char*name){
 const char*slash=std::strrchr(launcher,'/');const size_t prefix=slash?size_t(slash-launcher)+1:0;
 if(prefix+std::strlen(name)>=512)return false;std::memcpy(out,launcher,prefix);std::strcpy(out+prefix,name);return true;
}
// Exercise the exact storage implementation used by the ARM foreground host.
#include "../platform/ndless/profile_storage.inc"
#include "../platform/ndless/preferences_storage.inc"
#include "../platform/ndless/feedback_storage.inc"
static std::string path(unsigned slot,unsigned index,bool removed=false){char names[ProfileFileCount][512];CHECK(profile_paths(slot,names,removed));return names[index];}
static Bytes selector(unsigned slot){Bytes b(spire::UiProfileSelectorBytes);spire::encode_profile_selector(slot,b.data());return b;}
static void setup(){
 files.clear();selected_profile=1;pending_profile=0;app={};profile_launcher="/documents/games/nSpire2.tns";
 CHECK(sibling(selector_path,profile_launcher,"nSpire2.slots.tns"));CHECK(sibling(selector_temp,profile_launcher,"nSpire2.slots.tmp.tns"));
 CHECK(sibling(selector_backup,profile_launcher,"nSpire2.slots.bak.tns"));CHECK(configure_profile_paths(1));
 files["/documents/games/nSpire2.profile.tns"]={7};files["/documents/games/nSpire2.settings.tns"]={8};files["/documents/games/nSpire2.lang.tns"]={9};
}
static void routing(){setup();CHECK(load_profile_selector()==1);
 CHECK(ProfileFileCount==9&&std::string(save_path)=="/documents/games/nSpire2.sav.tns");
 CHECK(std::string(settings_path)=="/documents/games/nSpire2.settings.tns");
 files[selector_path]=selector(3);CHECK(load_profile_selector()==3);files[selector_path].push_back(0);files[selector_backup]=selector(2);CHECK(load_profile_selector()==2);
 files.erase(selector_path);files.erase(selector_backup);
 CHECK(!request_profile(0,false)&&!request_profile(4,false));app.title=false;CHECK(!request_profile(2,false));app.title=true;
 CHECK(request_profile(2,false));CHECK(!request_profile(3,false));CHECK(current_profile()==1);service_profile();CHECK(app.success&&current_profile()==2);
 CHECK(files.at(selector_path)==selector(2));CHECK(std::string(save_path)=="/documents/games/nSpire2.sav.p2.tns");
 CHECK(std::string(temp_path)=="/documents/games/nSpire2.tmp.p2.tns");CHECK(std::string(progress_backup)=="/documents/games/nSpire2.progress.bak.p2.tns");
 CHECK(std::string(settings_path)=="/documents/games/nSpire2.settings.p2.tns");
 CHECK(std::string(settings_temp)=="/documents/games/nSpire2.settings.tmp.p2.tns"&&std::string(settings_backup)=="/documents/games/nSpire2.settings.bak.p2.tns");
 CHECK(files.at("/documents/games/nSpire2.profile.tns")==Bytes{7});
 CHECK(files.at("/documents/games/nSpire2.settings.tns")==Bytes{8}&&files.at("/documents/games/nSpire2.lang.tns")==Bytes{9});
 const auto saved=files;const std::string previous=save_path;fail_write=selector_temp;
 CHECK(request_profile(3,false));service_profile();CHECK(!app.success&&current_profile()==2&&previous==save_path&&files==saved);
 fail_rename=selector_temp;CHECK(request_profile(3,false));service_profile();CHECK(!app.success&&current_profile()==2&&previous==save_path&&files==saved);
 CHECK(request_profile(3,false));app.title=false;service_profile();CHECK(!app.success&&current_profile()==2&&files==saved);
}
static void summaries(){setup();spire::UiProgressSummary summary;CHECK(!profile_info(0,summary)&&!profile_info(1,summary));
 files[path(1,0)]={1,2,3,4};read_bytes=0;CHECK(profile_info(1,summary)&&summary.exists&&!summary.play_seconds&&!summary.last_saved_at);CHECK(read_bytes==1);
 files[path(1,0)].clear();CHECK(!profile_info(1,summary)&&!summary.exists);
 static spire::UiProgress progress;spire::clear_ui_progress(progress);progress.characters[0].play_seconds=15;progress.characters[1].play_seconds=27;progress.last_saved_at=123456;
 const auto count=spire::encode_ui_progress(progress,progress_bytes,sizeof progress_bytes);CHECK(count);
 files[path(2,3)]=Bytes(progress_bytes,progress_bytes+count);CHECK(profile_info(2,summary)&&summary.exists&&summary.play_seconds==42&&summary.last_saved_at==123456);
 files[path(3,3)]={9};files[path(3,5)]=files[path(2,3)];CHECK(profile_info(3,summary)&&summary.play_seconds==42);
 // Profile discovery reads the prefix of current NSH8 metadata too. The
 // tutorial preference remains owned by the full codec, not by a summary
 // scan or the profile-selector transaction.
 progress.tutorial_flags=spire::FtueInitialized|spire::ftue_bit(spire::FtueId::AcceptTutorials);
 const auto current_count=spire::encode_ui_progress(progress,progress_bytes,sizeof progress_bytes);
 CHECK(current_count&&progress_bytes[3]=='8');files[path(3,3)]=Bytes(progress_bytes,progress_bytes+current_count);
 CHECK(profile_info(3,summary)&&summary.exists&&summary.play_seconds==42&&summary.last_saved_at==123456);
 static spire::UiProgress restored;CHECK(spire::decode_ui_progress(files[path(3,3)].data(),current_count,restored));
 CHECK(restored.tutorial_flags==progress.tutorial_flags&&files[path(2,3)][3]=='2');
 CHECK(current_profile()==1);
}
static void recovered_selector(){setup();files[selector_path]={1,2,3};files[selector_backup]=selector(2);
 selected_profile=load_profile_selector();CHECK(selected_profile==2&&configure_profile_paths(selected_profile));
 const std::string previous=save_path;fail_rename=selector_temp;
 CHECK(request_profile(3,false));service_profile();
 CHECK(!app.success&&current_profile()==2&&previous==save_path&&load_profile_selector()==2);
 CHECK(files.at(selector_backup)==selector(2)&&!files.count(selector_temp));
 CHECK(request_profile(3,false));service_profile();CHECK(app.success&&current_profile()==3&&load_profile_selector()==3);
 CHECK(files.at(selector_path)==selector(3));
}
static void preferences(){setup();spire::UiPreferences first{},second{},loaded{};
 first.fast_mode=first.phobia_mode=first.show_run_timer=first.long_press=first.skip_intro_logo=true;first.screen_shake=4;
 second.keyboard_only=second.show_card_indices=true;second.text_effects=second.bestiary_actions_preferred=false;second.screen_shake=0;
 first.bindings=spire::default_input_bindings(true);CHECK(spire::rebind_input(first.bindings,0,unsigned(spire::InputCommand::Map),'B'));
 CHECK(spire::rebind_input(second.bindings,1,unsigned(spire::InputCommand::Map),'P'));
 CHECK(save_preferences(first)&&load_preferences(loaded)&&spire::ui_preference_flags(loaded)==spire::ui_preference_flags(first));
 CHECK(loaded.bindings_saved&&!std::memcmp(&loaded.bindings,&first.bindings,sizeof first.bindings));
 const auto slot_one=files.at(path(1,6));
 CHECK(slot_one.size()==spire::UiPreferencesBytes&&slot_one[4]==6);
 CHECK(request_profile(2,false));service_profile();CHECK(app.success&&!load_preferences(loaded));
 CHECK(save_preferences(second)&&load_preferences(loaded)&&spire::ui_preference_flags(loaded)==spire::ui_preference_flags(second));
 CHECK(!std::memcmp(&loaded.bindings,&second.bindings,sizeof second.bindings));
 const auto slot_two=files.at(path(2,6));CHECK(slot_one!=slot_two);
 CHECK(request_profile(3,false));service_profile();CHECK(app.success&&!load_preferences(loaded));
 CHECK(save_preferences({}));CHECK(request_profile(1,false));service_profile();CHECK(app.success);
 CHECK(load_preferences(loaded)&&spire::ui_preference_flags(loaded)==spire::ui_preference_flags(first));
 CHECK(!std::memcmp(&loaded.bindings,&first.bindings,sizeof first.bindings));
 CHECK(files.at(path(2,6))==slot_two&&files.at(path(1,6))==slot_one);
 const auto before=files;fail_write=settings_temp;CHECK(!save_preferences(second)&&files==before);
 fail_rename=settings_temp;CHECK(!save_preferences(second)&&files==before);
 CHECK(load_preferences(loaded)&&spire::ui_preference_flags(loaded)==spire::ui_preference_flags(first));
 CHECK(files.at("/documents/games/nSpire2.lang.tns")==Bytes{9}&&files.at("/documents/games/nSpire2.profile.tns")==Bytes{7});
}
static void removal(){setup();files[selector_path]=selector(1);
 for(unsigned slot=1;slot<=3;++slot)for(unsigned i=0;i<ProfileFileCount;++i)files[path(slot,i)]={uint8_t(slot),uint8_t(i)};
 // Fail after the run, progress and primary preference paths have moved:
 // rollback must restore the whole selected profile, including its settings.
 const auto saved=files;fail_rename=path(2,7);CHECK(request_profile(2,true));service_profile();CHECK(!app.success&&files==saved&&current_profile()==1);
 CHECK(request_profile(2,true));service_profile();CHECK(app.success&&current_profile()==1);
 for(unsigned i=0;i<ProfileFileCount;++i){CHECK(!files.count(path(2,i))&&!files.count(path(2,i,true)));CHECK(files.at(path(1,i))==saved.at(path(1,i))&&files.at(path(3,i))==saved.at(path(3,i)));}
 spire::UiProgressSummary summary;CHECK(!profile_info(2,summary));
 CHECK(request_profile(1,true));service_profile();CHECK(app.success&&current_profile()==1);
 for(unsigned i=0;i<ProfileFileCount;++i)CHECK(!files.count(path(1,i))&&files.count(path(3,i)));
 CHECK(files.at(selector_path)==selector(1)&&files.at("/documents/games/nSpire2.profile.tns")==Bytes{7});
 // Cleanup failure after all nine names moved does not falsely report a
 // rollback. The profile is absent; quarantined bytes are never loaded.
 fail_unlink=path(3,0,true);CHECK(request_profile(3,true));service_profile();CHECK(app.success&&!profile_info(3,summary));
 CHECK(files.count(path(3,0,true))&&!files.count(path(3,0)));
}
static void feedback(){
 setup();char destination[512],temporary[512];CHECK(profile_sibling(destination,"nSpire2-feedback.txt.tns",1));
 CHECK(profile_sibling(temporary,"nSpire2-feedback.tmp.tns",1));const char first[]="Synthetic local report\n",second[]="Replacement draft\n";
 const auto original=files;CHECK(write_feedback(first,sizeof first-1));CHECK(files.at(destination)==Bytes(first,first+sizeof first-1));
 for(const auto&pair:original)CHECK(files.at(pair.first)==pair.second);
 const auto saved=files;fail_write=temporary;CHECK(!write_feedback(second,sizeof second-1)&&files==saved);
 fail_rename=temporary;CHECK(!write_feedback(second,sizeof second-1)&&files==saved);
 CHECK(write_feedback(second,sizeof second-1)&&files.at(destination)==Bytes(second,second+sizeof second-1));
 CHECK(request_profile(2,false));service_profile();CHECK(app.success);CHECK(write_feedback(first,sizeof first-1));
 char second_destination[512];CHECK(profile_sibling(second_destination,"nSpire2-feedback.txt.tns",2));
 CHECK(files.at(second_destination)==Bytes(first,first+sizeof first-1)&&files.at(destination)==Bytes(second,second+sizeof second-1));
}
int main(){routing();summaries();recovered_selector();preferences();removal();feedback();std::printf("Ndless profiles: %u checks passed\n",checks);}
