#include "ui_profiles.hpp"
#include "ui_progress.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks;
static const char*scenario="setup";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Ndless history [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static std::map<std::string,Bytes> files;
static std::string fail_open,fail_close,fail_unlink,fail_seek,fail_tell,fail_read;
static std::vector<std::string> fail_renames;
static size_t write_limit=SIZE_MAX,written=0,read_fail_after=0,peak=0;
static unsigned max_write=0,reads=0,writes=0;
static bool reenter=false,replace_existing=false;
struct File {std::string path;size_t offset=0;};
static void count_peak(){size_t total=0;for(const auto&entry:files)total+=entry.second.size();peak=std::max(peak,total);}
static void*nd_open(const char*path,const char*mode){
 if(fail_open==path){fail_open.clear();return nullptr;}
 if(*mode=='w')files[path]={};else if(!files.count(path))return nullptr;
 return new File{path};
}
static unsigned nd_read(void*output,unsigned size,unsigned count,void*handle){
 ++reads;if(reenter){ResourceTransferLease nested(ResourceTransferUse::Bytes);CHECK(!nested);}
 auto&f=*static_cast<File*>(handle);const auto&b=files.at(f.path);
 size_t n=std::min(size_t(size)*count,b.size()-f.offset);
 if(fail_read==f.path){const size_t left=f.offset<read_fail_after?read_fail_after-f.offset:0;
  if(n>left){n=left;fail_read.clear();}}
 if(n)std::memcpy(output,b.data()+f.offset,n);f.offset+=n;return unsigned(n/size);
}
static unsigned nd_write(const void*input,unsigned size,unsigned count,void*handle){
 ++writes;max_write=std::max(max_write,size*count);
 if(reenter){ResourceTransferLease nested(ResourceTransferUse::Bytes);CHECK(!nested);}
 const size_t n=std::min(size_t(size)*count,written<write_limit?write_limit-written:0);written+=n;
 auto&f=*static_cast<File*>(handle);const auto*p=static_cast<const uint8_t*>(input);auto&b=files.at(f.path);
 b.insert(b.end(),p,p+n);f.offset+=n;count_peak();return unsigned(n/size);
}
static int nd_close(void*handle){auto*f=static_cast<File*>(handle);const bool fail=fail_close==f->path;
 if(fail)fail_close.clear();delete f;return fail?-1:0;}
static int nd_seek(void*handle,long offset,int origin){
 auto&f=*static_cast<File*>(handle);if(fail_seek==f.path){fail_seek.clear();return -1;}
 const long base=origin==2?long(files.at(f.path).size()):origin==1?long(f.offset):0;
 if(offset< -base||static_cast<unsigned long>(base+offset)>files.at(f.path).size())return -1;
 f.offset=size_t(base+offset);return 0;
}
static long nd_tell(void*handle){auto&f=*static_cast<File*>(handle);if(fail_tell==f.path){fail_tell.clear();return -1;}return long(f.offset);}
static int nd_rename(const char*from,const char*to){
 for(auto i=fail_renames.begin();i!=fail_renames.end();++i)if(*i==from){fail_renames.erase(i);return -1;}
 auto found=files.find(from);if(found==files.end()||(!replace_existing&&files.count(to)))return -1;
 files[to]=std::move(found->second);files.erase(found);return 0;
}
static int nd_unlink(const char*path){if(fail_unlink==path){fail_unlink.clear();return -1;}return files.erase(path)?0:-1;}
static char save_path[512],temp_path[512],backup_path[512],progress_path[512],progress_temp[512],progress_backup[512];
static char settings_path[512],settings_temp[512],settings_backup[512];
static struct AppResult {bool title=true;void finish_profile_change(bool){}} app;
static bool sibling(char*out,const char*launcher,const char*name){
 const char*slash=std::strrchr(launcher,'/');const size_t n=slash?size_t(slash-launcher)+1:0;
 if(n+std::strlen(name)>=512)return false;std::memcpy(out,launcher,n);std::strcpy(out+n,name);return true;
}
// Compile the actual calculator callbacks, including their profile summary
// routing and atomic rename decisions, against a fault-injecting filesystem.
#include "../platform/ndless/profile_storage.inc"
#include "../platform/ndless/progress_storage.inc"
static UiProgress value,loaded,decoded_reference;
static uint8_t encoded[UiProgressCapacity];
static const char*export_directory=nullptr;
static void reset_io(){fail_open.clear();fail_close.clear();fail_unlink.clear();fail_seek.clear();fail_tell.clear();fail_read.clear();fail_renames.clear();
 write_limit=SIZE_MAX;written=peak=0;max_write=reads=writes=0;reenter=replace_existing=false;count_peak();}
static void setup(){files.clear();reset_io();profile_launcher="/calc/game/nSpire2.tns";selected_profile=1;pending_profile=0;CHECK(configure_profile_paths(1));CHECK(!resource_transfer_uses);}
static Bytes canonical(const UiProgress&v){const size_t n=encode_ui_progress(v,encoded,sizeof encoded);CHECK(n);return Bytes(encoded,encoded+n);}
static void fixture(unsigned version){
 clear_ui_progress(value);value.current.serial=1;value.current.floor_count=1;value.last_saved_at=123456;
 value.characters[0].play_seconds=15;value.characters[1].play_seconds=27;
 if(version>=3){value.current.mode=unsigned(RunMode::Custom);value.current.modifiers=run_modifier_bit(RunModifier::Flight);}
 if(version>=4){value.map_drawing.seed=1;value.map_drawing.serial=1;value.map_drawing.count=1;value.map_drawing.points[0].y_flags=MapDrawingPoint::Start;}
 if(version>=5)value.test_subject_kills=3;
 if(version>=6)value.current.facts.version=1;
 if(version>=7){value.journal.owners[0].serial=1;value.journal.owners[0].observe(0,true);CHECK(value.journal.append(0,0,RunHistoryEvent::CardObtained,card_token({Bash,1})));}
 if(version>=8)value.tutorial_flags=FtueFresh|FtueSeenMask;
}
static Bytes legacy(unsigned version){
 fixture(version);Bytes b=canonical(value);CHECK(b[3]=='0'+std::max(2u,version));
 if(version==1){constexpr size_t discoveries=(CardKinds+7)/8+(WorldRelicKinds+7)/8+(PotionKinds+7)/8+(OriginalEventKinds+7)/8;
  const size_t tail=28+discoveries+CardKinds*4,n=b.size()-4-CardKinds*4;
  std::memmove(b.data()+24,b.data()+28,discoveries);std::memmove(b.data()+24+discoveries,b.data()+tail,b.size()-tail);
  b.resize(n);b[3]='1';for(unsigned i=0;i<4;++i)b[4+i]=uint8_t(n>>(i*8));}
 CHECK(decode_ui_progress(b.data(),b.size(),decoded_reference));return b;
}
static Bytes envelope(const Bytes&raw){
 void*f=nd_open("/envelope","wb");CHECK(f&&storage_codec::write(f,raw.data(),unsigned(raw.size())));CHECK(nd_close(f)==0);
 Bytes result=files.at("/envelope");files.erase("/envelope");return result;
}
static Bytes unpack_file(const Bytes&b){
 files["/decode"]=b;void*f=nd_open("/decode","rb");CHECK(f);unsigned n=0;storage_codec::ReadStatus status;
 CHECK(progress_file_read(f,encoded,n,status)&&status==storage_codec::ReadStatus::Complete);CHECK(nd_close(f)==0);
 files.erase("/decode");return Bytes(encoded,encoded+n);
}
static void export_bytes(const char*name,const Bytes&bytes){
 if(!export_directory)return;const std::string path=std::string(export_directory)+"/"+name;
 FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);CHECK(std::fwrite(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(std::fclose(f)==0);
}
static void versions(){
 scenario="raw and wrapped NSH1-8 retain the same canonical values and summary";
 for(unsigned version=1;version<=8;++version)for(unsigned packed=0;packed<2;++packed){
  setup();const Bytes raw=legacy(version),expected=canonical(decoded_reference),stored=packed?envelope(raw):raw;
  files[progress_path]=stored;const auto before=files;reenter=true;
  CHECK(load_progress(loaded)&&!resource_transfer_uses);CHECK(progress_recovery==ProgressRecovery::PrimaryValid);
  reenter=false;CHECK(canonical(loaded)==expected&&files==before);
  UiProgressSummary summary,original;CHECK(decode_ui_progress_summary(raw.data(),raw.size(),original));
  reenter=true;CHECK(profile_info(1,summary)&&!resource_transfer_uses);reenter=false;
  CHECK(summary.exists==original.exists&&summary.play_seconds==42&&summary.last_saved_at==original.last_saved_at);
  // NSH1's established canonical upgrade to NSH2 remains owned by the codec.
  CHECK(save_progress(loaded)&&!resource_transfer_uses);CHECK(unpack_file(files.at(progress_path))==expected);
  CHECK(!files.count(progress_temp)&&!files.count(progress_backup));CHECK(load_progress(loaded)&&canonical(loaded)==expected);
 }
}
static void maximum(){
 scenario="complete H8 uses the real writer, odd byte length and 512-byte blocks";setup();clear_ui_progress(value);value.count=32;value.next_serial=34;
 for(unsigned i=0;i<33;++i){auto&r=i<32?value.history[i]:value.current;r.serial=i+1;r.floor_count=UiHistoryFloors;r.deck_count=MaxDeck;r.relic_count=WorldRelicCapacity;r.facts.version=1;
  for(unsigned f=0;f<UiHistoryFloors;++f){r.floors[f].floor=uint16_t(f+1);r.floors[f].act=uint8_t(f%3);}
  for(unsigned n=0;n<WorldRelicCapacity;++n)r.relics[n]=uint16_t(n%WorldRelicKinds);
  auto&o=value.journal.owners[i];o.serial=i+1;for(auto&bits:o.known)bits=~uint32_t(0);}
 for(unsigned i=0;i<UiJournalEntries;++i)CHECK(value.journal.append(i%33,i%UiHistoryFloors,RunHistoryEvent::CardSkipped,card_token({Strike,uint8_t(i&1)})));
 for(unsigned i=0;i<MapDrawingCapacity;++i)CHECK(value.map_drawing.append(int(i%1921),int(i%3241),MapDrawingMode::Draw,true));
 for(unsigned category=0;category<4;++category){value.current_discoveries.count[category]=10;for(unsigned i=0;i<10;++i)value.current_discoveries.first[category][i]=uint16_t(i);}
 value.tutorial_flags=FtueFresh|FtueSeenMask;const Bytes raw=canonical(value);CHECK(raw.size()==295349&&raw[3]=='8');
 reenter=true;CHECK(save_progress(value)&&!resource_transfer_uses);reenter=false;const Bytes stored=files.at(progress_path);
 CHECK(stored.size()==74001&&max_write==storage_codec::BlockBytes&&unpack_file(stored)==raw);
 CHECK(load_progress(loaded)&&canonical(loaded)==raw);UiProgressSummary summary;CHECK(profile_info(1,summary)&&summary.exists);
 export_bytes("full-profile.nsh",raw);export_bytes("full-profile-storage.tns",stored);
 CHECK(4*stored.size()+6*121503+340==1025362);
 CHECK(4*stored.size()+3*121503+3*252928+340==1419637);
 std::printf("Full H8 fixture: canonical=%zu stored=%zu; dense peak=1025362, raw-run-backup migration peak=1419637. This is a concrete fixture, not every valid history's compression bound.\n",raw.size(),stored.size());
}
static void rejection(const Bytes&b,storage_codec::ReadStatus expected=storage_codec::ReadStatus::Invalid){
 files["/bad"]=b;std::vector<uint8_t>buffer(UiProgressCapacity+16,0xa5);void*f=nd_open("/bad","rb");CHECK(f);
 unsigned n=99;storage_codec::ReadStatus status;CHECK(!progress_file_read(f,buffer.data()+8,n,status)&&!n&&status==expected);CHECK(nd_close(f)==0);
 for(unsigned i=0;i<8;++i)CHECK(buffer[i]==0xa5&&buffer[buffer.size()-1-i]==0xa5);files.erase("/bad");
}
static void bounds(){
 scenario="bounded envelope truncation, trailing bytes and incompressible fallback";setup();const Bytes raw=legacy(8),packed=envelope(raw);CHECK(packed.size()<raw.size());
 for(size_t n=0;n<packed.size();++n)rejection(Bytes(packed.begin(),packed.begin()+n));
 Bytes bad=packed;bad.push_back(0);rejection(bad);bad=packed;for(unsigned i=4;i<8;++i)bad[i]=255;rejection(bad);
 for(unsigned n:{0u,7u,UiProgressCapacity+1}){bad=packed;for(unsigned i=0;i<4;++i)bad[4+i]=uint8_t(n>>(8*i));rejection(bad);}
 rejection(Bytes{'N','S','Z','1',8,0,0,0,255});rejection(Bytes(UiProgressCapacity+1,1));rejection(Bytes(2*UiProgressCapacity+9,1));
 Bytes incompressible(UiProgressCapacity);for(unsigned i=0;i<incompressible.size();++i)incompressible[i]=uint8_t(i%255+1);
 const Bytes plain=envelope(incompressible);CHECK(plain==incompressible&&unpack_file(plain)==incompressible);
 // Non-optimal one-byte literal packets are valid grammar, even though the
 // production writer always uses a smaller raw representation instead.
 Bytes single={'N','S','Z','1',17,0,0,0};Bytes expected;
 for(unsigned i=0;i<17;++i){single.push_back(0);single.push_back(uint8_t(i+1));expected.push_back(uint8_t(i+1));}
 CHECK(unpack_file(single)==expected);files["/run-alignment"]=single;void*f=nd_open("/run-alignment","rb");unsigned n=0;
 CHECK(!storage_codec::read(f,encoded,sizeof encoded,n,16,true)&&!n);CHECK(nd_close(f)==0);files.erase("/run-alignment");
 for(unsigned fault=0;fault<3;++fault){files["/bad"]=packed;if(!fault)fail_read="/bad",read_fail_after=10;else if(fault==1)fail_seek="/bad";else fail_tell="/bad";
  rejection(packed,storage_codec::ReadStatus::IoFailure);}
 scenario="summary still reads only canonical summary fields, not the history tail";
 bad=raw;bad.back()=0x80;CHECK(!decode_ui_progress(bad.data(),bad.size(),loaded));files[progress_path]=envelope(bad);
 UiProgressSummary summary;CHECK(profile_info(1,summary)&&summary.exists&&summary.play_seconds==42);CHECK(!load_progress(loaded)&&!resource_transfer_uses);
}
static void failures(){
 scenario="failed writes and atomic renames preserve the existing primary";
 for(unsigned fault=0;fault<7;++fault){setup();const Bytes old=legacy(8);files[progress_path]=old;files[progress_backup]=old;
  CHECK(load_progress(loaded));value=loaded;value.characters[2].play_seconds=321;reset_io();
  switch(fault){case 0:fail_open=progress_temp;break;case 1:write_limit=17;break;case 2:fail_close=progress_temp;break;
   case 3:fail_close=progress_path;break;case 4:fail_unlink=progress_backup;break;case 5:fail_renames={progress_path};break;case 6:fail_renames={progress_temp};break;}
  CHECK(!save_progress(value)&&!resource_transfer_uses);CHECK(files.at(progress_path)==old&&!files.count(progress_temp));
  reset_io();CHECK(load_progress(loaded)&&canonical(loaded)==old);
 }
 scenario="failed rollback retains a recoverable valid backup and next save works";
 setup();const Bytes old=legacy(8);files[progress_path]=old;CHECK(load_progress(loaded));value=loaded;value.last_saved_at=345678;const Bytes next=canonical(value);
 fail_renames={progress_temp,progress_backup};CHECK(!save_progress(value)&&!files.count(progress_path)&&files.at(progress_backup)==old&&!files.count(progress_temp));
 CHECK(load_progress(loaded)&&canonical(loaded)==old);CHECK(save_progress(value)&&unpack_file(files.at(progress_path))==next&&!files.count(progress_backup));
 scenario="cleanup failure after commit cannot invalidate the valid new primary";
 CHECK(load_progress(loaded));value=loaded;value.last_saved_at++;const Bytes committed=canonical(value);fail_unlink=progress_backup;
 CHECK(save_progress(value)&&files.count(progress_backup)&&unpack_file(files.at(progress_path))==committed);
 CHECK(load_progress(loaded)&&canonical(loaded)==committed&&progress_recovery==ProgressRecovery::PrimaryValid);
 CHECK(save_progress(value)&&!files.count(progress_backup));
}
static void recovery(){
 scenario="malformed primary never replaces the valid backup after recovery";
 for(unsigned fault=0;fault<4;++fault){setup();const Bytes raw=legacy(8),good=envelope(raw);Bytes bad=good;bad.pop_back();
  if(fault==3){Bytes invalid_canonical=raw;invalid_canonical.back()=0x80;bad=envelope(invalid_canonical);}
  files[progress_path]=bad;files[progress_backup]=good;CHECK(load_progress(loaded)&&canonical(loaded)==raw);
  CHECK(progress_recovery==ProgressRecovery::BackupAfterInvalidPrimary);value=loaded;value.last_saved_at++;const Bytes next=canonical(value);
  if(fault==1)fail_unlink=progress_path;else if(fault==2)fail_renames={progress_temp};
  if(fault==1||fault==2){CHECK(!save_progress(value)&&files.at(progress_backup)==good&&!files.count(progress_temp));
   if(fault==1)CHECK(files.at(progress_path)==bad);CHECK(load_progress(loaded)&&canonical(loaded)==raw);}
  CHECK(save_progress(value)&&!files.count(progress_backup)&&!files.count(progress_temp));
  CHECK(load_progress(loaded)&&canonical(loaded)==next&&progress_recovery==ProgressRecovery::PrimaryValid);
 }
 scenario="I/O uncertainty preserves both old files until a complete load succeeds";
 for(unsigned fault=0;fault<5;++fault){setup();const Bytes primary=legacy(8);value.last_saved_at=789;const Bytes backup=envelope(canonical(value));
  files[progress_path]=primary;files[progress_backup]=backup;
  if(!fault)fail_close=progress_path;else if(fault==1)fail_read=progress_path,read_fail_after=10;
  else if(fault==2)fail_seek=progress_path;else if(fault==3)fail_tell=progress_path;else fail_open=progress_path;
  CHECK(load_progress(loaded)&&loaded.last_saved_at==789&&progress_recovery==ProgressRecovery::Unknown);
  const auto old_files=files;CHECK(!save_progress(loaded)&&files==old_files);
  CHECK(load_progress(loaded)&&progress_recovery==ProgressRecovery::PrimaryValid&&loaded.last_saved_at==123456);
  CHECK(save_progress(loaded)&&!files.count(progress_backup));
 }
 scenario="optional atomic-replace support also preserves the uncertain backup until commit";
 setup();const Bytes raw=legacy(8);files[progress_path]=raw;files[progress_backup]=raw;fail_close=progress_path;CHECK(load_progress(loaded));
 replace_existing=true;value=loaded;value.last_saved_at++;CHECK(save_progress(value)&&!files.count(progress_backup));CHECK(load_progress(loaded)&&loaded.last_saved_at==value.last_saved_at);
 scenario="uncertain backup close never certifies the malformed primary for deletion";
 setup();const Bytes recovery_raw=legacy(8),recovery_good=envelope(recovery_raw);Bytes corrupt=recovery_good;corrupt.pop_back();
 files[progress_path]=corrupt;files[progress_backup]=recovery_good;fail_close=progress_backup;
 CHECK(!load_progress(loaded)&&progress_recovery==ProgressRecovery::Unknown);const auto preserved=files;
 CHECK(!save_progress(value)&&files==preserved);
 CHECK(load_progress(loaded)&&progress_recovery==ProgressRecovery::BackupAfterInvalidPrimary);
 UiProgressSummary summary;CHECK(profile_info(1,summary)&&progress_recovery==ProgressRecovery::BackupAfterInvalidPrimary);
 CHECK(save_progress(loaded)&&load_progress(loaded)&&canonical(loaded)==recovery_raw);
}
static void profiles(){
 scenario="three profiles retain separate raw/packed histories and recovery facts";setup();Bytes expected[3];
 for(unsigned slot=1;slot<=3;++slot){selected_profile=slot;CHECK(configure_profile_paths(slot)&&progress_recovery==ProgressRecovery::Unknown);
  fixture(8);value.last_saved_at=1000+slot;value.characters[0].play_seconds=slot*17;expected[slot-1]=canonical(value);
  CHECK(save_progress(value));UiProgressSummary summary;CHECK(profile_info(slot,summary)&&summary.last_saved_at==1000+slot);}
 for(unsigned slot=1;slot<=3;++slot){selected_profile=slot;CHECK(configure_profile_paths(slot));CHECK(load_progress(loaded)&&canonical(loaded)==expected[slot-1]);}
 const auto before=files;{ResourceTransferLease busy(ResourceTransferUse::Bytes);CHECK(busy);UiProgressSummary summary;
  CHECK(!load_progress(loaded)&&!save_progress(value)&&!profile_info(1,summary)&&files==before);}
 CHECK(!resource_transfer_uses);CHECK(configure_profile_paths(1)&&progress_recovery==ProgressRecovery::Unknown);
}
int main(int argc,char**argv){
 if(argc==3&&!std::strcmp(argv[1],"--export"))export_directory=argv[2];
 else if(argc!=1){std::fprintf(stderr,"usage: ndless_history_storage_regression [--export EXISTING_DIRECTORY]\n");return 2;}
 versions();maximum();bounds();failures();recovery();profiles();
 CHECK(!resource_transfer_uses);std::printf("Ndless history: %u checks passed, raw NSH1-8 and NSZ1, unchanged canonical bytes.\n",checks);
}
