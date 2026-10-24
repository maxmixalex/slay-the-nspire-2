// Local-only report export, sharing the established profile path and atomic rename helpers.
static bool write_feedback(const char*report,unsigned length){
    char path[512],temporary[512],backup[512];
    if(!profile_sibling(path,"nSpire2-feedback.txt.tns",selected_profile)||
       !profile_sibling(temporary,"nSpire2-feedback.tmp.tns",selected_profile)||
       !profile_sibling(backup,"nSpire2-feedback.bak.tns",selected_profile))return false;
    void*f=nd_open(temporary,"wb");if(!f)return false;
    bool ok=nd_write(report,1,length,f)==length;ok=(nd_close(f)==0)&&ok;
    if(!ok){nd_unlink(temporary);return false;}
    const int old=profile_file_present(path);
    if(old<0||(old&&(!remove_profile_file(backup)||nd_rename(path,backup)!=0))){nd_unlink(temporary);return false;}
    if(nd_rename(temporary,path)!=0){if(old)nd_rename(backup,path);nd_unlink(temporary);return false;}
    if(old)nd_unlink(backup);return true;
}
