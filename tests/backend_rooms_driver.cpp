// Optional asset-aware native oracle for this project's WASM backend (not PC parity).
#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace spire;
static App app;static SaveData storage;static std::string root,name;static unsigned saved;
static void write(const std::string&path,const void*p,size_t n){FILE*f=fopen(path.c_str(),"wb");if(!f||fwrite(p,1,n,f)!=n||fclose(f))std::exit(3);}
static bool save_cb(const State&s){if(!save(s,storage))return false;write(root+"/"+name+"-save-"+std::to_string(saved++)+".bin",storage.bytes,storage.size);return true;}
static bool load_cb(State&s){return load(s,storage.bytes,storage.size);}
static State fixture(EventId event,Character ch,bool shop){
 State s{};new_run(s,73,false,ch);
 if(shop){s.current_node=1;s.next_row=1;s.gold=500;choose_node(s,1);}
 else{s.phase=Phase::Event;s.event_id=event;s.current_node=3;s.next_row=1;s.floor=2;s.map[3].room=Room::Event;s.map[3].event=(uint8_t)event;if(event==EventId::DoorsOfLightAndDark)s.gold=0;}
 return s;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;root=argv[2];std::vector<uint8_t>pack(MaxPack);FILE*f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(pack.data(),1,pack.size(),f);fclose(f);
 const char*names[]={"aroma-transform","aroma-upgrade-silent","doors-remove","shop"};
 const std::vector<int>steps[]={
  {3,4,-1,15,3,-1,5,4,8,1,4,4,15,-1,5,9,1,4,4,8},
  {3,4,1,4,8,1,4,4,15,3,-1,5,9,1,4,4,8},
  {3,4,1,4,8,1,4,4,15,-1,5,9,1,4,4,8},
  {3,4,-1,4,1,15,-1,5,4,8,5,4,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,8,5,9}
 };
 FILE*manifest=fopen((root+"/scenarios.json").c_str(),"w");if(!manifest)return 3;fprintf(manifest,"{\"cases\":[");
 for(int c=0;c<4;c++){
  name=names[c];saved=0;app=App{};if(!app.init(pack.data(),n,{save_cb,load_cb}))return 4;
  State s=fixture(c==2?EventId::DoorsOfLightAndDark:EventId::AromaOfChaos,c==1?Character::Silent:Character::Ironclad,c==3);if(!save(s,storage))return 5;
  write(root+"/"+name+"-fixture.bin",storage.bytes,storage.size);
  fprintf(manifest,"%s{\"name\":\"%s\",\"steps\":[",c?",":"",name.c_str());unsigned index=0;
  for(int key:steps[c]){
   if(key<0)app.advance(99);else app.input((Key)key);app.render();
   if(!app.title&&!validate(app.game))return 6;
   write(root+"/"+name+"-frame-"+std::to_string(index)+".rgb",app.pixels,sizeof app.pixels);
   fprintf(manifest,"%s%d",index?",":"",key);++index;
  }
  if(c<3&&(app.game.phase!=Phase::Event||app.game.event_page<2))return 7;
  if(c==3&&(!(app.game.shop_sold&(1u<<ShopRemove))||app.game.phase!=Phase::Shop))return 8;
  fprintf(manifest,"],\"saves\":%u}",saved);
 }
 fprintf(manifest,"]}\n");fclose(manifest);puts("Wrote four controlled native room sequences");return 0;
}
