#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks,writes,summary_reads,active=1;
static const char*scenario="setup";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FTUE progress [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static App app;
static UiProgress fixture,decoded;
static SaveData raw,before,after;
static Bytes profiles[UiProfileCount];
static bool has_run[UiProfileCount]{},storage_ok=true;
static uint8_t encoded[UiProgressCapacity];
static const char*export_directory=nullptr;
static void export_bytes(const char*name,const uint8_t*bytes,size_t length){
 if(!export_directory)return;const std::string path=std::string(export_directory)+"/"+name;
 FILE*file=std::fopen(path.c_str(),"wb");CHECK(file);const bool written=std::fwrite(bytes,1,length,file)==length;
 const bool closed=std::fclose(file)==0;CHECK(written&&closed);
}
static void export_bytes(const char*name,const Bytes&bytes){export_bytes(name,bytes.data(),bytes.size());}
static unsigned current_profile(){return active;}
static bool load_run(State&s){return has_run[active-1]&&load(s,raw.bytes,raw.size);}
static bool read_progress(UiProgress&out){const auto&data=profiles[active-1];return decode_ui_progress(data.data(),data.size(),out);}
static bool write_progress(const UiProgress&value){
 if(!storage_ok)return false;const size_t n=encode_ui_progress(value,encoded,sizeof encoded);if(!n)return false;
 ++writes;profiles[active-1].assign(encoded,encoded+n);return true;
}
static bool profile_info(unsigned,UiProgressSummary&out){++summary_reads;out={};return false;}
static void select(unsigned slot){active=slot;app.progress_loaded=false;app.restore_progress();app.refresh_save();}
static Bytes encode(const UiProgress&value){const size_t n=encode_ui_progress(value,encoded,sizeof encoded);CHECK(n);return Bytes(encoded,encoded+n);}
static void check_simulation(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void legacy_fixture(unsigned version){
 clear_ui_progress(fixture);fixture.current.serial=1;fixture.current.floor_count=1;
 if(version>=3){fixture.current.mode=unsigned(RunMode::Custom);fixture.current.modifiers=run_modifier_bit(RunModifier::Flight);}
 if(version>=4){fixture.map_drawing.seed=1;fixture.map_drawing.serial=1;fixture.map_drawing.count=1;fixture.map_drawing.points[0].y_flags=MapDrawingPoint::Start;}
 if(version>=5)fixture.test_subject_kills=3;
 if(version>=6)fixture.current.facts.version=1;
 if(version>=7){fixture.journal.owners[0].serial=1;fixture.journal.owners[0].observe(0,true);CHECK(fixture.journal.append(0,0,RunHistoryEvent::CardObtained,card_token({Bash,1})));}
}
static void migration(){
 scenario="NSH1-7 remain an explicit unknown/suppressed compatibility state";
 for(unsigned version=2;version<=7;++version){legacy_fixture(version);Bytes old=encode(fixture);CHECK(old[3]=='0'+version);
  CHECK(decode_ui_progress(old.data(),old.size(),decoded)&&!decoded.tutorial_flags);
  app.progress=decoded;for(unsigned id=0;id<FtueCount;++id){CHECK(app.seen_ftue(FtueId(id))&&app.seen_popup(FtueId(id)));CHECK(!(app.progress.tutorial_flags&ftue_bit(FtueId(id))));}
  CHECK(encode(decoded)==old);
  if(version==2){constexpr size_t discoveries=(CardKinds+7)/8+(WorldRelicKinds+7)/8+(PotionKinds+7)/8+(OriginalEventKinds+7)/8;
   const size_t tail=28+discoveries+CardKinds*4,n=old.size()-4-CardKinds*4;std::memmove(old.data()+24,old.data()+28,discoveries);std::memmove(old.data()+24+discoveries,old.data()+tail,old.size()-tail);
   old.resize(n);old[3]='1';for(unsigned i=0;i<4;++i)old[4+i]=uint8_t(n>>(i*8));
   CHECK(decode_ui_progress(old.data(),old.size(),decoded)&&!decoded.tutorial_flags);}
 }
 scenario="NSH8 adds two bytes after an identical complete NSH7 prefix";
 legacy_fixture(7);const Bytes seven=encode(fixture);fixture.tutorial_flags=FtueFresh;const Bytes eight=encode(fixture);
 export_bytes("legacy-profile.nsh",seven);
 CHECK(eight[3]=='8'&&eight.size()==seven.size()+2&&!std::memcmp(eight.data()+8,seven.data()+8,seven.size()-8));
 CHECK(eight[eight.size()-2]==0&&eight.back()==0x60&&decode_ui_progress(eight.data(),eight.size(),decoded)&&decoded.tutorial_flags==FtueFresh);
 UiProgressSummary summary;CHECK(decode_ui_progress_summary(eight.data(),eight.size(),summary)&&summary.exists);
 for(uint16_t flags:{uint16_t(0x8000),uint16_t(0xffff),FtueEnabled,uint16_t(1)}){fixture.tutorial_flags=flags;CHECK(!encode_ui_progress(fixture,encoded,sizeof encoded));}
 fixture.tutorial_flags=FtueFresh|FtueSeenMask;CHECK(decode_ui_progress(encode(fixture).data(),eight.size(),decoded));
 Bytes malformed=eight;malformed.back()=0x80;CHECK(!decode_ui_progress(malformed.data(),malformed.size(),decoded));
 CHECK(!decode_ui_progress(eight.data(),eight.size()-1,decoded));malformed=eight;malformed.push_back(0);CHECK(!decode_ui_progress(malformed.data(),malformed.size(),decoded));
 malformed=eight;malformed[3]=':';CHECK(!decode_ui_progress(malformed.data(),malformed.size(),decoded)&&!decode_ui_progress_summary(malformed.data(),malformed.size(),summary));
}
static void profiles_and_preference(){
 scenario="new profiles initialize after the existing current-run read, without a profile scan";
 app.callbacks.load=load_run;app.callbacks.load_progress=read_progress;app.callbacks.save_progress=write_progress;
 app.callbacks.current_profile=current_profile;app.callbacks.profile_info=profile_info;
 new_run(app.game,812,false,Character::Defect);CHECK(save(app.game,raw)&&save(app.game,before));
 export_bytes("legacy-run.sav",raw.bytes,raw.size);
 for(auto&data:profiles)data.clear();for(auto&run:has_run)run=false;summary_reads=0;writes=0;
 select(1);CHECK(app.progress.tutorial_flags==FtueFresh&&!app.ftue_profile_pending&&!summary_reads&&app.progress_dirty);
 export_bytes("enabled-profile.nsh",encode(app.progress));
 for(unsigned i=0;i<FtueCount;++i)CHECK(!app.seen_ftue(FtueId(i))&&!app.seen_popup(FtueId(i)));
 CHECK(app.seen_ftue(FtueId::Count)&&app.seen_popup(FtueId::Count));check_simulation();
 scenario="No disables ordinary FTUE while an unseen Ascension popup remains available";
 app.mark_ftue(FtueId::AcceptTutorials);CHECK(writes==1&&profiles[0][3]=='8'&&app.seen_ftue(FtueId::AcceptTutorials));
 app.mark_ftue(FtueId::AcceptTutorials);CHECK(writes==1);
 app.set_ftues_enabled(false);CHECK(writes==2&&app.progress.tutorial_flags==(FtueInitialized|ftue_bit(FtueId::AcceptTutorials)));
 export_bytes("declined-profile.nsh",profiles[0]);
 CHECK(app.seen_ftue(FtueId::CombatRules)&&!app.seen_popup(FtueId::AscensionSingleplayer));
 app.mark_ftue(FtueId::AscensionSingleplayer);CHECK(app.seen_popup(FtueId::AscensionSingleplayer)&&!(app.progress.tutorial_flags&FtueEnabled));
 const Bytes first=profiles[0];check_simulation();
 scenario="legacy metadata and legacy raw run retain their old unobstructed UI";
 legacy_fixture(7);profiles[1]=encode(fixture);const Bytes old=profiles[1];select(2);
 CHECK(!app.progress.tutorial_flags&&!app.progress_dirty&&app.seen_ftue(FtueId::AcceptTutorials)&&app.seen_popup(FtueId::AscensionSingleplayer));
 CHECK(profiles[1]==old&&profiles[0]==first&&!summary_reads);
 const unsigned old_writes=writes;
 for(unsigned i=0;i<FtueCount;++i)app.mark_ftue(FtueId(i));
 CHECK(!app.progress.tutorial_flags&&!app.progress_dirty&&profiles[1]==old&&writes==old_writes);
 CHECK(app.seen_popup(FtueId::AscensionSingleplayer)); // Automatic marks cannot initialize an unknown old profile.
 has_run[2]=true;select(3);CHECK(app.has_save&&!app.progress.tutorial_flags&&profiles[2].empty()&&!app.progress_dirty&&!summary_reads);
 scenario="three-profile completion stays independent and reset does not change State";
 select(1);CHECK(app.progress.tutorial_flags==(FtueInitialized|ftue_bit(FtueId::AcceptTutorials)|ftue_bit(FtueId::AscensionSingleplayer)));
 app.reset_ftues();CHECK(app.progress.tutorial_flags==FtueFresh&&!app.seen_popup(FtueId::AscensionSingleplayer)&&profiles[1]==old&&profiles[2].empty());check_simulation();
 scenario="failed storage leaves tutorial progress dirty for the existing retry path";
 const Bytes previous=profiles[0];storage_ok=false;app.mark_ftue(FtueId::CombatRules);
 CHECK(app.seen_ftue(FtueId::CombatRules)&&app.progress_dirty&&profiles[0]==previous&&!app.persist_progress());check_simulation();
 storage_ok=true;CHECK(app.persist_progress()&&!app.progress_dirty&&profiles[0]!=previous);select(1);CHECK(app.seen_ftue(FtueId::CombatRules)&&!app.seen_ftue(FtueId::MapSelect));
 scenario="deleted profiles become fresh; no persistent host means no automatic tutorials";
 profiles[0].clear();has_run[0]=false;select(1);CHECK(app.progress.tutorial_flags==FtueFresh);
 app.callbacks={};app.progress_loaded=false;app.restore_progress();app.refresh_save();CHECK(!app.progress.tutorial_flags&&!app.ftue_profile_pending&&app.seen_ftue(FtueId::AcceptTutorials));
 app.callbacks.load=load_run;app.callbacks.load_progress=read_progress;app.callbacks.save_progress=write_progress;app.callbacks.current_profile=current_profile;
 app.progress_loaded=false;app.restore_progress();app.refresh_save();CHECK(!app.progress.tutorial_flags&&!app.ftue_profile_pending); // X11 headless has no interactive profile_info callback.
 scenario="late-tail damage is cleared before optional progress can be published";
 app.callbacks.load=load_run;app.callbacks.load_progress=read_progress;app.callbacks.save_progress=write_progress;app.callbacks.current_profile=current_profile;
 fixture.tutorial_flags=FtueFresh;profiles[0]=encode(fixture);profiles[0].back()=0x80;has_run[0]=true;select(1);
 CHECK(!app.progress.tutorial_flags&&!app.progress.current.serial&&!app.progress.journal.count&&app.has_save);check_simulation();
}
static void maximum(){
 scenario="full NSH8 adds only two bytes to the maximum NSH7 profile";clear_ui_progress(fixture);fixture.count=32;fixture.next_serial=34;
 for(unsigned i=0;i<33;++i){auto&r=i<32?fixture.history[i]:fixture.current;r.serial=i+1;r.floor_count=UiHistoryFloors;r.deck_count=MaxDeck;r.relic_count=WorldRelicCapacity;r.facts.version=1;
  for(unsigned f=0;f<UiHistoryFloors;++f){r.floors[f].floor=uint16_t(f+1);r.floors[f].act=uint8_t(f%3);}
  for(unsigned n=0;n<WorldRelicCapacity;++n)r.relics[n]=uint16_t(n%WorldRelicKinds);
  auto&o=fixture.journal.owners[i];o.serial=i+1;for(auto&bits:o.known)bits=~uint32_t(0);}
 for(unsigned i=0;i<UiJournalEntries;++i)CHECK(fixture.journal.append(i%33,i%UiHistoryFloors,RunHistoryEvent::CardSkipped,card_token({Strike,uint8_t(i&1)})));
 for(unsigned i=0;i<MapDrawingCapacity;++i)CHECK(fixture.map_drawing.append(int(i%1921),int(i%3241),MapDrawingMode::Draw,true));
 for(unsigned category=0;category<4;++category){fixture.current_discoveries.count[category]=10;for(unsigned i=0;i<10;++i)fixture.current_discoveries.first[category][i]=uint16_t(i);}
 const Bytes seven=encode(fixture);CHECK(seven.size()==295347&&seven[3]=='7');fixture.tutorial_flags=FtueFresh|FtueSeenMask;const Bytes eight=encode(fixture);
 CHECK(eight.size()==295349&&eight[3]=='8'&&!std::memcmp(eight.data()+8,seven.data()+8,seven.size()-8));
 CHECK(decode_ui_progress(eight.data(),eight.size(),decoded)&&encode(decoded)==eight);
 export_bytes("full-profile.nsh",eight);
 std::printf("Maximum NSH8=%zu bytes; UiProgress=%zu; tutorial field=%zu bytes.\n",eight.size(),sizeof(UiProgress),sizeof fixture.tutorial_flags);
 scenario="NSH9 adds only the fixed observed run metadata tail";
 fixture.current_saved_at=1792884600;fixture.current_saved_revision=17;
 for(auto&r:fixture.history)std::memcpy(r.build_id,UiCurrentBuildId,sizeof UiCurrentBuildId);
 std::memcpy(fixture.current.build_id,UiCurrentBuildId,sizeof UiCurrentBuildId);
 const Bytes nine=encode(fixture);CHECK(nine[3]=='9'&&nine.size()==eight.size()+UiProgressMetadataBytes);
 CHECK(!std::memcmp(nine.data()+8,eight.data()+8,eight.size()-8));
 CHECK(decode_ui_progress(nine.data(),nine.size(),decoded)&&encode(decoded)==nine);
 export_bytes("full-profile-v9.nsh",nine);
 std::printf("Maximum NSH9=%zu bytes; metadata tail=%u bytes.\n",nine.size(),UiProgressMetadataBytes);
}
int main(int argc,char**argv){
 if(argc==3&&!std::strcmp(argv[1],"--export"))export_directory=argv[2];
 else if(argc!=1){std::fprintf(stderr,"usage: ftue_progress_regression [--export EXISTING_DIRECTORY]\n");return 2;}
 migration();profiles_and_preference();maximum();std::printf("FTUE progress: %u checks passed, per-profile semantics and NSH1-8 migration.\n",checks);
}
