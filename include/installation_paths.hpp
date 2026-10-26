#pragma once
#include <stddef.h>

namespace spire {
enum class InstallationDirectory { Root, Data, State };
constexpr const char* SharedDataFilename="nSpire2.dat.tns";

// All paths are relative to the actual launcher, never the OS working folder.
// The unpublished port has one layout: executable at root, assets in data,
// and every mutable file in state. No legacy-path search or migration occurs.
inline bool installation_path(char*out,size_t capacity,const char*launcher,
                              InstallationDirectory directory,const char*name){
    if(!out||!capacity)return false;
    out[0]=0;size_t prefix=0;
    if(launcher)for(size_t i=0;launcher[i];++i){
        if(i>=capacity)return false;
        if(launcher[i]=='/')prefix=i+1;
    }
    const char*folder=directory==InstallationDirectory::Data?"data/":
                      directory==InstallationDirectory::State?"state/":"";
    size_t length=prefix;
    for(const char*p=folder;*p;++p)if(++length>=capacity)return false;
    for(const char*p=name;*p;++p)if(++length>=capacity)return false;
    size_t at=0;for(;at<prefix;++at)out[at]=launcher[at];
    while(*folder)out[at++]=*folder++;
    while(*name)out[at++]=*name++;
    out[at]=0;return true;
}
}
