namespace {
struct WorldContentIndex {const char* key;uint16_t index;};
static const WorldContentIndex world_text_index[]={
#include "world_text_index.inc"
};
static const WorldContentIndex native_ui_text_index[]={
#include "native_ui_text_index.inc"
};
static const WorldContentIndex world_image_index[]={
#include "world_image_index.inc"
};
static const WorldContentIndex native_ui_index[]={
 {"common.END_TURN",U_EndTurnNative},
 {"common.END_TURN_GLOW",U_EndTurnGlow},
 {"common.END_TURN_HOVER",U_EndTurnHover},
 {"common.HEALTH_BAR_BG",U_HealthBarBackground},
 {"common.HEALTH_BAR_BLOCK",U_HealthBarBlock},
 {"common.HEALTH_BAR_FILL",U_HealthBarFill},
 {"common.HEALTH_BAR_INFINITY",U_HealthBarInfinity},
 {"common.HEALTH_BAR_MASK",U_HealthBarMask},
 {"common.HEALTH_BAR_STROKE",U_HealthBarStroke},
 {"common.HOVER_TIP",U_HoverTip},
 {"common.HOVER_TIP_DEBUFF",U_HoverTipDebuff},
 {"common.PAUSE_BUTTON",U_PauseButton},
 {"common.PAUSE_BUTTON_HOVER",U_PauseButtonHover}
};
static const WorldContentIndex world_clip_index[]={
#include "world_clip_index.inc"
 {"~",0} // A terminal sentinel also permits resource-only metadata checkpoints.
};
static int wc_compare(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
static bool wc_join(char*out,unsigned capacity,const char*group,const char*key){
 if(!group||!key||!capacity)return false;unsigned n=0;
 for(const char*p=group;*p;p++){if(n+1>=capacity)return false;out[n++]=*p;}
 if(n+1>=capacity)return false;out[n++]='.';
 for(const char*p=key;*p;p++){if(n+1>=capacity)return false;out[n++]=*p;}
 out[n]=0;return true;
}
template<unsigned N> static int wc_find(const WorldContentIndex(&rows)[N],const char*group,const char*key){
 char joined[256];if(!wc_join(joined,sizeof joined,group,key))return -1;
 unsigned lo=0,hi=N;while(lo<hi){unsigned mid=(lo+hi)/2;if(wc_compare(rows[mid].key,joined)<0)lo=mid+1;else hi=mid;}
 return lo<N&&!wc_compare(rows[lo].key,joined)?rows[lo].index:-1;
}
}
const char* Content::world_string(const char*table,const char*key)const{
 if(version_==NativeUiContentVersion){int id=wc_find(native_ui_text_index,table,key);if(id>=0)return string(id);}
 return version_>=14?string(wc_find(world_text_index,table,key)):"";
}
const char* Content::world_title(const char*table,const char*key)const{
 if(!key)return "";char joined[192];unsigned n=0;
 while(key[n]&&n+7<sizeof joined){joined[n]=key[n];++n;}if(key[n])return "";
 const char*tail=".title";while(*tail)joined[n++]=*tail++;joined[n]=0;
 return world_string(table,joined);
}
int Content::world_image(const char*group,const char*key)const{
 if(version_==NativeUiContentVersion){int id=wc_find(native_ui_index,group,key);if(id>=0)return ui_image(id);}
 return version_>=14?ui_image(wc_find(world_image_index,group,key)):-1;
}
int Content::world_clip(const char*model,const char*action)const{
 return version_>=14?wc_find(world_clip_index,model,action):-1;
}
