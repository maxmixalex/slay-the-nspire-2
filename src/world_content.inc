#include "packed_content_index.hpp"
namespace {
using WorldContentIndex=packed_content_index::Entry;
static constexpr WorldContentIndex world_text_source[]={
#include "world_text_index.inc"
};
static constexpr auto world_text_layout=packed_content_index::layout(world_text_source);
static constexpr auto world_text_index=packed_content_index::make<world_text_layout.key_bytes,world_text_layout.group_bytes,world_text_layout.groups,
 (world_text_layout.key_bytes>65536&&world_text_layout.all_grouped&&world_text_layout.max_group_bytes<=65536)>(world_text_source);
static const WorldContentIndex native_ui_text_index[]={
#include "native_ui_text_index.inc"
};
static const WorldContentIndex selection_text_index[]={
#include "selection_text_index.inc"
};
static constexpr WorldContentIndex world_image_source[]={
#include "world_image_index.inc"
};
static constexpr auto world_image_layout=packed_content_index::layout(world_image_source);
static constexpr auto world_image_index=packed_content_index::make<world_image_layout.key_bytes,world_image_layout.group_bytes,world_image_layout.groups>(world_image_source);
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
static constexpr WorldContentIndex world_clip_source[]={
#include "world_clip_index.inc"
 {"~",0} // A terminal sentinel also permits resource-only metadata checkpoints.
};
static constexpr auto world_clip_layout=packed_content_index::layout(world_clip_source);
static constexpr auto world_clip_index=packed_content_index::make<world_clip_layout.key_bytes,world_clip_layout.group_bytes,world_clip_layout.groups>(world_clip_source);
static const WorldContentIndex phobia_clip_index[]={
#include "phobia_clip_index.inc"
};
static const WorldContentIndex phobia_background_index[]={
 {"phobia_backgrounds.FACTORY",PhobiaFactory},
 {"phobia_backgrounds.HIVE",PhobiaHive},
 {"phobia_backgrounds.KNOWLEDGE_DEMON_BOSS",PhobiaKnowledgeDemon},
 {"phobia_backgrounds.THE_INSATIABLE_BOSS",PhobiaInsatiable}
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
template<unsigned N,unsigned Bytes,unsigned GroupBytes,unsigned Groups,bool Relative>
static int wc_find(const packed_content_index::Table<N,Bytes,GroupBytes,Groups,Relative>&rows,const char*group,const char*key){
 // Keep the historical 255-byte composite-key limit without constructing
 // that temporary string. Domain and local key remain separate on disk.
 if(!group||!key)return -1;unsigned length=1;bool nested=false;
 for(const char*p=group;*p;++p){if(++length>=256)return -1;nested|=*p=='.';}
 for(const char*p=key;*p;++p)if(++length>=256)return -1;
 if(nested){
  // The former joined lookup also accepted a domain containing dots. Keep
  // that uncommon spelling equivalent instead of narrowing the public API.
  char joined[256];if(!wc_join(joined,sizeof joined,group,key))return -1;
  char*dot=joined;while(*dot!='.')++dot;*dot=0;return wc_find(rows,joined,dot+1);
 }
 unsigned lo=0,hi=Groups;
 while(lo<hi){const unsigned mid=(lo+hi)/2;if(wc_compare(rows.group(mid),group)<0)lo=mid+1;else hi=mid;}
 if(lo==Groups||wc_compare(rows.group(lo),group))return -1;
 const unsigned end=rows.end[lo],base=rows.group_base(lo);hi=end;lo=rows.first[lo];
 while(lo<hi){const unsigned mid=(lo+hi)/2;if(wc_compare(rows.key(mid,base),key)<0)lo=mid+1;else hi=mid;}
 return lo<end&&!wc_compare(rows.key(lo,base),key)?rows.values[lo]:-1;
}

}
int Content::world_string_id(const char*table,const char*key)const{
 if(has_selection_assets()){const int selection=wc_find(selection_text_index,table,key);if(selection>=0)return selection;}
 const int native=wc_find(native_ui_text_index,table,key);return native>=0?native:wc_find(world_text_index,table,key);
}
const char* Content::world_string(const char*table,const char*key)const{
 if(version_>=LocaleContentVersion){const char*text=locale_string(table,key);if(*text)return text;}
 if(version_>=NativeUiContentVersion){int id=wc_find(native_ui_text_index,table,key);if(id>=0)return string(id);}
 return version_>=14?string(wc_find(world_text_index,table,key)):"";
}
const char* Content::world_title(const char*table,const char*key)const{
 if(!key)return "";char joined[192];unsigned n=0;
 while(key[n]&&n+7<sizeof joined){joined[n]=key[n];++n;}if(key[n])return "";
 const char*tail=".title";while(*tail)joined[n++]=*tail++;joined[n]=0;
 return world_string(table,joined);
}
int Content::world_image(const char*group,const char*key)const{
 if(version_>=PhobiaContentVersion&&group&&!wc_compare(group,"phobia_backgrounds"))return ui_image(wc_find(phobia_background_index,group,key));
 if(version_>=NativeUiContentVersion){int id=wc_find(native_ui_index,group,key);if(id>=0)return ui_image(id);}
 return version_>=14?ui_image(wc_find(world_image_index,group,key)):-1;
}
int Content::world_clip(const char*model,const char*action)const{
 if(version_>=PhobiaContentVersion&&model&&!wc_compare(model,"phobia"))return wc_find(phobia_clip_index,model,action);
 return version_>=14?wc_find(world_clip_index,model,action):-1;
}
