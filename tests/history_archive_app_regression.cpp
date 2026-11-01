#include "app.hpp"
#include "history_archive_store.hpp"
#include "ui_core_observer.hpp"
#include "world_run.hpp"
#include "run_score.hpp"
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"history archive App:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static App app;
static UiProgress restored;
static std::vector<uint8_t> disk;
static bool storage_ok=true;
static unsigned archive_calls;
static const char* fixture_directory;
static uint8_t encoded[UiProgressCapacity],before_progress[UiProgressCapacity];
static SaveData before_state,after_state;
static bool length(void*,uint32_t&n){n=unsigned(disk.size());return true;}
static bool read(void*,uint32_t at,void*out,unsigned n){if(at>disk.size()||n>disk.size()-at)return false;std::memcpy(out,disk.data()+at,n);return true;}
static bool write(void*,uint32_t at,const void*in,unsigned n){if(!storage_ok)return false;if(at+n>disk.size())disk.resize(at+n);std::memcpy(disk.data()+at,in,n);return true;}
static bool flush(void*){return storage_ok;}
static UiHistoryArchiveResult archive(UiHistoryArchiveQuery&q,uint8_t*bytes,unsigned&n){
 ++archive_calls;history_archive::IO io{nullptr,length,read,write,flush};return history_archive::access(io,q,bytes,n);
}
static void start(unsigned seed){
 app.history_open=app.compendium_open=false;app.history_index=0;
 CHECK(app.prepare_history());new_run(app.game,seed,false,Character::Ironclad);
 CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.title=false;app.begin_progress();
}
static void reset(){clear_ui_progress(app.progress);app.progress_loaded=true;app.progress_dirty=false;app.history_open=app.compendium_open=false;app.history_index=0;disk.clear();storage_ok=true;app.callbacks.history_archive=archive;}
static void export_fixture(const char*label){
 if(!fixture_directory)return;
 const size_t n=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(n);
 const auto output=[&](const char*suffix,const uint8_t*bytes,size_t size){
  const std::string path=std::string(fixture_directory)+"/"+label+suffix;FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
  CHECK(std::fwrite(bytes,1,size,f)==size);CHECK(!std::fclose(f));
 };
 output(".progress.nsh",encoded,n);output(".history.tns",disk.data(),disk.size());
}
int main(int argc,char**argv){
 fixture_directory=argc>1?argv[1]:nullptr;
 reset();
 // Real observed runs pass through production finish/statistics/ring paths.
 for(unsigned i=1;i<=70;++i){start(1000+i);app.finish_progress(true);}
 CHECK(app.progress.count==UiHistoryCapacity&&app.progress.characters[0].losses==70);export_fixture("synthetic-70-runs");
 const size_t size=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(size&&decode_ui_progress(encoded,size,restored));
 CHECK(restored.count==UiHistoryCapacity&&restored.history_oldest_archived==0);
 app.progress=restored;app.open_history();
 for(unsigned i=70;i;--i){const auto*r=app.history_at(app.history_index);CHECK(r&&r->serial==i&&r->seed==1000+i&&r->abandoned);if(i>1)app.history_select(1);}
 // A rolled-back progress write cannot recycle an already archived serial.
 app.history_open=app.compendium_open=false;app.progress.next_serial=1;start(9999);CHECK(app.progress.current.serial==71);
 // Reopen an old NSH9 profile: preserve its bytes and copy all recent records
 // before exposing disk-backed history. No destructive save-version upgrade.
 reset();app.callbacks.history_archive=nullptr;
 for(unsigned i=1;i<=32;++i){start(2000+i);app.finish_progress(true);}
 const size_t legacy=encode_ui_progress(app.progress,before_progress,sizeof before_progress);CHECK(legacy&&before_progress[3]=='9');
 app.callbacks.history_archive=archive;app.open_history();CHECK(app.history_index==255&&app.history_at(255)->serial==32);
 const size_t migrated=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(migrated==legacy&&!std::memcmp(encoded,before_progress,legacy));
 for(unsigned i=32;i;--i){CHECK(app.history_at(app.history_index)->serial==i);if(i>1)app.history_select(1);}
 // No archive scan/write occurs for ordinary input below a window threshold.
 reset();start(3001);app.game.world.total_floor=1;app.progress.current.floors[0].floor=1;const unsigned ordinary=archive_calls;
 for(unsigned i=0;i<100;++i)CHECK(app.prepare_history());CHECK(archive_calls==ordinary);
 // Observe 200 distinct points through the production recording path. At the
 // first boundary, inject a failed archive and prove preflight retains both
 // canonical State and every progress byte before permitting another command.
 for(unsigned floor=2;floor<=200;++floor){
  if(floor==96){
   CHECK(app.progress.current.floor_count==95);
   const size_t n=encode_ui_progress(app.progress,before_progress,sizeof before_progress);CHECK(n&&save(app.game,before_state));
   storage_ok=false;{UiCoreObserver observer(app);CHECK(!observer);}CHECK(save(app.game,after_state));
   CHECK(before_state.size==after_state.size&&!std::memcmp(before_state.bytes,after_state.bytes,before_state.size));
   const size_t after=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(after==n&&!std::memcmp(encoded,before_progress,n));
   storage_ok=true;
  }
  CHECK(app.prepare_history());app.game.world.total_floor=uint16_t(floor);app.game.world.current_point=uint8_t(app.game.world.current_point^1u);
  ++app.game.revision;app.sync_progress();
  auto&point=app.progress.current.floors[app.progress.current.floor_count-1];
  if(floor<=20)point.map_type=unsigned(reference::PointType::Unknown);
  if(floor==10||floor==11){point.room=unsigned(WorldRoom::Rest);point.rest_choices=1;}
 }
 CHECK(!app.progress.current.partial&&app.progress.current.facts.complete);
 app.game.phase=Phase::Complete;app.finish_progress();
 CHECK(ui_run_score(app.progress.current).floor_points==2000);
 CHECK(ui_run_badge_rarity(app.progress.current,UiRunBadge::MysteryMachine)==1);
 CHECK(ui_run_badge_rarity(app.progress.current,UiRunBadge::Restful)==1);
 CHECK(ui_run_badge_rarity(app.progress.current,UiRunBadge::Restless)==0);
 const size_t paged=encode_ui_progress(app.progress,encoded,sizeof encoded);CHECK(paged&&encoded[3]=='A'&&decode_ui_progress(encoded,paged,restored));
 CHECK(ui_run_score(restored.current).floor_points==2000);
 export_fixture("synthetic-200-floors");app.open_history();unsigned total=0,last=200;
 for(;;){const auto*r=app.history_at(app.history_index);CHECK(r&&r->serial==1&&r->floor_count);
  for(unsigned i=r->floor_count;i;--i){CHECK(r->floors[i-1].floor==last);--last;++total;}
  const unsigned first=r->floors[0].floor;if(first==1)break;app.history_select(1);
 }
 CHECK(total==200&&last==0);
 // One exceptionally long room can also retain more than the journal window.
 // Earlier pages own their recorded rows; the latest page retains live totals.
 reset();start(4001);
 for(unsigned i=0;i<6000;++i){CHECK(app.prepare_history());app.observe_history(app.game,{RunHistoryEvent::CardObtained,uint16_t(card_token({Strike,0}))});}
 CHECK(!app.progress.current.partial);app.finish_progress(true);app.open_history();
 unsigned rows=0;for(unsigned page=0;page<2;++page){rows+=app.history_journal().count;if(!page)app.history_select(1);}
 CHECK(rows==6000);
 std::printf("%u checks; 70 actual runs, NSH9 migration/reopen, 200 floors, 6000 notices, archive failure preserves State/progress; UiProgress=%zu\n",checks,sizeof(UiProgress));
}
