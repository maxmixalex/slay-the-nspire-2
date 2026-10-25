namespace {
struct LocaleUiIndex {uint32_t hash;uint16_t index,length;};
static const LocaleUiIndex locale_ui_index[]={
#include "locale_ui_index.inc"
};
struct InlineLocaleUi {uint32_t hash;uint16_t length;const char* english;};
static const InlineLocaleUi locale_ui_inline[]={
#include "locale_ui_inline.inc"
};
static const WorldContentIndex locale_text_index[]={
#include "locale_text_index.inc"
};
static_assert(sizeof locale_ui_index/sizeof locale_ui_index[0]<=FrontendSettingsGeneralString-LocaleUiTextBase,
              "Existing locale UI bindings overlap the fixed frontend slots");
static const WorldContentIndex frontend_text_index[]={
 {"settings_ui.TAB_GENERAL",FrontendSettingsGeneralString},
 {"settings_ui.TAB_INPUT",FrontendSettingsInputString}
};
}
bool Content::english()const{
 return language_id_==1;
}
unsigned Content::language()const{
 return language_id_;
}
const char* Content::locale_string(const char*table,const char*key)const{
 if(has_startup_logo_assets()){
  static const WorldContentIndex startup_text_index[]={
   {"settings_ui.SKIP_INTRO_LOGO_DESCRIPTION",IntroLogoDescriptionString},
   {"settings_ui.SKIP_INTRO_LOGO_HEADER",IntroLogoHeaderString},
   {"settings_ui.TOAST_SKIP_INTRO_LOGO_OFF",IntroLogoOffString},
   {"settings_ui.TOAST_SKIP_INTRO_LOGO_ON",IntroLogoOnString}
  };
  const int index=wc_find(startup_text_index,table,key);if(index>=0)return string(index);
 }
 if(has_selection_assets()){const int selection=wc_find(selection_text_index,table,key);if(selection>=0)return string(selection);}
 if(version_>=HistoryUi8ContentVersion){
  static const WorldContentIndex mode_text_index[]={
#include "run_modes_text_index.inc"
  };
  const int index=wc_find(mode_text_index,table,key);if(index>=0)return string(index);
  static const WorldContentIndex auxiliary_text_index[]={
#include "auxiliary_text_index.inc"
  };
  const int auxiliary=wc_find(auxiliary_text_index,table,key);if(auxiliary>=0)return string(auxiliary);
 }
 if(version_>=ProfileContentVersion){
  static const WorldContentIndex profile_text_index[]={
#include "profile_text_index.inc"
  };
  const int index=wc_find(profile_text_index,table,key);if(index>=0)return string(index);
 }
 if(version_>=Ui7ContentVersion){
  static const WorldContentIndex ui7_text_index[]={
#include "ui7_text_index.inc"
  };
  const int index=wc_find(ui7_text_index,table,key);if(index>=0)return string(index);
 }
 if(version_>=Ui6ContentVersion){
  static const WorldContentIndex ui6_text_index[]={{"vfx.POWER_WEARS_OFF",Ui6PowerWearsOffString}};
  const int index=wc_find(ui6_text_index,table,key);if(index>=0)return string(index);
 }
 if(version_>=FrontendContentVersion){const int index=wc_find(frontend_text_index,table,key);if(index>=0)return string(index);}
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
 if(language()!=2){
  lo=0;hi=sizeof(locale_ui_inline)/sizeof(locale_ui_inline[0]);
  while(lo<hi){unsigned middle=(lo+hi)/2;if(locale_ui_inline[middle].hash<hash)lo=middle+1;else hi=middle;}
  if(lo<sizeof(locale_ui_inline)/sizeof(locale_ui_inline[0])&&locale_ui_inline[lo].hash==hash&&locale_ui_inline[lo].length==length)
   return locale_ui_inline[lo].english;
 }
 return source;
}
