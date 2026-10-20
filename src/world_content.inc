namespace {
struct WorldContentIndex {const char* key;uint16_t index;};
static const WorldContentIndex world_text_index[]={
#include "world_text_index.inc"
};
static const WorldContentIndex world_image_index[]={
#include "world_image_index.inc"
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
 return version_>=14?string(wc_find(world_text_index,table,key)):"";
}
const char* Content::world_title(const char*table,const char*key)const{
 if(!key)return "";char joined[192];unsigned n=0;
 while(key[n]&&n+7<sizeof joined){joined[n]=key[n];++n;}if(key[n])return "";
 const char*tail=".title";while(*tail)joined[n++]=*tail++;joined[n]=0;
 return world_string(table,joined);
}
int Content::world_image(const char*group,const char*key)const{
 return version_>=14?ui_image(wc_find(world_image_index,group,key)):-1;
}
int Content::world_clip(const char*model,const char*action)const{
 return version_>=14?wc_find(world_clip_index,model,action):-1;
}
