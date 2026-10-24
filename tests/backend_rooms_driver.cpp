// Optional asset-aware native oracle for this project's WASM backend (not PC parity).
#include "app.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;static SaveData storage,selection_before,selection_after;static std::string root,name;static unsigned saved;
static void write(const std::string&path,const void*p,size_t n){FILE*f=fopen(path.c_str(),"wb");if(!f||fwrite(p,1,n,f)!=n||fclose(f))std::exit(3);}
static bool save_cb(const State&s){if(!save(s,storage))return false;write(root+"/"+name+"-save-"+std::to_string(saved++)+".bin",storage.bytes,storage.size);return true;}
static bool load_cb(State&s){return load(s,storage.bytes,storage.size);}
static State fixture(EventId event,Character ch,bool shop){
 State s{};new_run(s,73,false,ch);
 if(shop){s.current_node=1;s.next_row=1;s.gold=500;choose_node(s,1);}
 else{s.phase=Phase::Event;s.event_id=event;s.current_node=3;s.next_row=1;s.floor=2;s.map[3].room=Room::Event;s.map[3].event=(uint8_t)event;if(event==EventId::DoorsOfLightAndDark)s.gold=0;}
 return s;
}
static const std::vector<CardKind> combat_cards[]={
 {Rage,Bash,IronWave,BodySlam},
 {Afterimage,PoisonedStab,CloakAndDagger,DaggerSpray},
 {DefectDarkness,DefectColdSnap,DefectFusion,DefectGlasswork,DefectMultiCast},
 {RegentArsenal,RegentWroughtInWar,RegentVenerate,RegentFallingStar,RegentRefineBlade,SovereignBlade},
 {NecroReaperForm,NecroBodyguard,NecroUnleash,NecroDeathbringer,NecroNoEscape,NecroTimesUp}
};
static State combat_fixture(Character character){
 State s{};new_run(s,73,true,character);s.energy=30;
 for(int i=0;i<s.enemy_count;i++)s.enemies[i].hp=s.enemies[i].max_hp=9999;
 while(s.hand.count){unsigned id=s.hand.ids[--s.hand.count];s.discard.ids[s.discard.count++]=(uint16_t)id;s.zones[id]=Zone::Discard;}
 if(character==Character::Defect)s.expansion.orb_slots=5;
 for(CardKind kind:combat_cards[(int)character])if(!lab_card(s,{kind,0}))std::exit(9);
 if(!validate(s))std::exit(10);return s;
}
static constexpr unsigned CombatCardStepCount=8,CombatCardSelectStep=2,CombatCardPlayStep=3;
static std::vector<int> combat_steps(unsigned character){
 std::vector<int> result={4}; // Load the controlled fixture from the title.
 for(unsigned i=0;i<combat_cards[character].size();i++){
  // NControllerCardPlay first selects the physical instance and restricts
  // target focus. A second Confirm plays it; selection must not mutate state.
  for(int key:{15,5,4,4,-1,-1,-1,-1})result.push_back(key);
 }
 for(int key:{8,6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,8,9,-1,7,4,5,5,18,3,5,8})result.push_back(key);
 return result;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;root=argv[2];FILE*f=fopen(argv[1],"rb");if(!f)return 2;std::vector<uint8_t>pack(spire_test::pack_size(f));size_t n=fread(pack.data(),1,pack.size(),f);fclose(f);
 const char*names[]={"aroma-transform","aroma-upgrade-silent","doors-remove","shop",
  "ironclad-combat","silent-combat","defect-orbs","regent-stars-forge","necro-osty-doom"};
 std::vector<int>steps[]={
  {4,-1,15,3,-1,5,4,8,1,4,4,15,-1,5,9,1,4,4,8},
  {4,1,4,8,1,4,4,15,3,-1,5,9,1,4,4,8},
  {4,1,4,8,1,4,4,15,-1,5,9,1,4,4,8},
  {4,-1,4,1,15,-1,5,4,8,5,4,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,8,5,9},
  combat_steps(0),combat_steps(1),combat_steps(2),combat_steps(3),combat_steps(4)
 };
 FILE*manifest=fopen((root+"/scenarios.json").c_str(),"w");if(!manifest)return 3;fprintf(manifest,"{\"cases\":[");
 for(int c=0;c<9;c++){
  name=names[c];saved=0;app=App{};
  State s=c>=4?combat_fixture((Character)(c-4)):fixture(c==2?EventId::DoorsOfLightAndDark:EventId::AromaOfChaos,c==1?Character::Silent:Character::Ironclad,c==3);if(!save(s,storage))return 5;
  if(!app.init(pack.data(),n,{save_cb,load_cb}))return 4;
  write(root+"/"+name+"-fixture.bin",storage.bytes,storage.size);
  fprintf(manifest,"%s{\"name\":\"%s\",\"steps\":[",c?",":"",name.c_str());unsigned index=0,script_index=0,readiness_steps=0;
  const auto record=[&](int key){
   if(key<0)app.advance(99);else app.input((Key)key);app.render();
   if(!app.title&&!validate(app.game))std::exit(6);
   write(root+"/"+name+"-frame-"+std::to_string(index)+".rgb",app.pixels,sizeof app.pixels);
   fprintf(manifest,"%s%d",index?",":"",key);++index;
  };
  for(int key:steps[c]){
   // Real packs first finish bounded resource preparation. A source-timed
   // multi-hit command can also exceed the old four 99ms playback frames.
   // Record every necessary update so WASM replays the same visible sequence;
   // retain every scripted detail/play/navigation/save action and its checks.
   if(c>=4&&key>=0){unsigned waits=0;
    while(app.combat_preparation.active||app.combat_actions_locked()){
     if(++waits>1024){std::fprintf(stderr,"%s did not become ready before script step %u\n",name.c_str(),script_index);return 13;}
     record(-1);++readiness_steps;
    }
   }
   const bool card_step=c>=4&&script_index>0&&(script_index-1)/CombatCardStepCount<combat_cards[c-4].size();
   const unsigned card_offset=script_index?((script_index-1)%CombatCardStepCount):CombatCardStepCount;
   int selected=-1;
   if(card_step&&card_offset==CombatCardSelectStep){
    if(app.cursor<0||app.cursor>=app.game.hand.count||app.keyboard_card>=0||!save(app.game,selection_before))return 14;
    selected=app.game.hand.ids[app.cursor];
   }
   const uint32_t revision=app.game.revision;record(key);
   if(card_step&&card_offset==CombatCardSelectStep){
    if(app.keyboard_card!=selected||app.game.revision!=revision||app.game.phase!=Phase::Combat||
       !save(app.game,selection_after)||selection_before.size!=selection_after.size||
       std::memcmp(selection_before.bytes,selection_after.bytes,selection_before.size))return 14;
   }
   if(card_step&&card_offset==CombatCardPlayStep){
    if(app.game.revision==revision||app.game.phase!=Phase::Combat)return 11;
   }
   ++script_index;
  }
  if(c<3&&(app.game.phase!=Phase::Event||app.game.event_page<2))return 7;
  if(c==3&&(!(app.game.shop_sold&(1u<<ShopRemove))||app.game.phase!=Phase::Shop))return 8;
  if(c>=4&&(app.game.phase!=Phase::Combat||app.game.turn!=2||saved!=3))return 12;
  fprintf(manifest,"],\"saves\":%u}",saved);
  std::printf("%s: %u frames, %u recorded readiness updates, %u saves\n",name.c_str(),index,readiness_steps,saved);
 }
 fprintf(manifest,"]}\n");fclose(manifest);puts("Wrote four controlled room sequences and five character combat sequences");return 0;
}
