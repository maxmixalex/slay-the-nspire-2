// Foreground-only profile storage. The existing run/progress buffers are
// reused; opening this menu never constructs another State or UiProgress.
#include "storage_codec.inc"
#include "installation_paths.hpp"
static const char*profile_launcher="";
static char selector_path[512],selector_temp[512],selector_backup[512];
static unsigned selected_profile=1,pending_profile=0;
static bool pending_profile_delete=false;
enum class ProgressRecovery:uint8_t { Unknown,PrimaryValid,BackupAfterInvalidPrimary };
// One process-only fact from the existing full load. A profile switch clears
// it; a read/close failure cannot certify that the primary is corrupt.
static ProgressRecovery progress_recovery=ProgressRecovery::Unknown;
static bool progress_file_read(void*file,uint8_t*bytes,unsigned&size,storage_codec::ReadStatus&status){
    size=0;status=storage_codec::ReadStatus::IoFailure;
    if(nd_seek(file,0,2)!=0)return false;const long physical=nd_tell(file);
    if(physical<0)return false;
    // Even a legal stream of one-byte literal packets is bounded. The writer
    // selects NSZ1 only when smaller, but reading keeps the complete grammar.
    constexpr unsigned bound=2*spire::UiProgressCapacity+storage_codec::HeaderBytes;
    if(physical<storage_codec::HeaderBytes||static_cast<unsigned long>(physical)>bound){status=storage_codec::ReadStatus::Invalid;return false;}
    if(nd_seek(file,0,0)!=0)return false;
    return storage_codec::read(file,bytes,spire::UiProgressCapacity,size,storage_codec::HeaderBytes,false,&status,unsigned(physical));
}
static constexpr unsigned ProfileFileCount=9;
static const char*const profile_files[]={"nSpire2.sav.tns","nSpire2.tmp.tns","nSpire2.bak.tns",
    "nSpire2.progress.tns","nSpire2.progress.tmp.tns","nSpire2.progress.bak.tns",
    "nSpire2.settings.tns","nSpire2.settings.tmp.tns","nSpire2.settings.bak.tns"};
static const char*const profile_removed_files[]={"nSpire2.sav.deleted.tns","nSpire2.tmp.deleted.tns","nSpire2.bak.deleted.tns",
    "nSpire2.progress.deleted.tns","nSpire2.progress.tmp.deleted.tns","nSpire2.progress.bak.deleted.tns",
    "nSpire2.settings.deleted.tns","nSpire2.settings.tmp.deleted.tns","nSpire2.settings.bak.deleted.tns"};
static_assert(sizeof profile_files/sizeof profile_files[0]==ProfileFileCount&&sizeof profile_removed_files/sizeof profile_removed_files[0]==ProfileFileCount,"Every profile-owned file has an isolated delete path");
static bool profile_sibling(char*out,const char*name,unsigned profile){
    char base[512];return spire::installation_path(base,sizeof base,profile_launcher,spire::InstallationDirectory::State,name)&&spire::profile_scoped_path(out,512,base,profile);
}
static bool profile_paths(unsigned profile,char(&paths)[ProfileFileCount][512],bool removed=false){
    for(unsigned i=0;i<ProfileFileCount;++i)if(!profile_sibling(paths[i],removed?profile_removed_files[i]:profile_files[i],profile))return false;
    return true;
}
static void install_profile_paths(const char(&paths)[ProfileFileCount][512]){
    char*const destinations[]={save_path,temp_path,backup_path,progress_path,progress_temp,progress_backup,settings_path,settings_temp,settings_backup};
    for(unsigned i=0;i<ProfileFileCount;++i)for(unsigned at=0;;++at){destinations[i][at]=paths[i][at];if(!paths[i][at])break;}
    progress_recovery=ProgressRecovery::Unknown;
}
static bool configure_profile_paths(unsigned profile){
    char paths[ProfileFileCount][512];if(!profile_paths(profile,paths))return false;install_profile_paths(paths);return true;
}
static unsigned current_profile(){return selected_profile;}
static int read_profile_selector(const char*path,unsigned&value){
    void*f=nd_open(path,"rb");if(!f)return 0;
    uint8_t bytes[spire::UiProfileSelectorBytes+1],canonical[spire::UiProfileSelectorBytes];
    const unsigned n=nd_read(bytes,1,sizeof bytes,f);if(nd_close(f)!=0)return -1;
    if(n!=spire::UiProfileSelectorBytes)return 0;
    value=spire::decode_profile_selector(bytes,n);spire::encode_profile_selector(value,canonical);
    for(unsigned i=0;i<n;++i)if(bytes[i]!=canonical[i])return 0;return 1;
}
static unsigned load_profile_selector(){
    const char*const paths[]={selector_path,selector_backup};
    for(const char*path:paths){unsigned value;if(read_profile_selector(path,value)>0)return value;}return 1;
}
static int profile_file_present(const char*path){void*f=nd_open(path,"rb");return !f?0:nd_close(f)==0?1:-1;}
static bool remove_profile_file(const char*path){const int present=profile_file_present(path);return present==0||(present>0&&nd_unlink(path)==0);}
static bool store_profile_selector(unsigned profile){
    uint8_t bytes[spire::UiProfileSelectorBytes];spire::encode_profile_selector(profile,bytes);
    void*f=nd_open(selector_temp,"wb");if(!f)return false;
    bool ok=nd_write(bytes,1,sizeof bytes,f)==sizeof bytes;ok=(nd_close(f)==0)&&ok;
    if(!ok){nd_unlink(selector_temp);return false;}
    const int old=profile_file_present(selector_path);
    unsigned previous=1;const int valid=old>0?read_profile_selector(selector_path,previous):0;
    if(old<0||valid<0){nd_unlink(selector_temp);return false;}
    if(valid){
        if(!remove_profile_file(selector_backup)||nd_rename(selector_path,selector_backup)!=0){nd_unlink(selector_temp);return false;}
    }else if(old&&nd_unlink(selector_path)!=0){nd_unlink(selector_temp);return false;}
    // A corrupt primary may have booted from a valid backup. Never replace
    // that recovery copy with corrupt bytes before the new selector commits.
    if(nd_rename(selector_temp,selector_path)!=0){if(valid)nd_rename(selector_backup,selector_path);nd_unlink(selector_temp);return false;}
    if(valid)nd_unlink(selector_backup);return true;
}
static bool profile_info(unsigned profile,spire::UiProgressSummary&out){
    out={};if(profile<1||profile>spire::UiProfileCount)return false;
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Bytes);if(!transfer)return false;
    uint8_t*progress_bytes=transfer.bytes();
    char path[512];
    const unsigned candidates[]={3,5};for(unsigned index:candidates){if(!profile_sibling(path,profile_files[index],profile))return false;
        void*f=nd_open(path,"rb");if(!f)continue;
        unsigned n=0;storage_codec::ReadStatus status;const bool read=progress_file_read(f,progress_bytes,n,status);
        const bool closed=nd_close(f)==0;
        if(read&&closed&&spire::decode_ui_progress_summary(progress_bytes,n,out))return true;
    }
    // Old ports have no UI-history file. The first byte is only an existence
    // hint; Continue still performs the complete ordinary save validation.
    out={};if(!profile_sibling(path,profile_files[0],profile))return false;
    void*f=nd_open(path,"rb");if(!f)return false;uint8_t first;
    const bool nonempty=nd_read(&first,1,1,f)==1,closed=nd_close(f)==0;
    out.exists=nonempty&&closed;return out.exists;
}
static bool request_profile(unsigned profile,bool erase){
    if(!app.title||profile<1||profile>spire::UiProfileCount||pending_profile)return false;
    pending_profile=profile;pending_profile_delete=erase;return true;
}
static bool erase_profile_files(unsigned profile){
    char paths[ProfileFileCount][512],removed[ProfileFileCount][512];if(!profile_paths(profile,paths)||!profile_paths(profile,removed,true))return false;
    unsigned present=0,moved=0;
    for(unsigned i=0;i<ProfileFileCount;++i){const int exists=profile_file_present(paths[i]);if(exists<0||!remove_profile_file(removed[i]))return false;if(exists)present|=1u<<i;}
    // Quarantine all nine names before making deletion visible to the App.
    // A failed rename restores every earlier file. Once all names moved,
    // unlink is cleanup: stale quarantined bytes can never become Continue.
    for(unsigned i=0;i<ProfileFileCount;++i)if(present&(1u<<i)){
        if(nd_rename(paths[i],removed[i])!=0){for(unsigned j=0;j<i;++j)if(moved&(1u<<j))nd_rename(removed[j],paths[j]);return false;}
        moved|=1u<<i;
    }
    for(unsigned i=0;i<ProfileFileCount;++i)if(moved&(1u<<i))nd_unlink(removed[i]);
    if(profile==selected_profile)progress_recovery=ProgressRecovery::Unknown;return true;
}
static void service_profile(){
    if(!pending_profile)return;const unsigned target=pending_profile;pending_profile=0;
    bool ok=false;
    if(app.title){
        if(pending_profile_delete)ok=erase_profile_files(target);
        else{char paths[ProfileFileCount][512];ok=profile_paths(target,paths)&&store_profile_selector(target);
            if(ok){install_profile_paths(paths);selected_profile=target;}}
    }
    app.finish_profile_change(ok);
}
