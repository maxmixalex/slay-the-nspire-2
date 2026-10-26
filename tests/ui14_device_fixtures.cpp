// Explicit controlled device fixtures, using the production save/progress
// codecs and ordinary resume/console/core commands. No calculator image is
// opened by this exporter. Compile against the verified native core library.
#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
using namespace spire;
static App app;
static State loaded,after;
static UiProgress decoded;
static SaveData original,roundtrip;
static std::vector<uint8_t> input,progress_bytes;
static uint8_t progress_buffer[UiProgressCapacity];
static std::string output;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"device fixture:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> read(const char*name){
 FILE*f=std::fopen(name,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);CHECK(!std::fseek(f,0,SEEK_SET));
 std::vector<uint8_t> bytes(size_t(n),0);CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));return bytes;
}
static void write(const std::string&name,const void*data,size_t count){
 FILE*f=std::fopen((output+"/"+name).c_str(),"wb");CHECK(f);CHECK(std::fwrite(data,1,count,f)==count);CHECK(!std::fclose(f));
}
static bool load_run(State&s){return load(s,input.data(),input.size());}
static bool no_progress(UiProgress&){return false;}
static bool profile_info(unsigned,UiProgressSummary&out){out={};return false;}
static unsigned current_profile(){return 1;}
static bool store_progress(const UiProgress&p){
 const size_t n=encode_ui_progress(p,progress_buffer,sizeof progress_buffer);if(!n)return false;
 progress_bytes.assign(progress_buffer,progress_buffer+n);return true;
}
static void matching_progress(const std::string&stem){
 CHECK(load(loaded,input.data(),input.size())&&validate(loaded)&&save(loaded,original));
 CHECK(input.size()==original.size&&!std::memcmp(input.data(),original.bytes,original.size));
 app.callbacks={};app.callbacks.load=load_run;app.callbacks.load_progress=no_progress;app.callbacks.save_progress=store_progress;
 app.callbacks.profile_info=profile_info;app.callbacks.current_profile=current_profile;
 app.progress_loaded=false;app.restore_progress();CHECK(app.ftue_profile_pending);CHECK(app.refresh_save());
 CHECK(!app.ftue_profile_pending&&!app.progress.tutorial_flags);CHECK(app.load_saved_run());
 const auto&r=app.progress.current;CHECK(r.serial&&r.partial&&!r.facts.complete&&r.seed==loaded.seed&&r.run_seed==loaded.world.run_seed);
 CHECK(r.character==unsigned(loaded.character)&&r.ascension==loaded.world.ascension&&r.floor==loaded.world.total_floor);
 for(unsigned i=0;i<FtueCount;++i)CHECK(app.seen_ftue(FtueId(i))&&app.seen_popup(FtueId(i)));
 CHECK(save(app.game,roundtrip)&&original.size==roundtrip.size&&!std::memcmp(original.bytes,roundtrip.bytes,original.size));
 CHECK(app.persist_progress()&&!progress_bytes.empty());clear_ui_progress(decoded);
 CHECK(decode_ui_progress(progress_bytes.data(),progress_bytes.size(),decoded)&&!decoded.tutorial_flags&&decoded.current.partial);
 CHECK(decoded.current.seed==loaded.seed&&decoded.current.run_seed==loaded.world.run_seed);
 const size_t n=encode_ui_progress(decoded,progress_buffer,sizeof progress_buffer);
 CHECK(n==progress_bytes.size()&&!std::memcmp(progress_buffer,progress_bytes.data(),n));
 write(stem+".sav.tns",original.bytes,original.size);write(stem+".progress.tns",progress_bytes.data(),progress_bytes.size());
 std::printf("%s: valid NSR=%u, matching NSH%c=%zu, partial=%u, FTUE=%u, phase=%u, character=%u, seed=%08x, floor=%u\n",
  stem.c_str(),unsigned(original.size),progress_bytes[3],progress_bytes.size(),decoded.current.partial,decoded.tutorial_flags,
  unsigned(loaded.phase),unsigned(loaded.character),loaded.seed,unsigned(loaded.world.total_floor));
}
static unsigned dazed(const State&s){unsigned n=0;for(unsigned i=0;i<s.card_count;++i)n+=s.cards[i].kind==Dazed;return n;}
int main(int argc,char**argv){
 CHECK(argc==3);output=argv[2];std::filesystem::create_directories(output);
 input=read(argv[1]);matching_progress("ui14-merchant");CHECK(loaded.phase==Phase::Shop);
 app.callbacks={};CHECK(app.debug_command("new IRONCLAD 1104089578"));CHECK(app.debug_command("fight HauntedShipNormal"));
 std::printf("HauntedShip setup: valid=%u phase=%u enemies=%u encounter=%u model=%u node=%u reply=%s\n",
  unsigned(validate(app.game)),unsigned(app.game.phase),unsigned(app.game.enemy_count),unsigned(app.game.enemy_world.encounter),
  unsigned(app.game.enemy_world.models[0].kind),unsigned(app.game.enemy_world.models[0].node),app.debug_reply);
 CHECK(validate(app.game));CHECK(app.game.phase==Phase::Combat);CHECK(app.game.enemy_count==1);
 CHECK(app.game.enemy_world.models[0].kind==EnemyId::HauntedShip&&app.game.enemy_world.models[0].node==2);
 const unsigned count=dazed(app.game),cards=app.game.card_count;CHECK(count==0);
 CHECK(save(app.game,original));input.assign(original.bytes,original.bytes+original.size);
 after=app.game;CHECK(end_turn(after)&&validate(after));CHECK(dazed(after)==count+5&&after.card_count==cards+5);
 unsigned discarded=0;for(unsigned i=0;i<after.discard.count;++i)discarded+=after.cards[after.discard.ids[i]].kind==Dazed;
 CHECK(discarded==5);matching_progress("ui14-enemy-generated-cards");
 const char note[]="Controlled UI14 device fixtures. Not natural runs and not release contents.\n"
  "ui14-merchant: canonical bytes match the supplied complete-paired-merchant save. Its progress is produced by ordinary refresh_save/load_saved_run/begin_progress(true), marks the unobserved prefix partial, and has no FTUE prompts.\n"
  "ui14-enemy-generated-cards: source console new IRONCLAD 1104089578, then fight HauntedShipNormal. Do not play a card. Press End Turn once. The initial HAUNT_MOVE applies its real Weak and generates exactly five Dazed cards into Discard. No five-card animation is pre-inserted into the save. A separate actual end_turn execution verifies all five generated instances.\n"
  "Install the selected save as state/nSpire2.sav.tns and its matching progress as state/nSpire2.progress.tns on an isolated test profile. Readback both files. Do not overwrite the natural-run profile for measurement.\n";
 write("README.txt",note,sizeof note-1);std::printf("UI14 device fixtures: %u checks passed\n",checks);
}
