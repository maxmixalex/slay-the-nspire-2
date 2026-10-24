// Fixed eight-byte preferences use the active profile's already-resolved
// paths. Slot1 retains the historical names; language remains host-global.
static bool load_preferences(spire::UiPreferences& value){
    void*f=nd_open(settings_path,"rb");if(!f)return false;
    uint8_t bytes[spire::UiPreferencesBytes+1];const unsigned n=nd_read(bytes,1,sizeof bytes,f);
    const bool closed=nd_close(f)==0;return closed&&spire::decode_ui_preferences(bytes,n,value);
}
static bool save_preferences(const spire::UiPreferences& value){
    uint8_t bytes[spire::UiPreferencesBytes];spire::encode_ui_preferences(value,bytes);
    void*f=nd_open(settings_temp,"wb");if(!f)return false;
    bool ok=nd_write(bytes,1,sizeof bytes,f)==sizeof bytes;ok=(nd_close(f)==0)&&ok;
    if(!ok){nd_unlink(settings_temp);return false;}
    const int old=profile_file_present(settings_path);
    if(old<0){nd_unlink(settings_temp);return false;}
    if(old){if(!remove_profile_file(settings_backup)||nd_rename(settings_path,settings_backup)!=0){nd_unlink(settings_temp);return false;}}
    if(nd_rename(settings_temp,settings_path)!=0){if(old)nd_rename(settings_backup,settings_path);nd_unlink(settings_temp);return false;}
    if(old)nd_unlink(settings_backup);return true;
}
