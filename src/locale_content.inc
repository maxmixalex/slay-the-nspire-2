namespace {
struct LocaleUiIndex {uint32_t hash;uint16_t index,length;};
static const LocaleUiIndex locale_ui_index[]={
#include "locale_ui_index.inc"
};
static const WorldContentIndex locale_text_index[]={
#include "locale_text_index.inc"
};
}
bool Content::english()const{
 if(version_<LocaleContentVersion)return false;const char*p=string(LocaleMarkerString);
 return p[0]=='e'&&p[1]=='n'&&p[2]=='g'&&!p[3];
}
const char* Content::locale_string(const char*table,const char*key)const{
 return version_>=LocaleContentVersion?string(wc_find(locale_text_index,table,key)):"";
}
const char* Content::ui_text(const char*source)const{
 if(!source)return "";if(version_<LocaleContentVersion)return source;
 uint32_t hash=2166136261u;unsigned length=0;bool multibyte=false;
 for(const unsigned char*p=(const unsigned char*)source;*p;++p){hash=(hash^*p)*16777619u;++length;multibyte|=*p>=128;}
 if(!multibyte)return source;
 unsigned lo=0,hi=sizeof(locale_ui_index)/sizeof(locale_ui_index[0]);
 while(lo<hi){unsigned middle=(lo+hi)/2;if(locale_ui_index[middle].hash<hash)lo=middle+1;else hi=middle;}
 if(lo<sizeof(locale_ui_index)/sizeof(locale_ui_index[0])&&locale_ui_index[lo].hash==hash&&locale_ui_index[lo].length==length)
  return string(locale_ui_index[lo].index);
 return source;
}
