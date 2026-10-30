// Language is host-global; startup reads remain available before the runtime
// is armed. Requests and writes cannot revive a failed live resource session.
static unsigned available_languages(){return installed_languages;}
static bool request_language(unsigned language){
    if(!resource_mutation_allowed()||!spire::locale_language_valid(language)||
       !(installed_languages&spire::locale_language_mask(language)))return false;
    pending_language=language;return true;
}
static spire::BootLanguage language_preference(){
    void*f=nd_open(language_path,"rb");if(!f)return spire::BootLanguage::Unknown;
    uint8_t bytes[9];const unsigned count=nd_read(bytes,1,sizeof bytes,f);nd_close(f);
    return spire::boot_preference(bytes,count);
}
static void store_language(spire::BootLanguage language){
    if(!resource_mutation_allowed())return;
    void*f=nd_open(language_path,"wb");if(!f)return;
    bool good=nd_write(spire::boot_preference_bytes(language),1,8,f)==8;
    good=(nd_close(f)==0)&&good;if(!good)nd_unlink(language_path);
}
